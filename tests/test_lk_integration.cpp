/// @file test_lk_integration.cpp
/// @brief Integration tests for Lin-Kernighan with GA framework
///
/// Tests verify LK integration with genetic algorithms, creating memetic
/// algorithms that combine evolutionary search with local optimization.

#include <algorithm>
#include <random>
#include <vector>

#include <evolab/core/ga.hpp>
#include <evolab/local_search/lk.hpp>
#include <evolab/local_search/two_opt.hpp>
#include <evolab/operators/crossover.hpp>
#include <evolab/operators/mutation.hpp>
#include <evolab/operators/selection.hpp>
#include <evolab/problems/tsp.hpp>

#include "test_helper.hpp"

using namespace evolab;

// Helper to create random TSP
problems::TSP create_random_tsp(int n, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(0.0, 100.0);
    std::vector<std::pair<double, double>> cities;
    for (int i = 0; i < n; ++i) {
        cities.emplace_back(dist(rng), dist(rng));
    }
    return problems::TSP(cities);
}

// Test LK integration with basic GA
int test_lk_with_basic_ga() {
    TestResult result;

    // Create small TSP instance
    int n = 15;
    problems::TSP tsp = create_random_tsp(n, 42);

    // Create GA with LK local search
    operators::TournamentSelection selection(3);
    operators::PMXCrossover crossover;
    operators::SwapMutation mutation;
    local_search::LinKernighan lk(10, 3);

    auto ga = core::make_ga(selection, crossover, mutation, lk);

    // Configure GA
    core::GAConfig config;
    config.population_size = 20;
    config.max_generations = 10;
    config.crossover_prob = 0.8;
    config.mutation_prob = 0.2;
    config.elite_ratio = 0.1;
    config.seed = 12345;

    // Run GA
    auto result_ga = ga.run(tsp, config);

    // Verify we got a valid result
    result.assert_true(result_ga.best_fitness.value > 0, "GA should return valid fitness");
    result.assert_eq(n, static_cast<int>(result_ga.best_genome.size()),
                     "Best genome should have correct size");

    // Verify tour validity
    std::vector<int> sorted = result_ga.best_genome;
    std::sort(sorted.begin(), sorted.end());
    for (int i = 0; i < n; ++i) {
        result.assert_eq(i, sorted[i], "Best genome should be valid permutation");
    }

    return result.summary();
}

// Test memetic GA (GA + LK) vs pure GA performance comparison
int test_memetic_vs_pure_ga() {
    TestResult result;

    // Create TSP instance
    int n = 20;
    problems::TSP tsp = create_random_tsp(n, 42);

    // Configure both GAs with same parameters for fair comparison
    core::GAConfig config;
    config.population_size = 30;
    config.max_generations = 20;
    config.crossover_prob = 0.9;
    config.mutation_prob = 0.1;
    config.elite_ratio = 0.1;
    config.seed = 999;

    double pure_fitness = 0.0;
    double memetic_fitness = 0.0;

    // Pure GA (no local search)
    {
        operators::TournamentSelection selection(3);
        operators::PMXCrossover crossover;
        operators::SwapMutation mutation;
        local_search::NoLocalSearch no_ls;

        auto pure_ga = core::make_ga(selection, crossover, mutation, no_ls);
        auto pure_result = pure_ga.run(tsp, config);

        pure_fitness = pure_result.best_fitness.value;
        result.assert_true(pure_fitness > 0, "Pure GA should return valid fitness");
    }

    // Memetic GA (with LK)
    {
        operators::TournamentSelection selection(3);
        operators::PMXCrossover crossover;
        operators::SwapMutation mutation;
        local_search::LinKernighan lk(10, 3);

        auto memetic_ga = core::make_ga(selection, crossover, mutation, lk);
        auto memetic_result = memetic_ga.run(tsp, config);

        memetic_fitness = memetic_result.best_fitness.value;
        result.assert_true(memetic_fitness > 0, "Memetic GA should return valid fitness");
    }

    // For TSP, lower fitness is better (minimization)
    // Memetic GA should achieve equal or better fitness than pure GA
    result.assert_true(memetic_fitness <= pure_fitness,
                       "Memetic GA should achieve better or equal fitness compared to pure GA "
                       "(lower is better for TSP)");

    return result.summary();
}

// Test LK with different crossover operators
int test_lk_with_different_crossovers() {
    TestResult result;

    int n = 12;
    problems::TSP tsp = create_random_tsp(n, 42);

    core::GAConfig config;
    config.population_size = 15;
    config.max_generations = 5;
    config.seed = 555;

    operators::TournamentSelection selection(2);
    operators::SwapMutation mutation;
    local_search::LinKernighan lk(8, 2);

    // Test with PMX
    {
        operators::PMXCrossover pmx;
        auto ga = core::make_ga(selection, pmx, mutation, lk);
        auto result_ga = ga.run(tsp, config);
        result.assert_true(result_ga.best_fitness.value > 0, "PMX + LK should work");
    }

    // Test with Order Crossover
    {
        operators::OrderCrossover ox;
        auto ga = core::make_ga(selection, ox, mutation, lk);
        auto result_ga = ga.run(tsp, config);
        result.assert_true(result_ga.best_fitness.value > 0, "OrderCrossover + LK should work");
    }

    // Test with Cycle Crossover
    {
        operators::CycleCrossover cx;
        auto ga = core::make_ga(selection, cx, mutation, lk);
        auto result_ga = ga.run(tsp, config);
        result.assert_true(result_ga.best_fitness.value > 0, "CycleCrossover + LK should work");
    }

    return result.summary();
}

// Test that LK improves solutions during evolution
int test_lk_improves_during_evolution() {
    TestResult result;

    int n = 18;
    problems::TSP tsp = create_random_tsp(n, 123);

    // Create initial random population to get baseline fitness
    std::vector<int> initial_tour(n);
    std::iota(initial_tour.begin(), initial_tour.end(), 0);
    std::mt19937 rng(777);
    std::shuffle(initial_tour.begin(), initial_tour.end(), rng);
    double initial_fitness = tsp.evaluate(initial_tour).value;

    core::GAConfig config;
    config.population_size = 25;
    config.max_generations = 15;
    config.crossover_prob = 0.9;
    config.mutation_prob = 0.15;
    config.elite_ratio = 0.1;
    config.seed = 777;

    operators::TournamentSelection selection(3);
    operators::PMXCrossover crossover;
    operators::InversionMutation mutation;
    local_search::LinKernighan lk(12, 4);

    auto ga = core::make_ga(selection, crossover, mutation, lk);
    auto result_ga = ga.run(tsp, config);

    // Verify valid solution
    result.assert_true(result_ga.best_fitness.value > 0, "Memetic GA should find valid solution");
    result.assert_true(result_ga.generations > 0, "Should run at least one generation");

    // For TSP (minimization), final fitness should be strictly better than initial
    result.assert_true(result_ga.best_fitness.value < initial_fitness,
                       "GA with LK should improve over initial random solution");

    return result.summary();
}

int main() {
    std::cout << "Running Lin-Kernighan Integration Tests\n";
    std::cout << "=========================================\n\n";

    int failed = 0;

    failed += test_lk_with_basic_ga();
    failed += test_memetic_vs_pure_ga();
    failed += test_lk_with_different_crossovers();
    failed += test_lk_improves_during_evolution();

    std::cout << "\n=========================================\n";
    if (failed == 0) {
        std::cout << "All integration tests passed!\n";
    } else {
        std::cout << failed << " test(s) failed!\n";
    }

    return failed;
}
