#pragma once

/// @file two_opt.hpp
/// @brief 2-opt local search optimization with C++23 performance enhancements
///
/// This header implements high-performance 2-opt local search for TSP problems.
/// Features first/best improvement modes, candidate list support, and modern C++23
/// optimizations for research-grade memetic algorithms.

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <random>
#include <type_traits>
#include <vector>

// EvoLab dependencies - core concepts and TSP problem definition
#include <evolab/core/concepts.hpp>        // Type constraints for local search interface
#include <evolab/problems/tsp.hpp>         // TSP problem class with distance calculations
#include <evolab/utils/compiler_hints.hpp> // Branch prediction hints

namespace evolab::local_search {

/// Minimum gain threshold for accepting an improvement in local search
/// Values below this threshold are considered numerical noise
constexpr double MIN_IMPROVEMENT_GAIN = 1e-9;

/// 2-opt local search for TSP problems
class TwoOpt {
    bool first_improvement_;
    std::size_t max_iterations_;

  public:
    explicit TwoOpt(bool first_improvement = false, std::size_t max_iterations = 0)
        : first_improvement_(first_improvement), max_iterations_(max_iterations) {}

    /// Improve TSP tour using 2-opt
    core::Fitness improve(const problems::TSP& problem, problems::TSP::GenomeT& tour,
                          [[maybe_unused]] std::mt19937& rng) const {
        const int n = static_cast<int>(tour.size());
        // Tours with < 4 cities cannot be improved by 2-opt (need at least 2 edges to swap)
        // This is unlikely since typical TSP instances have many cities
        if (EVOLAB_UNLIKELY(n < 4))
            return problem.evaluate(tour);

        // Clear cache for fresh local search
        problem.clear_distance_cache();

        bool improved = true;
        std::size_t iterations = 0;
        core::Fitness current_fitness = problem.evaluate(tour);

        // Performance-critical design: first/best improvement strategies are
        // intentionally separated to hoist the branch outside the hot nested loops.
        // This eliminates ~500k branch predictions for n=1000 TSP, preventing
        // pipeline stalls. Code duplication is an acceptable tradeoff for performance
        // in research-grade optimization algorithms.
        while (improved && (max_iterations_ == 0 || iterations < max_iterations_)) {
            improved = false;
            int best_i = -1, best_j = -1;
            double best_gain = MIN_IMPROVEMENT_GAIN;

            if (first_improvement_) {
                for (int i = 0; i < n - 1; ++i) {
                    for (int j = i + 2; j < n; ++j) {
                        // Skip adjacent edges in circular tour
                        if (EVOLAB_UNLIKELY(j == n - 1 && i == 0))
                            continue;

                        // Use cached gain calculation for better performance
                        const double gain = problem.two_opt_gain_cached(tour, i, j);

                        // First improvement: apply first improving move immediately
                        if (EVOLAB_UNLIKELY(gain > MIN_IMPROVEMENT_GAIN)) {
                            problem.apply_two_opt(tour, i, j);
                            current_fitness = core::Fitness{current_fitness.value - gain};
                            improved = true;
                            goto next_iteration;
                        }
                    }
                }
            } else {
                for (int i = 0; i < n - 1; ++i) {
                    for (int j = i + 2; j < n; ++j) {
                        // Skip adjacent edges in circular tour
                        if (EVOLAB_UNLIKELY(j == n - 1 && i == 0))
                            continue;

                        // Use cached gain calculation for better performance
                        const double gain = problem.two_opt_gain_cached(tour, i, j);

                        // Best improvement: track best move across all candidates
                        if (gain > best_gain) {
                            best_gain = gain;
                            best_i = i;
                            best_j = j;
                        }
                    }
                }

                // Apply best improvement if found
                if (best_i != -1) {
                    problem.apply_two_opt(tour, best_i, best_j);
                    current_fitness = core::Fitness{current_fitness.value - best_gain};
                    improved = true;
                }
            }

        next_iteration:
            iterations++;
        }

        return current_fitness;
    }

    /// Generic improve method for concept compliance
    template <core::Problem P>
    core::Fitness improve(const P& problem, typename P::GenomeT& genome, std::mt19937& rng) const {
        if constexpr (std::is_same_v<P, problems::TSP>) {
            // Calls non-template overload (C++ overload resolution prioritizes non-template
            // functions)
            return improve(problem, genome, rng);
        } else {
            // For non-TSP problems, return current fitness without improvement
            return problem.evaluate(genome);
        }
    }

    bool first_improvement() const { return first_improvement_; }
    std::size_t max_iterations() const { return max_iterations_; }
};

/// Random 2-opt (selects random edges for improvement)
class Random2Opt {
    std::size_t num_attempts_;

  public:
    explicit Random2Opt(std::size_t num_attempts = 100) : num_attempts_(num_attempts) {}

