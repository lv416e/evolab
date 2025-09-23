#pragma once

/// @file numa_allocator.hpp
/// @brief NUMA-aware memory allocation utilities for high-performance computing
///
/// This header provides NUMA-aware memory resources that can improve performance
/// on multi-socket systems by ensuring memory is allocated close to the CPU that
/// will access it. The implementation is optional and falls back gracefully when
/// NUMA support is not available.

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <unordered_map>

// Optional NUMA support - only include if available
#ifdef EVOLAB_NUMA_SUPPORT
#include <numa.h>
#include <numaif.h>
#include <sched.h>
#endif

namespace evolab::utils {

/// NUMA-aware memory resource for optimal memory placement on multi-socket systems
///
/// This memory resource attempts to allocate memory on the local NUMA node or a
/// specified node to minimize memory access latency. When NUMA support is not
/// available or compiled out, it falls back to standard allocation.
///
/// Usage:
/// @code
/// // Allocate on local NUMA node
/// auto local_resource = NumaMemoryResource::create_local();
/// std::pmr::vector<int> data(local_resource.get());
///
/// // Allocate on specific node
/// auto node_resource = NumaMemoryResource::create_on_node(1);
/// std::pmr::vector<double> node_data(node_resource.get());
/// @endcode
class NumaMemoryResource : public std::pmr::memory_resource {
  private:
    int numa_node_;       ///< NUMA node ID (-1 for local node)
    bool numa_available_; ///< Whether NUMA is available on this system

    /// Track allocation method and original pointer for proper deallocation
    struct AllocationInfo {
        bool is_numa_allocated;
        void* original_ptr;         ///< Original pointer for over-allocated NUMA memory
        std::size_t original_bytes; ///< Original size for over-allocated NUMA memory
    };
    mutable std::unordered_map<void*, AllocationInfo> allocations_;
    mutable std::mutex allocations_mutex_;

  public:
    /// Constructor for NUMA memory resource
    ///
    /// @param node_id NUMA node ID (-1 for local node)
    explicit NumaMemoryResource(int node_id) : numa_node_(node_id) {
#ifdef EVOLAB_NUMA_SUPPORT
        numa_available_ = numa_available() >= 0;
#else
        numa_available_ = false;
#endif
    }

    /// Create NUMA memory resource for local node
    ///
    /// @return Unique pointer to memory resource that allocates on local NUMA node
    static std::unique_ptr<NumaMemoryResource> create_local() {
        return std::make_unique<NumaMemoryResource>(-1);
    }

    /// Create NUMA memory resource for specific node
    ///
    /// @param node_id The NUMA node ID to allocate on
    /// @return Unique pointer to memory resource that allocates on specified node
    static std::unique_ptr<NumaMemoryResource> create_on_node(int node_id) {
        return std::make_unique<NumaMemoryResource>(node_id);
    }

    /// Check if NUMA support is available on this system
    ///
    /// @return True if NUMA is available and this resource will use NUMA allocation
    [[nodiscard]] bool is_numa_available() const noexcept { return numa_available_; }

    /// Get the NUMA node this resource allocates on
    ///
    /// @return NUMA node ID (-1 for local node)
    [[nodiscard]] int numa_node() const noexcept { return numa_node_; }

    /// Get number of NUMA nodes available on this system
    ///
    /// @return Number of NUMA nodes (1 if NUMA not available)
    static int get_numa_node_count() noexcept {
#ifdef EVOLAB_NUMA_SUPPORT
        if (numa_available() >= 0) {
            return numa_max_node() + 1;
        }
#endif
        return 1;
    }

    /// Get current NUMA node for the calling thread
    ///
    /// @return Current NUMA node ID (0 if NUMA not available)
    static int get_current_numa_node() noexcept {
#ifdef EVOLAB_NUMA_SUPPORT
        if (numa_available() >= 0) {
            return numa_node_of_cpu(sched_getcpu());
        }
#endif
        return 0;
    }

  protected:
    /// Allocate memory using NUMA-aware allocation
    ///
    /// @param bytes Number of bytes to allocate
    /// @param alignment Memory alignment requirement
    /// @return Pointer to allocated memory
    /// @throws std::bad_alloc if allocation fails
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        // Precondition check for std::align and std::aligned_alloc
        assert((alignment > 0) && ((alignment & (alignment - 1)) == 0) &&
               "alignment must be a power of two");

