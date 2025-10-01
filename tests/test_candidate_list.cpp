/// @file test_candidate_list.cpp
/// @brief Comprehensive test suite for candidate list k-nearest neighbor functionality
///
/// Tests verify correctness of k-NN pre-computation, API completeness, and integration
/// with TSP problem instances following TDD methodology.

#include <cmath>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <evolab/problems/tsp.hpp>
#include <evolab/utils/candidate_list.hpp>

#include "test_helper.hpp"

using namespace evolab;

int test_construction_basic() {
    TestResult result;

    // Create simple 5-city distance matrix
    std::vector<std::vector<double>> dist = {{0.0, 2.0, 3.0, 4.0, 5.0},
                                             {2.0, 0.0, 1.5, 3.5, 4.5},
                                             {3.0, 1.5, 0.0, 2.5, 3.5},
                                             {4.0, 3.5, 2.5, 0.0, 1.0},
                                             {5.0, 4.5, 3.5, 1.0, 0.0}};

    utils::CandidateList cl(dist, 3);

    result.assert_eq(5, cl.size(), "Candidate list size should be 5");
    result.assert_eq(3, cl.k(), "k value should be 3");

    for (int i = 0; i < cl.size(); ++i) {
        const auto& candidates = cl.get_candidates(i);
        result.assert_eq(3, static_cast<int>(candidates.size()),
                         "Each city should have exactly 3 candidates");
    }

    return result.summary();
}

int test_construction_edge_cases() {
    TestResult result;

    std::vector<std::vector<double>> dist = {
        {0.0, 1.0, 2.0, 3.0}, {1.0, 0.0, 1.5, 2.5}, {2.0, 1.5, 0.0, 1.8}, {3.0, 2.5, 1.8, 0.0}};

    // Test k = 1 (minimum useful value)
    {
        utils::CandidateList cl(dist, 1);
        result.assert_eq(1, cl.k(), "k=1 should be preserved");
        for (int i = 0; i < cl.size(); ++i) {
            result.assert_eq(1, static_cast<int>(cl.get_candidates(i).size()),
                             "k=1 should give 1 candidate per city");
        }
    }

    // Test k = n-1 (maximum valid value)
    {
        utils::CandidateList cl(dist, 3);
        result.assert_eq(3, cl.k(), "k=n-1 should be preserved");
        for (int i = 0; i < cl.size(); ++i) {
            result.assert_eq(3, static_cast<int>(cl.get_candidates(i).size()),
                             "k=n-1 should give all other cities as candidates");
        }
    }

    // Test k = 0 (invalid, should auto-correct to n-1)
    {
        utils::CandidateList cl(dist, 0);
        result.assert_eq(3, cl.k(), "k=0 should auto-correct to n-1");
    }

    // Test k > n (invalid, should auto-correct to n-1)
    {
        utils::CandidateList cl(dist, 10);
        result.assert_eq(3, cl.k(), "k > n should auto-correct to n-1");
    }

    // Test k < 0 (invalid, should auto-correct to n-1)
    {
        utils::CandidateList cl(dist, -7);
        result.assert_eq(3, cl.k(), "k < 0 should auto-correct to n-1");
    }

    return result.summary();
}

