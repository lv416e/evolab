#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <unordered_set>

#include <evolab/evolab.hpp>

// Simple test framework
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

    void assert_equals(double expected, double actual, const std::string& message,
                       double tolerance = 1e-9) {
        bool passed_test = std::abs(expected - actual) < tolerance;
        assert_true(passed_test, message + " (expected: " + std::to_string(expected) +
                                     ", actual: " + std::to_string(actual) + ")");
    }

    void print_summary() {
        std::cout << "\n=== Test Summary ===\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        if (failed == 0) {
            std::cout << "All tests passed! ✓\n";
        } else {
            std::cout << "Some tests failed! ✗\n";
        }
    }

    bool all_passed() const { return failed == 0; }
};

using namespace evolab;

bool is_valid_permutation(const std::vector<int>& perm, int n) {
    if (static_cast<int>(perm.size()) != n)
        return false;

    std::unordered_set<int> seen;
    for (int x : perm) {
        if (x < 0 || x >= n || seen.count(x))
            return false;
        seen.insert(x);
    }
    return true;
}

void test_selection_operators() {
    TestResult result;

    // Create test population and fitnesses
    std::vector<std::vector<int>> population = {{0, 1, 2}, {1, 2, 0}, {2, 0, 1}, {0, 2, 1}};
    std::vector<core::Fitness> fitnesses = {core::Fitness{10.0}, core::Fitness{5.0},
                                            core::Fitness{15.0}, core::Fitness{8.0}};

    std::mt19937 rng(42);

    // Test tournament selection
    operators::TournamentSelection tournament(2);
    std::vector<int> selected_counts(population.size(), 0);

    for (int i = 0; i < 1000; ++i) {
        auto selected = tournament.select(population, fitnesses, rng);
        result.assert_true(selected < population.size(),
                           "Tournament selection returns valid index");
        selected_counts[selected]++;
    }

    // Better individuals (lower fitness) should be selected more often
    result.assert_true(selected_counts[1] > selected_counts[0],
                       "Tournament favors better fitness (index 1 vs 0)");
    result.assert_true(selected_counts[1] > selected_counts[2],
                       "Tournament favors better fitness (index 1 vs 2)");

    // Test roulette wheel selection
    operators::RouletteWheelSelection roulette;
    std::fill(selected_counts.begin(), selected_counts.end(), 0);

    for (int i = 0; i < 1000; ++i) {
        auto selected = roulette.select(population, fitnesses, rng);
        result.assert_true(selected < population.size(), "Roulette selection returns valid index");
        selected_counts[selected]++;
    }

    // Better individuals should be selected more often in roulette wheel too
    result.assert_true(selected_counts[1] > selected_counts[2], "Roulette favors better fitness");

    // Test ranking selection
    operators::RankSelection ranking(1.5);
    std::fill(selected_counts.begin(), selected_counts.end(), 0);

    for (int i = 0; i < 1000; ++i) {
        auto selected = ranking.select(population, fitnesses, rng);
        result.assert_true(selected < population.size(), "Ranking selection returns valid index");
        selected_counts[selected]++;
    }

    result.assert_true(selected_counts[1] > selected_counts[2], "Ranking favors better fitness");

    result.print_summary();
}

