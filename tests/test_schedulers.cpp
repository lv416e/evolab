#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include <evolab/evolab.hpp>

using namespace evolab::schedulers;
using namespace evolab::operators;

struct TestResult {
    int passed = 0;
    int failed = 0;

    void assert_true(bool condition, const std::string& message) {
        if (condition) {
            passed++;
            std::cout << "[PASS] " << message << "\n";
        } else {
            failed++;
            std::cout << "[FAIL] " << message << "\n";
        }
    }

    void assert_eq(int expected, int actual, const std::string& message) {
        assert_true(expected == actual, message + " (expected: " + std::to_string(expected) +
                                            ", actual: " + std::to_string(actual) + ")");
    }

    void assert_eq(size_t expected, size_t actual, const std::string& message) {
        assert_true(expected == actual, message + " (expected: " + std::to_string(expected) +
                                            ", actual: " + std::to_string(actual) + ")");
    }

    void assert_eq(double expected, double actual, const std::string& message,
                   double tolerance = 1e-9) {
        assert_true(std::abs(expected - actual) < tolerance,
                    message + " (expected: " + std::to_string(expected) +
                        ", actual: " + std::to_string(actual) + ")");
    }

    void assert_ge(int value, int min_value, const std::string& message) {
        assert_true(value >= min_value, message + " (" + std::to_string(value) +
                                            " >= " + std::to_string(min_value) + ")");
    }

    void assert_lt(int value, int max_value, const std::string& message) {
        assert_true(value < max_value, message + " (" + std::to_string(value) + " < " +
                                           std::to_string(max_value) + ")");
    }

    void assert_gt(size_t value, size_t min_value, const std::string& message) {
        assert_true(value > min_value, message + " (" + std::to_string(value) + " > " +
                                           std::to_string(min_value) + ")");
    }

    int summary() {
        std::cout << "\n=== Scheduler Test Summary ===\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Total:  " << (passed + failed) << "\n";
        return failed == 0 ? 0 : 1;
    }
};

void test_ucb_scheduler_initialization(TestResult& result) {
    std::mt19937_64 rng(42);
    UCBScheduler scheduler(3, 2.0, rng);

    result.assert_eq(scheduler.get_stats().size(), static_cast<size_t>(3),
                     "UCB scheduler has correct number of operators");

    for (const auto& stat : scheduler.get_stats()) {
        result.assert_eq(stat.selection_count, 0, "Initial selection count is zero");
        result.assert_eq(stat.total_reward, 0.0, "Initial total reward is zero");
        result.assert_eq(stat.avg_reward, 0.0, "Initial average reward is zero");
        result.assert_eq(stat.success_rate, 0.0, "Initial success rate is zero");
    }
}

void test_ucb_scheduler_selection(TestResult& result) {
    std::mt19937_64 rng(42);
    UCBScheduler scheduler(3, 2.0, rng);

    std::vector<int> selections;
    for (int i = 0; i < 10; ++i) {
        int selected = scheduler.select_operator();
        selections.push_back(selected);
        scheduler.update_reward(selected, 1.0);
    }

    result.assert_gt(selections.size(), static_cast<size_t>(0), "Made selections");

    for (int sel : selections) {
        result.assert_ge(sel, 0, "Selection is non-negative");
        result.assert_lt(sel, 3, "Selection is within bounds");
    }
}

void test_ucb_scheduler_reward_update(TestResult& result) {
    std::mt19937_64 rng(42);
    UCBScheduler scheduler(2, 2.0, rng);

    scheduler.select_operator();
    scheduler.update_reward(0, 5.0);

    scheduler.select_operator();
    scheduler.update_reward(1, -2.0);

    const auto& stats = scheduler.get_stats();
    result.assert_eq(stats[0].selection_count, 1, "Operator 0 selection count updated");
    result.assert_eq(stats[0].total_reward, 5.0, "Operator 0 total reward updated");
    result.assert_eq(stats[0].avg_reward, 5.0, "Operator 0 average reward calculated");
    result.assert_eq(stats[0].success_count, 1, "Operator 0 success count updated");

    result.assert_eq(stats[1].selection_count, 1, "Operator 1 selection count updated");
    result.assert_eq(stats[1].total_reward, -2.0, "Operator 1 total reward updated");
    result.assert_eq(stats[1].avg_reward, -2.0, "Operator 1 average reward calculated");
    result.assert_eq(stats[1].success_count, 0, "Operator 1 success count correct");
}

void test_thompson_sampling_initialization(TestResult& result) {
    std::mt19937_64 rng(42);
    ThompsonSamplingScheduler scheduler(3, 0.0, rng);

    result.assert_eq(scheduler.get_stats().size(), static_cast<size_t>(3),
                     "Thompson sampling scheduler has correct number of operators");
    result.assert_eq(scheduler.get_reward_threshold(), 0.0, "Initial reward threshold is correct");

    for (const auto& stat : scheduler.get_stats()) {
        result.assert_eq(stat.selection_count, 0, "Initial selection count is zero");
        result.assert_eq(stat.total_reward, 0.0, "Initial total reward is zero");
    }
}

