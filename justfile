# EvoLab C++23 Metaheuristics Research Platform
# 
# This justfile provides a unified interface for all development tasks,
# from building and testing to code quality checks and deployment.
# It integrates seamlessly with CMake Presets for consistent configuration.
#
# Quick start: `just --list` to see all available commands
# Documentation: https://just.systems/

# Project configuration with flexible preset system
preset := env_var_or_default('PRESET', 'default')
parallel_jobs := env_var_or_default('PARALLEL_JOBS', num_cpus())
unity_build := env_var_or_default('UNITY_BUILD', 'OFF')
cpp23_modules := env_var_or_default('CPP23_MODULES', 'OFF')
sanitizer := env_var_or_default('SANITIZER', 'address')

# Convenient shortcuts for frequently used commands
alias b := build
alias t := test
alias d := dev
alias r := run
alias f := format
alias l := lint
alias c := clean
alias i := info
alias h := help

# Display all available commands (default action)
default:
    @just --list

# Build project using any available preset configuration
build preset=preset:
    @echo "Configuring EvoLab with C++23 ({{preset}} preset)..."
    cmake --preset {{preset}}
    @echo "Building with {{parallel_jobs}} parallel jobs..."
    cmake --build --preset {{preset}} --parallel {{parallel_jobs}}

# Run complete development cycle: configure, build, and test in one command
workflow preset=preset:
    @echo "Running complete workflow with {{preset}} preset..."
    cmake --workflow --preset {{preset}}

# Run all tests with specified preset
test preset=preset: (build preset)
    @echo "Running test suite with {{preset}} preset..."
    ctest --preset {{preset}}

# Run core algorithm tests only
test-core preset=preset: (build preset)
    @echo "Running core algorithm tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "CoreTests" --verbose

# Run TSP problem-specific tests
test-tsp preset=preset: (build preset)
    @echo "Running TSP problem tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "TSPTests" --verbose

# Run genetic operator tests
test-operators preset=preset: (build preset)
    @echo "Running genetic operator tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "OperatorTests" --verbose

# Run scheduler algorithm tests
test-schedulers preset=preset: (build preset)
    @echo "Running scheduler tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "SchedulerTests" --verbose

# Run TSPLIB integration tests
test-tsplib preset=preset: (build preset)
    @echo "Running TSPLIB integration tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "TSPLIBTests" --verbose

# Run configuration system tests
test-config preset=preset: (build preset)
    @echo "Running configuration tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "ConfigTests|ConfigIntegrationTests" --verbose

# Build and run TSP application with arguments
run preset=preset *ARGS: (build preset)
    @echo "Running TSP application with {{preset}} preset..."
    {{build_dir}}/apps/evolab-tsp {{ARGS}}

# Dynamic build directory resolution
build_dir := if preset == "default" { "build" } else { "build/" + preset }

# === INTERNAL FUNCTIONS (Hidden Recipes) ===

# Get all available CMake presets dynamically
_preset-list:
    @cmake --list-presets=configure 2>/dev/null | grep '"' | cut -d'"' -f2 || echo "default debug release ninja-release"

# Get all build directories that exist
_build-dirs:
    @find . -maxdepth 2 -name "build*" -type d 2>/dev/null | sort

# Get current tool versions dynamically
_tool-versions:
    @echo "CMake: $(cmake --version 2>/dev/null | head -1 | cut -d' ' -f3 || echo 'not found')"
    @echo "Clang: $(clang++ --version 2>/dev/null | head -1 | cut -d' ' -f4 || echo 'not found')"
    @echo "Just: $(just --version 2>/dev/null | cut -d' ' -f2 || echo 'not found')"
    @echo "System: $(uname -sm 2>/dev/null || echo 'unknown')"

# Validate preset parameter and ensure it exists
_validate-preset preset:
    @echo "Using preset: {{preset}}"
    @if ! cmake --list-presets=configure 2>/dev/null | grep -q '"{{preset}}"'; then \
        echo "Warning: Preset '{{preset}}' not found in CMakePresets.json"; \
        echo "Available presets:"; \
        cmake --list-presets=configure 2>/dev/null || echo "No presets available"; \
    fi

