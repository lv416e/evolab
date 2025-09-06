#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <evolab/evolab.hpp>

#include "test_helper.hpp"

using namespace evolab;
using namespace evolab::config;

// Helper function to create temporary TOML file for testing
std::filesystem::path create_test_config(const std::string& content) {
    auto temp_path = std::filesystem::temp_directory_path() / "test_integration_config.toml";
    std::ofstream file(temp_path);
    file << content;
    file.close();
    return temp_path;
}

void test_config_to_ga_config_conversion() {
    TestResult result;

    // Test that Config can be converted to GAConfig
    const std::string toml_content = R"(
        [ga]
        population_size = 128
        max_generations = 2000
        elite_ratio = 0.03
        seed = 123
        
        [operators]
        crossover = { type = "EAX", probability = 0.85 }
        mutation = { type = "inversion", probability = 0.15 }
        
        [termination]
        max_generations = 2000
        stagnation_generations = 150
        time_limit_minutes = 10
    )";

    auto temp_file = create_test_config(toml_content);
    auto config = Config::from_file(temp_file.string());

    // Convert Config to GAConfig
    core::GAConfig ga_config = config.to_ga_config();

    result.assert_eq(static_cast<size_t>(128), ga_config.population_size,
                     "Population size conversion");
    result.assert_eq(static_cast<size_t>(2000), ga_config.max_generations,
                     "Max generations conversion");
    result.assert_eq(0.85, ga_config.crossover_prob, "Crossover probability conversion");
    result.assert_eq(0.15, ga_config.mutation_prob, "Mutation probability conversion");
    result.assert_eq(0.03, ga_config.elite_ratio, "Elite ratio conversion");
    result.assert_eq(static_cast<size_t>(123), static_cast<size_t>(ga_config.seed),
                     "Seed conversion");
    result.assert_eq(static_cast<size_t>(150), ga_config.stagnation_limit,
                     "Stagnation limit conversion");

    // Check time limit conversion (minutes to milliseconds)
    auto expected_time_ms = std::chrono::milliseconds(static_cast<long>(10 * 60 * 1000));
    result.assert_eq(expected_time_ms.count(), ga_config.time_limit.count(),
                     "Time limit conversion");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_ga_with_config_object() {
    TestResult result;

    const std::string toml_content = R"(
        [ga]
        population_size = 64
        max_generations = 100
        seed = 42
        
        [operators]
        crossover = { type = "PMX", probability = 0.9 }
        mutation = { type = "swap", probability = 0.1 }
        selection = { type = "tournament", tournament_size = 3 }
    )";

    auto temp_file = create_test_config(toml_content);
    auto config = Config::from_file(temp_file.string());

    // Create a simple TSP problem for testing
    auto tsp = problems::create_random_tsp(10, 100.0, 42);

    // Create GA with config
    auto ga = factory::make_tsp_ga_from_config(config);

    // Run GA with config (just a few generations to test it works)
    auto result_run = ga.run(tsp, config.to_ga_config());

    // Verify that GA ran successfully
    result.assert_true(result_run.generations > 0, "GA ran at least one generation");
    result.assert_true(result_run.evaluations > 0, "GA performed evaluations");
    result.assert_true(result_run.best_fitness.value > 0, "GA found a solution");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_mab_scheduler_with_config() {
    TestResult result;

    const std::string toml_content = R"(
        [scheduler]
        enabled = true
        type = "ucb"
        operators = ["EAX", "PMX", "OX"]
        window_size = 50
        exploration_rate = 1.5
    )";

    auto temp_file = create_test_config(toml_content);
    auto config = Config::from_file(temp_file.string());

    // Create UCB scheduler from config
    auto scheduler = factory::make_ucb_scheduler_from_config<problems::TSP>(config);

    result.assert_true(config.scheduler.enabled, "Scheduler is enabled");
    result.assert_eq(std::string("ucb"), config.scheduler.type, "Scheduler type is UCB");
    result.assert_eq(1.5, config.scheduler.exploration_rate, "Exploration rate from config");
    result.assert_eq(static_cast<size_t>(3), config.scheduler.operators.size(),
                     "Number of operators");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_command_line_override() {
    TestResult result;

    const std::string toml_content = R"(
        [ga]
        population_size = 256
        max_generations = 1000
        
        [operators]
        crossover = { type = "EAX", probability = 0.9 }
        mutation = { type = "inversion", probability = 0.1 }
    )";

    auto temp_file = create_test_config(toml_content);
    auto config = Config::from_file(temp_file.string());

    // Apply command-line overrides
    ConfigOverrides overrides;
    overrides.population_size = 512;
    overrides.max_generations = 500;
    overrides.crossover_probability = 0.95;
    overrides.seed = 999;

    config.apply_overrides(overrides);

    // Check that overrides were applied
    result.assert_eq(static_cast<size_t>(512), config.ga.population_size,
                     "Population size override");
    result.assert_eq(static_cast<size_t>(500), config.ga.max_generations,
                     "Max generations override");
    result.assert_eq(0.95, config.operators.crossover.probability,
                     "Crossover probability override");
    result.assert_eq(static_cast<size_t>(999), static_cast<size_t>(config.ga.seed),
                     "Seed override");

    // Check that non-overridden values remain unchanged
    result.assert_eq(0.1, config.operators.mutation.probability, "Mutation probability unchanged");
    result.assert_eq(std::string("inversion"), config.operators.mutation.type,
                     "Mutation type unchanged");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_local_search_integration() {
    TestResult result;

    const std::string toml_content = R"(
        [ga]
        population_size = 32
        max_generations = 50
        
        [local_search]
        enabled = true
        type = "2-opt"
        max_iterations = 100
        probability = 0.3
    )";

    auto temp_file = create_test_config(toml_content);
    auto config = Config::from_file(temp_file.string());

    // Create GA with local search from config
    auto ga_with_ls = factory::make_tsp_ga_with_local_search_from_config(config);

    // Test that GA with local search can be created and run
    auto tsp = problems::create_random_tsp(10, 100.0, 42);
    auto result_run = ga_with_ls.run(tsp, config.to_ga_config());

    result.assert_true(result_run.generations > 0, "GA with local search ran");
    result.assert_true(config.local_search.enabled, "Local search is enabled");
    result.assert_eq(std::string("2-opt"), config.local_search.type, "Local search type is 2-opt");
    result.assert_eq(static_cast<size_t>(100), config.local_search.max_iterations,
                     "Local search iterations");
    result.assert_eq(0.3, config.local_search.probability, "Local search probability");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

