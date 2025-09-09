/**
 * @file parallel-evaluation.cpp
 * @brief Demonstration of parallel fitness evaluation using Intel TBB
 *
 * This example shows how to use EvoLab's TBBExecutor for high-performance
 * parallel fitness evaluation. The executor provides deterministic results
 * with const-correct stateless design following C++23 best practices.
 *
 * Compile with TBB:
 *   g++ -std=c++23 -I../../include parallel-evaluation.cpp -ltbb -o parallel-evaluation
 *
 * Run with:
 *   ./parallel-evaluation
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include <evolab/evolab.hpp>
#include <evolab/parallel/tbb_executor.hpp>

using namespace evolab;

// Helper function to create a test population
std::vector<problems::TSP::GenomeT> create_population(const problems::TSP& tsp,
                                                      size_t population_size, uint64_t seed = 456) {
    std::vector<problems::TSP::GenomeT> population;
    population.reserve(population_size);

    std::mt19937 rng(seed);
    for (size_t i = 0; i < population_size; ++i) {
        population.push_back(tsp.random_genome(rng));
    }

    return population;
}

// Sequential fitness evaluation for comparison
std::vector<core::Fitness>
evaluate_sequential(const problems::TSP& tsp,
                    const std::vector<problems::TSP::GenomeT>& population) {
    std::vector<core::Fitness> fitnesses;
    fitnesses.reserve(population.size());

    for (const auto& genome : population) {
        fitnesses.push_back(tsp.evaluate(genome));
    }

    return fitnesses;
}

int main() {
    std::cout << "EvoLab Parallel Evaluation Example\n";
    std::cout << "===================================\n\n";

    // Check if TBB is available
#ifdef EVOLAB_HAVE_TBB
    std::cout << "Intel TBB: Available ✓\n";
#else
    std::cout << "Intel TBB: Not available ✗\n";
    std::cout << "This example requires Intel TBB for parallel execution.\n";
    std::cout << "Please install TBB and rebuild with -DEVOLAB_USE_TBB=ON\n";
    return 1;
#endif

    // Create a computationally intensive TSP instance
    constexpr size_t num_cities = 100;
    constexpr size_t population_size = 1000;

    auto tsp = problems::create_random_tsp(num_cities, 100.0, 42);
    auto population = create_population(tsp, population_size, 456);

    std::cout << "Problem setup:\n";
    std::cout << "  TSP cities: " << num_cities << "\n";
    std::cout << "  Population: " << population_size << "\n";
    std::cout << "  Hardware threads: " << std::thread::hardware_concurrency() << "\n\n";

    // Create TBB executor with deterministic seed
    parallel::TBBExecutor executor(789);

    std::cout << "Executor configuration:\n";
    std::cout << "  Base seed: " << executor.get_seed() << "\n";
    std::cout << "  Design: Const-correct stateless\n";
    std::cout << "  Thread safety: Guaranteed by design\n\n";

    // Benchmark sequential evaluation
    std::cout << "Sequential evaluation...\n";
    auto start_seq = std::chrono::high_resolution_clock::now();
    auto sequential_fitnesses = evaluate_sequential(tsp, population);
    auto end_seq = std::chrono::high_resolution_clock::now();
    auto seq_time = std::chrono::duration<double>(end_seq - start_seq).count();

    // Benchmark parallel evaluation
    std::cout << "Parallel evaluation...\n";
    auto start_par = std::chrono::high_resolution_clock::now();
    auto parallel_fitnesses = executor.parallel_evaluate(tsp, population);
    auto end_par = std::chrono::high_resolution_clock::now();
    auto par_time = std::chrono::duration<double>(end_par - start_par).count();

    // Verify correctness - results should be identical
    bool results_match = true;
    for (size_t i = 0; i < sequential_fitnesses.size(); ++i) {
        if (sequential_fitnesses[i].value != parallel_fitnesses[i].value) {
            results_match = false;
            break;
        }
    }

    // Display performance results
    std::cout << "\nPerformance Results:\n";
    std::cout << "===================\n";
    std::cout << "Sequential time: " << std::fixed << std::setprecision(3) << seq_time
              << " seconds\n";
    std::cout << "Parallel time:   " << std::fixed << std::setprecision(3) << par_time
              << " seconds\n";

    if (par_time > 0) {
        double speedup = seq_time / par_time;
        std::cout << "Speedup:         " << std::fixed << std::setprecision(2) << speedup << "x\n";

        auto hw_threads = std::thread::hardware_concurrency();
        if (hw_threads > 0) {
            double efficiency = (speedup / hw_threads) * 100.0;
            std::cout << "Efficiency:      " << std::fixed << std::setprecision(1) << efficiency
                      << "% (on " << hw_threads << " cores)\n";
        }
    }

    // Verify correctness
    std::cout << "\nCorrectness Verification:\n";
    std::cout << "========================\n";
    std::cout << "Results identical: " << (results_match ? "Yes ✓" : "No ✗") << "\n";

    if (!results_match) {
        std::cout << "ERROR: Parallel and sequential results differ!\n";
        return 1;
    }

    // Demonstrate deterministic behavior
    std::cout << "\nDeterminism Test:\n";
    std::cout << "================\n";
    std::cout << "Running parallel evaluation multiple times with same seed...\n";

    // Create fresh executor instances with same seed
    parallel::TBBExecutor executor1(789);
    parallel::TBBExecutor executor2(789);

    auto fitnesses1 = executor1.parallel_evaluate(tsp, population);
    auto fitnesses2 = executor2.parallel_evaluate(tsp, population);

    bool deterministic = true;
    for (size_t i = 0; i < fitnesses1.size(); ++i) {
        if (fitnesses1[i].value != fitnesses2[i].value) {
            deterministic = false;
            break;
        }
    }

    std::cout << "Multiple runs identical: " << (deterministic ? "Yes ✓" : "No ✗") << "\n";

    if (!deterministic) {
        std::cout << "ERROR: Results are not deterministic!\n";
        return 1;
    }

    // Demonstrate stateless design - multiple calls on same executor
    std::cout << "\nStateless Design Test:\n";
    std::cout << "=====================\n";
    std::cout << "Multiple calls on same executor instance...\n";

    auto call1 = executor.parallel_evaluate(tsp, population);
    auto call2 = executor.parallel_evaluate(tsp, population);

    bool stateless = true;
    for (size_t i = 0; i < call1.size(); ++i) {
        if (call1[i].value != call2[i].value) {
            stateless = false;
            break;
        }
    }

    std::cout << "Subsequent calls identical: " << (stateless ? "Yes ✓" : "No ✗") << "\n";

    if (!stateless) {
        std::cout << "ERROR: Executor is not stateless!\n";
        return 1;
    }

    std::cout << "\nAll tests passed! The TBBExecutor provides:\n";
    std::cout << "  ✓ High-performance parallel evaluation\n";
    std::cout << "  ✓ Deterministic reproducible results\n";
    std::cout << "  ✓ Thread-safe const-correct design\n";
    std::cout << "  ✓ Stateless architecture\n";

    std::cout << "\nExample completed successfully!\n";
    return 0;
}