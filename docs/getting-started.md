# Getting Started with EvoLab

EvoLab is a modern C++23 framework for evolutionary algorithms with high-performance parallel execution capabilities. This guide will help you get up and running with your first evolutionary algorithm.

## Prerequisites

Before using EvoLab, ensure you have:

- **C++23 compatible compiler** - GCC 12+, Clang 15+, or MSVC 19.35+
- **CMake 3.25+** - For building the project
- **Intel TBB** (optional) - For parallel fitness evaluation
- **OpenMP** (optional) - For additional parallelization options

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/lv416e/evolab.git
cd evolab
```

### 2. Install Dependencies

#### Ubuntu/Debian
```bash
sudo apt install libtbb-dev
```

#### macOS (with Homebrew)
```bash
brew install tbb
```

#### Windows (with vcpkg)
```bash
vcpkg install tbb
```

### 3. Build the Project

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Build Options

Configure the build with these CMake options:

```bash
# Enable/disable parallel execution (default: ON)
cmake -B build -DEVOLAB_USE_TBB=ON

# Build tests (default: ON)  
cmake -B build -DEVOLAB_BUILD_TESTS=ON

# Build benchmarks (default: ON)
cmake -B build -DEVOLAB_BUILD_BENCHMARKS=ON
```

## Your First Evolutionary Algorithm

Here's a complete example that solves the Traveling Salesman Problem (TSP) using EvoLab:

```cpp
#include <iostream>
#include <evolab/evolab.hpp>

using namespace evolab;

int main() {
    // Create a random TSP instance with 50 cities
    auto tsp = problems::create_random_tsp(50, 100.0, 42);
    
    // Create a genetic algorithm with advanced operators
    auto ga = factory::make_tsp_ga_basic();
    
    // Configure the algorithm parameters
    core::GAConfig ga_config{
        .population_size = 100,
        .max_generations = 500,
        .crossover_prob = 0.9,
        .mutation_prob = 0.1,
        .seed = 123
    };
    
    // Run the evolution
    std::cout << "Solving TSP with " << tsp.num_cities() << " cities...\n";
    auto result = ga.run(tsp, ga_config);
    
    // Display results
    std::cout << "Best fitness: " << result.best_fitness.value << "\n";
    std::cout << "Generations: " << result.generations << "\n";
    std::cout << "Evaluations: " << result.evaluations << "\n";
    
    return 0;
}
```

### Compile and Run

```bash
# Compile your program
g++ -std=c++23 -I include my_program.cpp -o my_program

# Run it
./my_program
```

## Algorithm Variants

EvoLab provides several pre-configured algorithm variants:

### Basic Algorithm
```cpp
auto ga = factory::make_tsp_ga_basic();
```
- Tournament selection (size 4)
- Order Crossover (OX)
- Swap mutation
- 2-opt local search

### Advanced Algorithm  
```cpp
auto ga = factory::make_tsp_ga_advanced();
```
- Tournament selection (size 7)
- Edge Recombination Crossover (EAX)
- Adaptive mutation
- Candidate list 2-opt

### Configuration-Based Algorithm
```cpp
// Load configuration from TOML file
auto config = config::Config::from_file("config/advanced.toml");
auto ga = factory::make_tsp_ga_from_config(config);
```

## Parallel Execution

EvoLab supports high-performance parallel fitness evaluation using Intel TBB:

```cpp
#include <evolab/parallel/tbb_executor.hpp>

// Create parallel executor
evolab::parallel::TBBExecutor executor(123);  // seed for reproducibility

// Create problem and population
auto tsp = problems::create_random_tsp(100);
std::vector<TSP::GenomeT> population = /* ... your population ... */;

// Parallel fitness evaluation
auto fitnesses = executor.parallel_evaluate(tsp, population);
```

Key features of parallel execution:
- **Deterministic**: Same seed produces identical results
- **Thread-safe**: Const-correct stateless design  
- **High performance**: Measured 1.33x speedup on 150-city instances

## Problem Types

### Traveling Salesman Problem (TSP)

```cpp
// Random TSP instance
auto tsp = problems::create_random_tsp(
    50,      // number of cities
    100.0,   // coordinate range [0, 100.0]
    42       // random seed
);

// Load from TSPLIB format
io::TSPLIBParser parser;
auto instance = parser.parse_file("data/berlin52.tsp");
auto tsp = problems::TSP::from_tsplib(instance);
```

### Custom Problems

Implement the `Problem` concept for your optimization problem:

```cpp
template <typename T>
concept Problem = requires(T t, typename T::GenomeT genome) {
    typename T::GenomeT;
    { t.evaluate(genome) } -> std::convertible_to<core::Fitness>;
    { t.random_genome(std::declval<std::mt19937&>()) } -> std::same_as<typename T::GenomeT>;
};
```

## Configuration Files

Use TOML configuration files for complex algorithm setups:

```toml
[ga]
population_size = 512
max_generations = 2000
seed = 42

[operators.selection]
type = "tournament"
tournament_size = 7

[operators.crossover]
type = "OX"
probability = 0.9

[operators.mutation]
type = "swap"
probability = 0.1

[local_search]
enabled = true
type = "2opt"
first_improvement = true
max_iterations = 1000
```

Load and use:

```cpp
auto config = config::Config::from_file("my_config.toml");
auto ga = factory::make_tsp_ga_from_config(config);
```

## Running Tests

Verify your installation by running the test suite:

```bash
# Build tests
cmake --build build --target test_core

# Run tests
./build/tests/test_core
./build/tests/test_parallel
./build/tests/test_operators
```

## Next Steps

- **[Examples Directory](examples/)** - Explore more complex usage patterns
- **[Technical Decisions](technical-decisions/)** - Understand design choices  
- **API Reference** - Detailed class and function documentation (coming soon)

## Getting Help

If you encounter issues:

1. Check the [examples](examples/) for similar use cases
2. Review the [technical decisions](technical-decisions/) for design rationale
3. Open an issue on GitHub with a minimal reproducible example

## Performance Tips

- Use **Release build** (`-DCMAKE_BUILD_TYPE=Release`) for benchmarking
- Enable **TBB parallel execution** for large populations
- Use **candidate lists** for local search on large instances
- Set appropriate **population sizes** (100-500 for most problems)