void test_thompson_sampling_selection(TestResult& result) {
    std::mt19937_64 rng(42);
    ThompsonSamplingScheduler scheduler(3, 0.0, rng);

    std::vector<int> selections;
    for (int i = 0; i < 20; ++i) {
        int selected = scheduler.select_operator();
        selections.push_back(selected);

        double reward = (selected == 0) ? 2.0 : -1.0;
        scheduler.update_reward(selected, reward);
    }

    result.assert_eq(selections.size(), static_cast<size_t>(20), "Made 20 selections");

    for (int sel : selections) {
        result.assert_ge(sel, 0, "Selection is non-negative");
        result.assert_lt(sel, 3, "Selection is within bounds");
    }

    const auto& stats = scheduler.get_stats();
    bool has_selections = false;
    for (const auto& stat : stats) {
        if (stat.selection_count > 0) {
            has_selections = true;
            break;
        }
    }
    result.assert_true(has_selections, "At least one operator was selected");
}

void test_thompson_sampling_reward_threshold(TestResult& result) {
    std::mt19937_64 rng(42);
    ThompsonSamplingScheduler scheduler(2, 1.0, rng);

    scheduler.select_operator();
    scheduler.update_reward(0, 2.0); // Above threshold

    scheduler.select_operator();
    scheduler.update_reward(1, 0.5); // Below threshold

    const auto& stats = scheduler.get_stats();
    result.assert_eq(1, stats[0].success_count, "Operator 0 success from above-threshold reward");
    result.assert_eq(0, stats[1].success_count,
                     "Operator 1 no success from below-threshold reward");
}

void test_adaptive_operator_selector_basic(TestResult& result) {
    std::mt19937_64 rng(42);
    using TSP = evolab::problems::TSP;
    UCBOperatorSelector<TSP> selector(3, 2.0, rng);

    PMXCrossover pmx;
    OrderCrossover ox;
    CycleCrossover cx;

    selector.add_operator(pmx, "PMX");
    selector.add_operator(ox, "OX");
    selector.add_operator(cx, "CX");

    result.assert_eq(selector.get_operator_count(), static_cast<size_t>(3),
                     "Selector has correct operator count");
    result.assert_eq(selector.get_operator_names().size(), static_cast<size_t>(3),
                     "Selector has correct number of operator names");
    result.assert_true(selector.get_operator_names()[0] == "PMX", "First operator name is PMX");
    result.assert_true(selector.get_operator_names()[1] == "OX", "Second operator name is OX");
    result.assert_true(selector.get_operator_names()[2] == "CX", "Third operator name is CX");
}

void test_adaptive_operator_selector_crossover(TestResult& result) {
    std::mt19937_64 rng(42);
    using TSP = evolab::problems::TSP;
    UCBOperatorSelector<TSP> selector(2, 2.0, rng);

    PMXCrossover pmx;
    OrderCrossover ox;

    selector.add_operator(pmx, "PMX");
    selector.add_operator(ox, "OX");

    // Create a simple TSP problem
    std::vector<double> distances(25);
    for (int i = 0; i < 25; ++i)
        distances[i] = static_cast<double>(i % 10 + 1);
    TSP problem(5, distances);

    std::vector<int> parent1 = {0, 1, 2, 3, 4};
    std::vector<int> parent2 = {4, 3, 2, 1, 0};

    std::mt19937 rng32(rng()); // Convert to 32-bit RNG for crossover operators
    auto [child1, child2] = selector.apply_crossover(problem, parent1, parent2, rng32);

    result.assert_eq(child1.size(), parent1.size(), "Child1 has correct size");
    result.assert_eq(child2.size(), parent2.size(), "Child2 has correct size");

    result.assert_ge(selector.get_last_selection(), 0, "Selected operator is non-negative");
    result.assert_lt(selector.get_last_selection(), 2, "Selected operator is within bounds");
}

void test_adaptive_operator_selector_reward_tracking(TestResult& result) {
    std::mt19937_64 rng(42);
    using TSP = evolab::problems::TSP;
    ThompsonOperatorSelector<TSP> selector(2, 0.0, rng);

    PMXCrossover pmx;
    OrderCrossover ox;

    selector.add_operator(pmx, "PMX");
    selector.add_operator(ox, "OX");

    // Create a simple TSP problem
    std::vector<double> distances(25);
    for (int i = 0; i < 25; ++i)
        distances[i] = static_cast<double>(i % 10 + 1);
    TSP problem(5, distances);

    std::vector<int> parent1 = {0, 1, 2, 3, 4};
    std::vector<int> parent2 = {4, 3, 2, 1, 0};

    std::mt19937 rng32(rng()); // Convert to 32-bit RNG for crossover operators
    auto [child1, child2] = selector.apply_crossover(problem, parent1, parent2, rng32);
    selector.report_fitness_improvement(1.5);

    const auto& stats = selector.get_operator_stats();
    int selected_op = selector.get_last_selection();

    result.assert_eq(stats[selected_op].selection_count, 1,
                     "Selected operator selection count updated");
    result.assert_eq(stats[selected_op].total_reward, 1.5,
                     "Selected operator total reward updated");
    result.assert_eq(selector.get_last_improvement(), 1.5, "Last improvement tracked correctly");
}

