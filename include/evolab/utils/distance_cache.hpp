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
#include <new>
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

    // Cache line size for false sharing prevention
    // Use hardware value if available, otherwise typical 64 bytes
#ifdef __cpp_lib_hardware_interference_size
    static constexpr std::size_t cache_line_size = std::hardware_destructive_interference_size;
#else
    static constexpr std::size_t cache_line_size = 64;
#endif

    // Align to cache line to prevent false sharing between entries
    struct alignas(cache_line_size) CacheEntry {
        std::uint64_t key{0}; // Packed (i, j) as single 64-bit value
        T value{};
        bool valid{false};
        std::atomic_flag lock{}; // Spinlock for atomicity
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

    /// Get cache index from key using XOR folding to mix both i and j
    /// Folds 64-bit key by XORing upper 32 bits (i) with lower 32 bits (j)
    /// This ensures both indices influence the hash, preventing systematic collisions
    static constexpr std::size_t cache_index(std::uint64_t key) noexcept {
        return static_cast<std::size_t>((key ^ (key >> 32)) & (CacheSize - 1));
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
        // Spinlock provides all necessary memory synchronization
        while (entry.lock.test_and_set(std::memory_order_acquire)) {
            EVOLAB_PAUSE(); // Yield CPU to reduce contention
        }

        bool found = false;
        if (entry.valid && entry.key == key) {
            out_value = entry.value;
            found = true;
        }

        entry.lock.clear(std::memory_order_release);

        // Update statistics outside critical section to minimize lock hold time
        // Cache hit is the expected common case (hint to compiler for branch prediction)
        if (EVOLAB_LIKELY(found)) {
            hits_.fetch_add(1, std::memory_order_relaxed);
        } else {
            misses_.fetch_add(1, std::memory_order_relaxed);
        }
        return found;
    }

    /// Insert distance into cache (thread-safe)
    /// Uses spinlock to ensure atomic key-value pair updates
    void put(int i, int j, T value) const noexcept {
        const std::uint64_t key = pack_key(i, j);
        const std::size_t idx = cache_index(key);
        auto& entry = entries_[idx];

        // Acquire spinlock to prevent interleaved writes from multiple threads
        while (entry.lock.test_and_set(std::memory_order_acquire)) {
            EVOLAB_PAUSE(); // Yield CPU to reduce contention on hyper-threaded cores
        }

        // Spinlock guarantees atomicity and provides memory synchronization
        entry.key = key;
        entry.value = value;
        entry.valid = true;

        // Release spinlock with release barrier to ensure visibility
        entry.lock.clear(std::memory_order_release);
    }

    /// Clear all cache entries (thread-safe)
    /// Note: Entries are invalidated one by one. Concurrent put() calls may
    /// re-populate entries during clear(). No global snapshot is guaranteed.
    /// This incremental invalidation is suitable for advisory cache storage.
    /// Statistics are not reset - use reset_stats() explicitly if needed.
    void clear() const noexcept {
        for (auto& entry : entries_) {
            // Acquire spinlock to prevent races with concurrent put()
            while (entry.lock.test_and_set(std::memory_order_acquire)) {
                EVOLAB_PAUSE(); // Yield CPU to reduce contention on hyper-threaded cores
            }

            // Spinlock provides memory synchronization
            entry.valid = false;

            // Release spinlock
            entry.lock.clear(std::memory_order_release);
        }
    }

    /// Reset cache statistics (thread-safe)
    /// Note: This is separate from clear() to avoid race conditions with
    /// concurrent try_get() operations that may increment counters.
    void reset_stats() const noexcept {
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
