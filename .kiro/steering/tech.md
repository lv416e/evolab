# Technology Stack

## Architecture
- **Library Type**: Header-only template library
- **Design Pattern**: Policy-based design with concepts
- **Core Architecture**: Modular components with clear separation of concerns
- **Performance Focus**: Zero-cost abstractions, compile-time optimization

## Language & Standards
- **Primary Language**: C++23
- **Required Features**: Concepts, ranges, structured bindings, if constexpr
- **Compiler Support**: GCC 12+, Clang 15+, MSVC 19.35+
- **Build Standard**: CMAKE_CXX_STANDARD 23

## Build System
- **Tool**: CMake 3.22+
- **Build Type**: Interface library (header-only)
- **Target**: EvoLab::evolab
- **Installation**: Proper CMake package with config files
- **Components**: Core library, tests, benchmarks (optional)

## Development Environment

### Required Tools
- Modern C++ compiler with C++23 support
- CMake 3.22 or higher
- Git for version control

### Optional Dependencies
- **oneTBB**: Intel Threading Building Blocks for parallel evaluation
- **OpenMP**: Alternative parallelization backend
- **Google Test**: Unit testing framework (fetched automatically)
- **Google Benchmark**: Performance benchmarking (fetched automatically)

### Code Quality Tools
- **Formatter**: clang-format with .clang-format configuration
- **Static Analysis**: clang-tidy with .clang-tidy configuration
- **Pre-commit Hooks**: lefthook for automated checks
- **CI/CD**: GitHub Actions for build and test automation

## Common Commands

### Building
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Testing
```bash
cmake --build build --target test
./build/tests/test_core
./build/tests/test_operators
./build/tests/test_tsp
```

### Running Examples
```bash
./build/apps/evolab-tsp <problem_file.tsp>
```

### Development Hooks
```bash
lefthook install  # Install git hooks
lefthook run pre-commit  # Manual pre-commit check
```

## Environment Variables
- **EVOLAB_DATA_DIR**: Path to TSP/VRP problem instances (optional)
- **EVOLAB_LOG_LEVEL**: Logging verbosity (DEBUG, INFO, WARNING, ERROR)
- **OMP_NUM_THREADS**: OpenMP thread count for parallel evaluation

## Port Configuration
Not applicable - EvoLab is a library, not a service

## Performance Optimizations

### Compiler Flags
- **Release**: `-O3 -march=native -DNDEBUG`
- **Debug**: `-O0 -g -fsanitize=address,undefined`
- **Profile**: `-O3 -g -fno-omit-frame-pointer`

### Data Structure Choices
- Row-major distance matrices for cache locality
- Vector-based permutation representation
- Thread-local random number generators
- Pre-allocated memory pools for operators

### Parallelization Strategy
- Thread-pool based population evaluation
- Island model for distributed evolution
- Lock-free data structures where possible
- SIMD vectorization for distance calculations