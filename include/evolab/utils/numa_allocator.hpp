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
#include <limits>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <malloc.h> // for _aligned_malloc/_aligned_free
#endif

// Optional NUMA support - only include if available
#ifdef EVOLAB_NUMA_SUPPORT
#include <numa.h>
#include <numaif.h>
#include <sched.h>
#endif

namespace evolab::utils {

/// Thread-safe NUMA system detection
namespace detail {
/// Program-wide cached NUMA availability (one initialization across all TUs)
inline const bool kNumaAvailable = []() noexcept {
#ifdef EVOLAB_NUMA_SUPPORT
    return numa_available() >= 0;
#else
    return false;
#endif
}();

/// Thread-safe accessor
inline bool is_numa_system_available() noexcept {
    return kNumaAvailable;
}

/// Get list of available NUMA node IDs (handles sparse topologies)
///
/// @return Vector of actual NUMA node IDs available on the system
inline std::vector<int> get_available_numa_nodes() {
    std::vector<int> nodes;
#ifdef EVOLAB_NUMA_SUPPORT
    if (is_numa_system_available()) {
        // Check if we have multiple nodes before enumerating
        const int n = numa_num_configured_nodes();
        const int node_count = n > 0 ? n : 1;

        if (node_count > 1) {
            // Prefer the calling thread's allowed memory nodes
            if (auto* mask = numa_get_mems_allowed(); mask) {
                const int max_id = numa_max_node();
                for (int id = 0; id <= max_id; ++id) {
                    if (numa_bitmask_isbitset(mask, id)) {
                        nodes.push_back(id);
                    }
                }
                numa_bitmask_free(mask);
            } else if (numa_all_nodes_ptr) {
                const int max_id = numa_max_node();
                for (int id = 0; id <= max_id; ++id) {
                    if (numa_bitmask_isbitset(numa_all_nodes_ptr, id)) {
                        nodes.push_back(id);
                    }
                }
            }
        }
    }
#endif
    if (nodes.empty()) {
        nodes.push_back(0); // Fallback for UMA or if enumeration fails
    }
    return nodes;
}
} // namespace detail

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
/// std::pmr::vector<int> data{std::pmr::polymorphic_allocator<int>(local_resource.get())};
///
/// // Allocate on specific node
/// auto node_resource = NumaMemoryResource::create_on_node(1);
/// std::pmr::vector<double>
/// node_data{std::pmr::polymorphic_allocator<double>(node_resource.get())};
/// @endcode
class NumaMemoryResource : public std::pmr::memory_resource {
  private:
    int numa_node_;       ///< NUMA node ID (-1 for local node)
    bool numa_available_; ///< Whether NUMA is available on this system

    /// Allocation method for correct deallocation
    enum class DeallocationKind {
        Numa,       ///< NUMA-allocated memory (numa_free)
        WinAligned, ///< Windows _aligned_malloc (_aligned_free)
        StdFree     ///< Standard allocation (std::free)
    };

    /// Track allocation method and original pointer for proper deallocation
    ///
    /// ## Design Note: Mutex-Protected Allocation Tracking
    ///
    /// This implementation uses mutex-protected std::unordered_map for allocation tracking
    /// rather than lock-free inline headers for the following reasons:
    ///
    /// 1. **GA Usage Pattern**: Genetic algorithms typically allocate large memory blocks
    ///    (populations, fitness arrays) once at startup and deallocate at shutdown.
    ///    The allocation/deallocation frequency is very low compared to memory access.
    ///
    /// 2. **Safety First**: Mutex-protected approach eliminates complex pointer arithmetic,
    ///    alignment calculations, and potential memory corruption bugs that are common
    ///    with inline header approaches.
    ///
    /// 3. **NUMA Benefits**: The primary NUMA performance benefit comes from memory
    ///    locality during access (fitness evaluation, selection), not allocation speed.
    ///    System calls to numa_alloc_* typically dwarf mutex overhead.
    ///
    /// 4. **Maintainability**: Current approach is easier to debug, understand, and
    ///    modify. Allocation tracking is explicit and observable.
    ///
    /// Alternative lock-free implementations using inline allocation headers could
    /// be considered if profiling reveals mutex contention as a bottleneck in
    /// high-frequency allocation scenarios.
    struct AllocationInfo {
        DeallocationKind kind;
        void* original_ptr;         ///< Original pointer for over-allocated NUMA memory
        std::size_t original_bytes; ///< Original size for over-allocated NUMA memory
    };
    std::unordered_map<void*, AllocationInfo> allocations_;
    std::mutex allocations_mutex_;

