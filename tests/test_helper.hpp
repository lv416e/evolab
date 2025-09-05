#pragma once

#include <iostream>
#include <string>

// Common test framework - no external dependencies
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

    void print_summary() {
        std::cout << "\n=== Test Summary ===\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Total:  " << (passed + failed) << "\n";

        if (failed == 0) {
            std::cout << "All tests passed! ✓\n";
        } else {
            std::cout << "Some tests failed! ✗\n";
        }
    }

    int summary() {
        std::cout << "\n=== Test Summary ===\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Total:  " << (passed + failed) << "\n";
        return failed == 0 ? 0 : 1;
    }

    bool all_passed() const { return failed == 0; }
};