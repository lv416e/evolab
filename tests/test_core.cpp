#include <evolab/evolab.hpp>
#include <iostream>
#include <cassert>
#include <sstream>

// Simple test framework - no external dependencies
struct TestResult {
    int passed = 0;
    int failed = 0;
    
    void assert_true(bool condition, const std::string& message) {
        if (condition) {
            passed++;
            std::cout << "[PASS] " << message << "\n";
        } else {
            failed++;
            std::cout << "[FAIL] " << message << "\n";
        }
    }
    
    void assert_equals(double expected, double actual, const std::string& message, double tolerance = 1e-9) {
        bool passed_test = std::abs(expected - actual) < tolerance;
        assert_true(passed_test, message + 
            " (expected: " + std::to_string(expected) + 
            ", actual: " + std::to_string(actual) + ")");
    }
    
    void print_summary() {
        std::cout << "\n=== Test Summary ===\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Total:  " << (passed + failed) << "\n";
        
        if (failed == 0) {
            std::cout << "All tests passed! ✓\n";
        } else {
            std::cout << "Some tests failed! ✗\n";
        }
    }
    
    bool all_passed() const { return failed == 0; }
};

using namespace evolab;

void test_fitness() {
    TestResult result;
    
    // Test basic fitness operations
    core::Fitness f1{10.0};
    core::Fitness f2{20.0};
    
    result.assert_true(f1 < f2, "Fitness comparison less than");
    result.assert_true(f2 > f1, "Fitness comparison greater than");
    result.assert_true(f1 == core::Fitness{10.0}, "Fitness equality");
    result.assert_true(f1 != f2, "Fitness inequality");
    
    // Test arithmetic operations
    f1 += core::Fitness{5.0};
    result.assert_equals(15.0, f1.value, "Fitness addition");
    
    f1 *= 2.0;
    result.assert_equals(30.0, f1.value, "Fitness multiplication");
    
    result.print_summary();
}

void test_concepts() {
    TestResult result;
    
    // Test that TSP satisfies Problem concept
    problems::TSP tsp = problems::create_random_tsp(10, 100.0, 42);
    
    result.assert_true(std::same_as<problems::TSP::Gene, int>, "TSP gene type is int");
    result.assert_true(std::same_as<problems::TSP::GenomeT, std::vector<int>>, "TSP genome type is vector<int>");
    result.assert_equals(10, static_cast<double>(tsp.size()), "TSP size");
    
    // Test genome generation and evaluation
    std::mt19937 rng(42);
    auto genome = tsp.random_genome(rng);
    
    result.assert_equals(10, static_cast<double>(genome.size()), "Random genome size");
    result.assert_true(tsp.is_valid_tour(genome), "Random genome is valid tour");
    
    auto fitness = tsp.evaluate(genome);
    result.assert_true(fitness.value > 0.0, "Fitness is positive");
    
    result.print_summary();
}

void test_ga_config() {
    TestResult result;
    
    // Test default configuration
    core::GAConfig config;
    result.assert_equals(256, static_cast<double>(config.population_size), "Default population size");
    result.assert_equals(5000, static_cast<double>(config.max_generations), "Default max generations");
    result.assert_equals(0.9, config.crossover_prob, "Default crossover probability");
    result.assert_equals(0.2, config.mutation_prob, "Default mutation probability");
    result.assert_equals(1, static_cast<double>(config.seed), "Default seed");
    
    // Test custom configuration
    core::GAConfig custom{
        .population_size = 100,
        .max_generations = 500,
        .crossover_prob = 0.8,
        .mutation_prob = 0.1,
        .seed = 12345
    };
    
    result.assert_equals(100, static_cast<double>(custom.population_size), "Custom population size");
    result.assert_equals(500, static_cast<double>(custom.max_generations), "Custom max generations");
    result.assert_equals(0.8, custom.crossover_prob, "Custom crossover probability");
    result.assert_equals(0.1, custom.mutation_prob, "Custom mutation probability");
    result.assert_equals(12345, static_cast<double>(custom.seed), "Custom seed");
    
    result.print_summary();
}

void test_basic_ga() {
    TestResult result;
    
    // Create small TSP problem
    auto tsp = problems::create_random_tsp(5, 100.0, 42);
    
    // Create simple GA
    auto ga = factory::make_ga_basic();
    
    // Run for few generations
    core::GAConfig config{
        .population_size = 20,
        .max_generations = 10,
        .seed = 42
    };
    
    auto ga_result = ga.run(tsp, config);
    
    result.assert_true(ga_result.generations <= 10, "GA terminated within generation limit");
    result.assert_true(ga_result.evaluations > 0, "GA performed evaluations");
    result.assert_true(ga_result.best_fitness.value > 0.0, "GA found solution with positive fitness");
    result.assert_true(tsp.is_valid_tour(ga_result.best_genome), "GA solution is valid tour");
    result.assert_equals(5, static_cast<double>(ga_result.best_genome.size()), "Solution has correct size");
    
    result.print_summary();
}

int main() {
    std::cout << "Running EvoLab Core Tests\n";
    std::cout << std::string(30, '=') << "\n\n";
    
    std::cout << "Testing Fitness class...\n";
    test_fitness();
    
    std::cout << "\nTesting Concepts...\n";
    test_concepts();
    
    std::cout << "\nTesting GA Configuration...\n";
    test_ga_config();
    
    std::cout << "\nTesting Basic GA...\n";
    test_basic_ga();
    
    std::cout << "\n" << std::string(30, '=') << "\n";
    std::cout << "Core tests completed.\n";
    
    return 0;
}