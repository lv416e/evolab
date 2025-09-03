#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <evolab/evolab.hpp>

using namespace evolab::io;

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
                   double tolerance = 1e-6) {
        assert_true(std::abs(expected - actual) < tolerance,
                    message + " (expected: " + std::to_string(expected) +
                        ", actual: " + std::to_string(actual) + ")");
    }

    void assert_eq(const std::string& expected, const std::string& actual,
                   const std::string& message) {
        assert_true(expected == actual,
                    message + " (expected: '" + expected + "', actual: '" + actual + "')");
    }

    template <typename T>
    void assert_eq(T expected, T actual, const std::string& message) {
        assert_true(expected == actual,
                    message + " (expected: " + std::to_string(static_cast<int>(expected)) +
                        ", actual: " + std::to_string(static_cast<int>(actual)) + ")");
    }

    int summary() {
        std::cout << "\n=== TSPLIB Test Summary ===\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Total:  " << (passed + failed) << "\n";
        return failed == 0 ? 0 : 1;
    }
};

void test_basic_parsing(TestResult& result) {
    std::string tsp_content = R"(
NAME : test4
COMMENT : 4-city test instance
TYPE : TSP
DIMENSION : 4
EDGE_WEIGHT_TYPE : EUC_2D
NODE_COORD_SECTION
1 0.0 0.0
2 1.0 0.0
3 1.0 1.0
4 0.0 1.0
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(tsp_content);

    result.assert_eq("test4", instance.name, "Instance name parsed correctly");
    result.assert_eq("4-city test instance", instance.comment, "Instance comment parsed correctly");
    result.assert_eq(4, instance.dimension, "Dimension parsed correctly");
    result.assert_eq(EdgeWeightType::EUC_2D, instance.edge_weight_type,
                     "Edge weight type parsed correctly");
    result.assert_eq(4, static_cast<int>(instance.node_coords.size()),
                     "Node coordinates count correct");

    result.assert_eq(0.0, instance.node_coords[0].first, "First node x coordinate");
    result.assert_eq(0.0, instance.node_coords[0].second, "First node y coordinate");
    result.assert_eq(1.0, instance.node_coords[1].first, "Second node x coordinate");
    result.assert_eq(0.0, instance.node_coords[1].second, "Second node y coordinate");
}

void test_distance_calculations(TestResult& result) {
    std::string tsp_content = R"(
NAME : square4
COMMENT : 4 cities forming a unit square
TYPE : TSP
DIMENSION : 4
EDGE_WEIGHT_TYPE : EUC_2D
NODE_COORD_SECTION
1 0.0 0.0
2 1.0 0.0
3 1.0 1.0
4 0.0 1.0
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(tsp_content);

    result.assert_eq(0.0, instance.calculate_distance(0, 0),
                     "Distance from node to itself is zero");
    result.assert_eq(1.0, instance.calculate_distance(0, 1),
                     "Distance between adjacent horizontal nodes");
    result.assert_eq(1.0, instance.calculate_distance(1, 2),
                     "Distance between adjacent vertical nodes");
    result.assert_eq(1.0, instance.calculate_distance(2, 3), "Distance across square horizontally");
    result.assert_eq(1.0, instance.calculate_distance(3, 0), "Distance across square vertically");
    result.assert_eq(1.0, instance.calculate_distance(0, 2), "Distance across square diagonally",
                     0.1);
    result.assert_eq(1.0, instance.calculate_distance(1, 3), "Distance across square diagonally",
                     0.1);
}

void test_explicit_matrix_full(TestResult& result) {
    std::string tsp_content = R"(
NAME : test3_explicit
COMMENT : 3-city test with explicit distances
TYPE : TSP
DIMENSION : 3
EDGE_WEIGHT_TYPE : EXPLICIT
EDGE_WEIGHT_FORMAT : FULL_MATRIX
EDGE_WEIGHT_SECTION
 0  10  20
10   0  30
20  30   0
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(tsp_content);

    result.assert_eq(EdgeWeightType::EXPLICIT, instance.edge_weight_type,
                     "Explicit edge weight type");
    result.assert_eq(EdgeWeightFormat::FULL_MATRIX, instance.edge_weight_format,
                     "Full matrix format");
    result.assert_eq(9, static_cast<int>(instance.distance_matrix.size()), "Distance matrix size");

    result.assert_eq(0.0, instance.calculate_distance(0, 0), "Distance matrix (0,0)");
    result.assert_eq(10.0, instance.calculate_distance(0, 1), "Distance matrix (0,1)");
    result.assert_eq(20.0, instance.calculate_distance(0, 2), "Distance matrix (0,2)");
    result.assert_eq(10.0, instance.calculate_distance(1, 0), "Distance matrix (1,0)");
    result.assert_eq(30.0, instance.calculate_distance(1, 2), "Distance matrix (1,2)");
    result.assert_eq(30.0, instance.calculate_distance(2, 1), "Distance matrix (2,1)");
}

