#pragma once

#include <compare>
#include <concepts>
#include <random>
#include <span>
#include <utility>
#include <vector>

namespace evolab::core {

/// Fitness value for optimization problems
struct Fitness {
    double value;

    constexpr Fitness() : value(0.0) {} // Initialize to deterministic value
    constexpr explicit Fitness(double v) : value(v) {}

    constexpr auto operator<=>(const Fitness&) const = default;

    constexpr Fitness& operator+=(const Fitness& other) {
        value += other.value;
        return *this;
    }
    constexpr Fitness& operator*=(double factor) {
        value *= factor;
        return *this;
    }
};

/// Generic genome representation
template <typename Gene>
using Genome = std::vector<Gene>;

/// Concept for optimization problems
template <typename P>
concept Problem = requires(const P& problem, const typename P::GenomeT& genome) {
    typename P::Gene;
    typename P::GenomeT;

    // Must be able to evaluate a genome
    { problem.evaluate(genome) } -> std::convertible_to<Fitness>;

    // Must be able to generate a random genome
    { problem.random_genome(std::declval<std::mt19937&>()) } -> std::same_as<typename P::GenomeT>;

    // Must provide problem size/dimension
    { problem.size() } -> std::convertible_to<std::size_t>;
};

/// Concept for genetic operators
template <typename Op, typename P>
concept GeneticOperator =
    Problem<P> && requires(Op& op, const P& problem, const typename P::GenomeT& parent1,
                           const typename P::GenomeT& parent2, std::mt19937& rng) {
        // Selection operators
        typename Op::is_selection_operator;

        // Crossover operators
        typename Op::is_crossover_operator;

        // Mutation operators
        typename Op::is_mutation_operator;
    };

/// Concept for selection operators in genetic algorithms
///
/// This concept defines the interface for selection operators that choose parents
/// for reproduction based on fitness values. Selection operators receive a span
/// of fitness values and return the index of the selected individual.
///
/// @tparam S The selection operator type
/// @tparam P The problem type (must satisfy Problem concept)
///
/// ## API Change Notice (v2.0+)
/// **Breaking Change**: The selection interface has been modernized to use `std::span<const
/// Fitness>` instead of accepting full population objects. This change provides significant
/// performance improvements and better separation of concerns.
///
/// ### Migration from v1.x:
/// ```cpp
/// // v1.x (deprecated)
/// class OldSelection {
/// public:
///     std::size_t select(const Population<GenomeT>& population, std::mt19937& rng) const {
///         auto fitnesses = population.fitness_values(); // Extract fitness span
///         // ... selection logic
///     }
/// };
///
/// // v2.0+ (current)
/// class NewSelection {
/// public:
///     std::size_t select(std::span<const Fitness> fitnesses, std::mt19937& rng) const {
///         // ... same selection logic, fitness span provided directly
///     }
/// };
/// ```
///
/// ### Benefits of the new interface:
/// - **Performance**: Zero-copy access to fitness values via std::span
/// - **Cache efficiency**: Improved memory access patterns for selection operations
/// - **Separation of concerns**: Selection operators only need fitness values, not full genomes
/// - **Modern C++**: Leverages C++20/23 span features for type-safe array access
/// - **Testability**: Easier to unit test with simple fitness arrays
///
/// ## Requirements:
/// - `selector.select(fitnesses, rng)` must return a valid index (0 <= index < fitnesses.size())
/// - Must not modify the fitness values (const span)
/// - Should be deterministic given the same RNG state
/// - Precondition: fitnesses.size() >= 1 (empty spans are invalid input)
///
/// @see TournamentSelection, RouletteWheelSelection, RankSelection for examples
/// @since v2.0.0 (API breaking change from v1.x)
template <typename S, typename P>
concept SelectionOperator =
    Problem<P> &&
    requires(const S& selector, std::span<const Fitness> fitnesses, std::mt19937& rng) {
        // Select parents for reproduction using fitness-based selection
        // Returns index of selected individual within the provided fitness span
        // (index corresponds to position in the span, not global population position)
        { selector.select(fitnesses, rng) } -> std::same_as<std::size_t>;
    };

/// Concept for crossover operators
template <typename C, typename P>
concept CrossoverOperator =
    Problem<P> && requires(const C& crossover, const P& problem, const typename P::GenomeT& parent1,
                           const typename P::GenomeT& parent2, std::mt19937& rng) {
        // Produce offspring from two parents
        {
            crossover.cross(problem, parent1, parent2, rng)
        } -> std::same_as<std::pair<typename P::GenomeT, typename P::GenomeT>>;
    };

/// Concept for mutation operators
template <typename M, typename P>
concept MutationOperator = Problem<P> && requires(const M& mutator, const P& problem,
                                                  typename P::GenomeT& genome, std::mt19937& rng) {
    // Mutate a genome in place
    { mutator.mutate(problem, genome, rng) } -> std::same_as<void>;
};

/// Concept for local search operators
template <typename L, typename P>
concept LocalSearchOperator =
    Problem<P> && requires(const L& local_search, const P& problem, typename P::GenomeT& genome,
                           std::mt19937& rng) {
        // Improve a genome using local search
        { local_search.improve(problem, genome, rng) } -> std::same_as<Fitness>;
    };

/// Concept for repair operators
template <typename R, typename P>
concept RepairOperator =
    Problem<P> && requires(const R& repair, const P& problem, typename P::GenomeT& genome) {
        // Repair an invalid genome to make it valid
        { repair.repair(problem, genome) } -> std::same_as<void>;
    };

} // namespace evolab::core
