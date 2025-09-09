#include <chrono>
#include <iomanip>
#include <random>
#include <thread>
#include <vector>

#include <evolab/evolab.hpp>
#include <evolab/parallel/tbb_executor.hpp>

#include "test_helper.hpp"

// Explicit using declarations following C++23 best practices for namespace management
// Provides clear dependency tracking while preventing namespace pollution and improving
// compile-time performance through selective symbol importation
using evolab::core::Fitness;
using evolab::parallel::TBBExecutor;
using evolab::problems::create_random_tsp;
using evolab::problems::TSP;

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

static void test_parallel_evaluation_correctness() {
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
        result.assert_true(sequential_fitnesses[i].value == parallel_fitnesses[i].value,
                           "Parallel and sequential fitness should be identical " +
                               std::to_string(i) +
                               " (expected: " + std::to_string(sequential_fitnesses[i].value) +
                               ", actual: " + std::to_string(parallel_fitnesses[i].value) + ")");
    }

    result.print_summary();
}

static void test_rng_reproducibility_and_reset() {
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
        result.assert_true(fitnesses1[i].value == fitnesses2[i].value,
                           "RNG reproducibility: results should be identical with same seed " +
                               std::to_string(i) +
                               " (expected: " + std::to_string(fitnesses1[i].value) +
                               ", actual: " + std::to_string(fitnesses2[i].value) + ")");
    }

    // Test stateless design: Fresh executor instance with same seed behaves identically
    // The new stateless design eliminates the need for reset_rngs() by ensuring
    // each parallel_evaluate() call operates independently with per-call state management
    TBBExecutor executor3(456); // Same seed as executor1
    auto fitnesses3 = executor3.parallel_evaluate(tsp, population);

    result.assert_eq(fitnesses1.size(), fitnesses3.size(),
                     "Stateless reproducibility: fitness vector sizes must match");

    for (std::size_t i = 0; i < fitnesses1.size(); ++i) {
        result.assert_true(
            fitnesses1[i].value == fitnesses3[i].value,
            "Stateless reproducibility: fresh executor with same seed must be identical " +
                std::to_string(i) + " (expected: " + std::to_string(fitnesses1[i].value) +
                ", actual: " + std::to_string(fitnesses3[i].value) + ")");
    }

    // Enhanced stateless verification: Multiple calls on same executor instance
    // Tests that each parallel_evaluate() call on the same executor produces identical,
    // independent results due to per-call state management (const-correct stateless design)
    TBBExecutor executor4(456); // Same seed for deterministic behavior
    auto first_call = executor4.parallel_evaluate(tsp, population);
    auto second_call = executor4.parallel_evaluate(tsp, population);
    auto third_call = executor4.parallel_evaluate(tsp, population);

    result.assert_eq(first_call.size(), second_call.size(),
                     "Same executor multiple calls: first and second call sizes must match");
    result.assert_eq(second_call.size(), third_call.size(),
                     "Same executor multiple calls: second and third call sizes must match");

    // Verify all three calls on same executor produce identical results
    for (std::size_t i = 0; i < first_call.size(); ++i) {
        result.assert_true(first_call[i].value == second_call[i].value,
                           "Same executor stateless: first vs second call must be identical " +
                               std::to_string(i) +
                               " (first: " + std::to_string(first_call[i].value) +
                               ", second: " + std::to_string(second_call[i].value) + ")");

        result.assert_true(second_call[i].value == third_call[i].value,
                           "Same executor stateless: second vs third call must be identical " +
                               std::to_string(i) +
                               " (second: " + std::to_string(second_call[i].value) +
                               ", third: " + std::to_string(third_call[i].value) + ")");
    }

    result.print_summary();
}