void test_explicit_matrix_upper_row(TestResult& result) {
    std::string tsp_content = R"(
NAME : test4_upper
COMMENT : 4-city test with upper triangular matrix
TYPE : TSP
DIMENSION : 4
EDGE_WEIGHT_TYPE : EXPLICIT
EDGE_WEIGHT_FORMAT : UPPER_ROW
EDGE_WEIGHT_SECTION
10 20 30
25 35
40
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(tsp_content);

    result.assert_eq(EdgeWeightFormat::UPPER_ROW, instance.edge_weight_format, "Upper row format");
    result.assert_eq(6, static_cast<int>(instance.distance_matrix.size()),
                     "Distance matrix size for upper triangular");

    result.assert_eq(0.0, instance.calculate_distance(0, 0), "Self distance is zero");
    result.assert_eq(10.0, instance.calculate_distance(0, 1), "Distance (0,1)");
    result.assert_eq(20.0, instance.calculate_distance(0, 2), "Distance (0,2)");
    result.assert_eq(30.0, instance.calculate_distance(0, 3), "Distance (0,3)");
    result.assert_eq(25.0, instance.calculate_distance(1, 2), "Distance (1,2)");
    result.assert_eq(35.0, instance.calculate_distance(1, 3), "Distance (1,3)");
    result.assert_eq(40.0, instance.calculate_distance(2, 3), "Distance (2,3)");

    // Test symmetry
    result.assert_eq(10.0, instance.calculate_distance(1, 0), "Symmetric distance (1,0)");
    result.assert_eq(25.0, instance.calculate_distance(2, 1), "Symmetric distance (2,1)");
    result.assert_eq(40.0, instance.calculate_distance(3, 2), "Symmetric distance (3,2)");
}

void test_explicit_matrix_lower_row(TestResult& result) {
    std::string tsp_content = R"(
NAME : test4_lower
COMMENT : 4-city test with lower triangular matrix
TYPE : TSP
DIMENSION : 4
EDGE_WEIGHT_TYPE : EXPLICIT
EDGE_WEIGHT_FORMAT : LOWER_ROW
EDGE_WEIGHT_SECTION
10
20 25
30 35 40
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(tsp_content);

    result.assert_eq(EdgeWeightFormat::LOWER_ROW, instance.edge_weight_format, "Lower row format");
    result.assert_eq(6, static_cast<int>(instance.distance_matrix.size()),
                     "Distance matrix size for lower triangular");

    result.assert_eq(0.0, instance.calculate_distance(0, 0), "Self distance is zero");
    result.assert_eq(10.0, instance.calculate_distance(1, 0), "Distance (1,0)");
    result.assert_eq(20.0, instance.calculate_distance(2, 0), "Distance (2,0)");
    result.assert_eq(25.0, instance.calculate_distance(2, 1), "Distance (2,1)");
    result.assert_eq(30.0, instance.calculate_distance(3, 0), "Distance (3,0)");
    result.assert_eq(35.0, instance.calculate_distance(3, 1), "Distance (3,1)");
    result.assert_eq(40.0, instance.calculate_distance(3, 2), "Distance (3,2)");

    // Test symmetry
    result.assert_eq(10.0, instance.calculate_distance(0, 1), "Symmetric distance (0,1)");
    result.assert_eq(20.0, instance.calculate_distance(0, 2), "Symmetric distance (0,2)");
    result.assert_eq(25.0, instance.calculate_distance(1, 2), "Symmetric distance (1,2)");
    result.assert_eq(30.0, instance.calculate_distance(0, 3), "Symmetric distance (0,3)");
    result.assert_eq(35.0, instance.calculate_distance(1, 3), "Symmetric distance (1,3)");
    result.assert_eq(40.0, instance.calculate_distance(2, 3), "Symmetric distance (2,3)");
}

