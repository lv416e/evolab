#pragma once

/// @file selection.hpp
/// @brief Selection operators for evolutionary algorithms with modern C++23 features
///
/// This header implements various selection strategies including tournament, roulette wheel,
/// and rank-based selection. All operators use concepts for compile-time type validation.

#include <algorithm>
#include <cassert>
#include <numeric>
#include <random>
#include <span>
#include <vector>

// EvoLab core concepts - fundamental type requirements for selection operators
#include <evolab/core/concepts.hpp>

namespace evolab::operators {

/// Tournament selection operator
class TournamentSelection {
    std::size_t tournament_size_;

  public:
    explicit TournamentSelection(std::size_t tournament_size = 4)
        : tournament_size_(std::max<std::size_t>(1, tournament_size)) {}

    std::size_t select(std::span<const core::Fitness> fitnesses, std::mt19937& rng) const {

        assert(!fitnesses.empty());
        if (fitnesses.size() == 1) {
            return 0;
        }

        std::uniform_int_distribution<std::size_t> dist(0, fitnesses.size() - 1);

        std::size_t best_idx = dist(rng);
        core::Fitness best_fitness = fitnesses[best_idx];

        for (std::size_t i = 1; i < tournament_size_; ++i) {
            std::size_t candidate = dist(rng);
            if (fitnesses[candidate] < best_fitness) {
                best_idx = candidate;
                best_fitness = fitnesses[candidate];
            }
        }

        return best_idx;
    }

    std::size_t tournament_size() const { return tournament_size_; }
};

/// Roulette wheel selection (implemented for minimization by inverting weights)
class RouletteWheelSelection {
  public:
    std::size_t select(std::span<const core::Fitness> fitnesses, std::mt19937& rng) const {

        assert(!fitnesses.empty());
        if (fitnesses.size() == 1) {
            return 0;
        }

        // Convert to positive weights (assume minimization, so invert)
        const auto [min_it, max_it] =
            std::minmax_element(fitnesses.begin(), fitnesses.end(),
                                [](const auto& a, const auto& b) { return a.value < b.value; });
        double min_fitness = min_it->value;
        double max_fitness = max_it->value;
        double range = max_fitness - min_fitness;

        // First pass: calculate total weight (avoiding heap allocation)
        double total_weight = 0.0;
        for (const auto& fitness : fitnesses) {
            // Convert minimization to maximization weights
            double weight = (range == 0.0) ? 1.0 : (max_fitness - fitness.value + 1.0);
            total_weight += weight;
        }

        std::uniform_real_distribution<double> dist(0.0, total_weight);
        const double target = dist(rng);

        // Second pass: find target bucket without storing weights
        double cumulative = 0.0;
        for (std::size_t i = 0; i < fitnesses.size(); ++i) {
            const double weight = (range == 0.0) ? 1.0 : (max_fitness - fitnesses[i].value + 1.0);
            cumulative += weight;
            if (cumulative >= target) {
                return i;
            }
        }

        return fitnesses.size() - 1; // Fallback
    }
};

/// Ranking selection
class RankSelection {
    double selection_pressure_; // 1.0 = no pressure, 2.0 = maximum pressure

  public:
    explicit RankSelection(double pressure = 1.5) : selection_pressure_(pressure) {
        assert(pressure >= 1.0 && pressure <= 2.0);
    }

    std::size_t select(std::span<const core::Fitness> fitnesses, std::mt19937& rng) const {

        assert(!fitnesses.empty());
        if (fitnesses.size() == 1) {
            return 0;
        }

        // Create ranking
        std::vector<std::size_t> indices(fitnesses.size());
        std::iota(indices.begin(), indices.end(), 0);

        // Sort by fitness (best first for minimization)
        std::sort(indices.begin(), indices.end(),
                  [&](std::size_t a, std::size_t b) { return fitnesses[a] < fitnesses[b]; });

        // Calculate rank-based probabilities
        std::vector<double> probabilities;
        probabilities.reserve(fitnesses.size());

        const double n = static_cast<double>(fitnesses.size());
        double total_prob = 0.0;

        for (std::size_t rank = 0; rank < fitnesses.size(); ++rank) {
            // Rank 0 is best, rank n-1 is worst
            double prob = (2.0 - selection_pressure_) / n +
                          (2.0 * selection_pressure_ * (n - rank - 1)) / (n * (n - 1));
            probabilities.push_back(prob);
            total_prob += prob;
        }

        // Select based on probabilities
        std::uniform_real_distribution<double> dist(0.0, total_prob);
        double target = dist(rng);

        double cumulative = 0.0;
        for (std::size_t i = 0; i < probabilities.size(); ++i) {
            cumulative += probabilities[i];
            if (cumulative >= target) {
                return indices[i];
            }
        }

        return indices[0]; // Return best as fallback
    }

    double selection_pressure() const { return selection_pressure_; }
};

/// Steady-state selection (select best individuals)
class SteadyStateSelection {
    std::size_t num_best_;

  public:
    explicit SteadyStateSelection(std::size_t num_best = 1) : num_best_(num_best) {}

    std::size_t select(std::span<const core::Fitness> fitnesses, std::mt19937& rng) const {

        assert(!fitnesses.empty());
        if (fitnesses.size() == 1) {
            return 0;
        }

        std::vector<std::size_t> indices(fitnesses.size());
        std::iota(indices.begin(), indices.end(), 0);

        // Partial sort to get the best individuals
        std::size_t k = std::min(num_best_, fitnesses.size());
        k = std::max<std::size_t>(1, k);
        std::partial_sort(
            indices.begin(), indices.begin() + k, indices.end(),
            [&](std::size_t a, std::size_t b) { return fitnesses[a] < fitnesses[b]; });

        // Randomly select from the best k
        std::uniform_int_distribution<std::size_t> dist(0, k - 1);
        return indices[dist(rng)];
    }

    std::size_t num_best() const { return num_best_; }
};

} // namespace evolab::operators
