#pragma once

/// @file numa_allocator.hpp
/// @brief NUMA-aware memory allocation utilities for high-performance computing
///
/// This header provides NUMA-aware memory resources that can improve performance
/// on multi-socket systems by ensuring memory is allocated close to the CPU that
/// will access it. The implementation is optional and falls back gracefully when
/// NUMA support is not available.

#include <cstddef>
#include <cstdlib>
#include <memory_resource>

// Optional NUMA support - only include if available
#ifdef EVOLAB_NUMA_SUPPORT
#include <numa.h>
#include <numaif.h>
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
        // Ensure minimum alignment
        if (alignment < alignof(std::max_align_t)) {
            alignment = alignof(std::max_align_t);
        }

#ifdef EVOLAB_NUMA_SUPPORT
        if (numa_available_) {
            void* ptr = nullptr;

            if (numa_node_ == -1) {
                // Allocate on local NUMA node
                ptr = numa_alloc_local(bytes);
            } else {
                // Allocate on specific NUMA node
                ptr = numa_alloc_onnode(bytes, numa_node_);
            }

            if (ptr != nullptr) {
                // Check if alignment is satisfied
                if (reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0) {
                    return ptr;
                } else {
                    // Free and fall back to aligned allocation
                    numa_free(ptr, bytes);
                }
            }
        }
#endif

        // Fallback to standard aligned allocation
        void* ptr = std::aligned_alloc(alignment, bytes);
        if (!ptr) {
            throw std::bad_alloc{};
        }
        return ptr;
    }

    /// Deallocate memory allocated by this resource
    ///
    /// @param ptr Pointer to memory to deallocate
    /// @param bytes Size of the allocation
    /// @param alignment Alignment used for allocation
    void do_deallocate(void* ptr, [[maybe_unused]] std::size_t bytes,
                       [[maybe_unused]] std::size_t alignment) override {
#ifdef EVOLAB_NUMA_SUPPORT
        if (numa_available_) {
            numa_free(ptr, bytes);
            return;
        }
#endif
        // Fallback to standard deallocation
        std::free(ptr);
    }

    /// Check if this resource is equal to another
    ///
    /// @param other The other memory resource to compare with
    /// @return True if resources are equivalent
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        const auto* other_numa = dynamic_cast<const NumaMemoryResource*>(&other);
        return other_numa != nullptr && other_numa->numa_node_ == numa_node_ &&
               other_numa->numa_available_ == numa_available_;
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
    static thread_local std::vector<std::unique_ptr<NumaMemoryResource>> island_resources;

    const int node_count = NumaMemoryResource::get_numa_node_count();
    if (node_count <= 1 || island_id < 0) {
        return std::pmr::get_default_resource();
    }

    // Map island to NUMA node (round-robin)
    const int numa_node = island_id % node_count;

    // Ensure we have enough resources
    if (island_resources.size() <= static_cast<size_t>(island_id)) {
        island_resources.resize(island_id + 1);
    }

    if (!island_resources[island_id]) {
        island_resources[island_id] = NumaMemoryResource::create_on_node(numa_node);
    }

    return island_resources[island_id].get();
}

} // namespace evolab::utils