#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
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

    TestTSPInstance() : tsp(create_test_cities()), tour(tsp.num_cities()) {
        std::iota(tour.begin(), tour.end(), 0);
    }

  private:
    static std::vector<std::pair<double, double>> create_test_cities() {
        return {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0},
                {0.0, 1.0}, {1.0, 1.0}, {2.0, 1.0}, {3.0, 1.0}, {4.0, 1.0}};
    }
};

// Test fixture for UCBLocalSearchSelector tests
struct UCBSelectorFixture {
    std::mt19937 rng;
    TestTSPInstance tsp_instance;
    UCBLocalSearchSelector<TSP> selector;

    explicit UCBSelectorFixture(size_t num_ops = 2) : rng(42), selector(num_ops, 2.0, rng) {}

    void add_default_operators() {
        selector.add_operator(LinKernighan(20, 5), "LinKernighan");
        selector.add_operator(TwoOpt(), "TwoOpt");
    }

    void add_three_operators() {
        selector.add_operator(LinKernighan(20, 5), "LinKernighan");
        selector.add_operator(TwoOpt(), "TwoOpt");
        selector.add_operator(Random2Opt(100), "Random2Opt");
    }
};

// Test fixture for ThompsonLocalSearchSelector tests
struct ThompsonSelectorFixture {
    std::mt19937 rng;
    TestTSPInstance tsp_instance;
    ThompsonLocalSearchSelector<TSP> selector;

    explicit ThompsonSelectorFixture(size_t num_ops = 2, double reward_threshold = 0.0)
        : rng(42), selector(num_ops, reward_threshold, rng) {}

    void add_default_operators() {
        selector.add_operator(LinKernighan(20, 5), "LinKernighan");
        selector.add_operator(TwoOpt(), "TwoOpt");
    }
};

// Test LocalSearchOperator concept exists
void test_local_search_operator_concept(TestResult& result) {
    // This should compile if the concept exists
    static_assert(core::LocalSearchOperator<LinKernighan, TSP>);
    static_assert(core::LocalSearchOperator<TwoOpt, TSP>);
    static_assert(core::LocalSearchOperator<Random2Opt, TSP>);

    result.assert_true(true, "LocalSearchOperator concept compiles");
}

// Test AdaptiveLocalSearchSelector class exists
void test_adaptive_local_search_selector_exists(TestResult& result) {
    std::mt19937 rng(42);
    // Create selector with UCB scheduler
    UCBLocalSearchSelector<TSP> selector(2, 2.0, rng);

    result.assert_eq(selector.get_operator_count(), static_cast<size_t>(0),
                     "Selector initializes with zero operators");
}

// Test adding local search operators
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

// Test applying local search via selector
void test_apply_local_search(TestResult& result) {
    UCBSelectorFixture fixture;
    fixture.add_default_operators();

    auto initial_fitness = fixture.tsp_instance.tsp.evaluate(fixture.tsp_instance.tour);
    auto tour_copy = fixture.tsp_instance.tour;

    // Apply local search
    auto result_fitness =
        fixture.selector.apply_local_search(fixture.tsp_instance.tsp, tour_copy, fixture.rng);

    // Result should have improved (strict improvement expected for non-optimal initial tour)
    result.assert_lt(result_fitness.value, initial_fitness.value,
                     "Local search should improve the non-optimal initial tour");
    result.assert_ge(fixture.selector.get_last_selection(), 0, "Selected operator is non-negative");
    result.assert_lt(fixture.selector.get_last_selection(), 2,
                     "Selected operator is within bounds");
}

// Test performance tracking for local search
void test_performance_tracking(TestResult& result) {
    UCBSelectorFixture fixture;
    fixture.add_default_operators();

    // Perform multiple applications
    for (int i = 0; i < 10; ++i) {
        auto tour_copy = fixture.tsp_instance.tour;
        auto initial_fitness = fixture.tsp_instance.tsp.evaluate(tour_copy);

        auto final_fitness =
            fixture.selector.apply_local_search(fixture.tsp_instance.tsp, tour_copy, fixture.rng);

        // Report improvement
        fixture.selector.report_fitness_improvement(initial_fitness.value - final_fitness.value);
    }

    // Check that stats are tracked
    const auto& stats = fixture.selector.get_operator_stats();
    result.assert_eq(stats.size(), static_cast<size_t>(2), "Selector has stats for both operators");

    size_t total_selections = 0;
    for (const auto& stat : stats) {
        total_selections += stat.selection_count;
    }
    result.assert_eq(total_selections, static_cast<size_t>(10),
                     "Total selections equals number of applications");
}

// Test execution time tracking
void test_execution_time_tracking(TestResult& result) {
    UCBSelectorFixture fixture;
    fixture.add_default_operators();

    fixture.selector.apply_local_search(fixture.tsp_instance.tsp, fixture.tsp_instance.tour,
                                        fixture.rng);

    // Check that execution time was recorded
    result.assert_ge(fixture.selector.get_last_execution_time(), 0.0,
                     "Execution time is non-negative");
}

