# EvoLab Design Specification

## System Architecture

### Layered Architecture

```
┌─────────────────────────────────────────────────┐
│                Applications                      │
│         (evolab-tsp, evolab-vrp, etc.)          │
├─────────────────────────────────────────────────┤
│                    CLI/Config                    │
│        (Argument parsing, TOML/JSON config)      │
├─────────────────────────────────────────────────┤
│                 Problem Adapters                 │
│           (TSP, VRP, QAP, MaxCut)               │
├─────────────────────────────────────────────────┤
│              Algorithm Framework                 │
│     (GA, Memetic, Island Model, Schedulers)     │
├─────────────────────────────────────────────────┤
│                   Operators                      │
│ (Selection, Crossover, Mutation, Local Search)  │
├─────────────────────────────────────────────────┤
│                     Core                         │
│   (Concepts, Population, RNG, Evaluator)        │
├─────────────────────────────────────────────────┤
│                    Utils                         │
│  (Memory, SIMD, Candidate Lists, Statistics)    │
└─────────────────────────────────────────────────┘
```

## Core Design Patterns

### Policy-Based Design

```cpp
template<class Selection,
         class Crossover,
         class Mutation,
         class LocalSearch>
class GA {
    // Compile-time polymorphism for zero-overhead abstraction
};
```

### Problem Adapter Pattern

```cpp
template<class T>
concept Problem = requires(const T& p, const typename T::GenomeT& g) {
    typename T::GenomeT;
    typename T::Gene;
    { p.evaluate(g) } -> std::convertible_to<double>;
    { p.repair(g) } -> std::same_as<typename T::GenomeT>;
};
```

### CRTP for Static Polymorphism

```cpp
template<typename Derived>
class CrossoverBase {
    auto operator()(const GenomeT& p1, const GenomeT& p2) {
        return static_cast<Derived*>(this)->crossover_impl(p1, p2);
    }
};
```

## Data Structure Design

### Distance Matrix (TSP)

```cpp
class DistanceMatrix {
    std::vector<float> data;  // Row-major, cache-friendly
    size_t n;

    float operator()(size_t i, size_t j) const noexcept {
        return data[i * n + j];  // Single multiplication
    }
};
```

**Rationale**:
- Contiguous memory for cache efficiency
- Float vs double: 2x more data in cache
- Row-major for sequential access patterns

### Population Storage

```cpp
template<typename GenomeT>
class Population {
    std::vector<GenomeT> genomes;
    std::vector<double> fitness;
    std::vector<size_t> indices;  // For sorting without moving data
};
```

**Rationale**:
- Structure of Arrays (SoA) for better vectorization
- Separate fitness for parallel evaluation
- Index indirection for efficient sorting

### Candidate Lists

```cpp
struct CandidateList {
    static constexpr size_t K = 40;
    std::vector<std::array<uint32_t, K>> neighbors;

    // Pre-computed k-nearest for each node
    void build(const DistanceMatrix& dist);
};
```

**Rationale**:
- Fixed-size arrays for predictable memory layout
- uint32_t sufficient for most problems (<4B nodes)
- K=40 based on empirical studies

## Algorithm Implementations

### EAX Crossover Design

```cpp
class EAXCrossover {
    struct Edge { uint32_t from, to; };
    struct ABCycle {
        std::vector<std::vector<uint32_t>> cycles;
        void construct(const GenomeT& p1, const GenomeT& p2);
    };

    GenomeT crossover_impl(const GenomeT& p1, const GenomeT& p2) {
        // 1. Build edge sets
        // 2. Construct AB-cycles
        // 3. Select E-set
        // 4. Generate intermediate solution
        // 5. Apply repair if needed
    }
};
```

**Key Optimizations**:
- Edge hash table with linear probing
- Cycle detection with DFS
- Greedy repair for broken tours

### 2-opt with Delta Evaluation

```cpp
class TwoOpt {
    double delta_cost(const TSP& problem,
                      const GenomeT& tour,
                      size_t i, size_t j) {
        // Only compute difference, not full tour cost
        auto& dist = problem.dist;
        return dist(tour[i], tour[j]) +
               dist(tour[i+1], tour[j+1]) -
               dist(tour[i], tour[i+1]) -
               dist(tour[j], tour[j+1]);
    }
};
```

### Multi-Armed Bandit Scheduler

```cpp
class ThompsonSampling {
    struct OperatorStats {
        double alpha = 1.0;  // Success count
        double beta = 1.0;   // Failure count
    };

    std::vector<OperatorStats> stats;

    size_t select_operator(RNG& rng) {
        // Sample from Beta distribution for each operator
        // Select operator with highest sample
    }
};
```

## Parallelization Strategy

### Thread-Safe Random Number Generation

