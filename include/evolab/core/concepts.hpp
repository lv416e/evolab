#pragma once

#include <vector>
#include <concepts>
#include <random>

namespace evolab::core {

/// Fitness value for optimization problems
struct Fitness {
    double value;
    
    Fitness() = default;
    explicit Fitness(double v) : value(v) {}
    
    auto operator<=>(const Fitness&) const = default;
    
    Fitness& operator+=(const Fitness& other) { value += other.value; return *this; }
    Fitness& operator*=(double factor) { value *= factor; return *this; }
};

/// Generic genome representation
template<typename Gene>
using Genome = std::vector<Gene>;

/// Concept for optimization problems
template<typename P>
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
template<typename Op, typename P>
concept GeneticOperator = Problem<P> && requires(
    Op& op,
    const P& problem,
    const typename P::GenomeT& parent1,
    const typename P::GenomeT& parent2,
    std::mt19937& rng) {
    
    // Selection operators
    typename Op::is_selection_operator;
    
    // Crossover operators  
    typename Op::is_crossover_operator;
    
    // Mutation operators
    typename Op::is_mutation_operator;
};

/// Concept for selection operators
template<typename S, typename P>
concept SelectionOperator = Problem<P> && requires(
    const S& selector,
    const std::vector<typename P::GenomeT>& population,
    const std::vector<Fitness>& fitnesses,
    std::mt19937& rng) {
    
    // Select parents for reproduction
    { selector.select(population, fitnesses, rng) } -> std::same_as<std::size_t>;
};

/// Concept for crossover operators
template<typename C, typename P>
concept CrossoverOperator = Problem<P> && requires(
    const C& crossover,
    const P& problem,
    const typename P::GenomeT& parent1,
    const typename P::GenomeT& parent2,
    std::mt19937& rng) {
    
    // Produce offspring from two parents
    { crossover.cross(problem, parent1, parent2, rng) } -> std::same_as<std::pair<typename P::GenomeT, typename P::GenomeT>>;
};

/// Concept for mutation operators
template<typename M, typename P>
concept MutationOperator = Problem<P> && requires(
    const M& mutator,
    const P& problem,
    typename P::GenomeT& genome,
    std::mt19937& rng) {
    
    // Mutate a genome in place
    { mutator.mutate(problem, genome, rng) } -> std::same_as<void>;
};

/// Concept for local search operators
template<typename L, typename P>
concept LocalSearchOperator = Problem<P> && requires(
    const L& local_search,
    const P& problem,
    typename P::GenomeT& genome,
    std::mt19937& rng) {
    
    // Improve a genome using local search
    { local_search.improve(problem, genome, rng) } -> std::same_as<Fitness>;
};

/// Concept for repair operators
template<typename R, typename P>
concept RepairOperator = Problem<P> && requires(
    const R& repair,
    const P& problem,
    typename P::GenomeT& genome) {
    
    // Repair an invalid genome to make it valid
    { repair.repair(problem, genome) } -> std::same_as<void>;
};

} // namespace evolab::core