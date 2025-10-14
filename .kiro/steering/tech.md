# Technology Stack - EvoLab

## Architecture

### System Design
- **Architecture Pattern**: Header-only template library with concept-based design
- **Programming Paradigm**: Modern C++ with functional and object-oriented elements
- **Concurrency Model**: Optional parallelization via TBB or OpenMP
- **Build System**: CMake-based with FetchContent for dependencies

### Core Components
```
evolab/
├── core/          # Fundamental GA implementation
├── problems/      # Problem-specific adapters (TSP, VRP, QAP)
├── operators/     # Selection, crossover, mutation operators
├── local_search/  # 2-opt, 3-opt, Lin-Kernighan
├── schedulers/    # Adaptive operator selection (UCB)
├── config/        # TOML-based configuration system
├── io/            # TSPLIB parser and I/O utilities
└── parallel/      # Thread-pool and Island Model
```

## Language & Compiler

### Primary Language
- **C++23**: Core implementation language
- **Compiler Requirements**: 
  - GCC 12+ (recommended)
  - Clang 15+ (supported)
  - MSVC 19.35+ (supported)

### Language Features Used
- Concepts for template constraints
- Ranges for algorithm implementations
- std::format for string formatting
- Coroutines for async operations (planned)
- std::expected for error handling
- std::span for memory views
- Designated initializers for configs

## Build System & Dependencies

### Build Tools
- **CMake 3.22+**: Primary build system
- **FetchContent**: For dependency management
- **CTest**: For test execution
- **CPack**: For package generation (planned)

### Core Dependencies
```cmake
# Required
- C++23 standard library
- toml11 v4.2.0      # TOML configuration parsing

# Optional
- oneTBB             # Intel Threading Building Blocks
- OpenMP             # Parallel processing support
- Google Test        # Unit testing framework
- Google Benchmark   # Performance benchmarking
```

### Compiler Flags
```bash
# Debug Build
-Wall -Wextra -Wpedantic -g -O0

# Release Build  
-O3 -march=native -DNDEBUG
```

## Development Environment

### Required Tools
```bash
# Compilers (one of)
brew install gcc@12        # macOS
sudo apt install g++-12    # Ubuntu
winget install LLVM        # Windows

# Build System
brew install cmake         # macOS
sudo apt install cmake     # Ubuntu

# Git Hooks
brew install lefthook      # Fast git hooks manager
```

### IDE Support
- **VS Code**: Full IntelliSense with C++ extension
- **CLion**: CMake integration out-of-box
- **Xcode**: Generate project with CMake
- **Visual Studio**: CMake support built-in

### Development Setup
```bash
# Clone and setup
git clone <repo>
cd evolab
lefthook install  # Install git hooks

# Build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Common Commands

### Build Commands
```bash
# Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release build with optimizations
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Build with specific compiler
CC=gcc-12 CXX=g++-12 cmake -B build -S .

# Clean build
rm -rf build/
cmake -B build -S .
```

### Test Commands
```bash
# Run all tests
ctest --test-dir build

# Run specific test
./build/tests/test_core
./build/tests/test_operators
./build/tests/test_tsp

# Verbose test output
ctest --test-dir build --verbose
```

### Development Commands
```bash
# Format code (via git hook)
clang-format -i src/**/*.cpp include/**/*.hpp

# Static analysis
clang-tidy include/**/*.hpp

# Generate compile commands
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run benchmarks
./build/benchmarks/bench_operators
```

### Application Commands
```bash
# Run TSP solver
./build/apps/evolab-tsp --config configs/tsp_default.toml

# With custom instance
./build/apps/evolab-tsp --instance data/tsplib/pr2392.tsp

# Override config values
./build/apps/evolab-tsp --ga.population_size=512
```

## Environment Variables

### Build Configuration
```bash
# Compiler selection
export CC=gcc-12
export CXX=g++-12

# CMake options
export CMAKE_BUILD_TYPE=Release
export CMAKE_CXX_COMPILER_LAUNCHER=ccache

# Parallel build
export CMAKE_BUILD_PARALLEL_LEVEL=8
```

### Runtime Configuration
```bash
# Thread control
export OMP_NUM_THREADS=8      # OpenMP threads
export TBB_NUM_THREADS=8      # TBB threads

# Debug output
export EVOLAB_LOG_LEVEL=DEBUG
export EVOLAB_TRACE_OPERATORS=1
```

### Development Environment
```bash
# Enable sanitizers
export ASAN_OPTIONS=detect_leaks=1
export UBSAN_OPTIONS=print_stacktrace=1

# Profiling
export EVOLAB_PROFILE=1
export EVOLAB_PROFILE_OUTPUT=profile.json
```

## Port Configuration

### Standard Ports
- Not applicable (library/CLI application)

### File Paths
```bash
# Configuration
~/.evolab/config.toml     # User config (planned)
./configs/*.toml          # Project configs

# Data
./data/tsplib/            # TSPLIB instances
./data/solutions/         # Saved solutions

# Output
./results/                # Experiment results
./logs/                   # Execution logs
```

## Quality Assurance

### Code Quality Tools
- **clang-format**: Automated code formatting
- **clang-tidy**: Static analysis and linting
- **Google Test**: Unit testing framework
- **lefthook**: Git hooks for quality checks

### CI/CD Pipeline (Planned)
- GitHub Actions for automated testing
- Coverage reporting with gcov/lcov
- Benchmark regression detection
- Multi-compiler testing matrix

### Git Hooks (via lefthook)
```yaml
pre-commit:
  - Auto-format with clang-format
  - Build verification
  - Static analysis with clang-tidy

pre-push:
  - Full test suite execution
  - Benchmark regression check
```