  public:
    /// Constructor for NUMA memory resource
    ///
    /// @param node_id NUMA node ID (-1 for local node)
    explicit NumaMemoryResource(int node_id)
        : numa_node_(node_id), numa_available_(detail::is_numa_system_available()) {
#ifndef NDEBUG
        assert(node_id >= -1 && "node_id must be -1 (local) or a non-negative NUMA node id");
#endif
    }

    /// Destructor with debug assert for allocation tracking
    ~NumaMemoryResource() {
#ifndef NDEBUG
        std::lock_guard<std::mutex> lock(allocations_mutex_);
        assert(allocations_.empty() && "NumaMemoryResource destroyed with outstanding allocations");
#endif
    }

    /// Prevent accidental copies/moves that would duplicate allocation maps
    NumaMemoryResource(const NumaMemoryResource&) = delete;
    NumaMemoryResource& operator=(const NumaMemoryResource&) = delete;
    NumaMemoryResource(NumaMemoryResource&&) = delete;
    NumaMemoryResource& operator=(NumaMemoryResource&&) = delete;

    /// Create NUMA memory resource for local node
    ///
    /// @return Unique pointer to memory resource that allocates on local NUMA node
    [[nodiscard]] static std::unique_ptr<NumaMemoryResource> create_local() {
        return std::make_unique<NumaMemoryResource>(-1);
    }

    /// Create NUMA memory resource for specific node
    ///
    /// @param node_id The NUMA node ID to allocate on
    /// @return Unique pointer to memory resource that allocates on specified node
    [[nodiscard]] static std::unique_ptr<NumaMemoryResource> create_on_node(int node_id) {
#ifdef EVOLAB_NUMA_SUPPORT
        if (detail::is_numa_system_available()) {
            const auto nodes = detail::get_available_numa_nodes();
            bool valid = false;
            for (int n : nodes) {
                if (n == node_id) {
                    valid = true;
                    break;
                }
            }
            if (!valid) {
                assert(false && "Invalid NUMA node id; falling back to local resource");
                return std::make_unique<NumaMemoryResource>(-1);
            }
        }
#endif
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
        if (detail::is_numa_system_available()) {
            const int n = numa_num_configured_nodes();
            return n > 0 ? n : 1;
        }
#endif
        return 1;
    }

    /// Get current NUMA node for the calling thread
    ///
    /// @return Current NUMA node ID (0 if NUMA not available)
    static int get_current_numa_node() noexcept {
#ifdef EVOLAB_NUMA_SUPPORT
        if (detail::is_numa_system_available()) {
            const int cpu = sched_getcpu();
            if (cpu < 0)
                return 0; // Handle sched_getcpu() failure
            const int node = numa_node_of_cpu(cpu);
            return node >= 0 ? node : 0; // Handle numa_node_of_cpu() failure
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
        // Precondition check for std::align and aligned allocation functions
#ifndef NDEBUG
        assert((alignment > 0) && ((alignment & (alignment - 1)) == 0) &&
               "alignment must be a power of two");
#else
        if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
            throw std::bad_alloc{};
        }
#endif

        // Ensure minimum alignment and handle zero-byte allocations
        if (alignment < alignof(std::max_align_t)) {
            alignment = alignof(std::max_align_t);
        }
        if (bytes == 0) {
            bytes = alignment; // Prevent undefined behavior with zero-size allocations
        }

#ifdef EVOLAB_NUMA_SUPPORT
        if (numa_available_) {
            // Over-allocate to ensure we can find an aligned pointer (with overflow guard)
            const std::size_t overhead = alignment - 1;
            if (bytes > std::numeric_limits<std::size_t>::max() - overhead) {
                throw std::bad_alloc{};
            }
            std::size_t alloc_size = bytes + overhead;
            void* original_ptr = nullptr;

            // Treat any negative node ID as "local"
            if (numa_node_ < 0) {
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
                    std::unique_lock<std::mutex> lock(allocations_mutex_);
                    try {
                        allocations_.emplace(aligned_ptr, AllocationInfo{DeallocationKind::Numa,
                                                                         original_ptr, alloc_size});
                    } catch (...) {
                        lock.unlock();
                        numa_free(original_ptr, alloc_size);
                        throw;
                    }
                    return aligned_ptr;
                } else {
                    // Should not happen with proper over-allocation, but fallback
                    numa_free(original_ptr, alloc_size);
                }
            }
        }
#endif

