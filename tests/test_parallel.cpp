#include <chrono>
#include <random>
#include <vector>

#include <evolab/evolab.hpp>
#include <evolab/parallel/tbb_executor.hpp>

#include "test_helper.hpp"

using namespace evolab;
using namespace evolab::problems;
using namespace evolab::parallel;
using namespace evolab::core;

namespace {

// Generate test population
std::vector<TSP::GenomeT> create_test_population(const TSP& tsp, std::size_t population_size,
                                                 std::uint64_t seed = 123) {
    std::vector<TSP::GenomeT> population;
    population.reserve(population_size);

    std::mt19937 rng(seed);

    for (std::size_t i = 0; i < population_size; ++i) {
        population.push_back(tsp.random_genome(rng));
    }

    return population;
}

// Sequential evaluation for comparison
std::vector<Fitness> evaluate_sequential(const TSP& tsp,
                                         const std::vector<TSP::GenomeT>& population) {
    std::vector<Fitness> fitnesses;
    fitnesses.reserve(population.size());

    for (const auto& genome : population) {
        fitnesses.push_back(tsp.evaluate(genome));
    }

    return fitnesses;
}

} // anonymous namespace

void test_parallel_evaluation_correctness() {
    TestResult result;

    auto tsp = create_random_tsp(12, 100.0, 42);
    auto population = create_test_population(tsp, 50);

    // Sequential evaluation
    auto sequential_fitnesses = evaluate_sequential(tsp, population);

    // Parallel evaluation using TBB executor
    TBBExecutor executor(123); // Same seed for reproducibility
    auto parallel_fitnesses = executor.parallel_evaluate(tsp, population);

    // Results should be identical
    result.assert_eq(sequential_fitnesses.size(), parallel_fitnesses.size(),
                     "Fitness vector sizes should match");

    for (std::size_t i = 0; i < sequential_fitnesses.size(); ++i) {
        result.assert_eq(sequential_fitnesses[i].value, parallel_fitnesses[i].value,
                         "Parallel and sequential fitness should be identical " +
                             std::to_string(i));
    }

    result.print_summary();
}

void test_thread_safe_rng() {
    TestResult result;

    auto tsp = create_random_tsp(8, 100.0, 42);
    auto population = create_test_population(tsp, 100);

    // Multiple parallel runs with same seed should produce identical results
    TBBExecutor executor1(456);
    TBBExecutor executor2(456);

    auto fitnesses1 = executor1.parallel_evaluate(tsp, population);
    auto fitnesses2 = executor2.parallel_evaluate(tsp, population);

    result.assert_eq(fitnesses1.size(), fitnesses2.size(),
                     "RNG reproducibility: fitness vector sizes should match");

    for (std::size_t i = 0; i < fitnesses1.size(); ++i) {
        result.assert_eq(fitnesses1[i].value, fitnesses2[i].value,
                         "RNG reproducibility: results should be identical with same seed " +
                             std::to_string(i));
    }

    result.print_summary();
}

void test_performance_improvement() {
    TestResult result;

    auto tsp = create_random_tsp(20, 100.0, 42);
    auto population = create_test_population(tsp, 1000); // Larger population for meaningful timing

    // Time sequential evaluation
    auto start_seq = std::chrono::high_resolution_clock::now();
    auto sequential_fitnesses = evaluate_sequential(tsp, population);
    auto end_seq = std::chrono::high_resolution_clock::now();
    auto sequential_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_seq - start_seq);

    // Time parallel evaluation
    TBBExecutor executor(789);
    auto start_par = std::chrono::high_resolution_clock::now();
    auto parallel_fitnesses = executor.parallel_evaluate(tsp, population);
    auto end_par = std::chrono::high_resolution_clock::now();
    auto parallel_time = std::chrono::duration_cast<std::chrono::microseconds>(end_par - start_par);

    std::cout << "Sequential time: " << sequential_time.count() << " microseconds" << std::endl;
    std::cout << "Parallel time: " << parallel_time.count() << " microseconds" << std::endl;

    // Verify correctness
    result.assert_eq(sequential_fitnesses.size(), parallel_fitnesses.size(),
                     "Performance test: fitness vector sizes should match");

    // Note: We don't assert parallel is faster since it depends on system/TBB setup
    // But we log the timings for observation
    double speedup = static_cast<double>(sequential_time.count()) / parallel_time.count();
    std::cout << "Speedup: " << speedup << "x" << std::endl;

    result.print_summary();
}

int main() {
    std::cout << "Running EvoLab Parallel Tests" << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << std::endl;

    try {
        test_parallel_evaluation_correctness();
        std::cout << std::endl;

        test_thread_safe_rng();
        std::cout << std::endl;

        test_performance_improvement();
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "==============================" << std::endl;
    std::cout << "Parallel tests completed." << std::endl;

    return 0;
}