/// @file test_candidate_list.cpp
/// @brief Comprehensive test suite for candidate list k-nearest neighbor functionality
///
/// Tests verify correctness of k-NN pre-computation, API completeness, and integration
/// with TSP problem instances following TDD methodology.

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <evolab/problems/tsp.hpp>
#include <evolab/utils/candidate_list.hpp>

using namespace evolab;

class TestResult {
  private:
    int passed = 0;
    int failed = 0;
    std::vector<std::string> failures;

  public:
    void assert_true(bool condition, const std::string& message) {
        if (condition) {
            ++passed;
        } else {
            ++failed;
            failures.push_back("FAILED: " + message);
        }
    }

    void assert_eq(int expected, int actual, const std::string& message) {
        if (expected == actual) {
            ++passed;
        } else {
            ++failed;
            failures.push_back("FAILED: " + message + " (expected: " + std::to_string(expected) +
                               ", actual: " + std::to_string(actual) + ")");
        }
    }

    void assert_eq(double expected, double actual, const std::string& message,
                   double epsilon = 1e-9) {
        if (std::abs(expected - actual) < epsilon) {
            ++passed;
        } else {
            ++failed;
            failures.push_back("FAILED: " + message + " (expected: " + std::to_string(expected) +
                               ", actual: " + std::to_string(actual) + ")");
        }
    }

    int print_summary() {
        std::cout << "Passed: " << passed << ", Failed: " << failed << "\n";
        if (!failures.empty()) {
            std::cout << "Failures:\n";
            for (const auto& failure : failures) {
                std::cout << "  " << failure << "\n";
            }
        }
        std::cout << "\n";
        return failed;
    }
};

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

    for (int i = 0; i < 5; ++i) {
        const auto& candidates = cl.get_candidates(i);
        result.assert_eq(3, static_cast<int>(candidates.size()),
                         "Each city should have exactly 3 candidates");
    }

    return result.print_summary();
}

int test_construction_edge_cases() {
    TestResult result;

    std::vector<std::vector<double>> dist = {
        {0.0, 1.0, 2.0, 3.0}, {1.0, 0.0, 1.5, 2.5}, {2.0, 1.5, 0.0, 1.8}, {3.0, 2.5, 1.8, 0.0}};

    // Test k = 1 (minimum useful value)
    {
        utils::CandidateList cl(dist, 1);
        result.assert_eq(1, cl.k(), "k=1 should be preserved");
        for (int i = 0; i < 4; ++i) {
            result.assert_eq(1, static_cast<int>(cl.get_candidates(i).size()),
                             "k=1 should give 1 candidate per city");
        }
    }

    // Test k = n-1 (maximum valid value)
    {
        utils::CandidateList cl(dist, 3);
        result.assert_eq(3, cl.k(), "k=n-1 should be preserved");
        for (int i = 0; i < 4; ++i) {
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

    return result.print_summary();
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
                             "City 0 candidate " + std::to_string(i) + " should be " +
                                 std::to_string(expected[i]));
        }
    }

    // Verify city 2's nearest neighbors
    {
        const auto& candidates = cl.get_candidates(2);
        std::vector<int> expected = {3, 1, 0}; // sorted by distance: 3.0, 4.0, 5.0

        for (size_t i = 0; i < expected.size(); ++i) {
            result.assert_eq(expected[i], candidates[i],
                             "City 2 candidate " + std::to_string(i) + " should be " +
                                 std::to_string(expected[i]));
        }
    }

    return result.print_summary();
}

int test_mutual_candidates() {
    TestResult result;

    std::vector<std::vector<double>> dist = {
        {0.0, 1.0, 5.0, 8.0}, {1.0, 0.0, 4.0, 7.0}, {5.0, 4.0, 0.0, 3.0}, {8.0, 7.0, 3.0, 0.0}};

    utils::CandidateList cl(dist, 2);

    // City 0's candidates: [1, 2] (distances: 1.0, 5.0)
    // City 1's candidates: [0, 2] (distances: 1.0, 4.0)
    // They share each other as candidates
    result.assert_true(cl.are_mutual_candidates(0, 1),
                       "Cities 0 and 1 should be mutual candidates");

    // City 0's candidates: [1, 2]
    // City 3's candidates: [2, 1] (distances: 3.0, 7.0)
    // They don't share each other (asymmetric relationship)
    result.assert_true(!cl.are_mutual_candidates(0, 3),
                       "Cities 0 and 3 should not be mutual candidates");

    // City 2's candidates: [3, 1] (distances: 3.0, 4.0)
    // City 3's candidates: [2, 1] (distances: 3.0, 7.0)
    // They share each other
    result.assert_true(cl.are_mutual_candidates(2, 3),
                       "Cities 2 and 3 should be mutual candidates");

    // Test asymmetric candidate relationship explicitly
    // City 1's candidates: [0, 2] → 3 is NOT in 1's candidates
    // City 3's candidates: [2, 1] → 1 IS in 3's candidates
    // This is still mutual because 1 appears in 3's list (even though 3 doesn't appear in 1's)
    result.assert_true(cl.are_mutual_candidates(1, 3),
                       "Cities 1 and 3 should be mutual candidates (1 is in 3's list)");

    // Test truly asymmetric relationship: neither is in the other's list
    // City 0's candidates: [1, 2] → 3 is NOT in 0's candidates
    // City 3's candidates: [2, 1] → 0 is NOT in 3's candidates
    result.assert_true(!cl.are_mutual_candidates(0, 3),
                       "Cities 0 and 3 should not be mutual candidates (neither in other's list)");

    return result.print_summary();
}

