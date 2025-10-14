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

    /**
     * @brief Asserts that a value is greater than a specified minimum and reports the result.
     *
     * Uses `message` as the base text for the test report; the recorded output also includes the compared values.
     *
     * @param value Value under test.
     * @param min_value Lower bound that `value` must exceed.
     * @param message Base text describing the assertion; included in the test report.
     */
    void assert_gt(size_t value, size_t min_value, const std::string& message) {
        assert_true(value > min_value, message + " (" + std::to_string(value) + " > " +
                                           std::to_string(min_value) + ")");
    }

    /**
     * @brief Asserts that a floating-point value is greater than a given minimum and reports the comparison.
     *
     * Appends a parenthesized `value > min_value` comparison to the provided message when recording the assertion result.
     *
     * @param value The floating-point value under test.
     * @param min_value The value that `value` must exceed for the assertion to pass.
     * @param message Human-readable message describing the assertion context; the function appends the numeric comparison.
     */
    void assert_gt(double value, double min_value, const std::string& message) {
        assert_true(value > min_value, message + " (" + std::to_string(value) + " > " +
                                           std::to_string(min_value) + ")");
    }

    /**
     * @brief Asserts that a floating-point value is less than or equal to a given maximum and reports the result.
     *
     * Appends a comparison note to the provided message and delegates to the test runner's assertion mechanism,
     * updating pass/fail counts and printing the outcome.
     *
     * @param value The value under test.
     * @param max_value The inclusive upper bound that `value` must not exceed.
     * @param message Human-readable context included in the assertion output.
     */
    void assert_le(double value, double max_value, const std::string& message) {
        assert_true(value <= max_value, message + " (" + std::to_string(value) +
                                            " <= " + std::to_string(max_value) + ")");
    }

    /**
     * @brief Prints a human-readable summary of test results to standard output.
     *
     * Displays a header, the counts of passed and failed assertions, the total
     * number of assertions, and a final line stating whether all tests passed or
     * some tests failed.
     */
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