int test_nearest_neighbor_correctness() {
    TestResult result;

    // Known distance matrix where nearest neighbors are predictable
    std::vector<std::vector<double>> dist = {
        {0.0, 1.0, 5.0, 8.0, 3.0}, // City 0: nearest = [1, 4, 2]
        {1.0, 0.0, 4.0, 7.0, 2.0}, // City 1: nearest = [0, 4, 2]
        {5.0, 4.0, 0.0, 3.0, 6.0}, // City 2: nearest = [3, 1, 0]
        {8.0, 7.0, 3.0, 0.0, 9.0}, // City 3: nearest = [2, 1, 0]
        {3.0, 2.0, 6.0, 9.0, 0.0}  // City 4: nearest = [1, 0, 2]
    };

    utils::CandidateList cl(dist, 3);

    // Verify city 0's nearest neighbors
    {
        const auto& candidates = cl.get_candidates(0);
        result.assert_eq(3, static_cast<int>(candidates.size()), "City 0 should have 3 candidates");

        // Check that candidates are actually the 3 nearest (sorted by distance)
        std::vector<int> expected = {1, 4, 2}; // sorted by distance: 1.0, 3.0, 5.0

        // Verify all expected candidates are present in order
        for (size_t i = 0; i < expected.size(); ++i) {
            result.assert_eq(expected[i], candidates[i],
                             std::format("City 0 candidate {} should be {}", i, expected[i]));
        }
    }

    // Verify city 2's nearest neighbors
    {
        const auto& candidates = cl.get_candidates(2);
        std::vector<int> expected = {3, 1, 0}; // sorted by distance: 3.0, 4.0, 5.0

        for (size_t i = 0; i < expected.size(); ++i) {
            result.assert_eq(expected[i], candidates[i],
                             std::format("City 2 candidate {} should be {}", i, expected[i]));
        }
    }

    return result.summary();
}

int test_candidate_edges() {
    // NOTE: has_candidate_edge() uses OR logic (||), not AND logic (&&).
    // This is semantically correct for checking if an edge between two cities
    // should be considered in local search operations. It returns true if
    // EITHER city has the other in its candidate list.
    TestResult result;

    std::vector<std::vector<double>> dist = {
        {0.0, 1.0, 5.0, 8.0}, {1.0, 0.0, 4.0, 7.0}, {5.0, 4.0, 0.0, 3.0}, {8.0, 7.0, 3.0, 0.0}};

    utils::CandidateList cl(dist, 2);

    // City 0's candidates: [1, 2] (distances: 1.0, 5.0)
    // City 1's candidates: [0, 2] (distances: 1.0, 4.0)
    // Both have each other in their candidate lists → edge exists
    result.assert_true(cl.has_candidate_edge(0, 1),
                       "Cities 0 and 1 should have a candidate edge (bidirectional)");

    // City 0's candidates: [1, 2]
    // City 3's candidates: [2, 1] (distances: 3.0, 7.0)
    // Neither has the other in their candidate lists → no edge
    result.assert_true(!cl.has_candidate_edge(0, 3),
                       "Cities 0 and 3 should not have a candidate edge");

    // City 2's candidates: [3, 1] (distances: 3.0, 4.0)
    // City 3's candidates: [2, 1] (distances: 3.0, 7.0)
    // Both have each other → edge exists
    result.assert_true(cl.has_candidate_edge(2, 3),
                       "Cities 2 and 3 should have a candidate edge (bidirectional)");

    // Test unidirectional candidate edge
    // City 1's candidates: [0, 2] → 3 is NOT in 1's candidates
    // City 3's candidates: [2, 1] → 1 IS in 3's candidates
    // Edge exists because at least one direction exists (OR logic)
    result.assert_true(cl.has_candidate_edge(1, 3),
                       "Cities 1 and 3 should have a candidate edge (unidirectional: 3→1)");

    // Test with minimal k=1 to verify edge detection
    {
        std::vector<std::vector<double>> dist_asym = {
            {0.0, 1.0, 3.0}, {1.0, 0.0, 2.0}, {3.0, 2.0, 0.0}};
        utils::CandidateList cl_asym(dist_asym, 1);
        // For k=1: 0→{1}, 1→{0}, 2→{1}
        // (0,1): bidirectional edge (both have each other)
        result.assert_true(cl_asym.has_candidate_edge(0, 1),
                           "Cities 0 and 1 should have bidirectional candidate edge");

        // (1,2): unidirectional edge (2→1 only, not 1→2)
        result.assert_true(cl_asym.has_candidate_edge(1, 2),
                           "Cities 1 and 2 should have unidirectional candidate edge (2→1)");

        // (0,2): no edge (neither has the other)
        result.assert_true(!cl_asym.has_candidate_edge(0, 2),
                           "Cities 0 and 2 should have no candidate edge");
    }

    return result.summary();
}