int test_candidate_pairs() {
    TestResult result;

    std::vector<std::vector<double>> dist = {{0.0, 1.0, 5.0}, {1.0, 0.0, 4.0}, {5.0, 4.0, 0.0}};

    utils::CandidateList cl(dist, 1);

    auto pairs = cl.get_all_candidate_pairs();

    result.assert_true(pairs.size() > 0, "Should have at least one candidate pair");
    // Each city contributes k candidates, but pairs may be duplicated or asymmetric
    result.assert_true(pairs.size() <= 3, "Should have at most n*k unique pairs");

    // Verify all pairs satisfy i < j (no duplicates)
    for (const auto& [i, j] : pairs) {
        result.assert_true(i < j, "All pairs should have i < j");
        result.assert_true(i >= 0 && i < 3, "First element should be valid city index");
        result.assert_true(j >= 0 && j < 3, "Second element should be valid city index");
    }

    return result.print_summary();
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

    // For n=20, k should be around 2.0 * log(20) ≈ 2.0 * 2.996 ≈ 5.99
    int expected_k_default = static_cast<int>(2.0 * std::log(20.0));
    result.assert_true(cl.k() >= expected_k_default - 1, "Factory should set k near 2.0 * log(n)");
    result.assert_true(cl.k() <= expected_k_default + 2, "Factory should set k near 2.0 * log(n)");

    // Test custom k_factor
    auto cl_large = utils::make_candidate_list(dist, 5.0);
    int expected_k_large = static_cast<int>(5.0 * std::log(20.0));
    result.assert_true(cl_large.k() > cl.k(), "Larger k_factor should produce larger k");
    result.assert_true(cl_large.k() >= expected_k_large - 1,
                       "Factory should set k near 5.0 * log(n)");

    return result.print_summary();
}

int test_tsp_integration() {
    TestResult result;

    // Create small TSP instance
    auto tsp = problems::create_random_tsp(10, 100.0, 42);

    // Get candidate list through TSP interface
    const auto* cl_ptr = tsp.get_candidate_list(5);

    result.assert_true(cl_ptr != nullptr, "TSP should return valid candidate list pointer");
    result.assert_eq(10, cl_ptr->size(), "Candidate list size should match TSP size");
    result.assert_eq(5, cl_ptr->k(), "Candidate list k should match requested k");

    // Verify candidates are valid city indices
    for (int i = 0; i < 10; ++i) {
        const auto& candidates = cl_ptr->get_candidates(i);
        for (int c : candidates) {
            result.assert_true(c >= 0 && c < 10, "Candidate should be valid city index");
            result.assert_true(c != i, "City should not be its own candidate");
        }
    }

    // Test caching: second call with same k should return same pointer
    const auto* cl_ptr2 = tsp.get_candidate_list(5);
    result.assert_true(cl_ptr == cl_ptr2, "TSP should cache candidate lists (pointer identity)");

    // Verify structural equality as well (in case pointer identity isn't guaranteed)
    result.assert_eq(cl_ptr->size(), cl_ptr2->size(), "Cached list should have same size");
    result.assert_eq(cl_ptr->k(), cl_ptr2->k(), "Cached list should have same k");
    for (int i = 0; i < 10; ++i) {
        const auto& candidates1 = cl_ptr->get_candidates(i);
        const auto& candidates2 = cl_ptr2->get_candidates(i);
        result.assert_true(candidates1 == candidates2,
                           "Cached list should have identical candidates for city " +
                               std::to_string(i));
    }

    // Test different k creates new list
    const auto* cl_ptr3 = tsp.get_candidate_list(8);
    result.assert_true(cl_ptr3 != nullptr, "TSP should create new candidate list for different k");
    result.assert_eq(8, cl_ptr3->k(), "New candidate list should have requested k");

    return result.print_summary();
}

int test_large_instance_performance() {
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

    return result.print_summary();
}

int main() {
    std::cout << "=== Candidate List Comprehensive Test Suite ===\n\n";

    int total_failures = 0;

    std::cout << "Test: Basic Construction\n";
    total_failures += test_construction_basic();

    std::cout << "Test: Edge Cases (k boundaries)\n";
    total_failures += test_construction_edge_cases();

    std::cout << "Test: Nearest Neighbor Correctness\n";
    total_failures += test_nearest_neighbor_correctness();

    std::cout << "Test: Mutual Candidates\n";
    total_failures += test_mutual_candidates();

    std::cout << "Test: Candidate Pairs\n";
    total_failures += test_candidate_pairs();

    std::cout << "Test: Factory Function\n";
    total_failures += test_factory_function();

    std::cout << "Test: TSP Integration\n";
    total_failures += test_tsp_integration();

    std::cout << "Test: Large Instance Performance\n";
    total_failures += test_large_instance_performance();

    std::cout << "=== All Candidate List Tests Completed ===\n";

    if (total_failures > 0) {
        std::cout << "FAILURE: " << total_failures << " test(s) failed\n";
        return 1;
    }

    std::cout << "SUCCESS: All tests passed\n";
    return 0;
}