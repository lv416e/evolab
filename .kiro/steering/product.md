# Product Overview

## Product Description
EvoLab is a modern C++23 metaheuristics research platform designed for solving complex combinatorial optimization problems through evolutionary algorithms. It provides a high-performance, research-grade framework for developing and benchmarking genetic algorithms and memetic algorithms.

## Core Features
- **Research-Grade Genetic Algorithms**: Full reproducibility with seed management, comprehensive logging, and statistical analysis
- **Memetic Algorithm Support**: Hybrid approaches combining genetic operators with local search methods (2-opt, 3-opt, Lin-Kernighan)
- **Advanced Genetic Operators**: EAX crossover, PMX, OX, adaptive operator selection, multiple selection strategies
- **High-Performance Architecture**: Template-based design with SIMD support, optimized data structures, and parallel evaluation
- **Problem Adapters**: Specialized implementations for TSP, VRP, and QAP with problem-specific operators
- **Extensible Framework**: Clean interfaces for custom problems, operators, and local search methods

## Target Use Cases
- **Academic Research**: Benchmarking new metaheuristic algorithms and operators
- **Algorithm Development**: Creating and testing novel evolutionary approaches
- **Optimization Studies**: Solving large-scale combinatorial optimization problems
- **Performance Analysis**: Comparing different GA configurations and operators
- **Education**: Teaching metaheuristics and evolutionary computation concepts

## Key Value Proposition
- **Modern C++23 Implementation**: Leverages latest language features for performance and maintainability
- **Header-Only Design**: Easy integration into existing projects without complex build dependencies
- **Research Quality**: Built with academic rigor, supporting reproducible experiments
- **Production Performance**: Achieves >500M 2-opt evaluations/sec with optimized data structures
- **Comprehensive Testing**: Full test coverage ensuring reliability and correctness
- **Active Development**: Regular updates with new algorithms and optimizations

## Success Metrics
- TSP pr2392 (2392 cities): Near-optimal solutions in minutes
- Large-scale instances (usa13509): Competitive results in under an hour
- Easy integration: Header-only library usable with single include
- Research adoption: Citations and usage in academic papers