        // Ensure minimum alignment
        if (alignment < alignof(std::max_align_t)) {
            alignment = alignof(std::max_align_t);
        }

#ifdef EVOLAB_NUMA_SUPPORT
        if (numa_available_) {
            // Over-allocate to ensure we can find an aligned pointer
            std::size_t alloc_size = bytes + alignment - 1;
            void* original_ptr = nullptr;

            if (numa_node_ == -1) {
                // Allocate on local NUMA node
                original_ptr = numa_alloc_local(alloc_size);
            } else {
                // Allocate on specific NUMA node
                original_ptr = numa_alloc_onnode(alloc_size, numa_node_);
            }

            if (original_ptr != nullptr) {
                // Find aligned pointer within the over-allocated block
                void* aligned_ptr = original_ptr;
                std::size_t remaining_space = alloc_size;

                if (std::align(alignment, bytes, aligned_ptr, remaining_space)) {
                    std::lock_guard<std::mutex> lock(allocations_mutex_);
                    allocations_[aligned_ptr] = {true, original_ptr, alloc_size};
                    return aligned_ptr;
                } else {
                    // Should not happen with proper over-allocation, but fallback
                    numa_free(original_ptr, alloc_size);
                }
            }
        }
#endif

        // Fallback to standard aligned allocation
        // aligned_alloc requires size to be multiple of alignment
        std::size_t padded_bytes = bytes;
        const std::size_t remainder = padded_bytes % alignment;
        if (remainder != 0) {
            padded_bytes += (alignment - remainder);
        }

        void* ptr = std::aligned_alloc(alignment, padded_bytes);
        if (!ptr) {
            throw std::bad_alloc{};
        }
        std::lock_guard<std::mutex> lock(allocations_mutex_);
        allocations_[ptr] = {false, ptr, padded_bytes};
        return ptr;
    }

    /// Deallocate memory allocated by this resource
    ///
    /// @param ptr Pointer to memory to deallocate
    /// @param bytes Size of the allocation
    /// @param alignment Alignment used for allocation
    void do_deallocate(void* ptr, [[maybe_unused]] std::size_t bytes,
                       [[maybe_unused]] std::size_t alignment) override {
        AllocationInfo alloc_info{};
        bool found = false;
        {
            std::lock_guard<std::mutex> lock(allocations_mutex_);
            auto it = allocations_.find(ptr);
            if (it != allocations_.end()) {
                alloc_info = it->second;
                found = true;
                allocations_.erase(it);
            }
        }

        if (!found) {
            // Unknown pointer — indicates allocator mismatch; avoid UB from freeing.
#ifndef NDEBUG
            std::abort();
#else
            return;
#endif
        }

#ifdef EVOLAB_NUMA_SUPPORT
        if (numa_available_ && alloc_info.is_numa_allocated) {
            numa_free(alloc_info.original_ptr, alloc_info.original_bytes);
            return;
        }
#endif
        // Fallback to standard deallocation
        std::free(alloc_info.original_ptr);
    }

    /// Check if this resource is equal to another
    ///
    /// @param other The other memory resource to compare with
    /// @return True if resources are equivalent
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        // Use pointer identity - resources with separate allocation maps are not interchangeable
        return this == &other;
    }
};

/// NUMA-aware allocator factory functions

/// Create optimal memory resource for parallel genetic algorithm execution
///
/// This function analyzes the system and creates the most appropriate memory
/// resource for GA workloads:
/// - On NUMA systems: creates local node allocator
/// - On UMA systems: returns default resource
///
/// @return Memory resource optimized for the current system
inline std::pmr::memory_resource* create_optimized_ga_resource() {
    static thread_local std::unique_ptr<NumaMemoryResource> local_resource;

    if (NumaMemoryResource::get_numa_node_count() > 1) {
        // Multi-node system: use local NUMA allocation
        if (!local_resource) {
            local_resource = NumaMemoryResource::create_local();
        }
        return local_resource.get();
    } else {
        // Single-node system: use default resource
        return std::pmr::get_default_resource();
    }
}

/// Create memory resource for distributed island model
///
/// For island model GA where each island runs on a different NUMA node,
/// this creates node-specific allocators to minimize cross-node memory access.
///
/// @param island_id Island identifier (maps to NUMA node)
/// @return Memory resource for the specified island/node
inline std::pmr::memory_resource* create_island_resource(int island_id) {
    static thread_local std::unordered_map<int, std::unique_ptr<NumaMemoryResource>>
        island_resources;

    const int node_count = NumaMemoryResource::get_numa_node_count();
    if (node_count <= 1 || island_id < 0) {
        return std::pmr::get_default_resource();
    }

    // Map island to NUMA node (round-robin)
    const int numa_node = island_id % node_count;

    // Create resource if it doesn't exist
    auto& resource = island_resources[island_id];
    if (!resource) {
        resource = NumaMemoryResource::create_on_node(numa_node);
    }

    return resource.get();
}

} // namespace evolab::utils
