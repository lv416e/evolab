#include <cassert>
#include <iostream>
#include <memory_resource>
#include <vector>

#include <evolab/evolab.hpp>

#include "test_helper.hpp"

using namespace evolab;

void test_numa_allocator_basic() {
    TestResult result;

    // Test NUMA allocator creation
    auto local_resource = utils::NumaMemoryResource::create_local();
    result.assert_true(local_resource != nullptr, "Local NUMA resource created successfully");

    auto node_resource = utils::NumaMemoryResource::create_on_node(0);
    result.assert_true(node_resource != nullptr,
                       "Node-specific NUMA resource created successfully");

    // Test NUMA node properties
    result.assert_eq(-1, local_resource->numa_node(), "Local resource has correct node ID");
    result.assert_eq(0, node_resource->numa_node(), "Node resource has correct node ID");

    // Test system NUMA properties
    int node_count = utils::NumaMemoryResource::get_numa_node_count();
    result.assert_ge(node_count, 1, "System has at least one NUMA node");

    int current_node = utils::NumaMemoryResource::get_current_numa_node();
    result.assert_ge(current_node, 0, "Current NUMA node is valid");

    result.print_summary();
}

void test_numa_allocator_memory_operations() {
    TestResult result;

    // Test memory allocation with NUMA resource
    auto numa_resource = utils::NumaMemoryResource::create_local();

    // Test with Population class using NUMA allocator
    const size_t capacity = 100;
    core::Population<std::vector<int>> population(capacity, numa_resource.get());

    result.assert_eq(capacity, population.capacity(),
                     "Population with NUMA allocator has correct capacity");
    result.assert_true(numa_resource.get() == population.get_memory_resource(),
                       "Population uses correct memory resource");

    // Test memory allocation and deallocation
    std::vector<int> test_genome = {0, 1, 2, 3, 4};
    core::Fitness test_fitness{42.0};

    for (size_t i = 0; i < 50; ++i) {
        population.push_back(test_genome, test_fitness);
    }

    result.assert_eq(size_t{50}, population.size(),
                     "Population with NUMA allocator stores individuals correctly");

    // Verify memory operations work correctly
    for (size_t i = 0; i < population.size(); ++i) {
        result.assert_eq(size_t{5}, population.genome(i).size(),
                         "Genome stored correctly with NUMA allocator");
        result.assert_eq(42.0, population.fitness(i).value,
                         "Fitness stored correctly with NUMA allocator");
    }

    result.print_summary();
}

void test_numa_factory_functions() {
    TestResult result;

    // Test optimized GA resource factory
    auto ga_resource = utils::create_optimized_ga_resource();
    result.assert_true(ga_resource != nullptr, "Optimized GA resource created successfully");

    // Test island resource factory
    auto island_resource_0 = utils::create_island_resource(0);
    auto island_resource_1 = utils::create_island_resource(1);

    result.assert_true(island_resource_0 != nullptr, "Island resource 0 created successfully");
    result.assert_true(island_resource_1 != nullptr, "Island resource 1 created successfully");

    // Test with negative island ID (should return default resource)
    auto default_resource = utils::create_island_resource(-1);
    result.assert_true(std::pmr::get_default_resource() == default_resource,
                       "Negative island ID returns default resource");

    result.print_summary();
}

void test_numa_allocator_performance_hints() {
    TestResult result;

    // Test that NUMA allocator can be used for large allocations
    auto numa_resource = utils::NumaMemoryResource::create_local();

    // Create large population to test memory efficiency
    const size_t large_capacity = 10000;
    core::Population<std::vector<int>> large_population(large_capacity, numa_resource.get());

    result.assert_eq(large_capacity, large_population.capacity(),
                     "Large population with NUMA allocator created");

    // Fill with data to test allocation efficiency
    std::vector<int> large_genome(100); // 100-city TSP genome
    std::iota(large_genome.begin(), large_genome.end(), 0);
    core::Fitness large_fitness{1000.0};

    const size_t fill_count = 1000;
    for (size_t i = 0; i < fill_count; ++i) {
        large_population.push_back(large_genome, large_fitness);
    }

    result.assert_eq(fill_count, large_population.size(),
                     "Large population filled successfully with NUMA allocator");

    // Verify data integrity
    result.assert_eq(size_t{100}, large_population.genome(0).size(),
                     "Large genome stored correctly");
    result.assert_eq(1000.0, large_population.fitness(0).value, "Large fitness stored correctly");

    result.print_summary();
}

void test_numa_allocator_fallback() {
    TestResult result;

    // Test that allocator works even when NUMA is not available
    // This should always work regardless of system NUMA support

    auto numa_resource = utils::NumaMemoryResource::create_local();

    // Test basic memory operations
    core::Population<std::vector<int>> population(10, numa_resource.get());

    std::vector<int> genome = {1, 2, 3};
    core::Fitness fitness{99.0};

    population.push_back(genome, fitness);

    result.assert_eq(size_t{1}, population.size(), "NUMA allocator fallback works correctly");
    result.assert_eq(size_t{3}, population.genome(0).size(),
                     "Fallback allocation preserves data correctly");
    result.assert_eq(99.0, population.fitness(0).value,
                     "Fallback allocation preserves fitness correctly");

    result.print_summary();
}

int main() {
    std::cout << "Running EvoLab NUMA Allocator Tests\n";
    std::cout << std::string(40, '=') << "\n\n";

    std::cout << "Testing NUMA Allocator Basic Functionality...\n";
    test_numa_allocator_basic();

    std::cout << "\nTesting NUMA Allocator Memory Operations...\n";
    test_numa_allocator_memory_operations();

    std::cout << "\nTesting NUMA Factory Functions...\n";
    test_numa_factory_functions();

    std::cout << "\nTesting NUMA Allocator Performance Hints...\n";
    test_numa_allocator_performance_hints();

    std::cout << "\nTesting NUMA Allocator Fallback...\n";
    test_numa_allocator_fallback();

    std::cout << "\n" << std::string(40, '=') << "\n";
    std::cout << "NUMA allocator tests completed.\n";

    return 0;
}
