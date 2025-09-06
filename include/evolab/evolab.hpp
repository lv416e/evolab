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

// Schedulers
#include "schedulers/mab.hpp"

// Utilities
#include "utils/candidate_list.hpp"

// IO
#include "io/tsplib.hpp"

// Configuration
#include "config/config.hpp"

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

/// Create a TSP GA from configuration with PMX crossover
/// Default implementation for configuration-driven GA
///
/// NOTE: Current limitation - operator types are fixed at compile time due to C++ template design.
/// The configuration's operator.type field is currently not used to select operators dynamically.
/// To use different operators, use the specific factory functions below (e.g.,
/// make_tsp_ga_eax_from_config).
///
/// TODO: Future enhancement - implement runtime operator selection using std::variant or type
/// erasure
inline auto make_tsp_ga_from_config(const config::Config& cfg) {
    // LIMITATION: Always uses PMX crossover regardless of config.operators.crossover.type
    // This is due to C++'s compile-time template instantiation requirements
    // See GitHub issue #16 review comment for discussion
    return core::make_ga(operators::TournamentSelection{cfg.operators.selection.tournament_size},
                         operators::PMXCrossover{}, operators::SwapMutation{});
}

/// Create TSP GA with Edge Assembly Crossover (EAX)
inline auto make_tsp_ga_eax_from_config(const config::Config& cfg) {
    return core::make_ga(operators::TournamentSelection{cfg.operators.selection.tournament_size},
                         operators::EdgeRecombinationCrossover{}, operators::SwapMutation{});
}

/// Create TSP GA with Order Crossover (OX)
inline auto make_tsp_ga_ox_from_config(const config::Config& cfg) {
    return core::make_ga(operators::TournamentSelection{cfg.operators.selection.tournament_size},
                         operators::OrderCrossover{}, operators::SwapMutation{});
}

/// Create a TSP GA with local search from configuration
inline auto make_tsp_ga_with_local_search_from_config(const config::Config& cfg) {
    // LIMITATION: Always uses PMX crossover regardless of config.operators.crossover.type
    // This is due to C++'s compile-time template instantiation requirements
    // Use create_tsp_ga_dynamic_from_config for runtime operator selection
    return core::make_ga(
        operators::TournamentSelection{cfg.operators.selection.tournament_size},
        operators::PMXCrossover{}, operators::SwapMutation{},
        local_search::TwoOpt{cfg.local_search.first_improvement, cfg.local_search.max_iterations});
}

/// Create TSP GA with local search, using EAX crossover
inline auto make_tsp_ga_eax_with_local_search_from_config(const config::Config& cfg) {
    return core::make_ga(
        operators::TournamentSelection{cfg.operators.selection.tournament_size},
        operators::EdgeRecombinationCrossover{}, operators::SwapMutation{},
        local_search::TwoOpt{cfg.local_search.first_improvement, cfg.local_search.max_iterations});
}

/// Create TSP GA with local search, using OX crossover
inline auto make_tsp_ga_ox_with_local_search_from_config(const config::Config& cfg) {
    return core::make_ga(
        operators::TournamentSelection{cfg.operators.selection.tournament_size},
        operators::OrderCrossover{}, operators::SwapMutation{},
        local_search::TwoOpt{cfg.local_search.first_improvement, cfg.local_search.max_iterations});
}

/// Create UCB scheduler from configuration for a specific problem type
template <typename Problem>
inline auto make_ucb_scheduler_from_config(const config::Config& cfg) {
    return schedulers::UCBOperatorSelector<Problem>(cfg.scheduler.operators.size(),
                                                    cfg.scheduler.exploration_rate);
}

/// Create Thompson sampling scheduler from configuration
template <typename Problem>
inline auto make_thompson_scheduler_from_config(const config::Config& cfg) {
    return schedulers::ThompsonOperatorSelector<Problem>(cfg.scheduler.operators.size(),
                                                         0.0 // Default reward threshold
    );
}

} // namespace factory

} // namespace evolab
