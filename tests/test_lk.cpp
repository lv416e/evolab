/// @file test_lk.cpp
/// @brief Comprehensive test suite for Lin-Kernighan local search
///
/// Tests verify LK interface, tour validity preservation, fitness improvement,
/// and integration with TSP problems and candidate lists.

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

#include <evolab/local_search/lk.hpp>
#include <evolab/problems/tsp.hpp>

#include "test_helper.hpp"

using namespace evolab;

// Helper function to create random TSP instance
problems::TSP create_random_tsp(int n, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(0.0, 100.0);
    std::vector<std::pair<double, double>> cities;
    for (int i = 0; i < n; ++i) {
        cities.emplace_back(dist(rng), dist(rng));
    }
    return problems::TSP(cities);
}

// Test basic LK construction and interface
int test_lk_construction() {
    TestResult result;

    // Default construction
    local_search::LinKernighan lk1;
    result.assert_eq(20, lk1.k_nearest(), "Default k_nearest should be 20");
    result.assert_eq(5, lk1.max_depth(), "Default max_depth should be 5");

    // Custom construction
    local_search::LinKernighan lk2(30, 3);
    result.assert_eq(30, lk2.k_nearest(), "Custom k_nearest should be 30");
    result.assert_eq(3, lk2.max_depth(), "Custom max_depth should be 3");

    return result.summary();
}

// Test that LK maintains valid tour (all cities present, no duplicates)
int test_lk_maintains_tour_validity() {
    TestResult result;

    // Create small TSP instance
    int n = 10;
    problems::TSP tsp = create_random_tsp(n, 42);

    // Create random tour
    std::vector<int> tour(n);
    std::iota(tour.begin(), tour.end(), 0);
    std::mt19937 rng(123);
    std::shuffle(tour.begin(), tour.end(), rng);

    // Apply LK
    local_search::LinKernighan lk(5, 3);
    lk.improve(tsp, tour, rng);

    // Verify tour validity
    std::vector<int> sorted_tour = tour;
    std::sort(sorted_tour.begin(), sorted_tour.end());

    for (int i = 0; i < n; ++i) {
        result.assert_eq(i, sorted_tour[i], "Tour should contain all cities 0 to n-1");
    }

    return result.summary();
}

// Test that LK improves or maintains fitness
int test_lk_improves_fitness() {
    TestResult result;

    // Create TSP instance
    int n = 20;
    problems::TSP tsp = create_random_tsp(n, 42);

    // Create random tour
    std::vector<int> tour(n);
    std::iota(tour.begin(), tour.end(), 0);
    std::mt19937 rng(123);
    std::shuffle(tour.begin(), tour.end(), rng);

    double initial_fitness = tsp.evaluate(tour).value;

    // Apply LK
    local_search::LinKernighan lk(10, 4);
    core::Fitness final_fitness = lk.improve(tsp, tour, rng);

    // LK should not worsen the solution
    result.assert_true(final_fitness.value <= initial_fitness, "LK should not worsen fitness");

    return result.summary();
}

// Test LK with very small tour (edge cases)
int test_lk_small_tour() {
    TestResult result;

    // Tours with < 4 cities cannot be improved by k-opt
    std::vector<std::pair<double, double>> cities = {{0, 0}, {1, 0}, {1, 1}};
    problems::TSP tsp(cities);
    std::vector<int> tour = {0, 1, 2};
    std::mt19937 rng(123);

    local_search::LinKernighan lk;
    double fitness_before = tsp.evaluate(tour).value;
    core::Fitness fitness_after = lk.improve(tsp, tour, rng);

    // Should return fitness unchanged for small tours
    result.assert_eq(fitness_before, fitness_after.value,
                     "Small tour fitness should remain unchanged", 1e-9);

    return result.summary();
}