int test_candidate_pairs() {
    TestResult result;

    std::vector<std::vector<double>> dist = {{0.0, 1.0, 5.0}, {1.0, 0.0, 4.0}, {5.0, 4.0, 0.0}};

    utils::CandidateList cl(dist, 1);

    auto pairs = cl.get_all_candidate_pairs();

    // For n=3, k=1: Each city has 1 candidate
    // City 0: candidate 1 (distance 1.0)
    // City 1: candidate 0 (distance 1.0)
    // City 2: candidate 1 (distance 4.0)
    // Expected unique pairs: (0,1) and (1,2)
    result.assert_eq(2, static_cast<int>(pairs.size()), "Should have exactly 2 unique pairs");

    if (pairs.size() == 2) {
        result.assert_true(pairs[0] == std::make_pair(0, 1), "First pair should be (0, 1)");
        result.assert_true(pairs[1] == std::make_pair(1, 2), "Second pair should be (1, 2)");
    }

    // Verify all pairs satisfy i < j (no duplicates)
    for (const auto& [i, j] : pairs) {
        result.assert_true(i < j, "All pairs should have i < j");
        result.assert_true(i >= 0 && i < 3, "First element should be valid city index");
        result.assert_true(j >= 0 && j < 3, "Second element should be valid city index");
    }

    return result.summary();
}

int test_factory_function() {
    TestResult result;

    std::vector<std::vector<double>> dist(20, std::vector<double>(20, 0.0));

    // Fill with some distances
    for (int i = 0; i < 20; ++i) {
        for (int j = i + 1; j < 20; ++j) {
            double d = static_cast<double>(i + j);
            dist[i][j] = d;
            dist[j][i] = d;
        }
    }

    // Default k_factor = 2.0
    auto cl = utils::make_candidate_list(dist);

    // For n=20, k is std::max(5, static_cast<int>(2.0 * log(20)))
    int expected_k_default =
        std::max(5, static_cast<int>(2.0 * std::log(static_cast<double>(dist.size()))));
    result.assert_eq(expected_k_default, cl.k(),
                     "Factory should set k to expected value for default k_factor");

    // Test custom k_factor
    auto cl_large = utils::make_candidate_list(dist, 5.0);
    int expected_k_large =
        std::max(5, static_cast<int>(5.0 * std::log(static_cast<double>(dist.size()))));
    result.assert_eq(expected_k_large, cl_large.k(),
                     "Factory should set k to the expected value for k_factor=5.0");

    // Test trivial cases: n <= 1 should yield k=0
    {
        std::vector<std::vector<double>> d0;
        auto cl0 = utils::make_candidate_list(d0);
        result.assert_eq(0, cl0.size(), "n=0 should have size 0");
        result.assert_eq(0, cl0.k(), "n=0 should have k=0");
    }
    {
        std::vector<std::vector<double>> d1(1, std::vector<double>(1, 0.0));
        auto cl1 = utils::make_candidate_list(d1);
        result.assert_eq(1, cl1.size(), "n=1 should have size 1");
        result.assert_eq(0, cl1.k(), "n=1 should have k=0");
    }

    return result.summary();
}

