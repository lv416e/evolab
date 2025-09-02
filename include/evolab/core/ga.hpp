#pragma once

#include <algorithm>
#include <chrono>
#include <optional>
#include <random>
#include <vector>

#include "concepts.hpp"

namespace evolab::core {

/// Configuration for genetic algorithm
struct GAConfig {
    std::size_t population_size = 256;
    std::size_t max_generations = 5000;
    std::size_t max_evaluations = 0;         // 0 means unlimited
    std::chrono::milliseconds time_limit{0}; // 0 means no limit

    double crossover_prob = 0.9;
    double mutation_prob = 0.2;
    double elite_ratio = 0.02;

    std::uint64_t seed = 1;

    // Diversity and restart parameters
    double diversity_threshold = 0.01;
    std::size_t stagnation_limit = 100;

    // Logging and checkpoint
    std::size_t log_interval = 10;
    bool enable_checkpoints = false;
    std::string checkpoint_path;
};

/// Statistics for a single generation
struct GenerationStats {
    std::size_t generation;
    Fitness best_fitness;
    Fitness mean_fitness;
    Fitness worst_fitness;
    double diversity;
    std::chrono::milliseconds elapsed_time;
};

/// Result of genetic algorithm run
template <typename GenomeT>
struct GAResult {
    GenomeT best_genome;
    Fitness best_fitness;
    std::size_t generations;
    std::size_t evaluations;
    std::chrono::milliseconds total_time;
    std::vector<GenerationStats> history;
    bool converged = false;
};

/// Main genetic algorithm implementation
template <SelectionOperator Selection, CrossoverOperator Crossover, MutationOperator Mutation,
          LocalSearchOperator LocalSearch = void*, RepairOperator Repair = void*>
class GeneticAlgorithm {
  public:
    using SelectionT = Selection;
    using CrossoverT = Crossover;
    using MutationT = Mutation;
    using LocalSearchT = LocalSearch;
    using RepairT = Repair;

  private:
    Selection selection_;
    Crossover crossover_;
    Mutation mutation_;

    std::conditional_t<std::same_as<LocalSearch, void*>, std::monostate, LocalSearch> local_search_;
    std::conditional_t<std::same_as<Repair, void*>, std::monostate, Repair> repair_;

    mutable std::mt19937 rng_;

  public:
    GeneticAlgorithm(Selection sel, Crossover cross, Mutation mut)
        : selection_(std::move(sel)), crossover_(std::move(cross)), mutation_(std::move(mut)) {}

    GeneticAlgorithm(Selection sel, Crossover cross, Mutation mut, LocalSearch ls)
        : selection_(std::move(sel)), crossover_(std::move(cross)), mutation_(std::move(mut)),
          local_search_(std::move(ls)) {}

    GeneticAlgorithm(Selection sel, Crossover cross, Mutation mut, LocalSearch ls, Repair rep)
        : selection_(std::move(sel)), crossover_(std::move(cross)), mutation_(std::move(mut)),
          local_search_(std::move(ls)), repair_(std::move(rep)) {}

    /// Run genetic algorithm on the given problem
    template <Problem P>
    GAResult<typename P::GenomeT> run(const P& problem, const GAConfig& config = {}) {
        using GenomeT = typename P::GenomeT;

        rng_.seed(config.seed);

        const auto start_time = std::chrono::steady_clock::now();

        // Initialize population
        std::vector<GenomeT> population;
        std::vector<Fitness> fitnesses;
        population.reserve(config.population_size);
        fitnesses.reserve(config.population_size);

        for (std::size_t i = 0; i < config.population_size; ++i) {
            auto genome = problem.random_genome(rng_);
            repair_if_available(problem, genome);

            population.push_back(std::move(genome));
            fitnesses.push_back(problem.evaluate(population.back()));
        }

        // Track best solution
        auto best_idx = std::min_element(fitnesses.begin(), fitnesses.end()) - fitnesses.begin();
        auto best_genome = population[best_idx];
        auto best_fitness = fitnesses[best_idx];

        GAResult<GenomeT> result;
        result.history.reserve(config.max_generations);

        std::size_t evaluations = config.population_size;
        std::size_t stagnation_count = 0;

        for (std::size_t gen = 0; gen < config.max_generations; ++gen) {
            // Check termination conditions
            const auto current_time = std::chrono::steady_clock::now();
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);

            if (config.time_limit.count() > 0 && elapsed >= config.time_limit)
                break;
            if (config.max_evaluations > 0 && evaluations >= config.max_evaluations)
                break;

            // Create next generation
            std::vector<GenomeT> new_population;
            std::vector<Fitness> new_fitnesses;
            new_population.reserve(config.population_size);
            new_fitnesses.reserve(config.population_size);

            // Elite preservation
            const std::size_t elite_count =
                static_cast<std::size_t>(config.elite_ratio * config.population_size);
            if (elite_count > 0) {
                // Sort by fitness and keep elite
                std::vector<std::size_t> indices(population.size());
                std::iota(indices.begin(), indices.end(), 0);
                std::sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
                    return fitnesses[a] < fitnesses[b];
                });

                for (std::size_t i = 0; i < elite_count && i < indices.size(); ++i) {
                    new_population.push_back(population[indices[i]]);
                    new_fitnesses.push_back(fitnesses[indices[i]]);
                }
            }

