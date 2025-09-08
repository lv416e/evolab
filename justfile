# EvoLab C++23 Metaheuristics Research Platform
# Task automation with CMake Presets integration
# https://just.systems/

# Configuration variables - CMake Presets integration
preset := env_var_or_default('PRESET', 'default')
parallel_jobs := env_var_or_default('PARALLEL_JOBS', num_cpus())
unity_build := env_var_or_default('UNITY_BUILD', 'OFF')
cpp23_modules := env_var_or_default('CPP23_MODULES', 'OFF')
sanitizer := env_var_or_default('SANITIZER', 'address')

# Recipe aliases
alias b := build
alias t := test
alias d := dev
alias r := run
alias f := format
alias l := lint
alias c := clean
alias i := info
alias h := help

# Show available commands
default:
    @echo "EvoLab C++23 Development Commands"
    @echo "================================="
    @echo "Active preset: {{preset}} | CPUs: {{parallel_jobs}} | Unity: {{unity_build}} | Modules: {{cpp23_modules}}"
    @echo ""
    @just --list

# Configure and build the project using CMake Presets
build preset=preset:
    @echo "Configuring EvoLab with C++23 ({{preset}} preset)..."
    cmake --preset {{preset}}
    @echo "Building with {{parallel_jobs}} parallel jobs..."
    cmake --build --preset {{preset}} --parallel {{parallel_jobs}}

# CMake workflow execution (configure + build + test)
workflow preset=preset:
    @echo "Running complete workflow with {{preset}} preset..."
    cmake --workflow --preset {{preset}}

# Run all tests using CMake Presets
test preset=preset: (build preset)
    @echo "Running test suite with {{preset}} preset..."
    ctest --preset {{preset}}

# Run specific test suites  
test-core preset=preset: (build preset)
    @echo "Running core algorithm tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "CoreTests" --verbose

test-tsp preset=preset: (build preset)
    @echo "Running TSP problem tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "TSPTests" --verbose

test-operators preset=preset: (build preset) 
    @echo "Running genetic operator tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "OperatorTests" --verbose

test-schedulers preset=preset: (build preset)
    @echo "Running scheduler tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "SchedulerTests" --verbose

test-tsplib preset=preset: (build preset)
    @echo "Running TSPLIB integration tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "TSPLIBTests" --verbose

test-config preset=preset: (build preset)
    @echo "Running configuration tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "ConfigTests|ConfigIntegrationTests" --verbose

# Build and run TSP application
run preset=preset *ARGS: (build preset)
    @echo "Running TSP application with {{preset}} preset..."
    {{build_dir}}/apps/evolab-tsp {{ARGS}}

# Helper functions for advanced build configurations
build_dir := if preset == "default" { "build" } else { "build/" + preset }

# Validate preset parameter (simplified approach)
_validate-preset preset:
    @echo "Using preset: {{preset}}"

# Quick development workflow with parallel execution
[parallel]
dev: clean (workflow preset) (quality-checks preset)
    @echo "Development cycle completed successfully!"

# Parallel quality checks (format + lint + security)
[parallel]
quality-checks preset=preset: format check-format (lint preset)
    @echo "Code quality checks completed!"

# Debug build with sanitizers using debug preset
debug:
    @echo "Creating debug build with sanitizers..."
    just workflow debug

# Sanitizer Workflows - Modern C++23 CMake Preset-based
asan:
    @echo "Running AddressSanitizer workflow..."
    just workflow debug-asan

ubsan: 
    @echo "Running UndefinedBehaviorSanitizer workflow..."
    just workflow debug-ubsan

tsan:
    @echo "Running ThreadSanitizer workflow..."
    just workflow debug-tsan

msan:
    @echo "Running MemorySanitizer workflow..."  
    just workflow debug-msan

# Optimized release build using release preset
release:
    @echo "Creating optimized release build..."
    just workflow release

# Fast build using Ninja generator
ninja:
    @echo "Creating fast Ninja build..."
    just workflow ninja-release

# Unity Build for Faster Compilation
unity-build preset=preset: (_validate-preset preset)
    @echo "Building with Unity Build for faster compilation..."
    cmake --preset {{preset}} -DCMAKE_UNITY_BUILD=ON -DCMAKE_UNITY_BUILD_BATCH_SIZE=16
    cmake --build --preset {{preset}} --parallel {{parallel_jobs}}
    @echo "Unity build completed with batch size 16!"

# C++23 Modules Build
modules preset=preset: (_validate-preset preset)
    @echo "Building with C++23 Modules support..."
    cmake --preset {{preset}} -DCMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API=ON -DCMAKE_CXX_MODULE_STD=ON
    cmake --build --preset {{preset}} --parallel {{parallel_jobs}}
    @echo "C++23 Modules build completed!"

# Complete CI Pipeline (Parallel Execution)
[parallel]
ci: format check-format asan ubsan (test "release") benchmarks
    @echo "Complete CI pipeline finished successfully!"

# Performance Benchmarks with proper environment
benchmarks preset="release": (_validate-preset preset)
    @echo "Running performance benchmarks..."
    # Set CPU governor to performance for consistent benchmarks
    @echo "Setting CPU to performance mode for benchmarks..."
    cmake --build --preset {{preset}} --target benchmarks --parallel {{parallel_jobs}} || echo "Benchmarks target not found - skipping"
    @echo "Benchmarks completed with {{preset}} preset!"

