/// @file lk.hpp
/// @brief Lin-Kernighan local search optimization with limited depth
///
/// Implements a variable k-opt local search algorithm based on the Lin-Kernighan heuristic.
/// This implementation uses a limited-depth sequential edge exchange strategy guided by
/// candidate lists for research-grade memetic algorithms.

#pragma once

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

#include <evolab/core/concepts.hpp>
#include <evolab/problems/tsp.hpp>
#include <evolab/utils/candidate_list.hpp>
#include <evolab/utils/compiler_hints.hpp>

namespace evolab::local_search {

/// Simplified Lin-Kernighan local search with candidate list guidance
///
/// Current implementation: Candidate-list-guided 2-opt local search with
/// iterative improvement. This practical variant is well-suited for memetic
/// algorithms and provides good performance/quality tradeoff.
///
/// Algorithm:
/// 1. For each edge in the tour, try 2-opt moves with candidate neighbors
/// 2. Apply best improving move found (best improvement strategy)
/// 3. Repeat until no improvement found or max iterations reached
///
/// Key features:
/// - Candidate list guided edge selection (avoids exhaustive search)
/// - Best improvement search within candidate set
/// - Configurable iteration limit via max_depth parameter
/// - Maintains tour validity throughout
/// - Suitable for integration with genetic algorithms
///
/// @note Future enhancement: Multi-step sequential edge exchanges (true variable k-opt)
class LinKernighan {
    int k_nearest_; ///< Number of nearest neighbors to consider
    int max_depth_; ///< Maximum improvement iterations per improve() call

  public:
    /// Construct Lin-Kernighan local search
    ///
    /// @param k_nearest Number of nearest neighbors in candidate list (default: 20)
    /// @param max_depth Maximum improvement iterations per improve() call (default: 5)
    /// @throws std::invalid_argument if k_nearest < 1 or max_depth < 1
    explicit LinKernighan(int k_nearest = 20, int max_depth = 5)
        : k_nearest_(k_nearest), max_depth_(max_depth) {
        if (k_nearest < 1) {
            throw std::invalid_argument("k_nearest must be at least 1");
        }
        if (max_depth < 1) {
            throw std::invalid_argument("max_depth must be at least 1");
        }
    }

    /// Improve TSP tour using Lin-Kernighan heuristic
    ///
    /// @param problem TSP problem instance
    /// @param tour Tour to improve (modified in-place)
    /// @param rng Random number generator
    /// @return Final fitness after improvement
    core::Fitness improve(const problems::TSP& problem, problems::TSP::GenomeT& tour,
                          std::mt19937& rng) const {
        const int n = static_cast<int>(tour.size());

        // Tours with < 4 cities cannot be improved by k-opt
        if (EVOLAB_UNLIKELY(n < 4))
            return problem.evaluate(tour);

        problem.clear_distance_cache();

        // Get or create candidate list
        const auto* candidate_list = problem.get_candidate_list(k_nearest_);
        if (EVOLAB_UNLIKELY(candidate_list == nullptr)) {
            // Fallback: return current fitness if candidate list unavailable
            return problem.evaluate(tour);
        }

        core::Fitness current_fitness = problem.evaluate(tour);
        bool improved = true;
        int iteration = 0;

        // Build position mapping once for O(1) city position lookup
        // Updated incrementally after each 2-opt move to avoid O(n) rebuilds
        std::vector<int> position(n);
        for (int i = 0; i < n; ++i) {
            position[tour[i]] = i;
        }

        // Use max_depth to control improvement iterations
        // This balances solution quality with computational cost
        while (improved && iteration < max_depth_) {
            improved = false;
            iteration++;

            // Try to find the best improving k-opt move across all starting positions
            double best_gain = 0.0;
            int best_i = -1;
            int best_j = -1;

            // Try starting from each position in the tour to find the best 2-opt move
            for (int start_pos = 0; start_pos < n; ++start_pos) {
                // Attempt a sequential edge exchange starting from this position
                auto [gain, j] =
                    attempt_edge_exchange(problem, tour, start_pos, position, *candidate_list, rng);

                if (gain > best_gain) {
                    best_gain = gain;
                    best_i = start_pos;
                    best_j = j;
                }
            }

            if (best_gain > 0) {
                problem.apply_two_opt(tour, best_i, best_j);

                // Update position mapping for the reversed segment [best_i+1, best_j]
                // This is O(k) where k is segment length, avoiding full O(n) rebuild
                for (int i = best_i + 1; i <= best_j; ++i) {
                    position[tour[i]] = i;
                }

                current_fitness = core::Fitness{current_fitness.value - best_gain};
                improved = true;
            }
        }

        return current_fitness;
    }

    /// Generic improve method for concept compliance
    template <core::Problem P>
    core::Fitness improve(const P& problem, typename P::GenomeT& genome, std::mt19937& rng) const {
        if constexpr (std::is_same_v<P, problems::TSP>) {
            return improve(problem, genome, rng);
        } else {
            return problem.evaluate(genome);
        }
    }

    int k_nearest() const { return k_nearest_; }
    int max_depth() const { return max_depth_; }

  private:
    /// Attempt to find best 2-opt move starting from a given position
    ///
    /// Implements best improvement strategy:
    /// - Evaluate all 2-opt moves with candidates of city at start_pos
    /// - Return the best improving move (if any)
    ///
    /// @param problem TSP problem instance
    /// @param tour Current tour
    /// @param start_pos Starting position for 2-opt moves
    /// @param position City to position mapping
    /// @param candidate_list Candidate list for edge selection
    /// @param rng Random number generator
    /// @return Pair of (gain, j_index) where j_index is the second 2-opt position (-1 if no
    /// improvement)
    std::pair<double, int> attempt_edge_exchange(const problems::TSP& problem,
                                                 const std::vector<int>& tour, int start_pos,
                                                 const std::vector<int>& position,
                                                 const utils::CandidateList& candidate_list,
                                                 [[maybe_unused]] std::mt19937& rng) const {

        const int n = static_cast<int>(tour.size());
        double best_gain = 0.0;
        int best_j = -1;

        // Find best 2-opt move using candidate lists
        // This implements "best improvement" strategy: evaluate all candidates
        // on the ORIGINAL tour, then apply only the best move
        const int city_at_start = tour[start_pos];
        const auto& candidates = candidate_list.get_candidates(city_at_start);

        for (int candidate_city : candidates) {
            const int j = position[candidate_city];

            // Ensure valid 2-opt move (nodes cannot be adjacent in the tour)
            // Only skip moves between adjacent nodes, as 2-opt on adjacent edges is a no-op
            if (j == (start_pos + 1) % n || j == (start_pos + n - 1) % n) {
                continue;
            }

            // Compute gain for this 2-opt move on ORIGINAL tour
            const double gain = problem.two_opt_gain_cached(tour, start_pos, j);

            if (gain > best_gain) {
                best_gain = gain;
                best_j = j;
            }
        }

        return {best_gain, best_j};
    }
};

} // namespace evolab::local_search