int test_tsp_integration() {
    TestResult result;

    // Create small TSP instance
    constexpr int n = 10;
    auto tsp = problems::create_random_tsp(n, 100.0, 42);

    // Get candidate list through TSP interface
    const auto* cl_ptr = tsp.get_candidate_list(5);

    result.assert_true(cl_ptr != nullptr, "TSP should return valid candidate list pointer");
    result.assert_eq(n, cl_ptr->size(), "Candidate list size should match TSP size");
    result.assert_eq(5, cl_ptr->k(), "Candidate list k should match requested k");

    // Verify candidates are valid city indices
    for (int i = 0; i < n; ++i) {
        const auto& candidates = cl_ptr->get_candidates(i);
        for (int c : candidates) {
            result.assert_true(c >= 0 && c < n, "Candidate should be valid city index");
            result.assert_true(c != i, "City should not be its own candidate");
        }
    }

    // Test caching: second call with same k should return same pointer
    const auto* cl_ptr2 = tsp.get_candidate_list(5);
    result.assert_true(cl_ptr == cl_ptr2, "TSP should cache candidate lists (pointer identity)");

    // Verify structural equality as well (in case pointer identity isn't guaranteed)
    result.assert_eq(cl_ptr->size(), cl_ptr2->size(), "Cached list should have same size");
    result.assert_eq(cl_ptr->k(), cl_ptr2->k(), "Cached list should have same k");
    for (int i = 0; i < n; ++i) {
        const auto& candidates1 = cl_ptr->get_candidates(i);
        const auto& candidates2 = cl_ptr2->get_candidates(i);
        result.assert_true(
            candidates1 == candidates2,
            std::format("Cached list should have identical candidates for city {}", i));
    }

    // Test different k - this demonstrates the caching bug
    const auto* cl_ptr3 = tsp.get_candidate_list(8);
    result.assert_true(cl_ptr3 != nullptr,
                       "TSP should return valid candidate list for different k");
    result.assert_eq(8, cl_ptr3->k(), "New candidate list should have requested k");
    // NOTE: Due to the UB bug documented below, cl_ptr3 may equal cl_ptr (same address)
    // because TSP reuses the same std::optional storage. This is the memory safety issue!
    // Once the bug is fixed with proper caching (e.g., std::map<int, CandidateList>),
    // we should add: result.assert_true(cl_ptr3 != cl_ptr, "Different k should have different
    // pointers");

    // CRITICAL BUG (GitHub Issue #23): The current TSP caching implementation for candidate lists
    // is UNSAFE and causes use-after-free vulnerabilities. It reuses a single cache entry via
    // std::optional, which means any call to `get_candidate_list(k)` with a different `k` value
    // invalidates all previously returned pointers by resetting the optional and constructing a new
    // CandidateList in place. Using such invalidated pointers leads to undefined behavior,
    // potential crashes, and silent data corruption in production.
    //
    // This test verifies the behavior for a new `k`, but deliberately avoids using the old,
    // invalidated pointer to prevent triggering UB in the test suite itself.
    //
    // REQUIRED FIX (HIGHEST PRIORITY): Redesign the caching mechanism to be safe, for example:
    // - Use std::map<int, utils::CandidateList> to cache one list per k value
    // - Or use std::shared_ptr to enable safe pointer sharing
    // - Or document that only one k value should be used per TSP instance lifetime

    return result.summary();
}

int test_large_instance_scalability() {
    TestResult result;

    // Test with larger instance to verify scalability
    const int n = 100;
    std::vector<std::vector<double>> dist(n, std::vector<double>(n, 0.0));

    // Create distance matrix with pattern
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            double d = std::abs(i - j) * 10.0 + (i + j) * 0.1;
            dist[i][j] = d;
            dist[j][i] = d;
        }
    }

    // Construction should complete quickly
    utils::CandidateList cl(dist, 20);

    result.assert_eq(n, cl.size(), "Large instance should have correct size");
    result.assert_eq(20, cl.k(), "Large instance should preserve k");

    // Verify all candidate lists are properly sized
    for (int i = 0; i < n; ++i) {
        const auto& candidates = cl.get_candidates(i);
        result.assert_eq(20, static_cast<int>(candidates.size()),
                         "Each city should have correct number of candidates");
    }

    // Get all pairs (should handle large count efficiently)
    auto pairs = cl.get_all_candidate_pairs();
    result.assert_true(pairs.size() <= static_cast<size_t>(n * 20),
                       "Pair count should be at most n*k");

    return result.summary();
}