void test_manhattan_distance(TestResult& result) {
    std::string tsp_content = R"(
NAME : manhattan_test
COMMENT : Test Manhattan distance
TYPE : TSP
DIMENSION : 3
EDGE_WEIGHT_TYPE : MAN_2D
NODE_COORD_SECTION
1 0.0 0.0
2 3.0 4.0
3 1.0 2.0
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(tsp_content);

    result.assert_eq(EdgeWeightType::MAN_2D, instance.edge_weight_type, "Manhattan distance type");
    result.assert_eq(7.0, instance.calculate_distance(0, 1),
                     "Manhattan distance (0,1) = |3-0| + |4-0| = 7");
    result.assert_eq(3.0, instance.calculate_distance(0, 2),
                     "Manhattan distance (0,2) = |1-0| + |2-0| = 3");
    result.assert_eq(4.0, instance.calculate_distance(1, 2),
                     "Manhattan distance (1,2) = |3-1| + |4-2| = 4");
}

void test_maximum_distance(TestResult& result) {
    std::string tsp_content = R"(
NAME : maximum_test
COMMENT : Test Maximum distance
TYPE : TSP
DIMENSION : 3
EDGE_WEIGHT_TYPE : MAX_2D
NODE_COORD_SECTION
1 0.0 0.0
2 3.0 4.0
3 1.0 2.0
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(tsp_content);

    result.assert_eq(EdgeWeightType::MAX_2D, instance.edge_weight_type, "Maximum distance type");
    result.assert_eq(4.0, instance.calculate_distance(0, 1),
                     "Maximum distance (0,1) = max(|3-0|, |4-0|) = 4");
    result.assert_eq(2.0, instance.calculate_distance(0, 2),
                     "Maximum distance (0,2) = max(|1-0|, |2-0|) = 2");
    result.assert_eq(2.0, instance.calculate_distance(1, 2),
                     "Maximum distance (1,2) = max(|3-1|, |4-2|) = 2");
}

void test_ceil_euclidean_distance(TestResult& result) {
    std::string tsp_content = R"(
NAME : ceil_test
COMMENT : Test Ceiling Euclidean distance
TYPE : TSP
DIMENSION : 2
EDGE_WEIGHT_TYPE : CEIL_2D
NODE_COORD_SECTION
1 0.0 0.0
2 1.5 1.5
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(tsp_content);

    result.assert_eq(EdgeWeightType::CEIL_2D, instance.edge_weight_type,
                     "Ceiling Euclidean distance type");

    // Distance should be ceil(sqrt(1.5^2 + 1.5^2)) = ceil(sqrt(4.5)) = ceil(2.12...) = 3
    double expected_distance = std::ceil(std::sqrt(1.5 * 1.5 + 1.5 * 1.5));
    result.assert_eq(expected_distance, instance.calculate_distance(0, 1),
                     "Ceiling Euclidean distance calculation");
}

void test_full_distance_matrix(TestResult& result) {
    std::string tsp_content = R"(
NAME : test3
COMMENT : 3-city test
TYPE : TSP
DIMENSION : 3
EDGE_WEIGHT_TYPE : EUC_2D
NODE_COORD_SECTION
1 0.0 0.0
2 3.0 0.0
3 0.0 4.0
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(tsp_content);
    std::vector<double> full_matrix = instance.get_full_distance_matrix();

    result.assert_eq(9, static_cast<int>(full_matrix.size()), "Full distance matrix size");
    result.assert_eq(0.0, full_matrix[0], "Full matrix (0,0)");
    result.assert_eq(3.0, full_matrix[1], "Full matrix (0,1)");
    result.assert_eq(4.0, full_matrix[2], "Full matrix (0,2)");
    result.assert_eq(3.0, full_matrix[3], "Full matrix (1,0)");
    result.assert_eq(0.0, full_matrix[4], "Full matrix (1,1)");
    result.assert_eq(5.0, full_matrix[5], "Full matrix (1,2)");
}

