#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <vector>

#include <evolab/evolab.hpp>

#include "test_helper.hpp"

using namespace evolab;
using namespace evolab::schedulers;
using namespace evolab::local_search;
using namespace evolab::operators;
using namespace evolab::problems;

// Helper function to create a small TSP instance for testing
struct TestTSPInstance {
    TSP tsp;
    std::vector<int> tour;

    /**
     * @brief Create a small deterministic TSP instance and an initial identity tour.
     *
     * Initializes `tsp` with a fixed set of test cities (a simple 10-city grid)
     * and sets `tour` to the identity permutation [0, 1, ..., tsp.num_cities()-1].
     */
    TestTSPInstance() : tsp(create_test_cities()), tour(tsp.num_cities()) {
        std::iota(tour.begin(), tour.end(), 0);
    }

  private:
    /**
     * @brief Creates a small, deterministic set of city coordinates for TSP tests.
     *
     * The returned cities form a simple 2x5 grid: five cities on y=0 followed by five on y=1,
     * with x coordinates 0 through 4.
     *
     * @return std::vector<std::pair<double,double>> A list of 10 (x, y) city coordinates.
     */
    static std::vector<std::pair<double, double>> create_test_cities() {
        return {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0},
                {0.0, 1.0}, {1.0, 1.0}, {2.0, 1.0}, {3.0, 1.0}, {4.0, 1.0}};
    }
};

/**
 * @brief Verifies at compile time that common local search implementations satisfy the LocalSearchOperator concept.
 *
 * Performs static assertions for LinKernighan, TwoOpt, and Random2Opt specialized for TSP and records a passing assertion in the provided TestResult if compilation succeeds.
 *
 * @param result TestResult used to record the outcome of the compile-time check.
 */
void test_local_search_operator_concept(TestResult& result) {
    // This should compile if the concept exists
    static_assert(LocalSearchOperator<LinKernighan, TSP>);
    static_assert(LocalSearchOperator<TwoOpt, TSP>);
    static_assert(LocalSearchOperator<Random2Opt, TSP>);

    result.assert_true(true, "LocalSearchOperator concept compiles");
}

/**
 * @brief Verifies that a UCB-based adaptive local search selector initializes with no registered operators.
 *
 * @param result TestResult used to record the assertion outcome.
 */
void test_adaptive_local_search_selector_exists(TestResult& result) {
    std::mt19937 rng(42);
    // Create selector with UCB scheduler
    UCBLocalSearchSelector<TSP> selector(2, 2.0, rng);

    result.assert_eq(selector.get_operator_count(), static_cast<size_t>(0),
                     "Selector initializes with zero operators");
}

/**
 * @brief Verifies adding local search operators to a UCBLocalSearchSelector and validates operator count and names.
 *
 * Uses a deterministic RNG and three concrete local search operators to ensure the selector records
 * the correct number of operators and preserves the registration order of their names.
 *
 * @param result Test harness object used to record assertions and test outcomes.
 */
void test_add_local_search_operators(TestResult& result) {
    std::mt19937 rng(42);
    UCBLocalSearchSelector<TSP> selector(3, 2.0, rng);

    LinKernighan lk(20, 5);
    TwoOpt two_opt;
    Random2Opt random_2opt(100);

    selector.add_operator(lk, "LinKernighan");
    selector.add_operator(two_opt, "TwoOpt");
    selector.add_operator(random_2opt, "Random2Opt");

    result.assert_eq(selector.get_operator_count(), static_cast<size_t>(3),
                     "Selector has correct operator count");
    result.assert_eq(selector.get_operator_names().size(), static_cast<size_t>(3),
                     "Selector has correct number of operator names");
    result.assert_true(selector.get_operator_names()[0] == "LinKernighan",
                       "First operator name is LinKernighan");
    result.assert_true(selector.get_operator_names()[1] == "TwoOpt",
                       "Second operator name is TwoOpt");
    result.assert_true(selector.get_operator_names()[2] == "Random2Opt",
                       "Third operator name is Random2Opt");
}