#ifdef ENABLE_ASAN_DEMONSTRATION_TESTS
/// Demonstrates the use-after-free bug in TSP candidate list caching.
/// This test is DISABLED by default and should only be enabled when running with
/// AddressSanitizer (ASan) or MemorySanitizer (MSan) to verify the bug exists.
///
/// To enable: compile with -DENABLE_ASAN_DEMONSTRATION_TESTS and run with ASan:
///   cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -DENABLE_ASAN_DEMONSTRATION_TESTS" ..
///
/// This test will deliberately trigger undefined behavior (use-after-free) to
/// demonstrate the critical bug tracked in GitHub Issue #23.
int test_tsp_cache_use_after_free_demonstration() {
    TestResult result;

    auto tsp = problems::create_random_tsp(10, 100.0, 42);

    // Get candidate list with k=5
    const auto* cl_ptr_k5 = tsp.get_candidate_list(5);
    result.assert_true(cl_ptr_k5 != nullptr, "First candidate list should be valid");
    result.assert_eq(5, cl_ptr_k5->k(), "First candidate list should have k=5");

    // This call invalidates the memory cl_ptr_k5 points to
    // because TSP reuses a single cache entry
    const auto* cl_ptr_k8 = tsp.get_candidate_list(8);
    result.assert_true(cl_ptr_k8 != nullptr, "Second candidate list should be valid");
    result.assert_eq(8, cl_ptr_k8->k(), "Second candidate list should have k=8");

    // CRITICAL: Accessing cl_ptr_k5 now is use-after-free!
    // ASan/MSan should detect this and report an error.
    // In production, this could cause crashes or silent data corruption.
    [[maybe_unused]] int size = cl_ptr_k5->size(); // Use-after-free here!
    [[maybe_unused]] int k_value = cl_ptr_k5->k(); // Use-after-free here!

    // Try to use the invalidated pointer's data
    // This will likely show wrong values or crash
    std::cout << "WARNING: Accessing potentially invalidated pointer:\n";
    std::cout << "  Size: " << size << " (expected 10)\n";
    std::cout << "  k: " << k_value << " (expected 5, might show 8 due to corruption)\n";

    // Note: We can't assert correctness here because the behavior is undefined.
    // The purpose is to trigger ASan/MSan detection, not to validate results.

    return result.summary();
}
#endif // ENABLE_ASAN_DEMONSTRATION_TESTS

int main() {
    std::cout << "=== Candidate List Comprehensive Test Suite ===\n\n";

    using test_case = std::function<int()>;
    std::vector<std::pair<std::string, test_case>> tests = {
        {"Basic Construction", test_construction_basic},
        {"Edge Cases (k boundaries)", test_construction_edge_cases},
        {"Nearest Neighbor Correctness", test_nearest_neighbor_correctness},
        {"Candidate Edges", test_candidate_edges},
        {"Candidate Pairs", test_candidate_pairs},
        {"Factory Function", test_factory_function},
        {"TSP Integration", test_tsp_integration},
        {"Large Instance Scalability", test_large_instance_scalability},
    };

#ifdef ENABLE_ASAN_DEMONSTRATION_TESTS
    // Add ASan demonstration test only when explicitly enabled
    tests.emplace_back("TSP Cache Use-After-Free (ASan Demo)",
                       test_tsp_cache_use_after_free_demonstration);
    std::cout << "⚠️  ASan demonstration test enabled - expect memory errors!\n\n";
#endif

    int total_failures = 0;
    for (const auto& [name, func] : tests) {
        std::cout << "Test: " << name << "\n";
        int failures = func();
        total_failures += failures;
    }

    std::cout << "=== All Candidate List Tests Completed ===\n";

    if (total_failures > 0) {
        std::cout << "FAILURE: " << total_failures << " test(s) failed\n";
        return 1;
    }

    std::cout << "SUCCESS: All tests passed\n";
    return 0;
}