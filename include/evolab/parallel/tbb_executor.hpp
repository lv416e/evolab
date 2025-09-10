#pragma once

#ifdef EVOLAB_HAVE_TBB

#include <cstdint>
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
/// - **Cross-execution consistency**: Same results across different systems, TBB versions, thread
/// counts
/// - **Performance optimization**: Minimal overhead design for computation-intensive algorithms
///
/// Key features:
/// - Deterministic work distribution ensures reproducible results across all runs
/// - Static range partitioning for consistent chunk-to-thread mapping
/// - Exception-safe RAII design with proper resource management
/// - Optimized for deterministic algorithms (e.g., TSP, graph algorithms)
///
/// Design Choice: Prioritizes performance and simplicity for deterministic algorithms.
/// Trade-offs: Specialized for deterministic evaluation, requires separate design for stochastic
/// algorithms.
class TBBExecutor {
  private:
    // Immutable after construction - enables const-correctness and thread safety
    const std::uint64_t base_seed_;

  public:
    /// Constructs a thread-safe parallel executor with deterministic seeding
    ///
    /// The executor employs a stateless design pattern following C++23 best practices
    /// for concurrent programming. Each parallel_evaluate() call operates independently
    /// with per-call state management, eliminating shared mutable state and ensuring
    /// inherent thread safety without synchronization overhead.
    ///
    /// @param seed Base seed maintained for API compatibility and potential future extensions
    explicit TBBExecutor(std::uint64_t seed = 1) : base_seed_(seed) {}

    /// Performs thread-safe parallel fitness evaluation using stateless design
    ///
    /// This method implements C++23 const-correctness principles by declaring the
    /// operation as logically read-only. The stateless design eliminates data races
    /// through per-call state management with deterministic work distribution.
    ///
    /// Key design benefits:
    /// - **Thread Safety**: No shared mutable state prevents data races by design
    /// - **Const-Correctness**: Method contract guarantees no observable state changes
    /// - **Reproducible Determinism**: static_partitioner ensures identical work distribution
    /// - **Scientific Computing Ready**: Guarantees bit-identical results across runs
    /// - **Specialized Design**: Optimized for deterministic algorithms with maximum performance
    ///
    /// @param problem Problem instance providing fitness evaluation function
    /// @param population Vector of genomes to evaluate in parallel
    /// @return Vector of fitness values corresponding to input population order
    /// @throws std::exception if TBB encounters errors during parallel execution
    template <evolab::core::Problem P>
    [[nodiscard]] std::vector<evolab::core::Fitness>
    parallel_evaluate(const P& problem, const std::vector<typename P::GenomeT>& population) const {
        using Fitness = evolab::core::Fitness;

        std::vector<Fitness> fitnesses(population.size());

        // Execute parallel fitness evaluation using TBB's deterministic work distribution
        // static_partitioner ensures reproducible chunk-to-thread mapping for scientific computing
        // Simplified implementation optimized for deterministic algorithms (e.g., TSP)
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

    /// Retrieves the immutable base seed for deterministic parallel execution
    /// @return Base seed value for API compatibility and potential future extensions
    [[nodiscard]] constexpr std::uint64_t get_seed() const noexcept { return base_seed_; }

    // Note: RNG infrastructure removed following YAGNI principle
    // Simplified design optimized for deterministic algorithms,
    // eliminating unused overhead for performance-critical scientific computing

  private:
    // Thread-safety and determinism design notes (C++23 Best Practices):
    //
    // Immutable Design Pattern:
    // - base_seed_: const-qualified after construction, safe for concurrent access
    // - No shared mutable state eliminates data race possibilities by design
    // - Follows C++ Core Guidelines: "You cannot have a race condition on immutable data"
    //
    // Const-Correctness Implementation:
    // - parallel_evaluate() const-qualified expresses thread-safe contract
    // - Method semantics: "logically read-only operation safe for concurrent use"
    // - Aligns with C++11/23 const meaning: "safe to read concurrently"
    //
    // Stateless Architecture Benefits:
    // - Per-call state management prevents interference between invocations
    // - Eliminates synchronization overhead (no mutexes, locks, or atomics on critical path)
    // - Natural exception safety through RAII and automatic cleanup
    // - Simplified API surface - no state management methods required
    //
    // Deterministic Reproducibility Guarantees:
    // - static_partitioner ensures identical work distribution across executions
    // - Thread-assignment independence: deterministic results regardless of thread mapping
    // - Cross-execution consistency: identical results across systems, TBB versions, thread counts
    // - Identical population produces bit-identical results across all program executions
    //
    // Performance Characteristics:
    // - Predictable performance: deterministic work distribution
    // - Well-suited for balanced workloads like TSP fitness evaluation
    // - Cache-friendly: eliminates false sharing from shared atomic counters
    // - Memory efficient: minimal overhead design without unused infrastructure
    // - Optimized for deterministic algorithms: maximum performance for scientific computing
    //
    // Design Philosophy (YAGNI Principle Applied):
    // - Simplified implementation focusing on current requirements (deterministic evaluation)
    // - Eliminated unused RNG infrastructure for performance optimization
    // - Specialized for deterministic algorithms rather than general-purpose design
    // - Future stochastic support requires dedicated implementation with different trade-offs
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
       "\n" \
       "   2. Disable parallel execution:\n" \
       "      cmake -DEVOLAB_USE_TBB=OFF .\n" \
       "\n" \
       " For more information, see project documentation.\n" \
       "===================================================================="

#endif // EVOLAB_HAVE_TBB