        // Fallback to platform-specific aligned allocation
        void* ptr = nullptr;
        std::size_t alloc_size = bytes;
        DeallocationKind alloc_kind = DeallocationKind::StdFree;

#ifdef _WIN32
        // Windows: use _aligned_malloc (must be freed with _aligned_free)
        ptr = _aligned_malloc(bytes, alignment);
        alloc_kind = DeallocationKind::WinAligned;
#elif defined(__unix__) || defined(__APPLE__)
        // POSIX: use posix_memalign (freed with std::free)
        if (posix_memalign(&ptr, alignment, bytes) != 0) {
            ptr = nullptr;
        }
        alloc_kind = DeallocationKind::StdFree;
#else
        // C++17 fallback: std::aligned_alloc (requires size multiple of alignment)
        const std::size_t remainder = bytes % alignment;
        const std::size_t pad = (remainder == 0) ? 0 : (alignment - remainder);
        if (pad != 0 && bytes > (std::numeric_limits<std::size_t>::max)() - pad) {
            throw std::bad_alloc{};
        }
        alloc_size = bytes + pad;
        ptr = std::aligned_alloc(alignment, alloc_size);
        alloc_kind = DeallocationKind::StdFree;
#endif

        if (!ptr) {
            throw std::bad_alloc{};
        }

        std::unique_lock<std::mutex> lock(allocations_mutex_);
        try {
            allocations_.emplace(ptr, AllocationInfo{alloc_kind, ptr, alloc_size});
        } catch (...) {
            lock.unlock();
#ifdef _WIN32
            if (alloc_kind == DeallocationKind::WinAligned) {
                _aligned_free(ptr);
            } else
#endif
            {
                std::free(ptr);
            }
            throw;
        }
        return ptr;
    }

    /// Deallocate memory allocated by this resource
    ///
    /// @param ptr Pointer to memory to deallocate
    /// @param bytes Size of the allocation
    /// @param alignment Alignment used for allocation
    void do_deallocate(void* ptr, [[maybe_unused]] std::size_t bytes,
                       [[maybe_unused]] std::size_t alignment) noexcept override {
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
            // An unknown pointer indicates an allocator mismatch, which is a critical
            // logic error. Aborting prevents potential memory corruption or leaks.
            std::abort();
        }

        // Use appropriate deallocation based on allocation method
        switch (alloc_info.kind) {
#ifdef EVOLAB_NUMA_SUPPORT
        case DeallocationKind::Numa:
            numa_free(alloc_info.original_ptr, alloc_info.original_bytes);
            break;
#endif
        case DeallocationKind::StdFree:
            std::free(alloc_info.original_ptr);
            break;
#ifdef _WIN32
        case DeallocationKind::WinAligned:
            _aligned_free(alloc_info.original_ptr);
            break;
#endif
        default:
            // This should never happen if allocation tracking is correct
            std::abort();
        }
    }

    /// Check if this resource is equal to another
    ///
    /// Uses pointer identity for equality to ensure memory safety with stateful allocators.
    /// Two NumaMemoryResource instances with separate allocation tracking cannot safely
    /// deallocate each other's memory, even if they target the same NUMA node.
    ///
    /// @param other The other memory resource to compare with
    /// @return True if resources are equivalent (same instance)
    ///
    /// @note A configuration-based equality (same numa_node_) would only be safe with
    ///       stateless allocators or shared allocation tracking between instances.
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
/// ## ⚠️  CRITICAL THREAD-LOCAL LIFETIME WARNING ⚠️
/// **DANGER**: Returns a pointer to a thread_local resource that is destroyed when
/// the creating thread exits. Using this resource after thread termination causes
/// undefined behavior, memory corruption, or crashes.
///
/// **SAFE USAGE**: Only use within the same thread that called this function.
/// **UNSAFE**: Passing to other threads, storing globally, or using after thread exit.
///
/// **For cross-thread or long-lived usage**, use create_owned_optimized_ga_resource()
/// or construct resources directly via NumaMemoryResource::create_local().
///
/// @return Memory resource optimized for the current system (thread-local lifetime)
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
/// ## Memory Usage Note:
/// This function uses a thread_local cache indexed by NUMA node (not island_id).
/// Cache size is bounded by the number of NUMA nodes in the system (typically 1-8).
/// Large island_id values are handled via modulo mapping and sanity checking.
///
/// ## NUMA Topology Assumption:
/// Uses round-robin mapping (island_id % numa_node_count) which assumes islands
/// should be distributed evenly across NUMA nodes. Verify this matches your
/// application's locality requirements and system topology.
///
/// @param island_id Island identifier (maps to NUMA node via round-robin).
///                  Negative values return default resource.
///                  Note: cache size is bounded by NUMA node count; large IDs are modulo-mapped.
/// ## ⚠️  CRITICAL THREAD-LOCAL LIFETIME WARNING ⚠️
/// **DANGER**: Returns a pointer to a thread_local resource that is destroyed when
/// the creating thread exits. Using this resource after thread termination causes
/// undefined behavior, memory corruption, or crashes.
///
/// **SAFE USAGE**: Only use within the same thread that called this function.
/// **UNSAFE**: Passing to other threads, storing globally, or using after thread exit.
///
/// **For cross-thread or persistent usage**, use create_owned_island_resource()
/// or construct resources directly via NumaMemoryResource::create_on_node().
///
/// @return Memory resource for the specified island/node (thread-local lifetime)
inline std::pmr::memory_resource* create_island_resource(int island_id) {
    // Thread-local cache indexed by NUMA node (bounded by system's NUMA node count)
    static thread_local std::unordered_map<int, std::unique_ptr<NumaMemoryResource>>
        numa_node_resource_cache;

    // Recompute each call to respect current affinity/topology
    const auto available_nodes = detail::get_available_numa_nodes();

    const int node_count = static_cast<int>(available_nodes.size());
    if (node_count <= 1 || island_id < 0) {
        return std::pmr::get_default_resource();
    }

    // Safeguard against misuse with unexpectedly large island IDs, which might indicate a logic
    // error
    constexpr int MAX_ISLAND_ID_FOR_SANITY_CHECK = 10000;
    if (island_id > MAX_ISLAND_ID_FOR_SANITY_CHECK) {
        assert(false && "Island ID exceeds sanity check limit; falling back to default allocator.");
        return std::pmr::get_default_resource();
    }

    // Map island to actual NUMA node (round-robin over available nodes)
    const int numa_node = available_nodes[island_id % node_count];

    // Cache by NUMA node (map size naturally bounded by available_nodes.size())
    auto& resource = numa_node_resource_cache[numa_node];
    if (!resource) {
        resource = NumaMemoryResource::create_on_node(numa_node);
    }

    return resource.get();
}

