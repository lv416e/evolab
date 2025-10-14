# Product Overview - EvoLab

## Product Overview

EvoLab is a modern C++23 metaheuristics research platform designed for solving complex optimization problems. It provides a high-performance genetic algorithm framework for research-grade optimization, with a particular focus on combinatorial optimization problems.

## Core Features

- **High-Performance Genetic Algorithms**: Template-based GA implementation with concepts and compile-time optimization
- **Problem Adapters**: Built-in support for TSP, with extensible architecture for VRP and QAP
- **Hybrid Algorithms**: Memetic GA combining genetic operators with local search techniques
- **Advanced Crossover Operators**: PMX, OX, CX, and EAX crossover implementations
- **Local Search Integration**: 2-opt, 3-opt, and Lin-Kernighan heuristics
- **Adaptive Operator Selection**: UCB-based scheduler for dynamic operator selection
- **TSPLIB Support**: Full parser for standard TSP benchmark instances
- **Configuration System**: TOML-based configuration with hierarchical overrides
- **Research-Grade Features**: Full reproducibility with seed management, comprehensive logging, statistical analysis
- **Parallel Evaluation**: Thread-pool based parallel fitness evaluation (optional with TBB/OpenMP)

## Target Use Case

### Primary Use Cases
- **Academic Research**: Benchmarking new metaheuristic algorithms against state-of-the-art
- **Algorithm Development**: Testing novel genetic operators and hybrid approaches
- **Optimization Problems**: Solving TSP, VRP, QAP and similar combinatorial problems
- **Performance Analysis**: Evaluating algorithm performance with comprehensive metrics

### Specific Scenarios
- Researchers developing new crossover operators for permutation-based problems
- Benchmarking hybrid genetic algorithms against pure GA approaches
- Solving large-scale TSP instances (thousands of cities) with near-optimal results
- Comparing different selection strategies and population management techniques
- Testing the impact of local search integration on solution quality

## Key Value Proposition

### Unique Benefits
- **Modern C++23 Architecture**: Leverages concepts, ranges, and coroutines for clean, efficient code
- **Research-First Design**: Built specifically for academic research with full reproducibility
- **Template-Based Flexibility**: Zero-cost abstractions allow custom problem types without performance penalty
- **Production-Ready Performance**: Optimized for real-world problem sizes (>10,000 cities)
- **Comprehensive Operator Library**: State-of-the-art operators implemented and optimized

### Competitive Advantages
- **Speed**: 2-opt evaluations >500M ops/sec with candidate lists
- **Scalability**: Handles usa13509 (13,509 cities) competitively
- **Extensibility**: Clean interfaces for adding new problems and operators
- **Integration**: Native support for standard benchmark formats (TSPLIB)
- **Modern Tooling**: Git hooks, automated formatting, comprehensive testing

### Research Impact
- Enables rapid prototyping of new metaheuristic algorithms
- Provides reliable baseline implementations for comparison
- Supports reproducible research with seed management
- Facilitates algorithm hybridization experiments
- Offers production-grade performance for real applications

## Success Metrics

- **Performance**: Solve pr2392 to near-optimality in minutes
- **Scale**: Handle TSP instances with >10,000 cities efficiently
- **Quality**: Achieve within 1% of known optimal solutions on benchmarks
- **Adoption**: Used as reference implementation in research papers
- **Extensibility**: New problem types added without core modifications