void test_adaptive_operator_selector_fitness_change(TestResult& result) {
    std::mt19937_64 rng(42);
    using TSP = evolab::problems::TSP;
    UCBOperatorSelector<TSP> selector(1, 2.0, rng);

    PMXCrossover pmx;
    selector.add_operator(pmx, "PMX");

    // Create a simple TSP problem
    std::vector<double> distances(25);
    for (int i = 0; i < 25; ++i)
        distances[i] = static_cast<double>(i % 10 + 1);
    TSP problem(5, distances);

    std::vector<int> parent1 = {0, 1, 2, 3, 4};
    std::vector<int> parent2 = {4, 3, 2, 1, 0};

    std::mt19937 rng32(rng()); // Convert to 32-bit RNG for crossover operators
    auto [child1, child2] = selector.apply_crossover(problem, parent1, parent2, rng32);
    selector.report_fitness_change(100.0, 95.0); // Improvement of 5.0

    const auto& stats = selector.get_operator_stats();
    result.assert_eq(stats[0].selection_count, 1, "Operator selection count updated");
    result.assert_eq(stats[0].total_reward, 5.0,
                     "Operator total reward reflects fitness improvement");
    result.assert_eq(selector.get_last_improvement(), 5.0, "Last improvement calculated correctly");
}

void test_scheduler_reset(TestResult& result) {
    std::mt19937_64 rng(42);
    UCBScheduler scheduler(2, 2.0, rng);

    scheduler.select_operator();
    scheduler.update_reward(0, 3.0);
    scheduler.select_operator();
    scheduler.update_reward(1, -1.0);

    const auto& stats_before = scheduler.get_stats();
    result.assert_gt(stats_before[0].selection_count, 0, "Scheduler has state before reset");

    scheduler.reset();
    const auto& stats_after = scheduler.get_stats();

    for (const auto& stat : stats_after) {
        result.assert_eq(stat.selection_count, 0, "Selection count reset to zero");
        result.assert_eq(stat.total_reward, 0.0, "Total reward reset to zero");
        result.assert_eq(stat.avg_reward, 0.0, "Average reward reset to zero");
        result.assert_eq(stat.success_rate, 0.0, "Success rate reset to zero");
        result.assert_eq(stat.success_count, 0, "Success count reset to zero");
    }
}

void test_operator_stats_update(TestResult& result) {
    OperatorStats stats;

    result.assert_eq(stats.selection_count, 0, "Initial selection count is zero");
    result.assert_eq(stats.total_reward, 0.0, "Initial total reward is zero");
    result.assert_eq(stats.avg_reward, 0.0, "Initial average reward is zero");
    result.assert_eq(stats.success_rate, 0.0, "Initial success rate is zero");

    stats.update_reward(2.0);
    result.assert_eq(stats.selection_count, 1, "Selection count updated after positive reward");
    result.assert_eq(stats.total_reward, 2.0, "Total reward updated");
    result.assert_eq(stats.avg_reward, 2.0, "Average reward calculated");
    result.assert_eq(stats.success_count, 1, "Success count updated for positive reward");
    result.assert_eq(stats.success_rate, 1.0, "Success rate calculated");

    stats.update_reward(-1.0);
    result.assert_eq(stats.selection_count, 2, "Selection count updated after negative reward");
    result.assert_eq(stats.total_reward, 1.0, "Total reward accumulated correctly");
    result.assert_eq(stats.avg_reward, 0.5, "Average reward recalculated");
    result.assert_eq(stats.success_count, 1, "Success count unchanged for negative reward");
    result.assert_eq(stats.success_rate, 0.5, "Success rate recalculated");

    stats.reset();
    result.assert_eq(stats.selection_count, 0, "Selection count reset");
    result.assert_eq(stats.total_reward, 0.0, "Total reward reset");
}

int main() {
    std::cout << "=== EvoLab Scheduler Tests ===\n\n";

    TestResult result;

    test_ucb_scheduler_initialization(result);
    test_ucb_scheduler_selection(result);
    test_ucb_scheduler_reward_update(result);

    test_thompson_sampling_initialization(result);
    test_thompson_sampling_selection(result);
    test_thompson_sampling_reward_threshold(result);

    test_adaptive_operator_selector_basic(result);
    test_adaptive_operator_selector_crossover(result);
    test_adaptive_operator_selector_reward_tracking(result);
    test_adaptive_operator_selector_fitness_change(result);

    test_scheduler_reset(result);
    test_operator_stats_update(result);

    return result.summary();
}
