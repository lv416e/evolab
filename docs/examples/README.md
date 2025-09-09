# EvoLab Code Examples

This directory contains practical, compilable examples demonstrating EvoLab's capabilities. Each example focuses on specific features and can be used as a starting point for your own implementations.

## Quick Start

### Prerequisites

Before running examples, ensure you have:
- C++23 compatible compiler (GCC 12+, Clang 15+, or MSVC 19.35+)
- EvoLab project built (see [Getting Started](../getting-started.md))
- Intel TBB installed (for parallel examples)

### Compilation

From the examples directory:

```bash
# Basic compilation
g++ -std=c++23 -I../../include example.cpp -o example

# With TBB for parallel examples
g++ -std=c++23 -I../../include example.cpp -ltbb -o example

# With optimizations (recommended for performance testing)
g++ -std=c++23 -O3 -march=native -I../../include example.cpp -ltbb -o example
```

## Available Examples

### 1. Basic TSP Solver (`basic-tsp.cpp`)

**Purpose**: Demonstrates fundamental TSP solving with genetic algorithms

**Features**:
- Random TSP instance generation
- Basic genetic algorithm configuration
- Tournament selection, Order Crossover, 2-opt local search
- Performance timing and result validation

**Compile & Run**:
```bash
g++ -std=c++23 -I../../include basic-tsp.cpp -o basic-tsp
./basic-tsp
```

**Expected Output**:
```
EvoLab Basic TSP Example
========================

Problem: TSP with 30 cities
Coordinate range: [0, 100]

Algorithm Configuration:
  Population size: 100
  Max generations: 500
  ...

Best fitness: 245.67
Generations:  324
Solution:     Valid tour ✓
```

**Key Concepts Demonstrated**:
- `problems::create_random_tsp()` - Problem creation
- `factory::make_tsp_ga_basic()` - Algorithm factory
- `core::GAConfig` - Parameter configuration
- Result validation and timing

---

### 2. Parallel Evaluation (`parallel-evaluation.cpp`)

**Purpose**: High-performance parallel fitness evaluation using Intel TBB

**Features**:
- TBBExecutor for parallel evaluation
- Performance comparison (sequential vs parallel)
- Deterministic behavior verification
- Stateless design demonstration

**Requirements**: Intel TBB library

**Compile & Run**:
```bash
g++ -std=c++23 -I../../include parallel-evaluation.cpp -ltbb -o parallel-evaluation
./parallel-evaluation
```

**Expected Output**:
```
EvoLab Parallel Evaluation Example
===================================

Intel TBB: Available ✓
Problem setup:
  TSP cities: 100
  Population: 1000
  Hardware threads: 8

Performance Results:
Sequential time: 2.145 seconds
Parallel time:   0.892 seconds
Speedup:         2.41x
Efficiency:      30.1% (on 8 cores)

All tests passed! ✓
```

**Key Concepts Demonstrated**:
- `parallel::TBBExecutor` - Parallel execution
- Performance benchmarking techniques
- Deterministic parallel computation
- Const-correct stateless design

---

### 3. Configuration-Based (`config-based.cpp`)

**Purpose**: TOML-driven algorithm configuration without recompilation

**Features**:
- TOML configuration file creation and loading
- Dynamic algorithm selection based on config
- Multiple crossover operators (PMX, OX, EAX)
- Local search integration
- Evolution history logging

**Compile & Run**:
```bash
g++ -std=c++23 -I../../include config-based.cpp -o config-based

# Run with auto-generated config
./config-based

# Run with custom config file
./config-based my-config.toml
```

**Sample Configuration** (auto-generated `example-config.toml`):
```toml
[ga]
population_size = 256
max_generations = 1000
seed = 42

[operators.crossover]
type = "OX"           # Order Crossover
probability = 0.9

[local_search]
enabled = true
type = "2opt"
first_improvement = true
```

**Key Concepts Demonstrated**:
- `config::Config::from_file()` - Configuration loading
- Factory pattern for algorithm selection
- TOML configuration format
- Flexible operator selection

## Compilation Tips

### Debug vs Release Builds

**Debug** (for development):
```bash
g++ -std=c++23 -g -O0 -I../../include example.cpp -o example-debug
```

**Release** (for performance):
```bash
g++ -std=c++23 -O3 -march=native -DNDEBUG -I../../include example.cpp -o example-release
```

### Common Compilation Issues

**Issue**: "TBB not found"
```
Solution: Install TBB development package
  Ubuntu/Debian: sudo apt install libtbb-dev
  macOS: brew install tbb
  Windows: vcpkg install tbb
```

**Issue**: "C++23 features not available"
```
Solution: Use recent compiler version
  GCC 12+: g++-12 -std=c++23 ...
  Clang 15+: clang++-15 -std=c++23 ...
```

**Issue**: "Header not found"
```
Solution: Ensure correct include path
  -I../../include (from examples directory)
  -I../include (from project root)
```

## Creating Your Own Examples

### Template Structure

```cpp
/**
 * @file my-example.cpp
 * @brief Brief description of what this example demonstrates
 */

#include <iostream>
#include <evolab/evolab.hpp>

using namespace evolab;

int main() {
    std::cout << "My EvoLab Example\n";
    
    // Your code here
    
    return 0;
}
```

### Best Practices

1. **Include timing** for performance-critical code
2. **Validate solutions** with `tsp.is_valid_tour()`
3. **Use meaningful seeds** for reproducible results
4. **Show progress** for long-running algorithms
5. **Handle errors** gracefully with try/catch
6. **Document compilation** requirements in comments

### Common Patterns

**Problem Creation**:
```cpp
// Random instance
auto tsp = problems::create_random_tsp(cities, range, seed);

// From TSPLIB file
io::TSPLIBParser parser;
auto instance = parser.parse_file("data/berlin52.tsp");
auto tsp = problems::TSP::from_tsplib(instance);
```

**Algorithm Selection**:
```cpp
// Pre-configured algorithms
auto ga = factory::make_tsp_ga_basic();           // Simple setup
auto ga = factory::make_tsp_ga_advanced();        // Advanced operators

// Configuration-based
auto config = config::Config::from_file("config.toml");
auto ga = factory::make_tsp_ga_from_config(config);
```

**Performance Timing**:
```cpp
auto start = std::chrono::high_resolution_clock::now();
auto result = ga.run(tsp, config);
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration<double>(end - start).count();
```

## Next Steps

- **Modify examples** to experiment with different parameters
- **Create custom problems** by implementing the `Problem` concept
- **Explore advanced operators** in the main codebase
- **Check [Technical Decisions](../technical-decisions/)** for design rationale
- **Review [Getting Started](../getting-started.md)** for comprehensive setup guide

## Getting Help

If examples don't compile or run as expected:
1. Verify your compiler supports C++23
2. Check that EvoLab project builds successfully  
3. Ensure TBB is installed for parallel examples
4. Review compilation commands for correct paths
5. Open an issue with your compiler version and error message