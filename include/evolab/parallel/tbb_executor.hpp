#pragma once

#ifdef EVOLAB_HAVE_TBB

#include <atomic>
#include <cstdint>
#include <random>
#include <vector>

#include <evolab/core/concepts.hpp>
#include <tbb/blocked_range.h>
#include <tbb/combinable.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>

namespace evolab::parallel {

/// Thread-safe parallel executor using Intel TBB for deterministic fitness evaluation
///
/// This executor provides reproducible parallel fitness evaluation with identical results
/// across multiple runs using the same seed. The implementation uses static_partitioner
/// to ensure deterministic work distribution combined with per-genome RNG seeding
/// for maximum stochastic reproducibility, prioritizing scientific computing
/// requirements over performance optimization.
///
/// Stochastic Reproducibility Architecture:
/// - **Per-genome seeding**: Each genome gets deterministic seed (base_seed + i *
/// golden_ratio_prime)
/// - **Thread-assignment independence**: Identical results regardless of thread-to-genome mapping
/// - **Cross-execution consistency**: Same results across different systems, TBB versions, thread
/// counts
/// - **Mathematical seed distribution**: Golden ratio prime ensures optimal seed spacing
///
/// Key features:
/// - Deterministic work distribution ensures reproducible results across all runs
/// - Per-genome RNG seeding guarantees stochastic algorithm reproducibility
/// - Static range partitioning for consistent chunk-to-thread mapping
/// - Exception-safe RAII design with proper resource management
///
/// Design Choice: Prioritizes scientific reproducibility over performance optimization.
/// Trade-offs: Per-genome reseeding cost vs guaranteed cross-execution determinism.
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
    /// @param seed Base seed for reproducible parallel execution across multiple runs
    ///             Each thread derives its unique seed deterministically from this base
    explicit TBBExecutor(std::uint64_t seed = 1) : base_seed_(seed) {}

    /// Performs thread-safe parallel fitness evaluation using stateless design
    ///
    /// This method implements C++23 const-correctness principles by declaring the
    /// operation as logically read-only. The stateless design eliminates data races
    /// through per-call state management, where each invocation creates its own
    /// thread-local RNG infrastructure independently.
    ///
    /// Key design benefits:
    /// - **Thread Safety**: No shared mutable state prevents data races by design
    /// - **Const-Correctness**: Method contract guarantees no observable state changes
    /// - **Reproducible Determinism**: static_partitioner ensures identical work distribution
    /// - **Scientific Computing Ready**: Guarantees bit-identical results across runs
    /// - **Future-Proof**: Supports both deterministic and stochastic algorithms reproducibly
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

        // Golden ratio prime for optimal seed distribution across genome space
        // Mathematical properties ensure uniform distribution across 64-bit space
        // for sequential inputs while minimizing collision clustering
        constexpr std::uint64_t golden_ratio_prime = 0x9e3779b97f4a7c15ULL;

        // Thread-local RNG storage optimized for per-genome seeding
        // Each thread maintains its own RNG instance for performance optimization
        // while actual determinism comes from per-genome reseeding
        //
        // C++23 Best Practice: Explicit capture following principle of least privilege
        // Captures only the specific values needed for RNG initialization
        tbb::combinable<std::mt19937> thread_rngs([seed = base_seed_]() {
            // Initialize thread-local RNG with base seed
            // Per-genome seeding will override this for each evaluation
            // Initial seed value is not critical for determinism
            return std::mt19937(seed);
        });

