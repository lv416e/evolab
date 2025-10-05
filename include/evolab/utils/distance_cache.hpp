#pragma once

/// @file distance_cache.hpp
/// @brief Small cache for distance lookups in local search operations
///
/// Implements a direct-mapped cache for distance matrix accesses.
/// Reduces memory latency during local search by caching frequently
/// accessed distances. Cache size is tuned for L1 cache efficiency.

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "evolab/utils/compiler_hints.hpp"

namespace evolab::utils {

/// Direct-mapped cache for distance lookups
/// Uses a small cache size (64 entries) to fit in L1 cache
/// Thread-safe with atomic operations for parallel local search
template <typename T = double, std::size_t CacheSize = 64>
class DistanceCache {
    static_assert(CacheSize > 0, "CacheSize must be greater than 0");
    static_assert((CacheSize & (CacheSize - 1)) == 0, "CacheSize must be power of 2");

    struct CacheEntry {
        std::atomic<std::uint64_t> key{0}; // Packed (i, j) as single 64-bit value
        std::atomic<T> value{};
        std::atomic<bool> valid{false};
        mutable std::atomic_flag lock = ATOMIC_FLAG_INIT; // Spinlock for put() atomicity
    };

    mutable std::array<CacheEntry, CacheSize> entries_;
    mutable std::atomic<std::size_t> hits_{0};
    mutable std::atomic<std::size_t> misses_{0};

    /// Pack two indices into a single 64-bit key
    /// Supports full 32-bit indices without collision
    static constexpr std::uint64_t pack_key(int i, int j) noexcept {
        return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(i)) << 32) |
               static_cast<std::uint64_t>(static_cast<std::uint32_t>(j));
    }

    /// Get cache index from key using fast bit masking
    static constexpr std::size_t cache_index(std::uint64_t key) noexcept {
        return static_cast<std::size_t>(key & (CacheSize - 1));
    }

  public:
    DistanceCache() = default;

    /// Try to retrieve distance from cache (thread-safe)
    /// Returns true if found, false otherwise
    /// Uses spinlock to prevent data races with concurrent writers
    bool try_get(int i, int j, T& out_value) const noexcept {
        const std::uint64_t key = pack_key(i, j);
        const std::size_t idx = cache_index(key);
        auto& entry = entries_[idx];

        // Acquire spinlock to prevent races with writers
        // This ensures no torn reads when T is not natively atomic
        while (entry.lock.test_and_set(std::memory_order_acquire)) {
            EVOLAB_PAUSE(); // Yield CPU to reduce contention
        }

        bool found = false;
        if (entry.valid.load(std::memory_order_relaxed) &&
            entry.key.load(std::memory_order_relaxed) == key) {
            out_value = entry.value.load(std::memory_order_relaxed);
            hits_.fetch_add(1, std::memory_order_relaxed);
            found = true;
        } else {
            misses_.fetch_add(1, std::memory_order_relaxed);
        }

        entry.lock.clear(std::memory_order_release);
        return found;
    }

    /// Insert distance into cache (thread-safe)
    /// Uses spinlock to ensure atomic key-value pair updates
    void put(int i, int j, T value) noexcept {
        const std::uint64_t key = pack_key(i, j);
        const std::size_t idx = cache_index(key);
        auto& entry = entries_[idx];

        // Acquire spinlock to prevent interleaved writes from multiple threads
        while (entry.lock.test_and_set(std::memory_order_acquire)) {
            EVOLAB_PAUSE(); // Yield CPU to reduce contention on hyper-threaded cores
        }

        // The spinlock guarantees this update is atomic. All stores can use relaxed
        // memory order because the lock release provides the necessary synchronization barrier.
        entry.key.store(key, std::memory_order_relaxed);
        entry.value.store(value, std::memory_order_relaxed);
        entry.valid.store(true, std::memory_order_relaxed);

        // Release spinlock with release barrier to ensure visibility
        entry.lock.clear(std::memory_order_release);
    }

    /// Clear all cache entries (thread-safe)
    void clear() noexcept {
        for (auto& entry : entries_) {
            // Acquire spinlock to prevent races with concurrent put()
            while (entry.lock.test_and_set(std::memory_order_acquire)) {
                EVOLAB_PAUSE(); // Yield CPU to reduce contention on hyper-threaded cores
            }

            // Use release to ensure clear is visible to other threads
            entry.valid.store(false, std::memory_order_release);

            // Release spinlock
            entry.lock.clear(std::memory_order_release);
        }
        hits_.store(0, std::memory_order_relaxed);
        misses_.store(0, std::memory_order_relaxed);
    }

    /// Get cache statistics (thread-safe)
    std::pair<std::size_t, std::size_t> stats() const noexcept {
        return {hits_.load(std::memory_order_relaxed), misses_.load(std::memory_order_relaxed)};
    }

    /// Get cache hit rate (0.0 to 1.0) (thread-safe)
    double hit_rate() const noexcept {
        const std::size_t h = hits_.load(std::memory_order_relaxed);
        const std::size_t m = misses_.load(std::memory_order_relaxed);
        const std::size_t total = h + m;
        return total > 0 ? static_cast<double>(h) / total : 0.0;
    }

    /// Get cache size
    static constexpr std::size_t size() noexcept { return CacheSize; }
};

} // namespace evolab::utils