void test_crossover_operators() {
    TestResult result;

    auto tsp = problems::create_random_tsp(6, 100.0, 42);
    std::mt19937 rng(42);

    // Create two parent tours
    std::vector<int> parent1 = {0, 1, 2, 3, 4, 5};
    std::vector<int> parent2 = {5, 4, 3, 2, 1, 0};

    // Test PMX crossover
    operators::PMXCrossover pmx;
    auto [child1_pmx, child2_pmx] = pmx.cross(tsp, parent1, parent2, rng);

    result.assert_true(is_valid_permutation(child1_pmx, 6), "PMX child1 is valid permutation");
    result.assert_true(is_valid_permutation(child2_pmx, 6), "PMX child2 is valid permutation");
    result.assert_true(tsp.is_valid_tour(child1_pmx), "PMX child1 is valid tour");
    result.assert_true(tsp.is_valid_tour(child2_pmx), "PMX child2 is valid tour");

    // Test Order crossover
    operators::OrderCrossover ox;
    auto [child1_ox, child2_ox] = ox.cross(tsp, parent1, parent2, rng);

    result.assert_true(is_valid_permutation(child1_ox, 6), "OX child1 is valid permutation");
    result.assert_true(is_valid_permutation(child2_ox, 6), "OX child2 is valid permutation");
    result.assert_true(tsp.is_valid_tour(child1_ox), "OX child1 is valid tour");
    result.assert_true(tsp.is_valid_tour(child2_ox), "OX child2 is valid tour");

    // Test Cycle crossover
    operators::CycleCrossover cx;
    auto [child1_cx, child2_cx] = cx.cross(tsp, parent1, parent2, rng);

    result.assert_true(is_valid_permutation(child1_cx, 6), "CX child1 is valid permutation");
    result.assert_true(is_valid_permutation(child2_cx, 6), "CX child2 is valid permutation");
    result.assert_true(tsp.is_valid_tour(child1_cx), "CX child1 is valid tour");
    result.assert_true(tsp.is_valid_tour(child2_cx), "CX child2 is valid tour");

    // Test Edge Recombination crossover
    operators::EdgeRecombinationCrossover erx;
    auto [child1_erx, child2_erx] = erx.cross(tsp, parent1, parent2, rng);

    result.assert_true(is_valid_permutation(child1_erx, 6), "ERX child1 is valid permutation");
    result.assert_true(is_valid_permutation(child2_erx, 6), "ERX child2 is valid permutation");
    result.assert_true(tsp.is_valid_tour(child1_erx), "ERX child1 is valid tour");
    result.assert_true(tsp.is_valid_tour(child2_erx), "ERX child2 is valid tour");

    result.print_summary();
}

void test_mutation_operators() {
    TestResult result;

    auto tsp = problems::create_random_tsp(8, 100.0, 42);
    std::mt19937 rng(42);

    // Test each mutation operator
    std::vector<int> original = {0, 1, 2, 3, 4, 5, 6, 7};

    // Test swap mutation
    {
        auto genome = original;
        operators::SwapMutation swap;
        swap.mutate(tsp, genome, rng);

        result.assert_true(is_valid_permutation(genome, 8),
                           "Swap mutation produces valid permutation");
        result.assert_true(tsp.is_valid_tour(genome), "Swap mutation produces valid tour");

        // Should be different from original (with high probability)
        result.assert_true(genome != original, "Swap mutation changes genome");
    }

    // Test inversion mutation
    {
        auto genome = original;
        operators::InversionMutation inversion;
        inversion.mutate(tsp, genome, rng);

        result.assert_true(is_valid_permutation(genome, 8),
                           "Inversion mutation produces valid permutation");
        result.assert_true(tsp.is_valid_tour(genome), "Inversion mutation produces valid tour");
    }

    // Test scramble mutation
    {
        auto genome = original;
        operators::ScrambleMutation scramble;
        scramble.mutate(tsp, genome, rng);

        result.assert_true(is_valid_permutation(genome, 8),
                           "Scramble mutation produces valid permutation");
        result.assert_true(tsp.is_valid_tour(genome), "Scramble mutation produces valid tour");
    }

    // Test insertion mutation
    {
        auto genome = original;
        operators::InsertionMutation insertion;
        insertion.mutate(tsp, genome, rng);

        result.assert_true(is_valid_permutation(genome, 8),
                           "Insertion mutation produces valid permutation");
        result.assert_true(tsp.is_valid_tour(genome), "Insertion mutation produces valid tour");
    }

    // Test 2-opt mutation
    {
        auto genome = original;
        operators::TwoOptMutation two_opt;
        two_opt.mutate(tsp, genome, rng);

        result.assert_true(is_valid_permutation(genome, 8),
                           "2-opt mutation produces valid permutation");
        result.assert_true(tsp.is_valid_tour(genome), "2-opt mutation produces valid tour");
    }

    // Test adaptive mutation
    {
        auto genome = original;
        operators::AdaptiveMutation adaptive;
        adaptive.mutate(tsp, genome, rng);

        result.assert_true(is_valid_permutation(genome, 8),
                           "Adaptive mutation produces valid permutation");
        result.assert_true(tsp.is_valid_tour(genome), "Adaptive mutation produces valid tour");
    }

    result.print_summary();
}