    core::Fitness improve(const problems::TSP& problem, problems::TSP::GenomeT& tour,
                          std::mt19937& rng) const {
        const int n = static_cast<int>(tour.size());
        if (EVOLAB_UNLIKELY(n < 4))
            return problem.evaluate(tour);

        problem.clear_distance_cache();

        std::uniform_int_distribution<int> dist(0, n - 1);
        double best_gain = 0.0;
        int best_i = -1, best_j = -1;

        for (std::size_t attempt = 0; attempt < num_attempts_; ++attempt) {
            int i = dist(rng);
            int j = dist(rng);

            // Ensure valid 2-opt move
            while (i == j || std::abs(i - j) == 1 || (i == 0 && j == n - 1) ||
                   (j == 0 && i == n - 1)) {
                j = dist(rng);
            }

            if (i > j)
                std::swap(i, j);

            const double gain = problem.two_opt_gain_cached(tour, i, j);
            if (EVOLAB_UNLIKELY(gain > best_gain)) {
                best_gain = gain;
                best_i = i;
                best_j = j;
            }
        }

        core::Fitness current_fitness = problem.evaluate(tour);

        if (EVOLAB_UNLIKELY(best_gain > MIN_IMPROVEMENT_GAIN)) {
            problem.apply_two_opt(tour, best_i, best_j);
            current_fitness = core::Fitness{current_fitness.value - best_gain};
        }

        return current_fitness;
    }

    template <core::Problem P>
    core::Fitness improve(const P& problem, typename P::GenomeT& genome, std::mt19937& rng) const {
        if constexpr (std::is_same_v<P, problems::TSP>) {
            // Calls non-template overload (C++ overload resolution prioritizes non-template
            // functions)
            return improve(problem, genome, rng);
        } else {
            return problem.evaluate(genome);
        }
    }

    std::size_t num_attempts() const { return num_attempts_; }
};

/// Candidate list 2-opt (uses nearest neighbor lists for efficiency)
class CandidateList2Opt {
    int k_nearest_;
    bool first_improvement_;

  public:
    explicit CandidateList2Opt(int k_nearest = 20, bool first_improvement = true)
        : k_nearest_(k_nearest), first_improvement_(first_improvement) {}

    core::Fitness improve(const problems::TSP& problem, problems::TSP::GenomeT& tour,
                          [[maybe_unused]] std::mt19937& rng) const {
        const int n = problem.num_cities();
        if (EVOLAB_UNLIKELY(n < 4))
            return problem.evaluate(tour);

        problem.clear_distance_cache();

        // Get or create candidate list
        const auto* candidate_list = problem.get_candidate_list(k_nearest_);

        bool improved = true;
        core::Fitness current_fitness = problem.evaluate(tour);
        std::size_t iterations = 0;
        const std::size_t max_iterations = n * 10; // Prevent infinite loops

        while (improved && iterations < max_iterations) {
            improved = false;
            iterations++;
            int best_i = -1, best_j = -1;
            double best_gain = MIN_IMPROVEMENT_GAIN;

            // Build position mapping for O(1) city position lookup
            std::vector<int> position(n);
            for (int i = 0; i < n; ++i) {
                position[tour[i]] = i;
            }

            // Performance-critical: separate first/best improvement to avoid branching
            // in the hot candidate iteration loop (see TwoOpt::improve for rationale)
            if (first_improvement_) {
                for (int i = 0; i < n; ++i) {
                    const int city_i = tour[i];

                    // Get candidates for city at position i
                    const auto& candidates = candidate_list->get_candidates(city_i);

                    for (int candidate : candidates) {
                        const int j = position[candidate];

                        // Ensure we have a valid 2-opt move (i < j and not adjacent)
                        if (EVOLAB_UNLIKELY(j <= i || j == i + 1 || (i == 0 && j == n - 1))) {
                            continue;
                        }

                        const double gain = problem.two_opt_gain_cached(tour, i, j);

                        // First improvement: apply first improving move immediately
                        if (EVOLAB_UNLIKELY(gain > MIN_IMPROVEMENT_GAIN)) {
                            problem.apply_two_opt(tour, i, j);
                            current_fitness = core::Fitness{current_fitness.value - gain};
                            improved = true;
                            goto next_iteration;
                        }
                    }
                }
            } else {
                for (int i = 0; i < n; ++i) {
                    const int city_i = tour[i];

                    // Get candidates for city at position i
                    const auto& candidates = candidate_list->get_candidates(city_i);

                    for (int candidate : candidates) {
                        const int j = position[candidate];

                        // Ensure we have a valid 2-opt move (i < j and not adjacent)
                        if (EVOLAB_UNLIKELY(j <= i || j == i + 1 || (i == 0 && j == n - 1))) {
                            continue;
                        }

                        const double gain = problem.two_opt_gain_cached(tour, i, j);

                        // Best improvement: track best move across all candidates
                        if (gain > best_gain) {
                            best_gain = gain;
                            best_i = i;
                            best_j = j;
                        }
                    }
                }

                // Apply best improvement if found
                if (best_i != -1) {
                    problem.apply_two_opt(tour, best_i, best_j);
                    current_fitness = core::Fitness{current_fitness.value - best_gain};
                    improved = true;
                }
            }

        next_iteration:;
        }

        return current_fitness;
    }

    template <core::Problem P>
    core::Fitness improve(const P& problem, typename P::GenomeT& genome, std::mt19937& rng) const {
        if constexpr (std::is_same_v<P, problems::TSP>) {
            // Calls non-template overload (C++ overload resolution prioritizes non-template
            // functions)
            return improve(problem, genome, rng);
        } else {
            return problem.evaluate(genome);
        }
    }

  public:
    int k_nearest() const { return k_nearest_; }
    bool first_improvement() const { return first_improvement_; }
};

/// No-op local search (for algorithms that don't use local search)
class NoLocalSearch {
  public:
    template <core::Problem P>
    core::Fitness improve(const P& problem, typename P::GenomeT& genome,
                          [[maybe_unused]] std::mt19937& rng) const {
        return problem.evaluate(genome);
    }
};

} // namespace evolab::local_search
