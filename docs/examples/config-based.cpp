/**
 * @file config-based.cpp
 * @brief Configuration-driven genetic algorithm using TOML files
 *
 * This example demonstrates how to use TOML configuration files to set up
 * complex genetic algorithms with different operators, local search, and
 * termination criteria without recompiling code.
 *
 * Compile with:
 *   g++ -std=c++23 -I../../include config-based.cpp -o config-based
 *
 * Run with:
 *   ./config-based example-config.toml
 */

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include <evolab/evolab.hpp>

using namespace evolab;

// Create a sample TOML configuration file
void create_sample_config(const std::string& filename) {
    std::ofstream config_file(filename);
    if (!config_file) {
        throw std::runtime_error("Cannot create config file: " + filename);
    }

    config_file << R"(
# EvoLab TSP Genetic Algorithm Configuration
# This TOML file configures all aspects of the genetic algorithm

[ga]
population_size = 256
max_generations = 1000
seed = 42

[termination]
max_generations = 1000
stagnation_generations = 100
target_fitness = 0.0  # Set to problem-specific target if known
max_evaluations = 0   # 0 means no limit

[operators.selection]
type = "tournament"
tournament_size = 7

[operators.crossover]
type = "OX"           # Order Crossover (alternatives: PMX, EAX)
probability = 0.9

[operators.mutation]
type = "swap"         # Swap mutation
probability = 0.1

[local_search]
enabled = true
type = "2opt"
first_improvement = true
max_iterations = 1000

[logging]
verbose = true
log_interval = 50     # Log every N generations
save_history = true

[scheduler]
operators = ["PMX", "OX", "EAX"]  # Multi-armed bandit operator selection
exploration_rate = 0.1
)";

    std::cout << "Created sample configuration file: " << filename << "\n";
}

void print_config_summary(const config::Config& cfg) {
    std::cout << "Configuration Summary:\n";
    std::cout << "=====================\n";
    std::cout << "Population size:    " << cfg.ga.population_size << "\n";
    std::cout << "Max generations:    " << cfg.ga.max_generations << "\n";
    std::cout << "Random seed:        " << cfg.ga.seed << "\n";
    std::cout << "Selection:          " << cfg.operators.selection.type << " (size "
              << cfg.operators.selection.tournament_size << ")\n";
    std::cout << "Crossover:          " << cfg.operators.crossover.type << " (prob "
              << cfg.operators.crossover.probability << ")\n";
    std::cout << "Mutation:           " << cfg.operators.mutation.type << " (prob "
              << cfg.operators.mutation.probability << ")\n";
    std::cout << "Local search:       " << (cfg.local_search.enabled ? "Enabled" : "Disabled");
    if (cfg.local_search.enabled) {
        std::cout << " (" << cfg.local_search.type << ")";
    }
    std::cout << "\n";
    std::cout << "Verbose logging:    " << (cfg.logging.verbose ? "Yes" : "No") << "\n\n";
}

int main(int argc, char** argv) {
    std::cout << "EvoLab Configuration-Based Example\n";
    std::cout << "==================================\n\n";

    std::string config_filename;

    if (argc < 2) {
        // No config file provided, create a sample one
        config_filename = "example-config.toml";
        std::cout << "No configuration file provided. Creating sample config...\n\n";
        try {
            create_sample_config(config_filename);
        } catch (const std::exception& e) {
            std::cerr << "Error creating config file: " << e.what() << "\n";
            return 1;
        }
    } else {
        config_filename = argv[1];
    }

    // Load configuration from TOML file
    std::cout << "Loading configuration from: " << config_filename << "\n\n";

    config::Config cfg;
    try {
        cfg = config::Config::from_file(config_filename);
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << "\n";
        return 1;
    }

    print_config_summary(cfg);

    // Create TSP problem
    auto tsp = problems::create_random_tsp(50, 100.0, cfg.ga.seed);
    std::cout << "Created TSP instance with " << tsp.num_cities() << " cities\n\n";

    // Create GA based on configuration
    // Note: Current implementation has compile-time operator selection limitations
    // See factory function comments for details about operator type selection
    std::cout << "Creating genetic algorithm from configuration...\n";

    auto ga = [&cfg]() {
        const std::string& crossover_type = cfg.operators.crossover.type;

        if (cfg.local_search.enabled) {
            // With local search
            if (crossover_type == "EAX") {
                return factory::make_tsp_ga_eax_with_local_search_from_config(cfg);
            } else if (crossover_type == "OX") {
                return factory::make_tsp_ga_ox_with_local_search_from_config(cfg);
            } else {
                // Default to PMX
                return factory::make_tsp_ga_with_local_search_from_config(cfg);
            }
        } else {
            // Without local search
            if (crossover_type == "EAX") {
                return factory::make_tsp_ga_eax_from_config(cfg);
            } else if (crossover_type == "OX") {
                return factory::make_tsp_ga_ox_from_config(cfg);
            } else {
                // Default to PMX
                return factory::make_tsp_ga_from_config(cfg);
            }
        }
    }();

    // Convert configuration to GA config
    auto ga_config = cfg.to_ga_config();

    std::cout << "Starting evolution with configured parameters...\n\n";

    // Run the genetic algorithm
    auto start_time = std::chrono::high_resolution_clock::now();
    auto result = ga.run(tsp, ga_config);
    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration<double>(end_time - start_time).count();

    // Display results
    std::cout << "Evolution Results:\n";
    std::cout << "=================\n";
    std::cout << "Best fitness:     " << std::fixed << std::setprecision(2)
              << result.best_fitness.value << "\n";
    std::cout << "Generations:      " << result.generations << "\n";
    std::cout << "Evaluations:      " << result.evaluations << "\n";
    std::cout << "Runtime:          " << std::fixed << std::setprecision(3) << duration
              << " seconds\n";
    std::cout << "Converged:        " << (result.converged ? "Yes" : "No") << "\n";

    // Verify solution
    if (tsp.is_valid_tour(result.best_genome)) {
        std::cout << "Solution valid:   Yes ✓\n";
    } else {
        std::cout << "Solution valid:   No ✗\n";
        return 1;
    }

    // Show evolution history if verbose logging was enabled
    if (cfg.logging.verbose && cfg.logging.save_history && !result.history.empty()) {
        std::cout << "\nEvolution History (last 10 generations):\n";
        std::cout << "========================================\n";
        std::cout << std::setw(10) << "Gen" << std::setw(15) << "Best Fitness" << std::setw(15)
                  << "Mean Fitness" << std::setw(12) << "Time(ms)\n";
        std::cout << std::string(52, '-') << "\n";

        size_t start_idx = result.history.size() > 10 ? result.history.size() - 10 : 0;
        for (size_t i = start_idx; i < result.history.size(); ++i) {
            const auto& stats = result.history[i];
            std::cout << std::setw(10) << stats.generation << std::setw(15) << std::fixed
                      << std::setprecision(2) << stats.best_fitness.value << std::setw(15)
                      << std::fixed << std::setprecision(2) << stats.mean_fitness.value
                      << std::setw(12) << stats.elapsed_time.count() << "\n";
        }
    }

    std::cout << "\nConfiguration-based evolution completed successfully!\n";
    std::cout << "\nTo experiment with different settings:\n";
    std::cout << "1. Edit " << config_filename << "\n";
    std::cout << "2. Run: ./config-based " << config_filename << "\n";

    return 0;
}