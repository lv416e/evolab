#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>

#include <evolab/evolab.hpp>

#include "test_helper.hpp"

using namespace evolab;

void test_tsp_creation() {
    TestResult result;

    // Test random TSP creation
    auto tsp = problems::create_random_tsp(10, 100.0, 42);

    result.assert_equals(10, static_cast<double>(tsp.num_cities()),
                         "TSP has correct number of cities");
    result.assert_equals(10, static_cast<double>(tsp.size()), "TSP size matches num_cities");

    // Test distances are non-negative
    bool all_distances_valid = true;
    for (int i = 0; i < tsp.num_cities() && all_distances_valid; ++i) {
        for (int j = 0; j < tsp.num_cities(); ++j) {
            double dist = tsp.distance(i, j);
            if (i == j && dist != 0.0)
                all_distances_valid = false;
            if (i != j && dist <= 0.0)
                all_distances_valid = false;
        }
    }
    result.assert_true(all_distances_valid, "All distances are valid");

    // Test symmetry (if using Euclidean distances)
    bool symmetric = true;
    for (int i = 0; i < tsp.num_cities() && symmetric; ++i) {
        for (int j = i + 1; j < tsp.num_cities(); ++j) {
            if (std::abs(tsp.distance(i, j) - tsp.distance(j, i)) > 1e-9) {
                symmetric = false;
            }
        }
    }
    result.assert_true(symmetric, "Distance matrix is symmetric");

    result.print_summary();
}

void test_tsp_coordinates() {
    TestResult result;

    // Test TSP creation from coordinates
    std::vector<std::pair<double, double>> cities = {
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};

    problems::TSP tsp(cities);
    result.assert_equals(4, static_cast<double>(tsp.num_cities()),
                         "Coordinate TSP has correct size");

    // Test known distances
    result.assert_equals(0.0, tsp.distance(0, 0), "Distance to self is zero");
    result.assert_equals(1.0, tsp.distance(0, 1), "Distance (0,0) to (1,0) is 1");
    result.assert_equals(std::sqrt(2.0), tsp.distance(0, 2), "Distance (0,0) to (1,1) is sqrt(2)",
                         1e-9);
    result.assert_equals(1.0, tsp.distance(0, 3), "Distance (0,0) to (0,1) is 1");

    result.print_summary();
}

void test_tour_validation() {
    TestResult result;

    auto tsp = problems::create_random_tsp(5, 100.0, 42);

    // Test valid tour
    std::vector<int> valid_tour = {0, 1, 2, 3, 4};
    result.assert_true(tsp.is_valid_tour(valid_tour), "Identity tour is valid");

    // Test shuffled valid tour
    std::vector<int> shuffled_tour = {2, 0, 4, 1, 3};
    result.assert_true(tsp.is_valid_tour(shuffled_tour), "Shuffled tour is valid");

    // Test invalid tours
    std::vector<int> too_short = {0, 1, 2};
    result.assert_true(!tsp.is_valid_tour(too_short), "Too short tour is invalid");

    std::vector<int> too_long = {0, 1, 2, 3, 4, 5};
    result.assert_true(!tsp.is_valid_tour(too_long), "Too long tour is invalid");

    std::vector<int> duplicate = {0, 1, 2, 2, 4};
    result.assert_true(!tsp.is_valid_tour(duplicate), "Tour with duplicates is invalid");

    std::vector<int> out_of_range = {0, 1, 2, 3, 5};
    result.assert_true(!tsp.is_valid_tour(out_of_range), "Tour with out-of-range city is invalid");

    result.print_summary();
}

