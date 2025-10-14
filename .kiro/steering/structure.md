# Project Structure - EvoLab

## Root Directory Organization

```
evolab/
├── .claude/              # Claude Code configuration and commands
│   └── commands/         # Custom slash commands for development
├── .github/              # GitHub configuration
│   └── PULL_REQUEST_TEMPLATE.md
├── .kiro/                # Spec-driven development
│   ├── specs/            # Feature specifications
│   └── steering/         # Project steering documents
├── .serena/              # Serena MCP server memories
│   └── memories/         # Cached project knowledge
├── apps/                 # Executable applications
│   ├── CMakeLists.txt
│   └── tsp_main.cpp      # Main TSP solver application
├── benchmarks/           # Performance benchmarks
│   └── CMakeLists.txt
├── build/                # Build output directory (generated)
├── configs/              # Configuration files
│   ├── README.md
│   └── *.toml            # TOML config files
├── data/                 # Data files
│   └── tsplib/           # TSPLIB benchmark instances
├── include/              # Header files (main library)
│   └── evolab/           # Library headers
├── scripts/              # Utility scripts
├── src/                  # Implementation files (if needed)
├── tests/                # Unit tests
│   ├── CMakeLists.txt
│   └── test_*.cpp        # Test files
├── CMakeLists.txt        # Root build configuration
├── CLAUDE.md             # Claude Code instructions
├── CONTRIBUTING.md       # Contribution guidelines
├── README.md             # Project documentation
├── lefthook.yml          # Git hooks configuration
├── .clang-format         # Code formatting rules
└── .clang-tidy           # Static analysis configuration
```

## Subdirectory Structures

### Include Directory (`include/evolab/`)
```
include/evolab/
├── evolab.hpp            # Main include file
├── core/                 # Core GA implementation
│   ├── concepts.hpp      # C++ concepts for type constraints
│   ├── fitness.hpp       # Fitness evaluation
│   ├── ga.hpp            # Main GA class
│   ├── individual.hpp    # Individual representation
│   ├── population.hpp    # Population management
│   └── statistics.hpp    # Statistical tracking
├── config/               # Configuration system
│   ├── config.hpp        # Config structures
│   └── parser.hpp        # TOML parser integration
├── diversity/            # Diversity management
│   └── diversity.hpp     # Diversity metrics
├── io/                   # Input/Output utilities
│   └── tsplib_parser.hpp # TSPLIB file parser
├── local_search/         # Local search algorithms
│   ├── two_opt.hpp       # 2-opt implementation
│   ├── three_opt.hpp     # 3-opt implementation
│   └── lk.hpp            # Lin-Kernighan heuristic
├── operators/            # Genetic operators
│   ├── crossover/        # Crossover operators
│   │   ├── pmx.hpp       # Partially Mapped Crossover
│   │   ├── ox.hpp        # Order Crossover
│   │   ├── cx.hpp        # Cycle Crossover
│   │   └── eax.hpp       # Edge Assembly Crossover
│   ├── mutation/         # Mutation operators
│   │   ├── swap.hpp      # Swap mutation
│   │   ├── insert.hpp    # Insert mutation
│   │   └── inversion.hpp # Inversion mutation
│   └── selection/        # Selection operators
│       ├── tournament.hpp # Tournament selection
│       └── roulette.hpp  # Roulette wheel selection
├── parallel/             # Parallel processing
│   ├── thread_pool.hpp   # Thread pool implementation
│   └── island_model.hpp  # Island Model GA
├── problems/             # Problem-specific code
│   ├── tsp/              # TSP implementation
│   │   ├── tsp.hpp       # TSP problem definition
│   │   ├── instance.hpp  # TSP instance representation
│   │   └── distance.hpp  # Distance calculations
│   ├── vrp/              # VRP (planned)
│   └── qap/              # QAP (planned)
├── schedulers/           # Adaptive operator selection
│   └── ucb_scheduler.hpp # UCB-based scheduler
└── utils/                # Utility functions
    ├── random.hpp        # Random number generation
    ├── timer.hpp         # Performance timing
    └── logger.hpp        # Logging utilities
```

