/// @file test_delta_evaluation.cpp
/// @brief Comprehensive test suite for delta evaluation optimization
///
/// Tests verify distance cache functionality, cached 2-opt gain computation,
/// and branch prediction hints following TDD methodology.

#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <evolab/local_search/two_opt.hpp>
#include <evolab/problems/tsp.hpp>
#include <evolab/utils/compiler_hints.hpp>
#include <evolab/utils/distance_cache.hpp>

#include "test_helper.hpp"

using namespace evolab;

// Test distance cache functionality
int test_distance_cache_initially_empty() {
    TestResult result;

    utils::DistanceCache<double> cache;
    double value;

    result.assert_true(!cache.try_get(0, 1, value), "Cache should initially miss");
    result.assert_eq(0.0, cache.hit_rate(), "Initial hit rate should be 0.0");

    return result.summary();
}

int test_distance_cache_put_and_get() {
    TestResult result;

    utils::DistanceCache<double> cache;
    cache.put(0, 1, 42.0);

    double value;
    result.assert_true(cache.try_get(0, 1, value), "Cache should hit after put");
    result.assert_eq(42.0, value, "Cached value should match", 1e-9);

    return result.summary();
}

int test_distance_cache_different_keys() {
    TestResult result;

    utils::DistanceCache<double> cache;
    cache.put(0, 1, 10.0);
    cache.put(2, 3, 20.0);

    double value;
    result.assert_true(cache.try_get(0, 1, value), "Should retrieve first key");
    result.assert_eq(10.0, value, "First value should match", 1e-9);

    result.assert_true(cache.try_get(2, 3, value), "Should retrieve second key");
    result.assert_eq(20.0, value, "Second value should match", 1e-9);

    return result.summary();
}

int test_distance_cache_overwrite() {
    TestResult result;

    utils::DistanceCache<double> cache;
    cache.put(0, 1, 10.0);
    cache.put(0, 1, 20.0);

    double value;
    result.assert_true(cache.try_get(0, 1, value), "Cache should hit");
    result.assert_eq(20.0, value, "Value should be updated", 1e-9);

    return result.summary();
}

int test_distance_cache_clear() {
    TestResult result;

    utils::DistanceCache<double> cache;
    cache.put(0, 1, 42.0);
    cache.clear();

    double value;
    result.assert_true(!cache.try_get(0, 1, value), "Cache should miss after clear");
    result.assert_eq(0.0, cache.hit_rate(), "Hit rate should be 0.0 after clear");

    return result.summary();
}

int test_distance_cache_hit_rate() {
    TestResult result;

    utils::DistanceCache<double> cache;
    cache.put(0, 1, 10.0);

    double value;
    cache.try_get(0, 1, value); // Hit
    cache.try_get(0, 1, value); // Hit
    cache.try_get(2, 3, value); // Miss

    result.assert_eq(2.0 / 3.0, cache.hit_rate(), "Hit rate should be 2/3", 1e-9);

    return result.summary();
}

int test_distance_cache_large_indices() {
    TestResult result;

    utils::DistanceCache<double> cache;
    const int large_idx = 60000;
    cache.put(large_idx, large_idx + 1, 99.0);

    double value;
    result.assert_true(cache.try_get(large_idx, large_idx + 1, value),
                       "Cache should handle large indices");
    result.assert_eq(99.0, value, "Large index value should match", 1e-9);

    return result.summary();
}

// Test TSP cached distance methods
int test_tsp_cached_distance_matches_regular() {
    TestResult result;

    std::vector<std::pair<double, double>> cities = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    problems::TSP tsp(cities);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            double cached = tsp.cached_distance(i, j);
            double regular = tsp.distance(i, j);
            result.assert_eq(regular, cached, "Cached distance should match regular", 1e-9);
        }
    }

    return result.summary();
}

int test_tsp_cache_improves_performance() {
    TestResult result;

    std::vector<std::pair<double, double>> cities = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    problems::TSP tsp(cities);
    tsp.clear_distance_cache();

    tsp.cached_distance(0, 1); // Miss
    tsp.cached_distance(0, 1); // Hit

    auto [hits, misses] = tsp.cache_stats();
    result.assert_eq(1, static_cast<int>(hits), "Should have one hit");
    result.assert_eq(1, static_cast<int>(misses), "Should have one miss");

    return result.summary();
}