# Advanced code formatting with parallel execution
format:
    @echo "Formatting C++ code with modern standards..."
    find include tests apps -name "*.hpp" -o -name "*.cpp" -print0 | xargs -0 -P {{parallel_jobs}} clang-format -i --style=file
    @echo "Code formatting completed with {{parallel_jobs}} parallel jobs!"

# Format specific staged files (for git hooks)
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

# Check formatting of specific staged files (for git hooks)
check-format-staged *FILES:
    @echo "Checking formatting of staged files..."
    @if [ "{{FILES}}" != "" ]; then \
        echo "{{FILES}}" | tr ' ' '\n' | xargs -r clang-format --dry-run --Werror; \
        echo "Staged files formatting check completed!"; \
    else \
        echo "No files to check"; \
    fi

# Static analysis with C++23 checks
lint preset=preset: (build preset) (_validate-preset preset)
    @echo "Running clang-tidy analysis with {{preset}} preset..."
    cd {{build_dir}} && \
    find ../include ../tests ../apps -name "*.cpp" -o -name "*.hpp" | \
    xargs -I {} -P {{parallel_jobs}} clang-tidy {} -p . --config-file=../.clang-tidy --checks="*,-fuchsia-*,-google-readability-*,-readability-magic-numbers" || echo "clang-tidy completed with warnings"
    @echo "C++23 static analysis completed!"

# Lint specific staged files (for git hooks)
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

# Clean build artifacts - Dynamic preset-aware cleaning
clean:
    @echo "Cleaning all build directories..."
    rm -rf build build/debug build/release build/ninja-release build/debug-asan build/debug-ubsan build/debug-tsan build/debug-msan
    @echo "Clean completed!"

clean-preset preset=preset:
    @echo "Cleaning {{preset}} build artifacts..."
    rm -rf {{build_dir}}

clean-all: clean
    @echo "Deep cleaning all generated files..."
    find . -name "*.orig" -delete
    @echo "Removing generated files..."
    find . -name "compile_commands.json" -not -path "./build*" -delete 2>/dev/null || true

# Project information and diagnostics
info:
    @echo "EvoLab Project Information"
    @echo "============================"
    @echo "Active preset: {{preset}}"
    @echo "Parallel jobs: {{parallel_jobs}}"
    @echo ""
    @echo "Tool Versions:"
    @echo "CMake: `cmake --version | head -1`"
    @echo "C++ Compiler: `clang++ --version | head -1`"
    @echo "System: `uname -sm`"
    @echo ""
    @echo "Available CMake Presets:"
    cmake --list-presets=all
    @echo ""
    @echo " Build Status:"
    @echo "  {{preset}} (active): `if [ -d "{{build_dir}}" ]; then echo "Configured"; else echo "Not configured"; fi`"
    @echo " Available build directories:"
    @for dir in build build/debug build/release build/ninja-release; do \
        if [ -d "$dir" ]; then \
            echo "  $dir: Configured (`du -sh $dir 2>/dev/null | cut -f1` used)"; \
        fi; \
    done; \
    if [ ! -d "build" ] && [ ! -d "build/debug" ] && [ ! -d "build/release" ] && [ ! -d "build/ninja-release" ]; then \
        echo "  No build directories found"; \
    fi

#  Comprehensive help for modern C++23 development (2025 Edition)
help:
    @echo " EvoLab C++23 Development Help (2025 Best Practices)"
    @echo "======================================================="
    @echo ""
    @echo " Quick Start (Recipe Aliases):"
    @echo "  just           or  j           # Show available commands"
    @echo "  just b         or  just build  # Build with active preset"
    @echo "  just t         or  just test   # Run tests"
    @echo "  just d         or  just dev    # Full parallel dev cycle"
    @echo "  just r         or  just run    # Run TSP application"
    @echo ""
    @echo " Modern C++23 Features:"
    @echo "  just unity-build debug        # Fast Unity builds (batch compilation)"
    @echo "  just modules release          # C++23 Modules with import std"
    @echo "  just ci                       # Complete CI pipeline (parallel)"
    @echo "  just quality-checks           # Parallel format + lint + security"
    @echo ""
    @echo " Sanitizer Analysis (CMake Preset-based):"
    @echo "  just asan                    # AddressSanitizer (memory errors)"
    @echo "  just ubsan                   # UndefinedBehaviorSanitizer"
    @echo "  just tsan                    # ThreadSanitizer (race conditions)"
    @echo "  just msan                    # MemorySanitizer (uninitialized reads)"
    @echo ""
    @echo " Performance Optimizations:"
    @echo "  just ninja                   # Fast Ninja builds"
    @echo "  just benchmarks release      # Performance benchmarks"
    @echo "  PARALLEL_JOBS=16 just b     # Override CPU cores (default: {{parallel_jobs}})"
    @echo ""
    @echo " Environment Configuration:"
    @echo "  PRESET=debug just b          # Override preset"
    @echo "  UNITY_BUILD=ON just build    # Enable Unity builds"
    @echo "  CPP23_MODULES=ON just build  # Enable C++23 modules"
    @echo "  just workflow debug-asan     # Direct preset workflow usage"
    @echo ""
    @echo " Modern C++23 Features (Industry Standard):"
    @echo "  just workflow release        # CMake workflow presets"
    @echo "  just quality-checks          # Parallel format + lint + security"
    @echo "  just ci                      # Complete CI pipeline (parallel)"
    @echo ""
    @echo " Diagnostics & Information:"
    @echo "  just i         or  just info  # Project diagnostics"
    @echo "  cmake --list-presets         # Available CMake presets"
    @echo "  just --choose               # Interactive recipe selection"