### Test Directory (`tests/`)
```
tests/
├── CMakeLists.txt        # Test build configuration
├── test_helper.hpp       # Common test utilities
├── test_core.cpp         # Core functionality tests
├── test_operators.cpp    # Operator tests
├── test_tsp.cpp          # TSP-specific tests
├── test_tsplib.cpp       # TSPLIB parser tests
├── test_config.cpp       # Configuration tests
├── test_config_integration.cpp  # Integration tests
└── test_schedulers.cpp   # Scheduler tests
```

### Configuration Directory (`configs/`)
```
configs/
├── README.md             # Configuration documentation
├── tsp_default.toml      # Default TSP configuration
├── tsp_large.toml        # Large instance configuration
├── tsp_memetic.toml      # Memetic algorithm config
└── experiments/          # Experimental configs
```

## Code Organization Patterns

### Header-Only Library Pattern
- All implementation in headers for template instantiation
- Use of `inline` and `constexpr` for ODR compliance
- Template specializations in separate headers

### Namespace Organization
```cpp
namespace evolab {
    namespace core { }      // Core GA functionality
    namespace operators { } // Genetic operators
    namespace problems {    // Problem-specific
        namespace tsp { }
    }
    namespace utils { }     // Utilities
}
```

### Template Organization
- Concepts defined before templates
- SFINAE replaced with requires clauses
- Explicit instantiation for common types

### Include Guards & Modules
```cpp
#pragma once  // Primary include guard

// Future module support
// export module evolab.core;
```

## File Naming Conventions

### Source Files
- **Headers**: `snake_case.hpp` (e.g., `two_opt.hpp`)
- **Implementation**: `snake_case.cpp` (if needed)
- **Tests**: `test_*.cpp` (e.g., `test_operators.cpp`)
- **Benchmarks**: `bench_*.cpp` (e.g., `bench_crossover.cpp`)

### Configuration Files
- **TOML configs**: `problem_variant.toml` (e.g., `tsp_large.toml`)
- **Build files**: `CMakeLists.txt` (exact case)
- **Documentation**: `UPPERCASE.md` for root, `lowercase.md` elsewhere

### Data Files
- **TSPLIB instances**: Original naming preserved (e.g., `pr2392.tsp`)
- **Solution files**: `instance_method.tour` (e.g., `pr2392_ga.tour`)
- **Results**: `experiment_date.json` (e.g., `tsp_20250101.json`)

## Import Organization

### Standard Headers First
```cpp
// Standard library
#include <vector>
#include <algorithm>
#include <concepts>

// Third-party libraries
#include <toml.hpp>

// Project headers
#include <evolab/core/ga.hpp>
```

### Header Dependencies
```cpp
// In crossover/pmx.hpp
#include <evolab/core/concepts.hpp>  // Required concepts
#include <evolab/core/individual.hpp> // Type definitions
#include <evolab/utils/random.hpp>    // Utilities
```

### Forward Declarations
```cpp
// Prefer forward declarations in headers
namespace evolab::core {
    template<typename T>
    class Population;
}
```

## Key Architectural Principles

### SOLID Principles
- **Single Responsibility**: Each operator/component has one job
- **Open/Closed**: Extensible via templates, closed for modification
- **Liskov Substitution**: All operators follow common interfaces
- **Interface Segregation**: Minimal concept requirements
- **Dependency Inversion**: Depend on concepts, not concrete types

### Design Patterns
- **Strategy Pattern**: Interchangeable operators via templates
- **Factory Pattern**: Problem and GA creation helpers
- **Builder Pattern**: Configuration construction
- **Observer Pattern**: Statistics collection (planned)

### Performance Principles
- **Zero-Cost Abstractions**: Templates over virtual functions
- **Cache Efficiency**: Data layout optimization
- **SIMD Friendly**: Aligned data for vectorization
- **Memory Pools**: Reusable allocations (planned)

### Research Principles
- **Reproducibility**: Seed management throughout
- **Modularity**: Easy to swap components
- **Extensibility**: New problems without core changes
- **Benchmarking**: Performance tracking built-in

### Code Quality Standards
- **Type Safety**: Strong typing with concepts
- **Error Handling**: std::expected over exceptions
- **RAII**: Resource management via constructors/destructors
- **Const Correctness**: Liberal use of const/constexpr
- **Documentation**: Doxygen comments on public APIs