#pragma once

#ifdef EVOLAB_HAVE_TBB

#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include <evolab/core/concepts.hpp>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>

namespace evolab::parallel {

/// Thread-safe parallel executor using Intel TBB for deterministic fitness evaluation
///
/// This executor provides reproducible parallel fitness evaluation with identical results
/// across multiple runs. The implementation uses static_partitioner to ensure deterministic
/// work distribution, optimized for performance-critical scientific computing applications.
///
/// Deterministic Execution Architecture:
/// - **Static partitioning**: Consistent work distribution across all executions
/// - **Thread-assignment independence**: Identical results regardless of thread-to-genome mapping
/// - **Cross-run consistency**: Identical results for identical inputs and build settings
///   regardless of worker-thread count or scheduling (assuming `evaluate()` is deterministic)
/// - **Performance optimization**: Minimal overhead design for computation-intensive algorithms
///
/// Key features:
/// - Deterministic work distribution ensures reproducible results across all runs
/// - Static range partitioning for deterministic chunk boundaries; actual worker-thread
///   assignment may vary across runs and platforms
/// - Exception-safe RAII design with proper resource management
/// - Optimized for deterministic algorithms (e.g., TSP, graph algorithms)
///
/// Design Choice: Prioritizes performance and simplicity for deterministic algorithms.
/// Trade-offs: Specialized for deterministic evaluation, requires separate design for stochastic
/// algorithms.
class TBBExecutor {
  public:
    /// Constructs a thread-safe parallel executor for deterministic evaluation
    ///
    /// The executor employs a stateless design pattern following C++23 best practices
    /// for concurrent programming. Each parallel_evaluate() call operates independently
    /// with per-call state management, eliminating shared mutable state and ensuring
    /// inherent thread safety without synchronization overhead.
    ///
    /// Future extensions for stochastic algorithms will add seed parameter with
    /// default value, maintaining backward compatibility per YAGNI principle.
    TBBExecutor() = default;

    /// Performs thread-safe parallel fitness evaluation using modern C++23 API design
    ///
    /// This method implements C++23 const-correctness principles with std::span parameter
    /// following C++ Core Guidelines F.16. The span-based interface provides enhanced
    /// flexibility, accepting any contiguous container while maintaining zero overhead.
    ///
    /// Modern API benefits:
    /// - **Universal Interface**: Accepts std::vector, std::array, C-arrays via std::span
    /// - **Enhanced Safety**: Bounds-aware view prevents common pointer+size errors
    /// - **Const-Correctness**: span<const T> clearly expresses read-only intent
    /// - **Zero Cost**: Direct conversion from containers with no performance overhead
    /// - **Subrange Support**: Enables efficient partial population processing via .subspan()
    ///
    /// Technical implementation:
    /// - **Thread Safety**: No shared mutable state prevents data races by design
    /// - **Reproducible Determinism**: static_partitioner ensures identical work distribution
    /// - **Scientific Computing Ready**: Guarantees reproducible results across runs
    ///   (bit-identical when evaluate() is deterministic and compiled with consistent FP settings)
    ///
    /// Exception Safety:
    /// If `problem.evaluate()` throws for any genome, the exception is propagated out of this
    /// function and no result is returned. Some elements of an internal buffer may have been
    /// written before the throw, but that buffer is destroyed during unwinding.
    /// For a strong guarantee, evaluate into a temporary buffer and commit on success.
    ///
    /// @tparam P Problem type satisfying evolab::core::Problem.
    /// @param problem Problem instance providing fitness evaluation function.
    ///
    /// @warning The problem's evaluate() method MUST be thread-safe for concurrent
    ///          execution on const objects. Ensure no mutable state modifications
    ///          occur without proper synchronization, as multiple threads will call
    ///          evaluate() simultaneously.
    /// @param population Contiguous sequence of genomes to evaluate in parallel
    /// @return Vector of fitness values corresponding to input population order
    /// @throws std::exception Propagates exceptions from `problem.evaluate()`; no vector is
    /// returned on throw. Also propagates TBB errors encountered during parallel execution.
    template <evolab::core::Problem P>
    [[nodiscard]] std::vector<evolab::core::Fitness>
    parallel_evaluate(const P& problem, std::span<const typename P::GenomeT> population) const {
        using Fitness = evolab::core::Fitness;

        // Guard against empty population for defensive programming
        // While TBB blocked_range handles empty ranges correctly, this explicit check
        // provides clear semantics and prevents potential edge cases in future TBB versions.
        if (population.empty()) {
            return std::vector<Fitness>{};
        }

        std::vector<Fitness> fitnesses(population.size());

        // Execute parallel fitness evaluation using TBB's deterministic work distribution
        // static_partitioner ensures reproducible chunk-to-thread mapping for scientific computing
        // Explicit capture follows C++23 best practices for maintainability and safety
        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(0, population.size()),
            [&problem, &fitnesses, &population](const tbb::blocked_range<std::size_t>& range) {
                // Process assigned range with thread-safe, cache-efficient evaluation
                // Each thread writes to distinct indices, preventing data races
                for (std::size_t i = range.begin(); i != range.end(); ++i) {
                    // Direct fitness evaluation for deterministic algorithms
                    // Optimized for performance-critical scientific computing
                    fitnesses[i] = problem.evaluate(population[i]);
                }
            },
            tbb::static_partitioner{});

        return fitnesses;
    }

    // Note: Stateless design following YAGNI principle
    // Eliminates unused complexity for current deterministic algorithms.
    // Future extensions for stochastic algorithms will add necessary state
    // without breaking existing code (non-breaking API evolution)

  private:
    // See docs/technical-decisions/{tbb-executor-*.md} for detailed rationale
    // (thread safety, determinism, partitioner choice, and future stochastic design).
};

} // namespace evolab::parallel

#else

// Dependency resolution design: Use #error for immediate, clear feedback on missing TBB
//
// Technical decision rationale:
// - Configuration dependency issue requires early compile-time detection
// - #error preprocessor provides immediate feedback with detailed resolution steps
// - Alternative approaches (deleted constructors, static_assert) delay error to instantiation
// - Standard-compliant approach avoiding static_assert(false) in non-template contexts
//
// Reference: C++23 best practices for conditional compilation and dependency management
#error "\n" \
       "====================================================================\n" \
       " TBB (Threading Building Blocks) is required for parallel execution\n" \
       " but was not found during configuration.\n" \
       "\n" \
       " Resolution options:\n" \
       "   1. Install Intel TBB development package:\n" \
       "      - Ubuntu/Debian: apt install libtbb-dev\n" \
       "      - RHEL/CentOS:   yum install tbb-devel\n" \
       "      - macOS:         brew install tbb\n" \
       "      - Windows:       vcpkg install tbb\n" \
       "      - Conda (conda-forge): conda install -c conda-forge tbb\n" \
       "        (Use the conda-forge channel for the most up-to-date oneTBB packages.)\n" \
       "\n" \
       "   2. Disable parallel execution:\n" \
       "      cmake -DEVOLAB_USE_TBB=OFF .\n" \
       "\n" \
       " For more information, see project documentation\n" \
       " (docs/technical-decisions/ and README.md) and Intel oneTBB docs:\n" \
       "   https://oneapi-src.github.io/oneTBB/\n" \
       "===================================================================="

#endif // EVOLAB_HAVE_TBB