static void test_performance_improvement() {
    TestResult result;

    // Performance evaluation using computationally intensive TSP instances
    // Large-scale problem configuration designed to overcome thread creation overhead
    // and demonstrate genuine parallel scalability benefits in production scenarios
    constexpr size_t tsp_cities = 150;       // 56x more computation than 20 cities
    constexpr size_t population_size = 1000; // Large population for statistical significance

    auto tsp = create_random_tsp(tsp_cities, 100.0, 42);
    auto population = create_test_population(tsp, population_size);

    std::cout << "Performance test configuration:\n";
    std::cout << "  TSP cities: " << tsp_cities << "\n";
    std::cout << "  Population size: " << population_size << "\n";
    std::cout << "  Theoretical computation: ~" << (tsp_cities * population_size)
              << " distance calculations (O(N_cities * Pop_size))\n"
              << std::endl;

    // JIT warm-up phase following C++23 benchmarking best practices
    // Initializes CPU caches, branch predictors, and memory allocators for
    // reliable performance measurements free from cold-start artifacts
    TBBExecutor executor(789);
    {
        auto warmup_population = create_test_population(tsp, 100);
        [[maybe_unused]] auto warmup_seq = evaluate_sequential(tsp, warmup_population);
        [[maybe_unused]] auto warmup_par = executor.parallel_evaluate(tsp, warmup_population);
    }

    // Multiple iterations for statistical reliability
    constexpr int benchmark_iterations = 3;
    std::vector<std::chrono::nanoseconds> sequential_times;
    std::vector<std::chrono::nanoseconds> parallel_times;
    sequential_times.reserve(benchmark_iterations);
    parallel_times.reserve(benchmark_iterations);

    std::vector<Fitness> sequential_fitnesses, parallel_fitnesses;

    for (int iter = 0; iter < benchmark_iterations; ++iter) {
        // Benchmark sequential evaluation using steady_clock (best practice for performance
        // measurement)
        auto start_seq = std::chrono::steady_clock::now();
        sequential_fitnesses = evaluate_sequential(tsp, population);
        auto end_seq = std::chrono::steady_clock::now();
        sequential_times.push_back(end_seq - start_seq);

        // Benchmark parallel evaluation
        auto start_par = std::chrono::steady_clock::now();
        parallel_fitnesses = executor.parallel_evaluate(tsp, population);
        auto end_par = std::chrono::steady_clock::now();
        parallel_times.push_back(end_par - start_par);
    }

    // Calculate statistics (median for robustness against outliers)
    std::sort(sequential_times.begin(), sequential_times.end());
    std::sort(parallel_times.begin(), parallel_times.end());

    // Robust median calculation for both odd and even sample sizes
    auto get_median = [](const auto& times) {
        const auto n = times.size();
        if (n == 0)
            return std::chrono::nanoseconds(0);
        if (n % 2 == 1) {
            return times[n / 2];
        } else {
            return (times[n / 2 - 1] + times[n / 2]) / 2;
        }
    };

    auto sequential_median = get_median(sequential_times);
    auto parallel_median = get_median(parallel_times);

    auto seq_microseconds =
        std::chrono::duration_cast<std::chrono::microseconds>(sequential_median);
    auto par_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(parallel_median);

    // Detailed performance reporting
    std::cout << "Benchmark results (median of " << benchmark_iterations << " runs):\n";
    std::cout << "  Sequential: " << seq_microseconds.count() << " μs\n";
    std::cout << "  Parallel:   " << par_microseconds.count() << " μs\n";

    if (par_microseconds.count() > 0) {
        double speedup = static_cast<double>(seq_microseconds.count()) / par_microseconds.count();
        std::cout << "  Speedup:    " << std::fixed << std::setprecision(2) << speedup << "x\n";

        // Performance efficiency analysis
        auto hardware_threads = std::thread::hardware_concurrency();
        if (hardware_threads > 0) {
            double efficiency = speedup / hardware_threads * 100.0;
            std::cout << "  Efficiency: " << std::fixed << std::setprecision(1) << efficiency
                      << "% (on " << hardware_threads << " cores)\n";
        }
    }
    std::cout << std::endl;

    // Verify correctness across all implementations
    // Critical: Ensure performance optimizations don't compromise result accuracy
    result.assert_eq(sequential_fitnesses.size(), parallel_fitnesses.size(),
                     "Performance test: fitness vector sizes must match");

    for (std::size_t i = 0; i < sequential_fitnesses.size(); ++i) {
        result.assert_true(
            sequential_fitnesses[i].value == parallel_fitnesses[i].value,
            "Performance test: parallel and sequential fitness must be identical for element " +
                std::to_string(i) + " (expected: " + std::to_string(sequential_fitnesses[i].value) +
                ", actual: " + std::to_string(parallel_fitnesses[i].value) + ")");
    }

    result.print_summary();
}

int main() {
    std::cout << "Running EvoLab Parallel Tests" << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << std::endl;

    try {
        test_parallel_evaluation_correctness();
        std::cout << std::endl;

        test_rng_reproducibility_and_reset();
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