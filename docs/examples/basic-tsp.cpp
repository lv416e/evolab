/**
 * @file basic-tsp.cpp
 * @brief Basic TSP solver using EvoLab genetic algorithm
 *
 * This example demonstrates how to solve a Traveling Salesman Problem using
 * EvoLab's basic genetic algorithm with tournament selection, order crossover,
 * and 2-opt local search.
 *
 * Compile with:
 *   g++ -std=c++23 -I../../include basic-tsp.cpp -o basic-tsp
 *
 * Run with:
 *   ./basic-tsp
 */

#include <chrono>
#include <iomanip>
#include <iostream>

#include <evolab/evolab.hpp>

using namespace evolab;

int main() {
    std::cout << "EvoLab Basic TSP Example\n";
    std::cout << "========================\n\n";

    // Create a random TSP instance with 30 cities
    // Parameters: num_cities, coordinate_range, random_seed
    auto tsp = problems::create_random_tsp(30, 100.0, 42);

    std::cout << "Problem: TSP with " << tsp.num_cities() << " cities\n";
    std::cout << "Coordinate range: [0, 100]\n\n";

    // Create basic genetic algorithm
    // This uses tournament selection, order crossover, swap mutation, and 2-opt local search
    auto ga = factory::make_tsp_ga_basic();

    // Configure algorithm parameters
    core::GAConfig config{
        .population_size = 100, // Population size
        .max_generations = 500, // Maximum generations
        .crossover_prob = 0.9,  // Crossover probability
        .mutation_prob = 0.1,   // Mutation probability
        .seed = 123             // Random seed for reproducibility
    };

    std::cout << "Algorithm Configuration:\n";
    std::cout << "  Population size: " << config.population_size << "\n";
    std::cout << "  Max generations: " << config.max_generations << "\n";
    std::cout << "  Crossover prob:  " << config.crossover_prob << "\n";
    std::cout << "  Mutation prob:   " << config.mutation_prob << "\n";
    std::cout << "  Random seed:     " << config.seed << "\n\n";

    // Run the genetic algorithm
    std::cout << "Running evolution...\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    auto result = ga.run(tsp, config);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end_time - start_time).count();

    // Display results
    std::cout << "\nResults:\n";
    std::cout << "========\n";
    std::cout << "Best fitness: " << std::fixed << std::setprecision(2) << result.best_fitness.value
              << "\n";
    std::cout << "Generations:  " << result.generations << "\n";
    std::cout << "Evaluations:  " << result.evaluations << "\n";
    std::cout << "Runtime:      " << std::fixed << std::setprecision(3) << duration << " seconds\n";
    std::cout << "Converged:    " << (result.converged ? "Yes" : "No") << "\n";

    // Verify solution validity
    if (tsp.is_valid_tour(result.best_genome)) {
        std::cout << "Solution:     Valid tour ✓\n";
    } else {
        std::cout << "Solution:     Invalid tour ✗\n";
        return 1;
    }

    // Show first few cities of the best tour
    std::cout << "\nBest tour (first 10 cities): ";
    for (size_t i = 0; i < std::min(size_t(10), result.best_genome.size()); ++i) {
        std::cout << result.best_genome[i];
        if (i < std::min(size_t(9), result.best_genome.size() - 1))
            std::cout << " → ";
    }
    if (result.best_genome.size() > 10) {
        std::cout << " → ...";
    }
    std::cout << "\n";

    std::cout << "\nExample completed successfully!\n";
    return 0;
}