// TDD RED: Test applying local search via selector
void test_apply_local_search(TestResult& result) {
    std::mt19937 rng(42);
    UCBLocalSearchSelector<TSP> selector(2, 2.0, rng);

    LinKernighan lk(20, 5);
    TwoOpt two_opt;

    selector.add_operator(lk, "LinKernighan");
    selector.add_operator(two_opt, "TwoOpt");

    TestTSPInstance test_instance;
    auto initial_fitness = test_instance.tsp.evaluate(test_instance.tour);
    auto tour_copy = test_instance.tour;

    // Apply local search
    auto result_fitness = selector.apply_local_search(test_instance.tsp, tour_copy, rng);

    // Result should have improved or stayed the same (minimization)
    result.assert_le(result_fitness.value, initial_fitness.value,
                     "Local search improves or maintains fitness");
    result.assert_ge(selector.get_last_selection(), 0, "Selected operator is non-negative");
    result.assert_lt(selector.get_last_selection(), 2, "Selected operator is within bounds");
}

/**
 * @brief Verifies that the UCB local-search selector tracks per-operator performance across repeated applications.
 *
 * Runs 10 local-search applications on a small deterministic TSP instance using two registered operators,
 * reports the observed fitness improvements to the selector, and asserts that the selector maintains stats
 * for both operators and that the sum of selection counts equals the number of applications.
 *
 * @param result TestResult accumulator used to record assertions and failures for this test.
 */
void test_performance_tracking(TestResult& result) {
    std::mt19937 rng(42);
    UCBLocalSearchSelector<TSP> selector(2, 2.0, rng);

    LinKernighan lk(20, 5);
    TwoOpt two_opt;

    selector.add_operator(lk, "LinKernighan");
    selector.add_operator(two_opt, "TwoOpt");

    TestTSPInstance test_instance;

    // Perform multiple applications
    for (int i = 0; i < 10; ++i) {
        auto tour_copy = test_instance.tour;
        auto initial_fitness = test_instance.tsp.evaluate(tour_copy);

        auto final_fitness = selector.apply_local_search(test_instance.tsp, tour_copy, rng);

        // Report improvement
        selector.report_fitness_improvement(initial_fitness.value - final_fitness.value);
    }

    // Check that stats are tracked
    const auto& stats = selector.get_operator_stats();
    result.assert_eq(stats.size(), static_cast<size_t>(2), "Selector has stats for both operators");

    size_t total_selections = 0;
    for (const auto& stat : stats) {
        total_selections += stat.selection_count;
    }
    result.assert_eq(total_selections, static_cast<size_t>(10),
                     "Total selections equals number of applications");
}

/**
 * @brief Verifies that a local search selector records a positive execution time after applying local search.
 *
 * Creates a UCBLocalSearchSelector with two local-search operators, applies it to a small deterministic TSP instance,
 * and asserts that the selector's last recorded execution time is greater than zero.
 *
 * @param result TestResult collector used to record the assertion outcome.
 */
void test_execution_time_tracking(TestResult& result) {
    std::mt19937 rng(42);
    UCBLocalSearchSelector<TSP> selector(2, 2.0, rng);

    LinKernighan lk(20, 5);
    TwoOpt two_opt;

    selector.add_operator(lk, "LinKernighan");
    selector.add_operator(two_opt, "TwoOpt");

    TestTSPInstance test_instance;

    selector.apply_local_search(test_instance.tsp, test_instance.tour, rng);

    // Check that execution time was recorded
    result.assert_gt(selector.get_last_execution_time(), 0.0, "Execution time is positive");
}

/**
 * @brief Runs repeated local-search applications to exercise improvement-rate tracking and validates reported rates.
 *
 * Executes multiple local-search applications using a Thompson sampling selector with two operators, reports per-run fitness improvements to the selector, and asserts that each operator's recorded success rate (for operators selected at least once) lies between 0.0 and 1.0 inclusive.
 *
 * @param result Test harness object used to record assertions and test outcomes.
 */
