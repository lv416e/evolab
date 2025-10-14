# EvoLab Requirements Specification

## Functional Requirements

### Core Genetic Algorithm (GA)

#### FR-GA-001: Population Management
- Support configurable population sizes (16-10000 individuals)
- Implement generational and steady-state replacement strategies
- Maintain elite preservation (configurable elite rate)

#### FR-GA-002: Selection Operators
- Tournament selection with configurable tournament size
- Roulette wheel selection
- Rank-based selection
- Support for selection pressure adjustment

#### FR-GA-003: Crossover Operators (TSP-specific)
- **PMX** (Partially Mapped Crossover): Standard permutation crossover
- **OX** (Order Crossover): Preserve relative order
- **EAX** (Edge Assembly Crossover): High-performance edge-preserving crossover
- Configurable crossover probability (0.0-1.0)

#### FR-GA-004: Mutation Operators
- Swap mutation: Exchange two positions
- Inversion mutation: Reverse subsequence
- Scramble mutation: Randomize subsequence
- Configurable mutation probability with adaptive adjustment

#### FR-GA-005: Termination Criteria
- Maximum generations
- Time limit
- Convergence detection (no improvement for N generations)
- Target fitness value

### Local Search Integration

#### FR-LS-001: 2-opt Optimization
- Delta evaluation for O(1) move cost computation
- First-improvement and best-improvement strategies
- Integration with candidate lists

#### FR-LS-002: 3-opt Optimization
- Efficient implementation with candidate lists
- Limited depth search option

#### FR-LS-003: Lin-Kernighan (LK)
- Adaptive depth control
- Integration with candidate lists
- Hybrid GA-LK memetic algorithm

### Adaptive Control

#### FR-AC-001: Multi-Armed Bandit Scheduler
- Thompson sampling for operator selection
- UCB (Upper Confidence Bound) strategy
- Track operator performance metrics
- Dynamic probability adjustment

#### FR-AC-002: Diversity Maintenance
- Edge overlap ratio calculation
- Kendall tau distance for permutations
- Crowding/niching strategies
- Duplicate detection and elimination

### Problem Adapters

#### FR-PA-001: TSP Adapter
- TSPLIB format parser
- Symmetric and asymmetric TSP support
- Cache-friendly distance matrix storage
- Solution validation

#### FR-PA-002: VRP Adapter (Future)
- Capacity constraints
- Time window constraints
- Multiple depots support
- Repair operators for constraint violations

#### FR-PA-003: QAP Adapter (Future)
- Flow and distance matrices
- Permutation matrix validation
- Problem-specific local search

### Parallelization

#### FR-PP-001: Multi-threaded Evaluation
- TBB/OpenMP integration
- Thread-safe random number generation
- Parallel fitness evaluation
- Load balancing

#### FR-PP-002: Island Model
- Multiple populations with different parameters
- Configurable migration policies
- Migration frequency and size control
- Topology configuration (ring, mesh, complete)

## Non-Functional Requirements

### Performance Requirements

#### NFR-PERF-001: Speed Targets
- 2-opt evaluations: >500M operations/second
- Population evaluation: <10ms for 1000 individuals on TSP-100
- Memory usage: <1GB for TSP-10000

#### NFR-PERF-002: Scalability
- Linear scaling with CPU cores (up to 32 cores)
- Support for problems up to 100,000 nodes
- Efficient memory management with pre-allocation

#### NFR-PERF-003: Optimization Levels
- SIMD vectorization for distance calculations
- Cache-friendly data layouts
- Branch prediction optimization

### Quality Requirements

#### NFR-QUAL-001: Reproducibility
- Deterministic results with fixed seed
- Complete configuration logging
- Build information tracking
- Platform-independent results

#### NFR-QUAL-002: Testing Coverage
- Unit tests for all operators
- Integration tests for GA pipeline
- Performance regression tests
- Permutation invariant validation

#### NFR-QUAL-003: Code Quality
- C++23 standard compliance
- Zero warnings with -Wall -Wextra
- Clang-tidy compliance
- Consistent formatting (clang-format)

### Usability Requirements

#### NFR-USE-001: Configuration
- TOML/JSON configuration files
- Command-line parameter override
- Default configurations for common scenarios
- Configuration validation

#### NFR-USE-002: Logging and Monitoring
- Evolution curve output (CSV)
- Best/average/worst fitness tracking
- Diversity metrics logging
- Real-time progress display

#### NFR-USE-003: Output Formats
- TSPLIB-compatible solution format
- JSON result summary
- Visualization support (PNG/SVG)
- Statistical reports

### Integration Requirements

#### NFR-INT-001: Library Design
- Header-only core components
- CMake integration
- pkg-config support
- Minimal dependencies

#### NFR-INT-002: Python Bindings (Future)
- pybind11 integration
- NumPy array support
- Jupyter notebook compatibility

## Benchmark Requirements

### Standard Benchmarks

#### BM-001: TSPLIB Instances
- eil51, eil76, eil101 (small)
- pcb442, pr1002, pr2392 (medium)
- usa13509, pla33810 (large)

#### BM-002: Performance Metrics
- Solution quality vs. best known
- Time to target quality
- Convergence speed
- Diversity over time

#### BM-003: Statistical Validation
- 30+ independent runs per configuration
- Wilcoxon rank-sum test
- Mean, median, standard deviation
- Success rate to target quality

## Constraints

### Technical Constraints
- C++23 compiler required (GCC 12+, Clang 15+, MSVC 2022+)
- CMake 3.22+ for build system
- 64-bit architecture assumed

### Resource Constraints
- Memory limit: 16GB for largest problems
- Time limit: 1 hour for competition instances
- Storage: <100MB for compiled binary

## Success Criteria

1. **Competitive Performance**: Within 1% of best-known solutions for standard TSPLIB
2. **Speed**: 10x faster than reference GA implementations
3. **Scalability**: Linear speedup up to 16 cores
4. **Reliability**: 100% reproducibility with fixed seeds
5. **Usability**: <5 minutes from download to first run