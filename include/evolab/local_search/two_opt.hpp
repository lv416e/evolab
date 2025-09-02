#pragma once

#include <algorithm>
#include <limits>
#include <random>
#include <vector>

#include "../core/concepts.hpp"
#include "../problems/tsp.hpp"

namespace evolab::local_search {

/// 2-opt local search for TSP problems
class TwoOpt {
    bool first_improvement_;
    std::size_t max_iterations_;

  public:
    explicit TwoOpt(bool first_improvement = false, std::size_t max_iterations = 0)
        : first_improvement_(first_improvement), max_iterations_(max_iterations) {}

    /// Improve TSP tour using 2-opt
    core::Fitness improve(const problems::TSP& problem, problems::TSP::GenomeT& tour,
                          std::mt19937& rng) const {
        const int n = static_cast<int>(tour.size());
        if (n < 4)
            return problem.evaluate(tour);

        bool improved = true;
        std::size_t iterations = 0;
        core::Fitness current_fitness = problem.evaluate(tour);

        while (improved && (max_iterations_ == 0 || iterations < max_iterations_)) {
            improved = false;

            for (int i = 0; i < n - 1 && !improved; ++i) {
                for (int j = i + 2; j < n && !improved; ++j) {
                    // Skip adjacent edges in circular tour
                    if (j == n - 1 && i == 0)
                        continue;

                    double gain = problem.two_opt_gain(tour, i, j);

                    if (gain > 1e-9) { // Found improvement
                        problem.apply_two_opt(tour, i, j);
                        current_fitness = core::Fitness{current_fitness.value - gain};
                        improved = true;

                        if (first_improvement_)
                            break;
                    }
                }
            }

            iterations++;
        }

        return current_fitness;
    }

    /// Generic improve method for concept compliance
    template <core::Problem P>
    core::Fitness improve(const P& problem, typename P::GenomeT& genome, std::mt19937& rng) const {
        if constexpr (std::is_same_v<P, problems::TSP>) {
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
        if (n < 4)
            return problem.evaluate(tour);

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

            double gain = problem.two_opt_gain(tour, i, j);
            if (gain > best_gain) {
                best_gain = gain;
                best_i = i;
                best_j = j;
            }
        }

        core::Fitness current_fitness = problem.evaluate(tour);

        if (best_gain > 1e-9) {
            problem.apply_two_opt(tour, best_i, best_j);
            current_fitness = core::Fitness{current_fitness.value - best_gain};
        }

        return current_fitness;
    }

    template <core::Problem P>
    core::Fitness improve(const P& problem, typename P::GenomeT& genome, std::mt19937& rng) const {
        if constexpr (std::is_same_v<P, problems::TSP>) {
            return improve(problem, genome, rng);
        } else {
            return problem.evaluate(genome);
        }
    }

    std::size_t num_attempts() const { return num_attempts_; }
};

/// Candidate list 2-opt (uses nearest neighbor lists for efficiency)
class CandidateList2Opt {
    std::size_t k_nearest_;
    bool first_improvement_;

    mutable std::vector<std::vector<int>> candidate_lists_;
    mutable bool lists_built_ = false;

  public:
    explicit CandidateList2Opt(std::size_t k_nearest = 20, bool first_improvement = true)
        : k_nearest_(k_nearest), first_improvement_(first_improvement) {}

    core::Fitness improve(const problems::TSP& problem, problems::TSP::GenomeT& tour,
                          std::mt19937& rng) const {
        const int n = problem.num_cities();
        if (n < 4)
            return problem.evaluate(tour);

        build_candidate_lists(problem);

        bool improved = true;
        core::Fitness current_fitness = problem.evaluate(tour);

        while (improved) {
            improved = false;

            for (int i = 0; i < n - 1 && !improved; ++i) {
                // Try edges to candidate cities
                for (int candidate_city : candidate_lists_[tour[i]]) {
                    // Find position of candidate city in tour
                    auto it = std::find(tour.begin(), tour.end(), candidate_city);
                    if (it == tour.end())
                        continue;

                    int j = static_cast<int>(it - tour.begin());

                    // Check if this is a valid 2-opt move
                    if (std::abs(i - j) <= 1 || (i == 0 && j == n - 1) || (j == 0 && i == n - 1))
                        continue;

                    if (i > j)
                        std::swap(i, j);

                    double gain = problem.two_opt_gain(tour, i, j);

                    if (gain > 1e-9) {
                        problem.apply_two_opt(tour, i, j);
                        current_fitness = core::Fitness{current_fitness.value - gain};
                        improved = true;

                        if (first_improvement_)
                            break;
                    }
                }
            }
        }

        return current_fitness;
    }

    template <core::Problem P>
    core::Fitness improve(const P& problem, typename P::GenomeT& genome, std::mt19937& rng) const {
        if constexpr (std::is_same_v<P, problems::TSP>) {
            return improve(problem, genome, rng);
        } else {
            return problem.evaluate(genome);
        }
    }

  private:
    void build_candidate_lists(const problems::TSP& problem) const {
        if (lists_built_)
            return;

        const int n = problem.num_cities();
        candidate_lists_.assign(n, std::vector<int>());

        for (int i = 0; i < n; ++i) {
            // Create list of all other cities with distances
            std::vector<std::pair<double, int>> distances;
            distances.reserve(n - 1);

            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    distances.emplace_back(problem.distance(i, j), j);
                }
            }

            // Sort by distance and take k nearest
            std::sort(distances.begin(), distances.end());
            std::size_t k = std::min(k_nearest_, distances.size());

            candidate_lists_[i].reserve(k);
            for (std::size_t idx = 0; idx < k; ++idx) {
                candidate_lists_[i].push_back(distances[idx].second);
            }
        }

        lists_built_ = true;
    }

  public:
    std::size_t k_nearest() const { return k_nearest_; }
    bool first_improvement() const { return first_improvement_; }
};

/// No-op local search (for algorithms that don't use local search)
class NoLocalSearch {
  public:
    template <core::Problem P>
    core::Fitness improve(const P& problem, typename P::GenomeT& genome, std::mt19937& rng) const {
        return problem.evaluate(genome);
    }
};

} // namespace evolab::local_search