// Test improvement rate tracking
void test_improvement_rate_tracking(TestResult& result) {
    ThompsonSelectorFixture fixture(2, 0.0);
    fixture.add_default_operators();

    // Perform multiple applications and track improvements
    for (int i = 0; i < 20; ++i) {
        auto tour_copy = fixture.tsp_instance.tour;
        auto initial_fitness = fixture.tsp_instance.tsp.evaluate(tour_copy);

        auto final_fitness =
            fixture.selector.apply_local_search(fixture.tsp_instance.tsp, tour_copy, fixture.rng);
        double improvement = initial_fitness.value - final_fitness.value;

        fixture.selector.report_fitness_improvement(improvement);
    }

    // Check improvement rate statistics
    const auto& stats = fixture.selector.get_operator_stats();
    for (const auto& stat : stats) {
        if (stat.selection_count > 0) {
            result.assert_ge(stat.success_rate, 0.0, "Success rate is non-negative");
            result.assert_le(stat.success_rate, 1.0, "Success rate is at most 1.0");
        }
    }
}

// Test Thompson Sampling with local search
void test_thompson_sampling_integration(TestResult& result) {
    ThompsonSelectorFixture fixture(2, 0.0);
    fixture.add_default_operators();

    // Run multiple iterations
    for (int i = 0; i < 10; ++i) {
        auto tour_copy = fixture.tsp_instance.tour;
        auto initial_fitness = fixture.tsp_instance.tsp.evaluate(tour_copy);
        auto final_fitness =
            fixture.selector.apply_local_search(fixture.tsp_instance.tsp, tour_copy, fixture.rng);
        fixture.selector.report_fitness_change(initial_fitness.value, final_fitness.value);
    }

    // Verify selector state
    result.assert_eq(fixture.selector.get_operator_count(), static_cast<size_t>(2),
                     "Selector has correct operator count");
    const auto& stats = fixture.selector.get_operator_stats();
    result.assert_eq(stats.size(), static_cast<size_t>(2), "Selector has stats for both operators");
}

// Test hybrid configuration with both crossover and local search
void test_hybrid_crossover_and_local_search(TestResult& result) {
    std::mt19937 rng(42);

    // Create crossover selector
    UCBOperatorSelector<TSP> crossover_selector(2, 2.0, rng);
    crossover_selector.add_operator(OrderCrossover(), "OX");
    crossover_selector.add_operator(PMXCrossover(), "PMX");

    // Create local search selector
    UCBLocalSearchSelector<TSP> ls_selector(2, 2.0, rng);
    ls_selector.add_operator(TwoOpt(), "TwoOpt");
    ls_selector.add_operator(Random2Opt(50), "Random2Opt");

    TestTSPInstance test_instance;

    // Simulate a memetic algorithm generation: crossover -> local search
    for (int gen = 0; gen < 5; ++gen) {
        // Apply crossover to create offspring
        auto [offspring1, offspring2] = crossover_selector.apply_crossover(
            test_instance.tsp, test_instance.tour, test_instance.tour, rng);

        // Report crossover improvement (simplified: use fixed reward for creating offspring)
        crossover_selector.report_fitness_improvement(0.0);

        // Evaluate offspring before local search
        auto fitness_before_ls1 = test_instance.tsp.evaluate(offspring1);
        auto fitness_before_ls2 = test_instance.tsp.evaluate(offspring2);

        // Apply local search to first offspring and report improvement
        auto fitness_after_ls1 = ls_selector.apply_local_search(test_instance.tsp, offspring1, rng);
        ls_selector.report_fitness_improvement(fitness_before_ls1.value - fitness_after_ls1.value);

        // Apply local search to second offspring and report improvement
        auto fitness_after_ls2 = ls_selector.apply_local_search(test_instance.tsp, offspring2, rng);
        ls_selector.report_fitness_improvement(fitness_before_ls2.value - fitness_after_ls2.value);
    }

    // Verify both selectors tracked statistics
    result.assert_eq(crossover_selector.get_operator_count(), static_cast<size_t>(2),
                     "Crossover selector has correct operator count");
    result.assert_eq(ls_selector.get_operator_count(), static_cast<size_t>(2),
                     "Local search selector has correct operator count");

    const auto& crossover_stats = crossover_selector.get_operator_stats();
    const auto& ls_stats = ls_selector.get_operator_stats();

    result.assert_eq(crossover_stats.size(), static_cast<size_t>(2),
                     "Crossover selector tracked both operators");
    result.assert_eq(ls_stats.size(), static_cast<size_t>(2),
                     "Local search selector tracked both operators");

    // Verify operators were actually used
    size_t total_crossover_selections = 0;
    size_t total_ls_selections = 0;
    for (const auto& stat : crossover_stats) {
        total_crossover_selections += stat.selection_count;
    }
    for (const auto& stat : ls_stats) {
        total_ls_selections += stat.selection_count;
    }

    result.assert_eq(total_crossover_selections, static_cast<size_t>(5),
                     "Crossover selector made expected number of selections");
    result.assert_eq(total_ls_selections, static_cast<size_t>(10),
                     "Local search selector made expected number of selections");
}

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
