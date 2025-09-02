# EvoLab

A modern C++23 metaheuristics research platform for solving complex optimization problems.

## Overview

EvoLab is a high-performance genetic algorithm framework designed for research-grade optimization of problems like the Traveling Salesman Problem (TSP), Vehicle Routing Problem (VRP), and Quadratic Assignment Problem (QAP).

### Key Features

- **Modern C++23**: Built with the latest C++ features including concepts, ranges, and coroutines
- **Research-Grade**: Full reproducibility with seed management, comprehensive logging, and statistical analysis
- **Hybrid Algorithms**: Memetic GA combining genetic operators with local search (2-opt, Lin-Kernighan)
- **Advanced Operators**: EAX crossover, adaptive operator selection, multiple selection strategies
- **High Performance**: Optimized data structures, SIMD support, and parallel evaluation
- **Extensible Architecture**: Template-based design with problem adapters and operator interfaces

## Quick Start

### Requirements

- C++23 compatible compiler (GCC 12+, Clang 15+, MSVC 19.35+)
- CMake 3.22+
- Optional: oneTBB, OpenMP for parallelization

### Building

```bash
git clone https://github.com/yourusername/evolab.git
cd evolab
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

### Basic Usage

```cpp
#include <evolab/evolab.hpp>
using namespace evolab;

// Create a TSP problem
auto tsp = problems::create_random_tsp(100);

// Configure and run genetic algorithm
auto ga = factory::make_tsp_ga_basic();
auto result = ga.run(tsp, core::GAConfig{
    .population_size = 256,
    .max_generations = 1000,
    .crossover_prob = 0.9,
    .mutation_prob = 0.1
});

std::cout << "Best solution: " << result.best_fitness.value << std::endl;
```

## Architecture

EvoLab uses a modular, template-based architecture:

- **Core**: Fundamental concepts, GA implementation, fitness evaluation
- **Problems**: TSP, VRP, QAP adapters with specialized operators
- **Operators**: Selection (tournament, roulette), crossover (PMX, OX, EAX), mutation
- **Local Search**: 2-opt, 3-opt, Lin-Kernighan for memetic algorithms
- **Parallel**: Thread-pool evaluation, Island Model for distributed evolution

## Performance

Target performance on modern hardware:
- 2-opt evaluations: >500M ops/sec (with candidate lists)
- TSP instances: pr2392 (2392 cities) solved to near-optimality in minutes
- Large scale: usa13509 (13509 cities) competitive results in under an hour

## Contributing

EvoLab welcomes contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Research

If you use EvoLab in research, please cite:

```bibtex
@software{evolab2025,
  title={EvoLab: A Modern C++23 Metaheuristics Platform},
  author={Your Name},
  year={2025},
  url={https://github.com/yourusername/evolab}
}
```

## License

[Apache-2.0](LICENSE) - See LICENSE file for details.

## Status

🚧 **Early Development** - Core features implemented, API subject to change.

### Roadmap

- [x] Core GA with C++23 concepts
- [x] TSP problem with basic operators
- [x] 2-opt local search
- [ ] Advanced crossover (EAX)
- [ ] Parallel evaluation
- [ ] Island Model
- [ ] VRP and QAP problems
- [ ] Comprehensive benchmarks
- [ ] Python bindings