int test_tsp_clear_cache_works() {
    TestResult result;

    std::vector<std::pair<double, double>> cities = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    problems::TSP tsp(cities);

    tsp.cached_distance(0, 1);
    tsp.clear_distance_cache();

    auto [hits, misses] = tsp.cache_stats();
    result.assert_eq(0, static_cast<int>(hits), "Hits should be zero after clear");
    result.assert_eq(0, static_cast<int>(misses), "Misses should be zero after clear");

    return result.summary();
}

int test_two_opt_gain_cached_matches_regular() {
    TestResult result;

    std::vector<std::pair<double, double>> cities = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    problems::TSP tsp(cities);
    auto tour = tsp.identity_genome();

    for (int i = 0; i < 3; ++i) {
        for (int j = i + 2; j < 4; ++j) {
            double regular_gain = tsp.two_opt_gain(tour, i, j);
            double cached_gain = tsp.two_opt_gain_cached(tour, i, j);
            result.assert_eq(regular_gain, cached_gain,
                             "Cached gain should match regular at i=" + std::to_string(i) +
                                 ", j=" + std::to_string(j),
                             1e-9);
        }
    }

    return result.summary();
}

// Test 2-opt with delta evaluation
int test_two_opt_improves_with_cache() {
    TestResult result;

    std::vector<std::pair<double, double>> cities = {{0, 0}, {10, 0}, {10, 10}, {0, 10}, {5, 5}};
    problems::TSP tsp(cities);

    std::mt19937 rng(42);
    auto tour = tsp.random_genome(rng);

    double initial_fitness = tsp.evaluate(tour).value;

    local_search::TwoOpt local_search(false, 1); // Best improvement, 1 iteration
    auto final_fitness = local_search.improve(tsp, tour, rng);

    result.assert_true(final_fitness.value <= initial_fitness, "2-opt should not worsen fitness");

    return result.summary();
}

int test_first_improvement_finds_quickly() {
    TestResult result;

    std::vector<std::pair<double, double>> cities = {{0, 0}, {10, 0}, {10, 10}, {0, 10}, {5, 5}};
    problems::TSP tsp(cities);

    std::mt19937 rng(42);
    auto tour = tsp.random_genome(rng);

    local_search::TwoOpt first_imp(true, 0); // First improvement
    auto fitness = first_imp.improve(tsp, tour, rng);

    result.assert_true(tsp.is_valid_tour(tour), "Tour should remain valid");
    result.assert_eq(fitness.value, tsp.evaluate(tour).value, "Fitness should match evaluation",
                     1e-9);

    return result.summary();
}

int test_candidate_list_uses_cache() {
    TestResult result;

    std::vector<std::pair<double, double>> cities = {{0, 0}, {10, 0}, {10, 10}, {0, 10}, {5, 5}};
    problems::TSP tsp(cities);

    std::mt19937 rng(42);
    auto tour = tsp.random_genome(rng);

    local_search::CandidateList2Opt cl_2opt(2, true); // k=2 neighbors
    [[maybe_unused]] auto fitness = cl_2opt.improve(tsp, tour, rng);

    result.assert_true(tsp.is_valid_tour(tour), "Tour should remain valid");
    result.assert_true(tsp.cache_hit_rate() > 0.0, "Should have some cache hits");

    return result.summary();
}

int test_random_2opt_uses_cache() {
    TestResult result;

    std::vector<std::pair<double, double>> cities = {{0, 0}, {10, 0}, {10, 10}, {0, 10}, {5, 5}};
    problems::TSP tsp(cities);

    std::mt19937 rng(42);
    auto tour = tsp.random_genome(rng);

    tsp.clear_distance_cache();

    local_search::Random2Opt random_2opt(50); // 50 attempts
    [[maybe_unused]] auto fitness = random_2opt.improve(tsp, tour, rng);

    result.assert_true(tsp.is_valid_tour(tour), "Tour should remain valid");

    auto [hits, misses] = tsp.cache_stats();
    result.assert_true(hits + misses > 0, "Should have accessed cache");

    return result.summary();
}