```cpp
class ThreadSafeRNG {
    static thread_local std::mt19937_64 engine;
    static std::atomic<uint64_t> global_seed;

    static void initialize() {
        engine.seed(global_seed.fetch_add(1));
    }
};
```

### Parallel Evaluation with TBB

```cpp
template<typename Problem>
void parallel_evaluate(const Problem& problem,
                      Population& pop) {
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, pop.size()),
        [&](const tbb::blocked_range<size_t>& r) {
            for (size_t i = r.begin(); i != r.end(); ++i) {
                pop.fitness[i] = problem.evaluate(pop.genomes[i]);
            }
        }
    );
}
```

### Island Model Architecture

```cpp
class IslandGA {
    struct Island {
        Population population;
        GAConfig config;
        std::thread worker;
    };

    std::vector<Island> islands;
    std::queue<MigrationPacket> migration_queue;

    void migrate() {
        // Ring topology: island i sends to (i+1) % n
        // Best + random individuals
    }
};
```

## Memory Management

### Pre-allocation Strategy

```cpp
class MemoryPool {
    std::vector<std::byte> buffer;
    size_t offset = 0;

    template<typename T>
    T* allocate(size_t count) {
        // Bump allocator for temporary objects
    }

    void reset() { offset = 0; }
};
```

### Small Vector Optimization

```cpp
template<typename T, size_t N>
class SmallVector {
    union {
        T stack_buffer[N];
        T* heap_buffer;
    };
    size_t size_;
    size_t capacity_;
    bool on_heap;
};
```

## Configuration System

### TOML Configuration Schema

```toml
[ga]
population_size = 256
elite_rate = 0.02
max_generations = 5000

[operators]
crossover = { type = "EAX", probability = 0.9 }
mutation = { type = "inversion", probability = 0.2 }
local_search = { type = "2-opt", iterations = 100 }

[scheduler]
type = "thompson"
operators = ["EAX", "PMX", "OX"]
window_size = 50

[parallel]
threads = 0  # 0 = auto-detect
island_count = 8
migration_interval = 50
```

## Error Handling

### Result Type Pattern

```cpp
template<typename T, typename E>
class Result {
    std::variant<T, E> data;
public:
    bool is_ok() const;
    T& value();
    E& error();
};
```

### Validation Strategy

```cpp
class Validator {
    static Result<void, std::string>
    validate_permutation(const std::vector<int>& tour) {
        // Check for duplicates and missing values
    }

    static Result<void, std::string>
    validate_config(const GAConfig& cfg) {
        // Range checks, consistency checks
    }
};
```

## Performance Optimizations

### Compiler Hints

```cpp
#define EVOLAB_LIKELY(x)   __builtin_expect(!!(x), 1)
#define EVOLAB_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define EVOLAB_RESTRICT    __restrict__
#define EVOLAB_ASSUME_ALIGNED(ptr, align) \
    __builtin_assume_aligned(ptr, align)
```

### SIMD Vectorization

```cpp
void compute_distances_simd(const float* EVOLAB_RESTRICT x,
                           const float* EVOLAB_RESTRICT y,
                           float* EVOLAB_RESTRICT out,
                           size_t n) {
    // AVX2/AVX-512 implementation
    for (size_t i = 0; i < n; i += 8) {
        __m256 vx = _mm256_load_ps(&x[i]);
        __m256 vy = _mm256_load_ps(&y[i]);
        __m256 diff = _mm256_sub_ps(vx, vy);
        __m256 sq = _mm256_mul_ps(diff, diff);
        _mm256_store_ps(&out[i], sq);
    }
}
```

## Testing Strategy

### Unit Test Categories

1. **Operator Correctness**: Permutation preservation, constraint satisfaction
2. **Algorithm Convergence**: Known optima for small problems
3. **Performance Regression**: Benchmark suite with timing thresholds
4. **Reproducibility**: Seed-based determinism across platforms

### Integration Test Scenarios

1. **End-to-end TSP solving**: TSPLIB instances
2. **Configuration validation**: Invalid configs should fail gracefully
3. **Parallel correctness**: Same results with different thread counts
4. **Memory leak detection**: Valgrind/AddressSanitizer clean

## Future Extensions

### GPU Acceleration
- CUDA/HIP for massive parallel evaluation
- GPU-friendly data structures

### Distributed Computing
- MPI for cluster deployment
- Checkpoint/restart for long runs

### Machine Learning Integration
- Learned operator selection
- Neural network guided local search

## Design Decisions Log

1. **Header-only for core**: Easier integration, better inlining
2. **Float vs Double**: Float for distance matrix (2x cache efficiency)
3. **C++23 concepts**: Better error messages, cleaner interfaces
4. **TBB over OpenMP**: Better task scheduling, composability
5. **TOML over YAML**: Simpler parser, sufficient features