// Test LK depth limit parameter
int test_lk_depth_limit() {
    TestResult result;

    // Create TSP instance
    int n = 15;
    problems::TSP tsp = create_random_tsp(n, 42);

    std::vector<int> tour1(n), tour2(n);
    std::iota(tour1.begin(), tour1.end(), 0);

    std::mt19937 rng_shuffle(123);
    std::shuffle(tour1.begin(), tour1.end(), rng_shuffle);
    tour2 = tour1; // Same starting tour

    // Test with different depths
    local_search::LinKernighan lk_depth2(10, 2);
    local_search::LinKernighan lk_depth5(10, 5);

    // Use same RNG seed for deterministic comparison
    std::mt19937 rng1(456), rng2(456);
    core::Fitness f1 = lk_depth2.improve(tsp, tour1, rng1);
    core::Fitness f2 = lk_depth5.improve(tsp, tour2, rng2);

    // Deeper search should yield a better or equal result (LK is deterministic)
    result.assert_true(
        f2.value <= f1.value,
        "Deeper search (depth 5) should be better or equal to shallower search (depth 2)");

    // Both should produce valid improvements
    result.assert_true(f1.value >= 0, "Depth-2 LK should produce valid fitness");
    result.assert_true(f2.value >= 0, "Depth-5 LK should produce valid fitness");

    return result.summary();
}

// Test LK with candidate lists
int test_lk_with_candidate_lists() {
    TestResult result;

    // Create TSP instance
    int n = 25;
    problems::TSP tsp = create_random_tsp(n, 42);

    // Create initial tour
    std::vector<int> initial_tour(n);
    std::iota(initial_tour.begin(), initial_tour.end(), 0);
    std::mt19937 rng(123);
    std::shuffle(initial_tour.begin(), initial_tour.end(), rng);

    double initial_fitness = tsp.evaluate(initial_tour).value;

    // Apply LK with different candidate list sizes from same starting point
    local_search::LinKernighan lk_k5(5, 3);
    local_search::LinKernighan lk_k15(15, 3);

    std::vector<int> tour1 = initial_tour;
    std::vector<int> tour2 = initial_tour;
    std::mt19937 rng1(456), rng2(456);

    core::Fitness f1 = lk_k5.improve(tsp, tour1, rng1);
    core::Fitness f2 = lk_k15.improve(tsp, tour2, rng2);

    // Both should improve or maintain fitness
    result.assert_true(f1.value <= initial_fitness, "LK with k=5 should not worsen fitness");
    result.assert_true(f2.value <= initial_fitness, "LK with k=15 should not worsen fitness");

    // Larger candidate list should yield better or equal result (k=15 is superset of k=5)
    result.assert_true(f2.value <= f1.value,
                       "Larger candidate list (k=15) should be better or equal to smaller (k=5)");

    return result.summary();
}

// Test LK generic improve interface
int test_lk_generic_interface() {
    TestResult result;

    // Test with TSP
    int n = 12;
    problems::TSP tsp = create_random_tsp(n, 42);
    std::vector<int> tour(n);
    std::iota(tour.begin(), tour.end(), 0);
    std::mt19937 rng(123);
    std::shuffle(tour.begin(), tour.end(), rng);

    local_search::LinKernighan lk;
    core::Fitness fitness = lk.template improve<problems::TSP>(tsp, tour, rng);

    result.assert_true(fitness.value > 0, "Generic interface should work with TSP");

    return result.summary();
}

int main() {
    std::cout << "Running Lin-Kernighan Local Search Tests\n";
    std::cout << "==========================================\n\n";

    int failed = 0;

    failed += test_lk_construction();
    failed += test_lk_maintains_tour_validity();
    failed += test_lk_improves_fitness();
    failed += test_lk_small_tour();
    failed += test_lk_depth_limit();
    failed += test_lk_with_candidate_lists();
    failed += test_lk_generic_interface();

    std::cout << "\n==========================================\n";
    if (failed == 0) {
        std::cout << "All tests passed!\n";
    } else {
        std::cout << failed << " test(s) failed!\n";
    }

    return failed;
}