// =============================================================================
// Safe Ownership Alternatives for Cross-Thread Usage
// =============================================================================
// These functions return owned resources that can safely cross thread boundaries
// and have controlled lifetimes, eliminating the thread-local dangling pointer risk.

/// Create owned memory resource for cross-thread or long-lived GA usage
///
/// **THREAD-SAFE**: Returns an owned resource that can be safely used across
/// threads or stored for long-term usage without lifetime concerns.
///
/// This is the safe alternative to create_optimized_ga_resource() when you need:
/// - Cross-thread resource sharing
/// - Long-lived containers that outlive the creating thread
/// - Deterministic resource lifetime management
///
/// @return Owned memory resource (nullptr indicates use std::pmr::get_default_resource())
/// @see create_optimized_ga_resource() for thread-local alternative
[[nodiscard]] inline std::unique_ptr<NumaMemoryResource> create_owned_optimized_ga_resource() {
    if (NumaMemoryResource::get_numa_node_count() > 1) {
        // Multi-node system: return owned local NUMA resource
        return NumaMemoryResource::create_local();
    }
    // Single-node system: return nullptr to indicate use of default resource
    return nullptr;
}

/// Create owned memory resource for cross-thread island model usage
///
/// **THREAD-SAFE**: Returns an owned resource that can be safely used across
/// threads or stored for long-term usage without lifetime concerns.
///
/// This is the safe alternative to create_island_resource() when you need:
/// - Cross-thread resource sharing between islands
/// - Long-lived island containers
/// - Deterministic resource lifetime management
///
/// @param island_id Island identifier (maps to NUMA node via round-robin)
/// @return Owned memory resource (nullptr indicates use std::pmr::get_default_resource())
/// @see create_island_resource() for thread-local alternative
[[nodiscard]] inline std::unique_ptr<NumaMemoryResource>
create_owned_island_resource(int island_id) {
    const int node_count = NumaMemoryResource::get_numa_node_count();
    if (node_count <= 1 || island_id < 0) {
        return nullptr; // Use default resource
    }

    // Recompute available nodes each time to respect current thread affinity/topology
    const auto available = detail::get_available_numa_nodes();
    if (available.size() > 1) {
        const int node = available[island_id % static_cast<int>(available.size())];
        return NumaMemoryResource::create_on_node(node);
    }

    return nullptr; // Fallback to default resource
}

/// Helper to get memory resource from owned resource or default
///
/// Convenience function to handle the owned resource pattern where nullptr
/// indicates the caller should use std::pmr::get_default_resource().
///
/// @param owned_resource The owned resource (may be nullptr)
/// @return Valid memory resource pointer (never nullptr)
[[nodiscard]] inline std::pmr::memory_resource*
get_resource_or_default(const std::unique_ptr<NumaMemoryResource>& owned_resource) {
    return owned_resource ? owned_resource.get() : std::pmr::get_default_resource();
}

} // namespace evolab::utils
