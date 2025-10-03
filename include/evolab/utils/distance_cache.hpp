#pragma once

/// @file distance_cache.hpp
/// @brief Small cache for distance lookups in local search operations
///
/// Implements a direct-mapped cache for distance matrix accesses.
/// Reduces memory latency during local search by caching frequently
/// accessed distances. Cache size is tuned for L1 cache efficiency.

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace evolab::utils {

/// Direct-mapped cache for distance lookups
/// Uses a small cache size (64 entries) to fit in L1 cache
template <typename T = double, std::size_t CacheSize = 64>
class DistanceCache {
    static_assert((CacheSize & (CacheSize - 1)) == 0, "CacheSize must be power of 2");

    struct CacheEntry {
        std::uint32_t key{0}; // Packed (i, j) as single 32-bit value
        T value{0};
        bool valid{false};
    };

    mutable std::array<CacheEntry, CacheSize> entries_;
    mutable std::size_t hits_{0};
    mutable std::size_t misses_{0};

    /// Pack two indices into a single 32-bit key
    /// Assumes i and j fit in 16 bits (max 65535 cities)
    static constexpr std::uint32_t pack_key(int i, int j) noexcept {
        return (static_cast<std::uint32_t>(i) << 16) | static_cast<std::uint32_t>(j);
    }

    /// Get cache index from key using fast bit masking
    static constexpr std::size_t cache_index(std::uint32_t key) noexcept {
        return key & (CacheSize - 1);
    }

  public:
    DistanceCache() = default;

    /// Try to retrieve distance from cache
    /// Returns true if found, false otherwise
    bool try_get(int i, int j, T& out_value) const noexcept {
        const std::uint32_t key = pack_key(i, j);
        const std::size_t idx = cache_index(key);
        const auto& entry = entries_[idx];

        if (entry.valid && entry.key == key) {
            out_value = entry.value;
            ++hits_;
            return true;
        }

        ++misses_;
        return false;
    }

    /// Insert distance into cache
    void put(int i, int j, T value) noexcept {
        const std::uint32_t key = pack_key(i, j);
        const std::size_t idx = cache_index(key);
        entries_[idx] = CacheEntry{key, value, true};
    }

    /// Clear all cache entries
    void clear() noexcept {
        for (auto& entry : entries_) {
            entry.valid = false;
        }
        hits_ = 0;
        misses_ = 0;
    }

    /// Get cache statistics
    std::pair<std::size_t, std::size_t> stats() const noexcept { return {hits_, misses_}; }

    /// Get cache hit rate (0.0 to 1.0)
    double hit_rate() const noexcept {
        const std::size_t total = hits_ + misses_;
        return total > 0 ? static_cast<double>(hits_) / total : 0.0;
    }

    /// Get cache size
    static constexpr std::size_t size() noexcept { return CacheSize; }
};

} // namespace evolab::utils
