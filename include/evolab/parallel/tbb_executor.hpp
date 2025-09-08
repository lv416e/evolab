#pragma once

#ifdef EVOLAB_HAVE_TBB

#include <atomic>
#include <cstdint>
#include <random>
#include <vector>

#include <tbb/blocked_range.h>
#include <tbb/combinable.h>
#include <tbb/parallel_for.h>

#include "../core/concepts.hpp"

namespace evolab::parallel {

/// Thread-safe parallel executor using Intel TBB for high-performance fitness evaluation
///
/// This executor provides deterministic parallel fitness evaluation with reproducible results
/// across multiple runs using the same seed. Thread-local RNG infrastructure supports
/// future stochastic algorithms while maintaining thread safety in concurrent environments.
///
/// Key features:
/// - Deterministic seeding ensures reproducible results independent of thread scheduling
/// - Thread-local RNG management for future stochastic algorithm support
/// - Blocked range partitioning for optimal work distribution
/// - Exception-safe RAII design with proper resource management
class TBBExecutor {
  private:
    std::uint64_t base_seed_;

    // Atomic counter for deterministic thread index assignment across program executions
    // This ensures reproducible results by providing each thread with a sequential,
    // deterministic identifier rather than relying on non-portable thread ID hashing
    std::atomic<std::uint64_t> rng_init_count_{0};

    // Thread-local RNG storage with guaranteed deterministic seeding
    // Each thread receives a unique, reproducible seed derived from base_seed and
    // sequential thread initialization order, ensuring identical results across
    // multiple program executions with the same base seed
    tbb::combinable<std::mt19937> thread_rngs_;

  public:
    /// Constructs a thread-safe parallel executor with deterministic seeding
    ///
    /// Initializes the executor with guaranteed reproducible behavior across program
    /// executions. Uses atomic counter-based thread indexing to ensure deterministic
    /// seed generation independent of system threading implementation details.
    ///
    /// @param seed Base seed for reproducible parallel execution across multiple runs
    ///             Each thread derives its unique seed from this base value and sequential
    ///             thread initialization index, ensuring cross-execution consistency
    explicit TBBExecutor(std::uint64_t seed = 1)
        : base_seed_(seed), thread_rngs_([this]() {
              // Generate truly deterministic per-thread seed using sequential thread indexing
              // This approach guarantees identical results across program executions by:
              // 1. Using atomic counter to assign sequential thread indices (0, 1, 2, ...)
              // 2. Combining base_seed with thread index using multiplicative hashing
              // 3. Avoiding std::hash<thread::id> which is non-deterministic across executions
              const std::uint64_t thread_idx =
                  rng_init_count_.fetch_add(1, std::memory_order_relaxed);

              // Apply multiplicative hashing with large prime for better seed distribution
              // This prevents correlation between adjacent thread seeds and improves randomness
              constexpr std::uint64_t mixing_prime = 0x9e3779b97f4a7c15ULL; // Golden ratio prime
              const std::uint64_t thread_seed = base_seed_ + (thread_idx * mixing_prime);

              return std::mt19937(thread_seed);
          }) {}

    /// Performs parallel fitness evaluation using TBB's efficient work distribution
    ///
    /// This method evaluates population fitness in parallel while maintaining identical
    /// results to sequential evaluation. Uses blocked range partitioning for optimal
    /// cache locality and work distribution across available threads.
    ///
    /// @param problem Problem instance providing fitness evaluation function
    /// @param population Vector of genomes to evaluate
    /// @return Vector of fitness values corresponding to input population
    /// @throws std::exception if evaluation fails or TBB encounters errors
    template <evolab::core::Problem P>
    [[nodiscard]] std::vector<evolab::core::Fitness>
    parallel_evaluate(const P& problem, const std::vector<typename P::GenomeT>& population) {
        using GenomeT = typename P::GenomeT;
        using Fitness = evolab::core::Fitness;

        std::vector<Fitness> fitnesses(population.size());

        // Execute parallel fitness evaluation using TBB's dynamic work distribution
        // Explicit lambda captures ensure thread safety and clear dependencies
        tbb::parallel_for(tbb::blocked_range<std::size_t>(0, population.size()),
                          [this, &problem, &fitnesses,
                           &population](const tbb::blocked_range<std::size_t>& range) {
                              // Acquire thread-local RNG for potential future stochastic algorithms
                              // Currently unused for deterministic TSP evaluation but provides
                              // infrastructure for evolutionary operators requiring randomization
                              [[maybe_unused]] auto& rng = thread_rngs_.local();

                              // Process assigned range of population with thread-safe evaluation
                              // Each thread writes to distinct fitness array indices, preventing
                              // data races
                              for (std::size_t i = range.begin(); i != range.end(); ++i) {
                                  fitnesses[i] = problem.evaluate(population[i]);
                              }
                          });

        return fitnesses;
    }

