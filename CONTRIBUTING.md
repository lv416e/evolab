# Contributing to EvoLab

Thank you for your interest in contributing to EvoLab! This document provides guidelines for contributing to the project.

## Table of Contents
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Code Standards](#code-standards)
- [Testing](#testing)
- [Performance](#performance)
- [Documentation](#documentation)
- [Pull Request Process](#pull-request-process)
- [Issue Guidelines](#issue-guidelines)

## Getting Started

### Prerequisites
- C++23 compatible compiler (GCC 12+, Clang 15+, MSVC 19.35+)
- CMake 3.22 or later
- Git
- Optional: oneTBB, OpenMP for parallelization

### Development Setup
```bash
# Clone the repository
git clone https://github.com/lv416e/evolab.git
cd evolab

# Create and configure build directory
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DEVOLAB_BUILD_TESTS=ON

# Build the project
cmake --build . --parallel

# Run tests
ctest --output-on-failure
```

### Development Tools
Ensure you have the following tools installed:
- `clang-format-17` for code formatting
- `clang-tidy-17` for static analysis
- `cmake` and `ninja-build` (recommended)

## Development Workflow

### Branch Naming
- `feat/feature-name` - New features
- `fix/issue-description` - Bug fixes
- `docs/documentation-update` - Documentation changes
- `refactor/component-name` - Code refactoring
- `perf/optimization-area` - Performance improvements

### Commit Messages
Follow the conventional commit format:
```
type(scope): description

[optional body]

[optional footer]
```

Examples:
```
feat(operators): add EAX crossover operator
fix(tsp): correct distance matrix indexing
docs(readme): update build instructions
perf(local-search): optimize 2-opt neighborhood evaluation
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`

### Development Process
1. Create a feature branch from `main`
2. Make your changes following code standards
3. Write or update tests for new functionality
4. Run the full test suite and ensure it passes
5. Format code using clang-format
6. Run static analysis with clang-tidy
7. Update documentation if needed
8. Create a pull request

## Code Standards

### C++ Standards
- Use C++23 features appropriately
- Prefer modern C++ idioms (RAII, smart pointers, ranges)
- Use concepts for template constraints
- Prefer `constexpr` and `noexcept` where applicable

### Naming Conventions
- **Classes/Structs**: `PascalCase` (e.g., `GeneticAlgorithm`)
- **Functions/Methods**: `snake_case` (e.g., `evaluate_fitness`)
- **Variables**: `snake_case` (e.g., `population_size`)
- **Private members**: Trailing underscore (e.g., `data_`)
- **Constants**: `UPPER_CASE` or `snake_case`
- **Template parameters**: `PascalCase` (e.g., `Problem`, `Operator`)

### Code Organization
- Header-only library design
- Use `#pragma once` for include guards
- Organize includes: system headers, third-party, project headers
- Keep functions reasonably sized (prefer < 50 lines)
- Use meaningful variable names
- Add comments for complex algorithms

### Error Handling
- Use assertions for debug-time checks
- Prefer error codes over exceptions in performance-critical paths
- Validate inputs in public APIs
- Use RAII for resource management

### Performance Guidelines
- Profile before optimizing
- Use cache-friendly data structures
- Prefer stack allocation over dynamic allocation
- Use move semantics appropriately
- Consider SIMD optimizations for hot paths

## Testing

### Unit Tests
- Write tests for all new functionality
- Use Google Test framework
- Aim for high code coverage
- Test edge cases and error conditions

### Test Organization
```
tests/
├── test_core.cpp          # Core framework tests
├── test_operators.cpp     # Genetic operators tests
├── test_tsp.cpp          # TSP-specific tests
└── benchmarks/           # Performance benchmarks
```

### Running Tests
```bash
# Run all tests
ctest --output-on-failure

# Run specific test
./tests/test_core

# Run with sanitizers
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
make && ctest
```

## Performance

### Benchmarking
- Benchmark performance-critical changes
- Use consistent test environments
- Compare against baseline performance
- Document performance characteristics

### Profiling
```bash
# Profile with perf (Linux)
perf record ./apps/evolab_tsp --instance data/pr2392.tsp
perf report

# Profile with Instruments (macOS)
instruments -t "Time Profiler" ./apps/evolab_tsp
```

## Documentation

### Code Documentation
- Use Doxygen-style comments (`///`)
- Document all public APIs
- Include usage examples
- Document performance characteristics

### API Documentation
```cpp
/// @brief Evaluates the fitness of a genome for the TSP
/// @param genome The tour representation as city indices
/// @return Fitness object with total tour distance
/// @complexity O(n) where n is number of cities
core::Fitness evaluate(const GenomeT& genome) const;
```

## Pull Request Process

### Before Submitting
- [ ] Code builds without warnings
- [ ] All tests pass
- [ ] Code is formatted with clang-format
- [ ] Static analysis passes (clang-tidy)
- [ ] Documentation updated if needed
- [ ] Performance regression tests pass

### PR Template
```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Performance improvement
- [ ] Documentation update
- [ ] Refactoring

## Testing
- [ ] Unit tests added/updated
- [ ] Manual testing performed
- [ ] Performance benchmarks run

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review completed
- [ ] Tests pass locally
- [ ] Documentation updated
```

### Review Process
1. Automated CI checks must pass
2. Manual code review (self-review for single developer)
3. Performance validation for critical changes
4. Merge after approval

## Issue Guidelines

### Bug Reports
Include:
- EvoLab version
- Operating system and compiler
- Minimal reproduction case
- Expected vs actual behavior
- Stack trace if applicable

### Feature Requests
Include:
- Use case description
- Proposed API if applicable
- Performance considerations
- Implementation ideas

### Performance Issues
Include:
- Profiling data
- System specifications
- Comparison with baseline
- Reproduction steps

## Code Quality

### Static Analysis
```bash
# Run clang-tidy
find include -name "*.hpp" | xargs clang-tidy-17

# Run with specific checks
clang-tidy-17 --checks='modernize-*,performance-*' include/**/*.hpp
```

### Formatting
```bash
# Format all source files
find include apps tests -name "*.hpp" -o -name "*.cpp" | \
  xargs clang-format-17 -i

# Check formatting without changes
find include apps tests -name "*.hpp" -o -name "*.cpp" | \
  xargs clang-format-17 --dry-run --Werror
```

### Memory Safety
- Run with AddressSanitizer for memory error detection
- Use Valgrind on Linux for additional checks
- Avoid raw pointers in favor of smart pointers
- Use span/string_view for safe array access

## Continuous Integration

The CI pipeline runs:
- Multi-platform builds (Linux, macOS, Windows)
- Code formatting checks
- Static analysis with clang-tidy
- Unit tests with multiple compilers
- Sanitizer builds (AddressSanitizer, UBSan)
- Performance regression tests

All checks must pass before merging.

## Getting Help

- Open an issue for bugs or feature requests
- Start a discussion for design questions
- Check existing issues and PRs first
- Be patient and respectful in communications

## License

By contributing to EvoLab, you agree that your contributions will be licensed under the Apache-2.0 license.