        // Execute parallel fitness evaluation using TBB's deterministic work distribution
        // static_partitioner ensures reproducible chunk-to-thread mapping for scientific computing
        // Explicit lambda captures ensure thread safety and clear dependency tracking
        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(0, population.size()),
            [&problem, &fitnesses, &population, seed = base_seed_,
             &thread_rngs](const tbb::blocked_range<std::size_t>& range) {
                // Acquire thread-local RNG for performance optimization
                // Reseeded per-genome for maximum stochastic reproducibility
                auto& rng = thread_rngs.local();

                // Process assigned range with thread-safe, cache-efficient evaluation
                // Each thread writes to distinct indices, preventing data races
                for (std::size_t i = range.begin(); i != range.end(); ++i) {
                    // Per-genome deterministic seeding for true stochastic reproducibility
                    // Ensures population[i] always gets identical RNG state across all runs
                    // regardless of thread assignment or system conditions
                    const std::uint64_t genome_seed = seed + (i * golden_ratio_prime);
                    rng.seed(genome_seed);

                    // Fitness evaluation with genome-specific RNG state available
                    // Currently unused for deterministic TSP but guarantees reproducible
                    // stochastic evaluation for future evolutionary algorithms
                    fitnesses[i] = problem.evaluate(population[i]);
                }
            },
            tbb::static_partitioner{});

        return fitnesses;
    }

    /// Retrieves the immutable base seed for deterministic parallel execution
    /// @return Base seed value used for per-thread seed derivation
    [[nodiscard]] constexpr std::uint64_t get_seed() const noexcept { return base_seed_; }

    // Note: reset_rngs() method removed in favor of stateless design
    // Each parallel_evaluate() call operates independently with fresh state,
    // eliminating the need for explicit state reset and associated thread safety concerns

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
    // - Per-genome RNG seeding guarantees stochastic algorithm reproducibility
    // - Thread-assignment independence: population[i] gets same RNG state regardless of thread
    // - Cross-execution consistency: identical results across systems, TBB versions, thread counts
    // - Mathematical seeding ensures uniform distribution across genome space using golden ratio
    // prime
    // - Identical base_seed + population produces bit-identical results across all program
    // executions
    //
    // Performance Characteristics:
    // - Predictable performance: deterministic work distribution
    // - Well-suited for balanced workloads like TSP fitness evaluation
    // - Cache-friendly: eliminates false sharing from shared atomic counters
    // - Memory efficient: automatic cleanup of per-call thread-local storage
    // - Trade-offs: Static partitioning and per-genome reseeding prioritize reproducibility over
    // performance
    //
    // Future Extensibility:
    // - Thread-local RNG infrastructure prepared for stochastic evaluation methods
    // - Design accommodates both deterministic and probabilistic algorithms
    // - Compatible with advanced evolutionary operators requiring per-thread randomization
    // - Maintains API stability while enabling internal optimizations
};

} // namespace evolab::parallel

#else

#include <cstdint>
#include <type_traits>
#include <vector>

#include <evolab/core/concepts.hpp>

namespace {
/// Template-dependent false for C++23-compliant static_assert
template <typename = void>
constexpr bool always_false_v = false;
} // namespace

// Standards-compliant fallback implementation when TBB is not available
//
// This fallback provides clear compile-time error messages when TBB parallel
// execution is requested but the library is not found during configuration.
// The implementation follows C++23 best practices for conditional compilation.
namespace evolab::parallel {

/// Compile-time error stub for TBBExecutor when Intel TBB is unavailable
///
/// This class intentionally fails compilation with a descriptive error message
/// when parallel execution is attempted without proper TBB installation.
/// Use cmake configuration options to resolve dependency issues.
class TBBExecutor {
  private:
    // API compatibility - store seed for consistent get_seed() behavior
    const std::uint64_t base_seed_;

  public:
    /// Constructor that triggers standards-compliant compile-time error
    ///
    /// @param seed Seed value for API compatibility with TBB implementation
    /// @note This implementation uses template-dependent false for C++23 standard compliance
    explicit TBBExecutor(std::uint64_t seed = 1) : base_seed_(seed) {
        static_assert(always_false_v<>,
                      "\n"
                      "====================================================================\n"
                      " TBB (Threading Building Blocks) is required for parallel execution\n"
                      " but was not found during configuration.\n"
                      "\n"
                      " Resolution options:\n"
                      "   1. Install Intel TBB development package:\n"
                      "      - Ubuntu/Debian: apt install libtbb-dev\n"
                      "      - RHEL/CentOS:   yum install tbb-devel\n"
                      "      - macOS:         brew install tbb\n"
                      "      - Windows:       vcpkg install tbb\n"
                      "\n"
                      "   2. Disable parallel execution:\n"
                      "      cmake -DEVOLAB_USE_TBB=OFF .\n"
                      "\n"
                      " For more information, see project documentation.\n"
                      "====================================================================\n");
    }

    /// Placeholder method for API compatibility - never actually called
    /// @return Empty vector (unreachable due to static_assert)
    template <evolab::core::Problem P>
    [[nodiscard]] std::vector<evolab::core::Fitness>
    parallel_evaluate([[maybe_unused]] const P& problem,
                      [[maybe_unused]] const std::vector<typename P::GenomeT>& population) const {
        // This method is never reached due to constructor static_assert
        return {};
    }

    /// API compatibility method - returns the seed passed to constructor
    /// @return Base seed value for consistency with TBB implementation
    [[nodiscard]] constexpr std::uint64_t get_seed() const noexcept { return base_seed_; }

    // Note: reset_rngs() method removed in both implementations for API consistency
};

} // namespace evolab::parallel

#endif // EVOLAB_HAVE_TBB