void test_local_search() {
    TestResult result;

    // Create TSP with known suboptimal tour
    std::vector<std::pair<double, double>> cities = {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0},
                                                     {2.0, 1.0}, {1.0, 1.0}, {0.0, 1.0}};
    problems::TSP tsp(cities);

    // Create a suboptimal tour (with crossing edges)
    std::vector<int> bad_tour = {0, 2, 1, 4, 3, 5}; // Has crossings
    auto original_fitness = tsp.evaluate(bad_tour);

    std::mt19937 rng(42);

    // Test 2-opt local search
    {
        auto tour = bad_tour;
        local_search::TwoOpt two_opt(false, 1000); // Best improvement
        auto improved_fitness = two_opt.improve(tsp, tour, rng);

        result.assert_true(tsp.is_valid_tour(tour), "2-opt produces valid tour");
        result.assert_true(improved_fitness.value <= original_fitness.value,
                           "2-opt improves or maintains fitness");
        result.assert_true(improved_fitness.value < original_fitness.value,
                           "2-opt actually improves suboptimal tour");
    }

    // Test random 2-opt
    {
        auto tour = bad_tour;
        local_search::Random2Opt random_two_opt(100);
        auto result_fitness = random_two_opt.improve(tsp, tour, rng);

        result.assert_true(tsp.is_valid_tour(tour), "Random 2-opt produces valid tour");
        result.assert_true(result_fitness.value <= original_fitness.value,
                           "Random 2-opt improves or maintains fitness");
    }

    // Test candidate list 2-opt
    {
        auto tour = bad_tour;
        local_search::CandidateList2Opt candidate_two_opt(4, true); // Small candidate list
        auto result_fitness = candidate_two_opt.improve(tsp, tour, rng);

        result.assert_true(tsp.is_valid_tour(tour), "Candidate 2-opt produces valid tour");
        result.assert_true(result_fitness.value <= original_fitness.value,
                           "Candidate 2-opt improves or maintains fitness");
    }

    // Test no-op local search
    {
        auto tour = bad_tour;
        local_search::NoLocalSearch no_op;
        auto result_fitness = no_op.improve(tsp, tour, rng);

        result.assert_true(tour == bad_tour, "No-op local search doesn't change tour");
        result.assert_equals(original_fitness.value, result_fitness.value,
                             "No-op returns original fitness");
    }

    result.print_summary();
}

void test_factory_functions() {
    TestResult result;

    auto tsp = problems::create_random_tsp(10, 100.0, 42);

    // Test basic GA factory
    auto ga_basic = factory::make_ga_basic();
    auto result_basic =
        ga_basic.run(tsp, core::GAConfig{.population_size = 20, .max_generations = 10, .seed = 42});

    result.assert_true(tsp.is_valid_tour(result_basic.best_genome),
                       "Basic GA factory produces valid solution");

    // Test TSP basic GA factory
    auto ga_tsp_basic = factory::make_tsp_ga_basic();
    auto result_tsp_basic = ga_tsp_basic.run(
        tsp, core::GAConfig{.population_size = 20, .max_generations = 10, .seed = 42});

    result.assert_true(tsp.is_valid_tour(result_tsp_basic.best_genome),
                       "TSP basic GA factory produces valid solution");

    // Test TSP advanced GA factory
    auto ga_tsp_advanced = factory::make_tsp_ga_advanced();
    auto result_tsp_advanced = ga_tsp_advanced.run(
        tsp, core::GAConfig{.population_size = 20, .max_generations = 10, .seed = 42});

    result.assert_true(tsp.is_valid_tour(result_tsp_advanced.best_genome),
                       "TSP advanced GA factory produces valid solution");

    result.print_summary();
}

int main() {
    std::cout << "Running EvoLab Operator Tests\n";
    std::cout << std::string(30, '=') << "\n\n";

    std::cout << "Testing Selection Operators...\n";
    test_selection_operators();

    std::cout << "\nTesting Crossover Operators...\n";
    test_crossover_operators();

    std::cout << "\nTesting Mutation Operators...\n";
    test_mutation_operators();

    std::cout << "\nTesting Local Search...\n";
    test_local_search();

    std::cout << "\nTesting Factory Functions...\n";
    test_factory_functions();

    std::cout << "\n" << std::string(30, '=') << "\n";
    std::cout << "Operator tests completed.\n";

    return 0;
}