void test_improvement_rate_tracking(TestResult& result) {
    std::mt19937 rng(42);
    ThompsonLocalSearchSelector<TSP> selector(2, 0.0, rng);

    LinKernighan lk(20, 5);
    TwoOpt two_opt;

    selector.add_operator(lk, "LinKernighan");
    selector.add_operator(two_opt, "TwoOpt");

    TestTSPInstance test_instance;

    // Perform multiple applications and track improvements
    for (int i = 0; i < 20; ++i) {
        auto tour_copy = test_instance.tour;
        auto initial_fitness = test_instance.tsp.evaluate(tour_copy);

        auto final_fitness = selector.apply_local_search(test_instance.tsp, tour_copy, rng);
        double improvement = initial_fitness.value - final_fitness.value;

        selector.report_fitness_improvement(improvement);
    }

    // Check improvement rate statistics
    const auto& stats = selector.get_operator_stats();
    for (const auto& stat : stats) {
        if (stat.selection_count > 0) {
            result.assert_ge(stat.success_rate, 0.0, "Success rate is non-negative");
            result.assert_le(stat.success_rate, 1.0, "Success rate is at most 1.0");
        }
    }
}

/**
 * @brief Runs an integration test of Thompson Sampling with local search operators on a TSP.
 *
 * Registers two local search operators (LinKernighan and TwoOpt), runs 10 iterations where it
 * applies selected local search to a test TSP tour and reports the observed fitness changes to the
 * selector, then verifies that the selector has recorded statistics for both operators.
 *
 * @param result Test harness object used to record assertions and the test outcome.
 */
void test_thompson_sampling_integration(TestResult& result) {
    std::mt19937 rng(42);
    ThompsonLocalSearchSelector<TSP> selector(2, 0.0, rng);

    LinKernighan lk(20, 5);
    TwoOpt two_opt;

    selector.add_operator(lk, "LinKernighan");
    selector.add_operator(two_opt, "TwoOpt");

    TestTSPInstance test_instance;

    // Run multiple iterations
    for (int i = 0; i < 10; ++i) {
        auto tour_copy = test_instance.tour;
        auto initial_fitness = test_instance.tsp.evaluate(tour_copy);
        auto final_fitness = selector.apply_local_search(test_instance.tsp, tour_copy, rng);
        selector.report_fitness_change(initial_fitness.value, final_fitness.value);
    }

    // Verify selector state
    result.assert_eq(selector.get_operator_count(), static_cast<size_t>(2),
                     "Selector has correct operator count");
    const auto& stats = selector.get_operator_stats();
    result.assert_eq(stats.size(), static_cast<size_t>(2), "Selector has stats for both operators");
}

/**
 * @brief Verifies that crossover and local-search UCB selectors can be instantiated together.
 *
 * Creates a UCBOperatorSelector for crossover and a UCBLocalSearchSelector for local search,
 * and asserts that each selector starts with zero registered operators.
 *
 * @param result TestResult collector used to record assertions and outcomes.
 */
void test_hybrid_crossover_and_local_search(TestResult& result) {
    std::mt19937 rng(42);

    // Create crossover selector
    UCBOperatorSelector<TSP> crossover_selector(2, 2.0, rng);

    // Create local search selector
    UCBLocalSearchSelector<TSP> ls_selector(2, 2.0, rng);

    // The existence of both types shows we can use them together
    result.assert_eq(crossover_selector.get_operator_count(), static_cast<size_t>(0),
                     "Crossover selector initializes with zero operators");
    result.assert_eq(ls_selector.get_operator_count(), static_cast<size_t>(0),
                     "Local search selector initializes with zero operators");
}

/**
 * @brief Executes the EvoLab MAB local search integration test suite.
 *
 * Runs all defined unit tests for adaptive local search selectors, operators,
 * performance and execution-time tracking, Thompson sampling integration, and
 * hybrid crossover/local-search interaction, then aggregates results.
 *
 * @return int Test suite summary code: `0` if all tests passed, non-zero otherwise.
 */
int main() {
    std::cout << "=== EvoLab MAB Local Search Integration Tests ===\n\n";

    TestResult result;

    test_local_search_operator_concept(result);
    test_adaptive_local_search_selector_exists(result);
    test_add_local_search_operators(result);
    test_apply_local_search(result);
    test_performance_tracking(result);
    test_execution_time_tracking(result);
    test_improvement_rate_tracking(result);
    test_thompson_sampling_integration(result);
    test_hybrid_crossover_and_local_search(result);

    return result.summary();
}