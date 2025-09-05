# EvoLab - C++23 Metaheuristics Research Platform

## Project Overview

EvoLab is a modern C++23 metaheuristics research platform designed for solving complex combinatorial optimization problems with a focus on performance, reproducibility, and extensibility.

## Core Features

### Implemented
- **Core GA Framework**: Template-based genetic algorithm with policy-based design
- **TSP Problem Adapter**: Efficient TSP solver with cache-friendly distance matrix
- **2-opt Local Search**: Delta evaluation for O(1) move cost computation
- **Professional Build System**: CMake with proper installation and test support
- **Header-only Design**: Easy integration as a library

### Target Implementation (Priority Order)

#### Phase 1: Advanced Operators
1. **EAX Crossover**: Edge Assembly Crossover for TSP/VRP
2. **Candidate Lists**: k-nearest neighbor preprocessing for faster local search
3. **PMX/OX Crossovers**: Standard permutation crossovers

#### Phase 2: Adaptive Control
1. **Multi-Armed Bandit Scheduler**: Thompson/UCB for operator selection
2. **Diversity Maintenance**: Crowding/niching strategies
3. **Restart Strategies**: Avoid premature convergence

#### Phase 3: Parallelization
1. **TBB Integration**: Parallel fitness evaluation
2. **Island Model**: Multiple populations with migration
3. **SIMD Optimizations**: Vectorized distance calculations

#### Phase 4: Extended Problems
1. **VRP Adapter**: Vehicle Routing Problem
2. **QAP Adapter**: Quadratic Assignment Problem
3. **Constraint Repair**: Problem-specific repair operators

## Performance Targets

- **2-opt evaluations**: >500M ops/sec with candidate lists
- **TSP pr2392**: Near-optimal solutions in minutes
- **TSP usa13509**: Competitive results in <1 hour with LK

## Development Principles

1. **Performance First**: Cache-friendly data structures, SIMD where applicable
2. **Reproducibility**: Seed management, complete logging, configuration tracking
3. **Extensibility**: Template-based design, problem adapter pattern
4. **Quality**: Comprehensive testing, CI/CD, code formatting standards

## Technical Stack

- **Language**: C++23 with concepts
- **Build**: CMake 3.22+
- **Parallelization**: TBB (optional), OpenMP (optional)
- **Testing**: Google Test
- **Dependencies**: Minimal (CLI11, fmt, nlohmann/json)
- **License**: Apache-2.0