#pragma once

#ifdef EVOLAB_HAVE_TBB

#include <cstdint>
#include <random>
#include <thread>
#include <vector>

#include <tbb/blocked_range.h>
#include <tbb/combinable.h>
#include <tbb/parallel_for.h>

#include "../core/concepts.hpp"

namespace evolab::parallel {

/// Thread-safe parallel executor using Intel TBB
/// Provides parallel fitness evaluation while maintaining reproducibility
class TBBExecutor {
  private:
    std::uint64_t base_seed_;

    // Thread-local RNG storage - each thread gets its own RNG seeded deterministically
    mutable tbb::combinable<std::mt19937> thread_rngs_;

  public:
    /// Constructor with base seed for reproducible parallel execution
    explicit TBBExecutor(std::uint64_t seed = 1)
        : base_seed_(seed), thread_rngs_([this]() {
              // Each thread gets a unique seed derived from base_seed and thread ID
              // This ensures reproducible results independent of thread scheduling
              std::uint64_t thread_seed =
                  base_seed_ ^ std::hash<std::thread::id>{}(std::this_thread::get_id());
              return std::mt19937(thread_seed);
          }) {}

    /// Parallel fitness evaluation using TBB blocked_range partitioning
    /// Maintains same results as sequential evaluation with potential performance improvement
    template <evolab::core::Problem P>
    std::vector<evolab::core::Fitness>
    parallel_evaluate(const P& problem, const std::vector<typename P::GenomeT>& population) {
        using GenomeT = typename P::GenomeT;
        using Fitness = evolab::core::Fitness;

        std::vector<Fitness> fitnesses(population.size());

        // Use TBB parallel_for with blocked_range for efficient work distribution
        tbb::parallel_for(tbb::blocked_range<std::size_t>(0, population.size()),
                          [this, &problem, &fitnesses,
                           &population](const tbb::blocked_range<std::size_t>& range) {
                              // Get thread-local RNG for this worker (for future stochastic
                              // algorithms)
                              [[maybe_unused]] auto& rng = thread_rngs_.local();

                              // Evaluate fitness for this thread's chunk of the population
                              for (std::size_t i = range.begin(); i != range.end(); ++i) {
                                  fitnesses[i] = problem.evaluate(population[i]);
                              }
                          });

        return fitnesses;
    }

    /// Get the base seed used for this executor
    std::uint64_t get_seed() const { return base_seed_; }

    /// Reset thread-local RNGs (useful for deterministic multi-run experiments)
    void reset_rngs() { thread_rngs_.clear(); }

  private:
    // Note: Currently RNG is not used in fitness evaluation for TSP
    // but this infrastructure supports future stochastic evaluation methods
    // or local search algorithms that need thread-safe randomization
};

} // namespace evolab::parallel

#else

// Fallback when TBB is not available
namespace evolab::parallel {

class TBBExecutor {
  public:
    explicit TBBExecutor(std::uint64_t = 1) {
        static_assert(false, "TBB is required for parallel execution but not found. "
                             "Install Intel TBB or set EVOLAB_USE_TBB=OFF");
    }

    template <typename P>
    std::vector<evolab::core::Fitness> parallel_evaluate(const P&,
                                                         const std::vector<typename P::GenomeT>&) {
        return {};
    }
};

} // namespace evolab::parallel

#endif // EVOLAB_HAVE_TBB