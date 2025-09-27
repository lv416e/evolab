#include <cassert>
#include <iostream>
#include <memory_resource>
#include <span>
#include <sstream>

#include <evolab/evolab.hpp>

#include "test_helper.hpp"

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
    result.assert_true(std::same_as<problems::TSP::GenomeT, std::vector<int>>,
                       "TSP genome type is vector<int>");
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
    result.assert_equals(256, static_cast<double>(config.population_size),
                         "Default population size");
    result.assert_equals(5000, static_cast<double>(config.max_generations),
                         "Default max generations");
    result.assert_equals(0.9, config.crossover_prob, "Default crossover probability");
    result.assert_equals(0.2, config.mutation_prob, "Default mutation probability");
    result.assert_equals(1, static_cast<double>(config.seed), "Default seed");

    // Test custom configuration
    core::GAConfig custom{.population_size = 100,
                          .max_generations = 500,
                          .crossover_prob = 0.8,
                          .mutation_prob = 0.1,
                          .seed = 12345};

    result.assert_equals(100, static_cast<double>(custom.population_size),
                         "Custom population size");
    result.assert_equals(500, static_cast<double>(custom.max_generations),
                         "Custom max generations");
    result.assert_equals(0.8, custom.crossover_prob, "Custom crossover probability");
    result.assert_equals(0.1, custom.mutation_prob, "Custom mutation probability");
    result.assert_equals(12345, static_cast<double>(custom.seed), "Custom seed");

    result.print_summary();
}

void test_population_basic() {
    TestResult result;

    // Test basic Population construction and SoA layout
    const size_t capacity = 100;
    core::Population<std::vector<int>> population(capacity);

    result.assert_eq(capacity, population.capacity(), "Population capacity matches constructor");
    result.assert_eq(size_t{0}, population.size(), "Population starts empty");
    result.assert_true(population.empty(), "Empty population returns true for empty()");

    // Test adding individuals
    std::vector<int> genome = {0, 1, 2, 3, 4};
    core::Fitness fitness{100.0};

    population.push_back(genome, fitness);
    result.assert_eq(size_t{1}, population.size(), "Population size increases after push_back");
    result.assert_true(!population.empty(), "Non-empty population returns false for empty()");

    // Test SoA access patterns
    auto& stored_genome = population.genome(0);
    auto stored_fitness = population.fitness(0);

    result.assert_eq(size_t{5}, stored_genome.size(), "Stored genome has correct size");
    result.assert_eq(100.0, stored_fitness.value, "Stored fitness has correct value");

    // Test batch access for vectorization
    auto genomes_span = population.genomes();
    auto fitness_span = population.fitness_values();

    result.assert_eq(size_t{1}, genomes_span.size(), "Genomes span has correct size");
    result.assert_eq(size_t{1}, fitness_span.size(), "Fitness span has correct size");

    result.print_summary();
}

void test_population_memory_efficiency() {
    TestResult result;

    // Test pre-allocation prevents additional allocations
    const size_t capacity = 1000;
    core::Population<std::vector<int>> population(capacity);

    // Fill to capacity - should not trigger reallocations
    std::vector<int> test_genome = {0, 1, 2, 3, 4};
    core::Fitness test_fitness{50.0};

    for (size_t i = 0; i < capacity; ++i) {
        population.push_back(test_genome, test_fitness);
    }

    result.assert_eq(capacity, population.size(), "Population filled to capacity");
    result.assert_eq(capacity, population.capacity(), "Capacity unchanged after filling");

    // Test memory layout for vectorization (addresses should be contiguous)
    auto genomes_span = population.genomes();
    auto fitness_span = population.fitness_values();

    result.assert_eq(capacity, genomes_span.size(), "Genomes span covers all individuals");
    result.assert_eq(capacity, fitness_span.size(), "Fitness span covers all individuals");

    // Verify separate storage (SoA pattern)
    void* genome_ptr = genomes_span.data();
    void* fitness_ptr = fitness_span.data();
    result.assert_true(genome_ptr != fitness_ptr, "Genomes and fitness stored separately");

    result.print_summary();
}

void test_population_custom_allocator() {
    TestResult result;

    // Test Population with custom PMR allocator
    std::pmr::monotonic_buffer_resource buffer_resource;

    const size_t capacity = 50;
    core::Population<std::vector<int>> population(capacity, &buffer_resource);

    result.assert_eq(capacity, population.capacity(),
                     "Population with custom allocator has correct capacity");

    // Add some individuals using custom allocator
    std::vector<int> genome = {0, 1, 2};
    core::Fitness fitness{75.0};

    population.push_back(genome, fitness);
    result.assert_eq(size_t{1}, population.size(), "Population with custom allocator works");

    result.print_summary();
}

void test_basic_ga() {
    TestResult result;

    // Create small TSP problem
    auto tsp = problems::create_random_tsp(5, 100.0, 42);

    // Create simple GA
    auto ga = factory::make_ga_basic();

    // Run for few generations with diversity tracking disabled for speed
    core::GAConfig config{.population_size = 20,
                          .max_generations = 10,
                          .seed = 42,
                          .enable_diversity_tracking = false};

    auto ga_result = ga.run(tsp, config);

    result.assert_true(ga_result.generations <= 10, "GA terminated within generation limit");
    result.assert_true(ga_result.evaluations > 0, "GA performed evaluations");
    result.assert_true(ga_result.best_fitness.value > 0.0,
                       "GA found solution with positive fitness");
    result.assert_true(tsp.is_valid_tour(ga_result.best_genome), "GA solution is valid tour");
    result.assert_equals(5, static_cast<double>(ga_result.best_genome.size()),
                         "Solution has correct size");

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

    std::cout << "\nTesting Population Basic Functionality...\n";
    test_population_basic();

    std::cout << "\nTesting Population Memory Efficiency...\n";
    test_population_memory_efficiency();

    std::cout << "\nTesting Population Custom Allocator...\n";
    test_population_custom_allocator();

    std::cout << "\nTesting Basic GA...\n";
    test_basic_ga();

    std::cout << "\n" << std::string(30, '=') << "\n";
    std::cout << "Core tests completed.\n";

    return 0;
}