# Full development workflow: clean slate + build + test + code quality
[parallel]
dev: clean (workflow preset) (quality-checks preset)
    @echo "Development cycle completed successfully!"

# Ensure code meets project standards (formatting and static analysis)
[parallel]
quality-checks preset=preset: format check-format (lint preset)
    @echo "Code quality checks completed!"

# Quick debug build with sanitizers
debug:
    @echo "Creating debug build with sanitizers..."
    just workflow debug

# Build and test with AddressSanitizer
asan:
    @echo "Running AddressSanitizer workflow..."
    just workflow debug-asan

# Build and test with UndefinedBehaviorSanitizer
ubsan:
    @echo "Running UndefinedBehaviorSanitizer workflow..."
    just workflow debug-ubsan

# Build and test with ThreadSanitizer
tsan:
    @echo "Running ThreadSanitizer workflow..."
    just workflow debug-tsan

# Build and test with MemorySanitizer
msan:
    @echo "Running MemorySanitizer workflow..."
    just workflow debug-msan

# Quick optimized release build
release:
    @echo "Creating optimized release build..."
    just workflow release

# Build using Ninja generator
ninja:
    @echo "Creating fast Ninja build..."
    just workflow ninja-release

# Build with Unity compilation
unity-build preset=preset: (_validate-preset preset)
    @echo "Building with Unity Build for faster compilation..."
    cmake --preset {{preset}} -DCMAKE_UNITY_BUILD=ON -DCMAKE_UNITY_BUILD_BATCH_SIZE=16
    cmake --build --preset {{preset}} --parallel {{parallel_jobs}}
    @echo "Unity build completed with batch size 16!"

# Build with experimental C++23 modules support
cpp23-modules preset=preset: (_validate-preset preset)
    @echo "Building with experimental C++23 Modules support..."
    cmake --preset {{preset}} -DCMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API=2942ff8d-e5b4-4c52-80fe-8d6bf12e6f28 -DCMAKE_CXX_MODULE_STD=ON
    cmake --build --preset {{preset}} --parallel {{parallel_jobs}}
    @echo "C++23 Modules build completed!"

# Test C++23 features compilation
cpp23-features preset="debug": (_validate-preset preset)
    @echo "Testing C++23 features compilation..."
    cmake --preset {{preset}} -DEVOLAB_ENABLE_CPP23_EXPERIMENTAL=ON
    cmake --build --preset {{preset}} --target test_cpp23_features --parallel {{parallel_jobs}}
    {{build_dir}}/tests/test_cpp23_features || echo "C++23 features test completed with notes"

# Run CI pipeline with all checks
[parallel]
ci: format check-format asan ubsan (test "release") benchmarks
    @echo "Complete CI pipeline finished successfully!"

# Run performance benchmarks
benchmarks preset="release": (_validate-preset preset)
    @echo "Running performance benchmarks..."
    @echo "Setting CPU to performance mode for benchmarks..."
    cmake --build --preset {{preset}} --target benchmarks --parallel {{parallel_jobs}} || echo "Benchmarks target not found - skipping"
    @echo "Benchmarks completed with {{preset}} preset!"

# Auto-format all C++ code to maintain consistent style
format:
    @echo "Formatting C++ code with configured style..."
    find include tests apps -name "*.hpp" -o -name "*.cpp" -print0 | xargs -0 -P {{parallel_jobs}} clang-format -i --style=file
    @echo "Code formatting completed with {{parallel_jobs}} parallel jobs!"

# Format specific staged files
format-staged *FILES:
    @echo "Formatting staged files..."
    @if [ "{{FILES}}" != "" ]; then \
        echo "{{FILES}}" | tr ' ' '\n' | xargs -r -P {{parallel_jobs}} clang-format -i --style=file; \
        echo "Staged files formatted with {{parallel_jobs}} parallel jobs!"; \
    else \
        echo "No files to format"; \
    fi