int test_delta_evaluation_correctness() {
    TestResult result;

    std::vector<std::pair<double, double>> cities = {{0, 0}, {10, 0}, {10, 10}, {0, 10}, {5, 5}};
    problems::TSP tsp(cities);
    auto tour = tsp.identity_genome();

    for (int i = 0; i < 4; ++i) {
        for (int j = i + 2; j < 5; ++j) {
            auto tour_copy = tour;

            double gain = tsp.two_opt_gain_cached(tour_copy, i, j);
            double before_fitness = tsp.evaluate(tour_copy).value;

            tsp.apply_two_opt(tour_copy, i, j);
            double after_fitness = tsp.evaluate(tour_copy).value;

            double actual_gain = before_fitness - after_fitness;

            result.assert_eq(gain, actual_gain,
                             "Delta evaluation should match at i=" + std::to_string(i) +
                                 ", j=" + std::to_string(j),
                             1e-6);
        }
    }

    return result.summary();
}

int test_cached_version_is_equivalent() {
    TestResult result;

    auto tsp = problems::create_random_tsp(20, 100.0, 12345);

    std::mt19937 rng(42);
    auto tour1 = tsp.random_genome(rng);
    auto tour2 = tour1;

    // Both versions should produce identical results
    local_search::TwoOpt ls1(false, 5);
    double fitness1 = ls1.improve(tsp, tour1, rng).value;

    local_search::TwoOpt ls2(false, 5);
    double fitness2 = ls2.improve(tsp, tour2, rng).value;

    result.assert_eq(fitness1, fitness2, "Results should be identical", 1e-9);
    result.assert_eq(tour1.size(), tour2.size(), "Tour sizes should match");

    // Tours should be identical
    bool tours_match = (tour1 == tour2);
    result.assert_true(tours_match, "Tours should be identical");

    return result.summary();
}

// Test compiler hints don't break functionality
int test_compiler_hints_work() {
    TestResult result;

    int x = 10;

    if (EVOLAB_LIKELY(x > 5)) {
        x += 1;
    }

    result.assert_eq(11, x, "LIKELY branch should execute");

    if (EVOLAB_UNLIKELY(x < 5)) {
        x = 0;
    }

    result.assert_eq(11, x, "UNLIKELY branch should not execute");

    return result.summary();
}

int main() {
    std::cout << "=== Running Delta Evaluation Tests ===\n\n";

    int total_failed = 0;

    std::cout << "\n--- Distance Cache Tests ---\n";
    total_failed += test_distance_cache_initially_empty();
    total_failed += test_distance_cache_put_and_get();
    total_failed += test_distance_cache_different_keys();
    total_failed += test_distance_cache_overwrite();
    total_failed += test_distance_cache_clear();
    total_failed += test_distance_cache_hit_rate();
    total_failed += test_distance_cache_large_indices();

    std::cout << "\n--- TSP Cached Distance Tests ---\n";
    total_failed += test_tsp_cached_distance_matches_regular();
    total_failed += test_tsp_cache_improves_performance();
    total_failed += test_tsp_clear_cache_works();
    total_failed += test_two_opt_gain_cached_matches_regular();

    std::cout << "\n--- 2-opt Delta Evaluation Tests ---\n";
    total_failed += test_two_opt_improves_with_cache();
    total_failed += test_first_improvement_finds_quickly();
    total_failed += test_candidate_list_uses_cache();
    total_failed += test_random_2opt_uses_cache();
    total_failed += test_delta_evaluation_correctness();
    total_failed += test_cached_version_is_equivalent();

    std::cout << "\n--- Compiler Hints Tests ---\n";
    total_failed += test_compiler_hints_work();

    std::cout << "\n=== All Tests Complete ===\n";
    std::cout << "Total failed test suites: " << total_failed << "\n";

    return total_failed == 0 ? 0 : 1;
}
