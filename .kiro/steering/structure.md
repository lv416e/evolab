# Project Structure

## Root Directory Organization
```
evolab/
├── .kiro/              # Kiro spec-driven development
│   ├── steering/       # Project context documents
│   └── specs/          # Feature specifications
├── apps/               # Example applications
├── benchmarks/         # Performance benchmarking code
├── build/              # CMake build output (gitignored)
├── cmake/              # CMake configuration files
├── configs/            # Configuration files for experiments
├── data/               # Test data and problem instances
├── include/evolab/     # Header-only library source
├── scripts/            # Utility scripts
├── src/                # Implementation files (if needed)
└── tests/              # Unit tests
```

## Subdirectory Structures

### Library Headers (`include/evolab/`)
```
include/evolab/
├── core/               # Core GA framework
│   ├── concepts.hpp    # C++23 concepts for type constraints
│   └── ga.hpp          # Main genetic algorithm implementation
├── diversity/          # Diversity maintenance strategies
├── io/                 # Input/output utilities
├── local_search/       # Local search methods
│   └── two_opt.hpp     # 2-opt local search
├── operators/          # Genetic operators
│   ├── crossover.hpp   # Crossover operators (PMX, OX, EAX)
│   ├── mutation.hpp    # Mutation operators
│   └── selection.hpp   # Selection operators (tournament, roulette)
├── parallel/           # Parallelization utilities
├── problems/           # Problem-specific adapters
│   └── tsp.hpp         # TSP problem implementation
├── schedulers/         # Operator scheduling strategies
├── utils/              # General utilities
└── evolab.hpp          # Main include file
```

### Test Structure (`tests/`)
```
tests/
├── CMakeLists.txt      # Test configuration
├── test_core.cpp       # Core framework tests
├── test_operators.cpp  # Operator tests
└── test_tsp.cpp        # TSP-specific tests
```

### Application Examples (`apps/`)
```
apps/
├── CMakeLists.txt      # Apps configuration
└── tsp_main.cpp        # TSP solver application
```

## Code Organization Patterns

### Header-Only Template Library
- All implementation in headers (.hpp files)
- Template definitions in same file as declarations
- Heavy use of concepts for compile-time type checking
- Policy-based design for customization

### Namespace Structure
```cpp
namespace evolab {
    namespace core { }      // Core GA components
    namespace operators { } // Genetic operators
    namespace problems { }  // Problem adapters
    namespace utils { }     // Utilities
}
```

### Class Hierarchy
- Abstract base classes with virtual interfaces (rare)
- Prefer composition over inheritance
- Template-based static polymorphism
- Concepts for type requirements

## File Naming Conventions

### Source Files
- **Headers**: `snake_case.hpp` (e.g., `two_opt.hpp`)
- **Implementation**: `snake_case.cpp` (only for non-template code)
- **Tests**: `test_<module>.cpp` (e.g., `test_operators.cpp`)
- **Apps**: `<app>_main.cpp` (e.g., `tsp_main.cpp`)

### Other Files
- **CMake**: `CMakeLists.txt`, `<Module>Config.cmake.in`
- **Config**: `.clang-format`, `.clang-tidy`, `lefthook.yml`
- **Documentation**: `README.md`, `CONTRIBUTING.md`, `CLAUDE.md`

## Import Organization

### Standard Order
1. Standard library headers
2. Third-party library headers
3. EvoLab public headers
4. EvoLab internal headers
5. Local/private headers

### Example
```cpp
#include <vector>           // Standard library
#include <algorithm>
#include <tbb/parallel_for.h>  // Third-party
#include <evolab/core/ga.hpp>  // EvoLab public
#include "internal_utils.hpp"  // Local/private
```

## Key Architectural Principles

### Performance First
- Zero-cost abstractions through templates
- Compile-time optimization over runtime flexibility
- Cache-friendly data structures
- SIMD and parallelization where beneficial

### Type Safety
- C++23 concepts for compile-time type checking
- Strong typing with minimal implicit conversions
- RAII for resource management
- No raw pointers in public APIs

### Modularity
- Clear separation between core framework and problem-specific code
- Pluggable operators and local search methods
- Minimal coupling between components
- Clean interfaces with well-defined contracts

### Testing Strategy
- Unit tests for all public interfaces
- Property-based testing for operators
- Benchmark tests for performance regression
- Integration tests for complete algorithms

### Documentation
- Doxygen comments for all public APIs
- README files in major directories
- Example code in apps/ directory
- Comprehensive user guide (planned)