void test_tour_file_output(TestResult& result) {
    std::vector<int> tour = {0, 1, 2, 3};
    std::string tour_filename = "test_tour.tour";

    TSPLIBParser::write_tour_file(tour_filename, "test_problem", tour, 123.45);

    std::ifstream file(tour_filename);
    result.assert_true(file.is_open(), "Tour file created successfully");

    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    result.assert_true(lines.size() >= 8, "Tour file has expected number of lines");
    result.assert_true(lines[0].find("test_problem") != std::string::npos,
                       "Tour file contains problem name");
    result.assert_true(lines[3].find("4") != std::string::npos, "Tour file contains dimension");
    result.assert_true(lines[4].find("123.45") != std::string::npos,
                       "Tour file contains tour length");
    result.assert_true(lines[5] == "TOUR_SECTION", "Tour file has TOUR_SECTION");
    result.assert_true(lines.back() == "EOF", "Tour file ends with EOF");

    // Cleanup
    std::filesystem::remove(tour_filename);
}

void test_error_handling(TestResult& result) {
    // Test invalid dimension
    std::string invalid_content = R"(
NAME : invalid
TYPE : TSP
DIMENSION : -1
EDGE_WEIGHT_TYPE : EUC_2D
EOF
)";

    try {
        TSPInstance instance = TSPLIBParser::parse_string(invalid_content);
        result.assert_true(false, "Should throw exception for invalid dimension");
    } catch (const std::exception& e) {
        result.assert_true(true, "Correctly throws exception for invalid dimension");
    }

    // Test out of range distance calculation
    std::string valid_content = R"(
NAME : test2
TYPE : TSP
DIMENSION : 2
EDGE_WEIGHT_TYPE : EUC_2D
NODE_COORD_SECTION
1 0.0 0.0
2 1.0 0.0
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(valid_content);

    try {
        instance.calculate_distance(0, 5); // Out of range
        result.assert_true(false, "Should throw exception for out of range indices");
    } catch (const std::out_of_range& e) {
        result.assert_true(true, "Correctly throws exception for out of range indices");
    }
}

void test_geographical_distance(TestResult& result) {
    // Test with simplified coordinates that should give predictable results
    std::string geo_content = R"(
NAME : geo_test
COMMENT : Test geographical distance
TYPE : TSP
DIMENSION : 2
EDGE_WEIGHT_TYPE : GEO
NODE_COORD_SECTION
1 0.0 0.0
2 1.0 1.0
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(geo_content);

    result.assert_eq(EdgeWeightType::GEO, instance.edge_weight_type, "Geographical distance type");

    // The exact value depends on the geographical distance formula
    // We just test that it calculates some reasonable distance > 0
    double distance = instance.calculate_distance(0, 1);
    result.assert_true(distance > 0.0, "Geographical distance calculation returns positive value");
}

void test_att_distance(TestResult& result) {
    std::string att_content = R"(
NAME : att_test
COMMENT : Test ATT distance
TYPE : TSP
DIMENSION : 2
EDGE_WEIGHT_TYPE : ATT
NODE_COORD_SECTION
1 0.0 0.0
2 10.0 10.0
EOF
)";

    TSPInstance instance = TSPLIBParser::parse_string(att_content);

    result.assert_eq(EdgeWeightType::ATT, instance.edge_weight_type, "ATT distance type");

    // ATT distance has a specific formula
    double distance = instance.calculate_distance(0, 1);
    result.assert_true(distance > 0.0, "ATT distance calculation returns positive value");

    // Test the specific ATT calculation manually
    double dx = 10.0, dy = 10.0;
    double rij = std::sqrt((dx * dx + dy * dy) / 10.0);
    double expected = std::round(rij);

    result.assert_eq(expected, distance, "ATT distance matches manual calculation");
}

int main() {
    std::cout << "=== EvoLab TSPLIB Parser Tests ===\n\n";

    TestResult result;

    test_basic_parsing(result);
    test_distance_calculations(result);
    test_explicit_matrix_full(result);
    test_explicit_matrix_upper_row(result);
    test_explicit_matrix_lower_row(result);
    test_manhattan_distance(result);
    test_maximum_distance(result);
    test_ceil_euclidean_distance(result);
    test_full_distance_matrix(result);
    test_tour_file_output(result);
    test_error_handling(result);
    test_geographical_distance(result);
    test_att_distance(result);

    return result.summary();
}