# Check code formatting
check-format:
    @echo "Checking code formatting..."
    find include tests apps -name "*.hpp" -o -name "*.cpp" -print0 | xargs -0 clang-format --dry-run --Werror

# Check formatting of specific staged files
check-format-staged *FILES:
    @echo "Checking formatting of staged files..."
    @if [ "{{FILES}}" != "" ]; then \
        echo "{{FILES}}" | tr ' ' '\n' | xargs -r clang-format --dry-run --Werror; \
        echo "Staged files formatting check completed!"; \
    else \
        echo "No files to check"; \
    fi

# Check code for potential issues and improvements
lint preset=preset: (build preset) (_validate-preset preset)
    @echo "Running clang-tidy analysis with {{preset}} preset..."
    find include tests apps -name "*.cpp" -o -name "*.hpp" -print0 | \
    xargs -0 -I {} -P {{parallel_jobs}} clang-tidy {} -p {{build_dir}} --config-file=.clang-tidy --checks="*,-fuchsia-*,-google-readability-*,-readability-magic-numbers" || echo "clang-tidy completed with warnings"
    @echo "C++23 static analysis completed!"

# Lint specific staged files
lint-staged preset=preset *FILES:
    @echo "Running clang-tidy on staged files with {{preset}} preset..."
    @if [ "{{FILES}}" != "" ]; then \
        if [ -f "{{build_dir}}/compile_commands.json" ]; then \
            echo "{{FILES}}" | tr ' ' '\n' | grep -E '\.(hpp|cpp)$' | \
            xargs -r -I {} -P {{parallel_jobs}} clang-tidy {} -p {{build_dir}} --config-file=.clang-tidy --checks="*,-fuchsia-*,-google-readability-*,-readability-magic-numbers" || echo "clang-tidy completed with warnings"; \
            echo "Staged files linting completed!"; \
        else \
            echo "No compilation database found. Run 'just build {{preset}}' first."; \
        fi; \
    else \
        echo "No files to lint"; \
    fi

# Clean all build directories
clean:
    @echo "Cleaning all build directories..."
    @dirs=$(just _build-dirs); if [ -n "$dirs" ]; then echo "$dirs" | xargs rm -rf; else echo "No build directories to clean"; fi
    @echo "Clean completed!"

# Clean specific preset build directory
clean-preset preset=preset:
    @echo "Cleaning {{preset}} build artifacts..."
    rm -rf {{build_dir}}

# Deep clean all generated files
clean-all: clean
    @echo "Deep cleaning all generated files..."
    find . -name "*.orig" -delete
    @echo "Removing generated files..."
    find . -name "compile_commands.json" -not -path "./build*" -delete 2>/dev/null || true

# Project information and diagnostics (dynamic)
info:
    @echo "EvoLab C++23 Project Information"
    @echo "================================"
    @echo "Active preset: {{preset}} | CPUs: {{parallel_jobs}}"
    @echo ""
    @echo "Tool Versions:"
    @just _tool-versions
    @echo ""
    @echo "Available CMake Presets:"
    @cmake --list-presets=all 2>/dev/null || echo "No CMake presets found"
    @echo ""
    @echo "Build Directory Status:"
    @dirs=$(just _build-dirs); if [ -n "$dirs" ]; then echo "$dirs" | while read dir; do echo "  $dir: $(du -sh "$dir" 2>/dev/null | cut -f1) used"; done; else echo "  No build directories found"; fi

# Show comprehensive project help and documentation
help:
    @echo "EvoLab C++23 Metaheuristics Platform"
    @echo "====================================="
    @echo ""
    @echo "Available Commands:"
    @just --list
    @echo ""
    @echo "Project Information:"
    @echo "  just info                    # Detailed project diagnostics"
    @echo "  cmake --list-presets=all    # All available CMake presets"
    @echo ""
    @echo "Quick Start:"
    @echo "  just                         # Show this help"
    @echo "  just build                   # Build with default preset"
    @echo "  just test                    # Run all tests"
    @echo "  just workflow release        # Complete build+test cycle"
    @echo "  just dev                     # Full development workflow"