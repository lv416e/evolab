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

namespace evolab::utils {

/// Direct-mapped cache for distance lookups
/// Uses a small cache size (64 entries) to fit in L1 cache
/// Thread-safe with atomic operations for parallel local search
template <typename T = double, std::size_t CacheSize = 64>
class DistanceCache {
    static_assert((CacheSize & (CacheSize - 1)) == 0, "CacheSize must be power of 2");

    struct CacheEntry {
        std::atomic<std::uint64_t> key{0}; // Packed (i, j) as single 64-bit value
        std::atomic<T> value{0};
        std::atomic<bool> valid{false};
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
    /// Uses double-check pattern to avoid race between key check and value read
    bool try_get(int i, int j, T& out_value) const noexcept {
        const std::uint64_t key = pack_key(i, j);
        const std::size_t idx = cache_index(key);
        auto& entry = entries_[idx];

        // Use acquire semantics on valid to ensure key/value writes are visible
        // Single check for validity and key match simplifies miss case handling
        if (entry.valid.load(std::memory_order_acquire) &&
            entry.key.load(std::memory_order_relaxed) == key) {

            // Read value, then re-check to protect against races with other writers
            out_value = entry.value.load(std::memory_order_relaxed);

            // The second check ensures that the entry was not invalidated or overwritten
            // between the first check and reading the value
            // Use acquire on valid to synchronize with clear()'s release
            if (entry.key.load(std::memory_order_relaxed) == key &&
                entry.valid.load(std::memory_order_acquire)) {
                hits_.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
        }

        // Single miss increment handles all miss cases
        misses_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    /// Insert distance into cache (thread-safe)
    void put(int i, int j, T value) noexcept {
        const std::uint64_t key = pack_key(i, j);
        const std::size_t idx = cache_index(key);
        auto& entry = entries_[idx];

        // Store key and value first with relaxed ordering
        entry.key.store(key, std::memory_order_relaxed);
        entry.value.store(value, std::memory_order_relaxed);
        // Use release semantics on valid to ensure key/value writes are visible
        // before valid becomes true
        entry.valid.store(true, std::memory_order_release);
    }

    /// Clear all cache entries (thread-safe)
    void clear() noexcept {
        for (auto& entry : entries_) {
            // Use release to ensure clear is visible to other threads
            entry.valid.store(false, std::memory_order_release);
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
