#pragma once

/// @file mutation.hpp
/// @brief Mutation operators for evolutionary algorithms following C++23 design patterns
///
/// This header provides various mutation strategies optimized for different problem types.
/// All operators follow concept-based design for type safety and performance.

#include <algorithm>
#include <random>
#include <vector>

// EvoLab core concepts - required for template type constraints
#include <evolab/core/concepts.hpp>

namespace evolab::operators {

/// Swap mutation for permutations
class SwapMutation {
  public:
    template <core::Problem P>
    void mutate([[maybe_unused]] const P& problem, typename P::GenomeT& genome,
                std::mt19937& rng) const {
        if (genome.size() < 2)
            return;

        std::uniform_int_distribution<std::size_t> dist(0, genome.size() - 1);
        std::size_t pos1 = dist(rng);
        std::size_t pos2 = dist(rng);

        // Ensure different positions
        while (pos1 == pos2 && genome.size() > 1) {
            pos2 = dist(rng);
        }

        std::swap(genome[pos1], genome[pos2]);
    }
};

/// Inversion mutation for permutations
class InversionMutation {
  public:
    template <core::Problem P>
    void mutate([[maybe_unused]] const P& problem, typename P::GenomeT& genome,
                std::mt19937& rng) const {
        if (genome.size() < 2)
            return;

        std::uniform_int_distribution<std::size_t> dist(0, genome.size() - 1);
        std::size_t pos1 = dist(rng);
        std::size_t pos2 = dist(rng);

        if (pos1 > pos2)
            std::swap(pos1, pos2);

        std::reverse(genome.begin() + pos1, genome.begin() + pos2 + 1);
    }
};

/// Scramble mutation for permutations
class ScrambleMutation {
  public:
    template <core::Problem P>
    void mutate([[maybe_unused]] const P& problem, typename P::GenomeT& genome,
                std::mt19937& rng) const {
        if (genome.size() < 2)
            return;

        std::uniform_int_distribution<std::size_t> dist(0, genome.size() - 1);
        std::size_t pos1 = dist(rng);
        std::size_t pos2 = dist(rng);

        if (pos1 > pos2)
            std::swap(pos1, pos2);

        std::shuffle(genome.begin() + pos1, genome.begin() + pos2 + 1, rng);
    }
};

/// Insertion mutation for permutations
class InsertionMutation {
  public:
    template <core::Problem P>
    void mutate([[maybe_unused]] const P& problem, typename P::GenomeT& genome,
                std::mt19937& rng) const {
        if (genome.size() < 2)
            return;

        std::uniform_int_distribution<std::size_t> dist(0, genome.size() - 1);
        std::size_t remove_pos = dist(rng);
        std::size_t insert_pos = dist(rng);

        // Ensure different positions
        while (remove_pos == insert_pos && genome.size() > 1) {
            insert_pos = dist(rng);
        }

        auto element = genome[remove_pos];
        genome.erase(genome.begin() + remove_pos);

        // Adjust insertion position if necessary
        if (insert_pos > remove_pos) {
            insert_pos--;
        }

        genome.insert(genome.begin() + insert_pos, element);
    }
};

/// Displacement mutation for permutations
class DisplacementMutation {
  public:
    template <core::Problem P>
    void mutate([[maybe_unused]] const P& problem, typename P::GenomeT& genome,
                std::mt19937& rng) const {
        if (genome.size() < 3)
            return;

        std::uniform_int_distribution<std::size_t> dist(0, genome.size() - 1);

        // Select a subsequence to displace
        std::size_t start = dist(rng);
        std::size_t end = dist(rng);
        if (start > end)
            std::swap(start, end);

        // Select insertion point
        std::size_t insert_pos = dist(rng);

        // Extract subsequence
        typename P::GenomeT subsequence(genome.begin() + start, genome.begin() + end + 1);

        // Remove subsequence from original position
        genome.erase(genome.begin() + start, genome.begin() + end + 1);

        // Adjust insertion position
        if (insert_pos > start) {
            insert_pos -= (end - start + 1);
        }

        // Insert subsequence at new position
        genome.insert(genome.begin() + insert_pos, subsequence.begin(), subsequence.end());
    }
};

/// Adaptive mutation that selects from multiple operators
class AdaptiveMutation {
    SwapMutation swap_;
    InversionMutation inversion_;
    ScrambleMutation scramble_;
    InsertionMutation insertion_;

    std::vector<double> weights_;

  public:
    explicit AdaptiveMutation(std::vector<double> weights = {0.4, 0.3, 0.2, 0.1})
        : weights_(std::move(weights)) {

        if (weights_.size() != 4) {
            weights_ = {0.4, 0.3, 0.2, 0.1};
        }

        // Normalize weights
        double sum = std::accumulate(weights_.begin(), weights_.end(), 0.0);
        if (sum > 0.0) {
            for (auto& w : weights_)
                w /= sum;
        }
    }

    template <core::Problem P>
    void mutate([[maybe_unused]] const P& problem, typename P::GenomeT& genome,
                std::mt19937& rng) const {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        double rand_val = dist(rng);

        double cumulative = 0.0;
        for (std::size_t i = 0; i < weights_.size(); ++i) {
            cumulative += weights_[i];
            if (rand_val <= cumulative) {
                switch (i) {
                case 0:
                    swap_.mutate(problem, genome, rng);
                    break;
                case 1:
                    inversion_.mutate(problem, genome, rng);
                    break;
                case 2:
                    scramble_.mutate(problem, genome, rng);
                    break;
                case 3:
                    insertion_.mutate(problem, genome, rng);
                    break;
                }
                break;
            }
        }
    }

    const std::vector<double>& weights() const { return weights_; }
};

/// Multiple swap mutation (performs k swaps)
class MultiSwapMutation {
    std::size_t num_swaps_;

  public:
    explicit MultiSwapMutation(std::size_t num_swaps = 2) : num_swaps_(num_swaps) {}

    template <core::Problem P>
    void mutate([[maybe_unused]] const P& problem, typename P::GenomeT& genome,
                std::mt19937& rng) const {
        if (genome.size() < 2)
            return;

        SwapMutation swap;
        for (std::size_t i = 0; i < num_swaps_; ++i) {
            swap.mutate(problem, genome, rng);
        }
    }

    std::size_t num_swaps() const { return num_swaps_; }
};

/// Lin-Kernighan style mutation (performs 2-opt moves)
class TwoOptMutation {
  public:
    template <core::Problem P>
    void mutate([[maybe_unused]] const P& problem, typename P::GenomeT& genome,
                std::mt19937& rng) const {
        if (genome.size() < 4)
            return; // Need at least 4 cities for 2-opt

        std::uniform_int_distribution<std::size_t> dist(0, genome.size() - 1);
        std::size_t i = dist(rng);
        std::size_t j = dist(rng);

        // Ensure valid 2-opt move (i and j shouldn't be adjacent)
        while (i == j || (i + 1) % genome.size() == j || (j + 1) % genome.size() == i) {
            j = dist(rng);
        }

        if (i > j)
            std::swap(i, j);

        // Apply 2-opt: reverse the segment between i+1 and j
        std::reverse(genome.begin() + i + 1, genome.begin() + j + 1);
    }
};

} // namespace evolab::operators
