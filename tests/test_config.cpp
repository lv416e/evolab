#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <evolab/config/config.hpp>

#include "test_helper.hpp"

using namespace evolab::config;

// Helper function to create temporary TOML file for testing
std::filesystem::path create_temp_toml(const std::string& content) {
    auto temp_path = std::filesystem::temp_directory_path() / "test_config.toml";
    std::ofstream file(temp_path);
    file << content;
    file.close();
    return temp_path;
}

void test_basic_ga_config() {
    TestResult result;

    // Test basic GA configuration parsing
    const std::string toml_content = R"(
        [ga]
        population_size = 100
        max_generations = 500
        elite_rate = 0.02
        seed = 42
    )";

    auto temp_file = create_temp_toml(toml_content);
    auto config = Config::from_file(temp_file.string());

    result.assert_eq(static_cast<size_t>(100), config.ga.population_size, "GA population size");
    result.assert_eq(static_cast<size_t>(500), config.ga.max_generations, "GA max generations");
    result.assert_eq(0.02, config.ga.elite_rate, "GA elite rate");
    result.assert_eq(static_cast<size_t>(42), static_cast<size_t>(config.ga.seed), "GA seed");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_operators_config() {
    TestResult result;

    const std::string toml_content = R"(
        [operators]
        crossover = { type = "EAX", probability = 0.9 }
        mutation = { type = "inversion", probability = 0.05 }
        selection = { type = "tournament", tournament_size = 5 }
    )";

    auto temp_file = create_temp_toml(toml_content);
    auto config = Config::from_file(temp_file.string());

    result.assert_eq(std::string("EAX"), config.operators.crossover.type, "Crossover type");
    result.assert_eq(0.9, config.operators.crossover.probability, "Crossover probability");
    result.assert_eq(std::string("inversion"), config.operators.mutation.type, "Mutation type");
    result.assert_eq(0.05, config.operators.mutation.probability, "Mutation probability");
    result.assert_eq(std::string("tournament"), config.operators.selection.type, "Selection type");
    result.assert_eq(static_cast<size_t>(5), config.operators.selection.tournament_size,
                     "Tournament size");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_local_search_config() {
    TestResult result;

    const std::string toml_content = R"(
        [local_search]
        enabled = true
        type = "2-opt"
        max_iterations = 100
        probability = 0.3
        candidate_list_size = 40
    )";

    auto temp_file = create_temp_toml(toml_content);
    auto config = Config::from_file(temp_file.string());

    result.assert_true(config.local_search.enabled, "Local search enabled");
    result.assert_eq(std::string("2-opt"), config.local_search.type, "Local search type");
    result.assert_eq(static_cast<size_t>(100), config.local_search.max_iterations,
                     "Local search iterations");
    result.assert_eq(0.3, config.local_search.probability, "Local search probability");
    result.assert_eq(static_cast<size_t>(40), config.local_search.candidate_list_size,
                     "Candidate list size");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_scheduler_config() {
    TestResult result;

    const std::string toml_content = R"(
        [scheduler]
        enabled = true
        type = "thompson"
        operators = ["EAX", "PMX", "OX"]
        window_size = 100
        exploration_rate = 2.0
    )";

    auto temp_file = create_temp_toml(toml_content);
    auto config = Config::from_file(temp_file.string());

    result.assert_true(config.scheduler.enabled, "Scheduler enabled");
    result.assert_eq(std::string("thompson"), config.scheduler.type, "Scheduler type");
    result.assert_eq(static_cast<size_t>(3), config.scheduler.operators.size(),
                     "Number of operators");
    result.assert_eq(std::string("EAX"), config.scheduler.operators[0], "First operator");
    result.assert_eq(std::string("PMX"), config.scheduler.operators[1], "Second operator");
    result.assert_eq(std::string("OX"), config.scheduler.operators[2], "Third operator");
    result.assert_eq(static_cast<size_t>(100), config.scheduler.window_size, "Window size");
    result.assert_eq(2.0, config.scheduler.exploration_rate, "Exploration rate");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_validation_population_size() {
    TestResult result;

    const std::string toml_content = R"(
        [ga]
        population_size = 0
        max_generations = 100
    )";

    auto temp_file = create_temp_toml(toml_content);

    try {
        auto config = Config::from_file(temp_file.string());
        result.assert_true(false, "Should throw exception for zero population size");
    } catch (const ConfigValidationError& e) {
        result.assert_true(true, "Correctly threw exception for zero population size");
    }

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_validation_probabilities() {
    TestResult result;

    const std::string toml_content = R"(
        [operators]
        crossover = { type = "PMX", probability = 1.5 }
    )";

    auto temp_file = create_temp_toml(toml_content);

    try {
        auto config = Config::from_file(temp_file.string());
        result.assert_true(false, "Should throw exception for probability > 1.0");
    } catch (const ConfigValidationError& e) {
        result.assert_true(true, "Correctly threw exception for invalid probability");
    }

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_complete_config() {
    TestResult result;

    const std::string toml_content = R"(
        [ga]
        population_size = 256
        max_generations = 1000
        elite_rate = 0.05
        seed = 2023
        
        [operators]
        crossover = { type = "EAX", probability = 0.9 }
        mutation = { type = "adaptive", probability = 0.02 }
        selection = { type = "ranking", selection_pressure = 1.8 }
        
        [local_search]
        enabled = true
        type = "2-opt"
        max_iterations = 200
        probability = 0.4
        
        [termination]
        max_generations = 1000
        time_limit_minutes = 30
        stagnation_generations = 100
        target_fitness = 0.0
        
        [logging]
        log_interval = 25
        verbose = true
        track_diversity = true
        save_evolution_curve = true
    )";

    auto temp_file = create_temp_toml(toml_content);
    auto config = Config::from_file(temp_file.string());

    // GA configuration
    result.assert_eq(static_cast<size_t>(256), config.ga.population_size, "GA population size");
    result.assert_eq(static_cast<size_t>(1000), config.ga.max_generations, "GA max generations");
    result.assert_eq(0.05, config.ga.elite_rate, "GA elite rate");
    result.assert_eq(static_cast<size_t>(2023), static_cast<size_t>(config.ga.seed), "GA seed");

    // Operators
    result.assert_eq(std::string("EAX"), config.operators.crossover.type, "Crossover type");
    result.assert_eq(std::string("adaptive"), config.operators.mutation.type, "Mutation type");
    result.assert_eq(1.8, config.operators.selection.selection_pressure, "Selection pressure");

    // Local search
    result.assert_true(config.local_search.enabled, "Local search enabled");
    result.assert_eq(std::string("2-opt"), config.local_search.type, "Local search type");

    // Termination
    result.assert_eq(static_cast<size_t>(1000), config.termination.max_generations,
                     "Termination max generations");
    result.assert_eq(30.0, config.termination.time_limit_minutes, "Time limit minutes");
    result.assert_eq(static_cast<size_t>(100), config.termination.stagnation_generations,
                     "Stagnation generations");

    // Logging
    result.assert_true(config.logging.verbose, "Verbose logging");
    result.assert_true(config.logging.track_diversity, "Track diversity");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_defaults() {
    TestResult result;

    const std::string toml_content = R"(
        [ga]
        population_size = 100
    )";

    auto temp_file = create_temp_toml(toml_content);
    auto config = Config::from_file(temp_file.string());

    // Should have sensible defaults for missing values
    result.assert_eq(static_cast<size_t>(100), config.ga.population_size,
                     "Specified population size");
    result.assert_eq(static_cast<size_t>(1000), config.ga.max_generations,
                     "Default max generations");
    result.assert_eq(0.02, config.ga.elite_rate, "Default elite rate");
    result.assert_gt(static_cast<size_t>(config.ga.seed), static_cast<size_t>(0),
                     "Default seed > 0");

    // Default operators should be set
    result.assert_true(!config.operators.crossover.type.empty(),
                       "Default crossover type not empty");
    result.assert_true(config.operators.crossover.probability > 0.0,
                       "Default crossover probability > 0");
    result.assert_true(config.operators.crossover.probability <= 1.0,
                       "Default crossover probability <= 1");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

int main() {
    std::cout << "=== Configuration System Tests ===\n\n";

    std::cout << "Test: Basic GA Configuration\n";
    test_basic_ga_config();

    std::cout << "\nTest: Operators Configuration\n";
    test_operators_config();

    std::cout << "\nTest: Local Search Configuration\n";
    test_local_search_config();

    std::cout << "\nTest: Scheduler Configuration\n";
    test_scheduler_config();

    std::cout << "\nTest: Validation - Population Size\n";
    test_validation_population_size();

    std::cout << "\nTest: Validation - Probabilities\n";
    test_validation_probabilities();

    std::cout << "\nTest: Complete Configuration\n";
    test_complete_config();

    std::cout << "\nTest: Configuration Defaults\n";
    test_defaults();

    std::cout << "\n=== All Configuration Tests Completed ===\n";
    return 0;
}