void test_genome_operations() {
    TestResult result;

    auto tsp = problems::create_random_tsp(6, 100.0, 42);

    // Test identity genome
    auto identity = tsp.identity_genome();
    result.assert_equals(6, static_cast<double>(identity.size()),
                         "Identity genome has correct size");
    result.assert_true(tsp.is_valid_tour(identity), "Identity genome is valid tour");

    for (int i = 0; i < 6; ++i) {
        result.assert_equals(static_cast<double>(i), static_cast<double>(identity[i]),
                             "Identity genome city " + std::to_string(i));
    }

    // Test random genome
    std::mt19937 rng(42);
    auto random_genome = tsp.random_genome(rng);
    result.assert_equals(6, static_cast<double>(random_genome.size()),
                         "Random genome has correct size");
    result.assert_true(tsp.is_valid_tour(random_genome), "Random genome is valid tour");

    // Test that random genome is actually randomized (not identity)
    bool is_different = false;
    for (int i = 0; i < 6; ++i) {
        if (random_genome[i] != i) {
            is_different = true;
            break;
        }
    }
    result.assert_true(is_different, "Random genome is different from identity");

    result.print_summary();
}

void test_two_opt_operations() {
    TestResult result;

    // Create simple TSP with known distances
    std::vector<std::pair<double, double>> cities = {
        {0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}};
    problems::TSP tsp(cities);

    // Test 2-opt gain calculation
    std::vector<int> tour = {0, 1, 2, 3}; // Rectangle tour

    // This should be a good tour already (perimeter = 8.0)
    double current_fitness = tsp.evaluate(tour).value;
    result.assert_equals(8.0, current_fitness, "Initial tour fitness is correct");

    // Test 2-opt gain (should be 0 or negative for optimal tour)
    double gain = tsp.two_opt_gain(tour, 0, 2);
    result.assert_true(gain <= 1e-9, "2-opt gain on optimal tour is non-positive");

    // Test bad tour
    std::vector<int> bad_tour = {0, 2, 1, 3}; // Crossing edges
    double bad_fitness = tsp.evaluate(bad_tour).value;
    result.assert_true(bad_fitness > current_fitness, "Bad tour has worse fitness");

    // Test 2-opt improvement on bad tour
    double improvement_gain = tsp.two_opt_gain(bad_tour, 0, 2);
    result.assert_true(improvement_gain > 1e-9, "2-opt finds improvement on bad tour");

    // Apply 2-opt and check improvement
    auto improved_tour = bad_tour;
    tsp.apply_two_opt(improved_tour, 0, 2);
    double improved_fitness = tsp.evaluate(improved_tour).value;
    result.assert_true(improved_fitness < bad_fitness, "2-opt application improves fitness");
    result.assert_true(tsp.is_valid_tour(improved_tour), "2-opt result is valid tour");

    result.print_summary();
}

void test_tsp_with_ga() {
    TestResult result;

    // Create small TSP and solve with GA
    auto tsp = problems::create_random_tsp(8, 100.0, 42);
    auto ga = factory::make_tsp_ga_basic();

    core::GAConfig config{.population_size = 50, .max_generations = 100, .seed = 42};

    auto ga_result = ga.run(tsp, config);

    result.assert_true(tsp.is_valid_tour(ga_result.best_genome), "GA produces valid TSP tour");
    result.assert_equals(8, static_cast<double>(ga_result.best_genome.size()),
                         "GA tour has correct size");
    result.assert_true(ga_result.best_fitness.value > 0.0, "GA fitness is positive");
    result.assert_true(ga_result.evaluations > config.population_size,
                       "GA performed sufficient evaluations");

    result.print_summary();
}

int main() {
    std::cout << "Running EvoLab TSP Tests\n";
    std::cout << std::string(30, '=') << "\n\n";

    std::cout << "Testing TSP Creation...\n";
    test_tsp_creation();

    std::cout << "\nTesting TSP from Coordinates...\n";
    test_tsp_coordinates();

    std::cout << "\nTesting Tour Validation...\n";
    test_tour_validation();

    std::cout << "\nTesting Genome Operations...\n";
    test_genome_operations();

    std::cout << "\nTesting 2-opt Operations...\n";
    test_two_opt_operations();

    std::cout << "\nTesting TSP with GA...\n";
    test_tsp_with_ga();

    std::cout << "\n" << std::string(30, '=') << "\n";
    std::cout << "TSP tests completed.\n";

    return 0;
}