            // Generate offspring
            while (new_population.size() < config.population_size) {
                // Selection
                auto parent1_idx = selection_.select(population, fitnesses, rng_);
                auto parent2_idx = selection_.select(population, fitnesses, rng_);

                auto offspring = population[parent1_idx];

                // Crossover
                if (std::uniform_real_distribution<>(0.0, 1.0)(rng_) < config.crossover_prob) {
                    auto [child1, child2] = crossover_.cross(problem, population[parent1_idx],
                                                             population[parent2_idx], rng_);
                    offspring = std::move(child1);

                    // Add second child if there's room
                    if (new_population.size() + 1 < config.population_size) {
                        // Mutation
                        if (std::uniform_real_distribution<>(0.0, 1.0)(rng_) <
                            config.mutation_prob) {
                            mutation_.mutate(problem, child2, rng_);
                        }
                        repair_if_available(problem, child2);

                        auto fitness2 = problem.evaluate(child2);
                        evaluations++;

                        // Local search if available
                        if constexpr (!std::same_as<LocalSearch, void*>) {
                            fitness2 = local_search_.improve(problem, child2, rng_);
                            evaluations++;
                        }

                        new_population.push_back(std::move(child2));
                        new_fitnesses.push_back(fitness2);
                    }
                }

                // Mutation
                if (std::uniform_real_distribution<>(0.0, 1.0)(rng_) < config.mutation_prob) {
                    mutation_.mutate(problem, offspring, rng_);
                }
                repair_if_available(problem, offspring);

                auto fitness = problem.evaluate(offspring);
                evaluations++;

                // Local search if available
                if constexpr (!std::same_as<LocalSearch, void*>) {
                    fitness = local_search_.improve(problem, offspring, rng_);
                    evaluations++;
                }

                new_population.push_back(std::move(offspring));
                new_fitnesses.push_back(fitness);
            }

            // Trim to exact population size
            if (new_population.size() > config.population_size) {
                new_population.resize(config.population_size);
                new_fitnesses.resize(config.population_size);
            }

            population = std::move(new_population);
            fitnesses = std::move(new_fitnesses);

            // Update best solution
            auto gen_best_idx =
                std::min_element(fitnesses.begin(), fitnesses.end()) - fitnesses.begin();
            if (fitnesses[gen_best_idx] < best_fitness) {
                best_genome = population[gen_best_idx];
                best_fitness = fitnesses[gen_best_idx];
                stagnation_count = 0;
            } else {
                stagnation_count++;
            }

            // Log statistics
            if (gen % config.log_interval == 0) {
                GenerationStats stats;
                stats.generation = gen;
                stats.best_fitness = best_fitness;
                stats.mean_fitness = Fitness(
                    std::accumulate(fitnesses.begin(), fitnesses.end(), 0.0,
                                    [](double sum, const Fitness& f) { return sum + f.value; }) /
                    fitnesses.size());
                stats.worst_fitness = *std::max_element(fitnesses.begin(), fitnesses.end());
                stats.diversity = calculate_diversity(population);
                stats.elapsed_time = elapsed;

                result.history.push_back(stats);
            }

            // Check convergence
            if (stagnation_count >= config.stagnation_limit) {
                result.converged = true;
                break;
            }
        }

        const auto end_time = std::chrono::steady_clock::now();

        result.best_genome = std::move(best_genome);
        result.best_fitness = best_fitness;
        result.generations = result.history.empty() ? 0 : result.history.back().generation;
        result.evaluations = evaluations;
        result.total_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        return result;
    }

  private:
    template <Problem P>
    void repair_if_available(const P& problem, typename P::GenomeT& genome) {
        if constexpr (!std::same_as<Repair, void*>) {
            repair_.repair(problem, genome);
        }
    }

    template <typename GenomeT>
    double calculate_diversity(const std::vector<GenomeT>& population) {
        // Simple diversity measure - can be specialized per problem type
        if (population.size() < 2)
            return 0.0;

        double total_distance = 0.0;
        std::size_t pairs = 0;

        for (std::size_t i = 0; i < population.size(); ++i) {
            for (std::size_t j = i + 1; j < population.size(); ++j) {
                // Hamming distance for now - should be specialized
                double distance = 0.0;
                for (std::size_t k = 0; k < std::min(population[i].size(), population[j].size());
                     ++k) {
                    if (population[i][k] != population[j][k]) {
                        distance += 1.0;
                    }
                }
                total_distance += distance;
                pairs++;
            }
        }

        return pairs > 0 ? total_distance / pairs : 0.0;
    }
};

/// Factory function for creating genetic algorithms
template <SelectionOperator Selection, CrossoverOperator Crossover, MutationOperator Mutation>
auto make_ga(Selection sel, Crossover cross, Mutation mut) {
    return GeneticAlgorithm<Selection, Crossover, Mutation>(std::move(sel), std::move(cross),
                                                            std::move(mut));
}

template <SelectionOperator Selection, CrossoverOperator Crossover, MutationOperator Mutation,
          LocalSearchOperator LocalSearch>
auto make_ga(Selection sel, Crossover cross, Mutation mut, LocalSearch ls) {
    return GeneticAlgorithm<Selection, Crossover, Mutation, LocalSearch>(
        std::move(sel), std::move(cross), std::move(mut), std::move(ls));
}

} // namespace evolab::core
