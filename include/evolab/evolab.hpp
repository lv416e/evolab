#pragma once

// Core components
#include "core/concepts.hpp"
#include "core/ga.hpp"

// Problems
#include "problems/tsp.hpp"

// Genetic operators
#include "operators/crossover.hpp"
#include "operators/mutation.hpp"
#include "operators/selection.hpp"

// Local search
#include "local_search/two_opt.hpp"

/**
 * @file evolab.hpp
 * @brief Main header for EvoLab - A modern C++23 metaheuristics research platform
 *
 * EvoLab is a high-performance, research-grade genetic algorithm framework designed
 * for solving complex optimization problems like TSP, VRP, and QAP.
 *
 * Key features:
 * - Modern C++23 with concepts and ranges
 * - Template-based architecture for zero-cost abstractions
 * - Advanced genetic operators (EAX, PMX, OX, etc.)
 * - Memetic algorithms with local search (2-opt, Lin-Kernighan)
 * - Parallel evaluation and Island Model
 * - Research-grade reproducibility and statistics
 *
 * Basic usage:
 * @code
 * #include <evolab/evolab.hpp>
 * using namespace evolab;
 *
 * // Create TSP problem
 * auto tsp = problems::create_random_tsp(100);
 *
 * // Configure genetic algorithm
 * auto ga = core::make_ga(
 *     operators::TournamentSelection{4},
 *     operators::OrderCrossover{},
 *     operators::SwapMutation{},
 *     local_search::TwoOpt{}
 * );
 *
 * // Run optimization
 * auto result = ga.run(tsp, core::GAConfig{
 *     .population_size = 256,
 *     .max_generations = 1000,
 *     .crossover_prob = 0.9,
 *     .mutation_prob = 0.1
 * });
 *
 * std::cout << "Best fitness: " << result.best_fitness.value << std::endl;
 * @endcode
 */

namespace evolab {

/// Current version
constexpr const char* VERSION = "0.1.0";

/// Common type aliases
namespace types {
using Fitness = core::Fitness;

template <typename T>
using Genome = core::Genome<T>;

using TSP = problems::TSP;
} // namespace types

/// Factory functions for common configurations
namespace factory {

/// Create a simple GA for TSP with tournament selection and 2-opt
inline auto make_tsp_ga_basic() {
    return core::make_ga(operators::TournamentSelection{4}, operators::OrderCrossover{},
                         operators::SwapMutation{},
                         local_search::TwoOpt{true, 1000} // First improvement, max 1000 iterations
    );
}

/// Create a memetic GA for TSP with advanced operators
inline auto make_tsp_ga_advanced() {
    return core::make_ga(operators::TournamentSelection{7}, operators::EdgeRecombinationCrossover{},
                         operators::AdaptiveMutation{}, local_search::CandidateList2Opt{20, true});
}

/// Create a basic GA without local search
inline auto make_ga_basic() {
    return core::make_ga(operators::TournamentSelection{4}, operators::PMXCrossover{},
                         operators::InversionMutation{}, local_search::NoLocalSearch{});
}

} // namespace factory

} // namespace evolab