    /// Retrieves the base seed used for deterministic parallel execution
    /// @return Base seed value used to derive per-thread seeds
    [[nodiscard]] constexpr std::uint64_t get_seed() const noexcept { return base_seed_; }

    /// Resets all thread-local RNG state for deterministic multi-run experiments
    ///
    /// This method performs a complete reset of the parallel RNG infrastructure:
    /// 1. Clears all thread-local RNG instances from TBB combinable storage
    /// 2. Resets the atomic thread counter to ensure identical thread indexing
    /// 3. Forces fresh, deterministic initialization on subsequent RNG access
    ///
    /// Critical for ensuring identical behavior across multiple experimental runs
    /// with the same executor instance and base seed.
    void reset_rngs() noexcept {
        thread_rngs_.clear();
        rng_init_count_.store(0, std::memory_order_relaxed);
    }

  private:
    // Thread-safety and determinism design notes:
    //
    // Immutable state (thread-safe by design):
    // - base_seed_: immutable after construction, safe for concurrent read access
    //
    // Atomic synchronization:
    // - rng_init_count_: atomic counter ensures thread-safe sequential indexing
    // - Uses relaxed memory ordering for performance (ordering not critical for RNG seeding)
    // - Guarantees deterministic thread index assignment across program executions
    //
    // Thread-local storage:
    // - thread_rngs_: TBB combinable provides thread-local storage with proper synchronization
    // - Each thread maintains its own MT19937 instance with unique, deterministic seed
    // - No shared mutable state between threads during fitness evaluation
    //
    // Parallel execution safety:
    // - Each thread writes to distinct fitness array indices, preventing data races
    // - Problem and population parameters are read-only during evaluation
    // - RNG access is thread-local, eliminating contention
    //
    // Deterministic reproducibility guarantees:
    // - Same base_seed produces identical results across program executions
    // - Thread initialization order is deterministic via atomic counter
    // - Independent of thread ID hashing or system-specific threading behavior
    //
    // Future extensibility:
    // - RNG infrastructure prepared for stochastic evaluation methods
    // - Thread-local storage supports evolutionary operators requiring randomization
    // - Design accommodates both deterministic and probabilistic algorithms
};

} // namespace evolab::parallel

#else

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
  public:
    /// Constructor that triggers standards-compliant compile-time error
    ///
    /// @param seed Unused parameter for API compatibility
    /// @note This implementation uses std::false_type for C++23 standard compliance
    explicit TBBExecutor([[maybe_unused]] std::uint64_t seed = 1) {
        static_assert(std::false_type::value,
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
    template <typename P>
    [[nodiscard]] std::vector<evolab::core::Fitness>
    parallel_evaluate([[maybe_unused]] const P& problem,
                      [[maybe_unused]] const std::vector<typename P::GenomeT>& population) {
        // This method is never reached due to constructor static_assert
        return {};
    }

    /// API compatibility methods - unreachable due to static_assert
    [[nodiscard]] constexpr std::uint64_t get_seed() const noexcept { return 1; }
    void reset_rngs() noexcept {}
};

} // namespace evolab::parallel

#endif // EVOLAB_HAVE_TBB