void test_diversity_settings_from_config() {
    TestResult result;

    const std::string toml_content = R"(
        [ga]
        population_size = 100
        
        [diversity]
        enabled = true
        
        [termination]
        stagnation_generations = 200
    )";

    auto temp_file = create_test_config(toml_content);
    auto config = Config::from_file(temp_file.string());

    // Convert to GAConfig and check diversity settings
    auto ga_config = config.to_ga_config();

    result.assert_true(ga_config.enable_diversity_tracking, "Diversity tracking enabled");
    result.assert_eq(static_cast<size_t>(200), ga_config.stagnation_limit,
                     "Stagnation limit from termination");

    std::filesystem::remove(temp_file);
    result.print_summary();
}

int main() {
    std::cout << "=== Configuration Integration Tests ===\n\n";

    std::cout << "Test: Config to GAConfig Conversion\n";
    test_config_to_ga_config_conversion();

    std::cout << "\nTest: GA with Config Object\n";
    test_ga_with_config_object();

    std::cout << "\nTest: MAB Scheduler with Config\n";
    test_mab_scheduler_with_config();

    std::cout << "\nTest: Command-Line Override\n";
    test_command_line_override();

    std::cout << "\nTest: Local Search Integration\n";
    test_local_search_integration();

    std::cout << "\nTest: Diversity Settings from Config\n";
    test_diversity_settings_from_config();

    std::cout << "\n=== All Configuration Integration Tests Completed ===\n";
    return 0;
}