#pragma once

/// @file population.hpp
/// @brief Population class with Structure-of-Arrays layout and PMR support for memory optimization
///
/// This header provides a Population class that separates genome and fitness storage for better
/// cache efficiency and vectorization support. Uses std::pmr for custom memory allocation
/// strategies.

#include <memory_resource>
#include <span>
#include <vector>

#include <evolab/core/concepts.hpp>

namespace evolab::core {

/// Population class with Structure-of-Arrays layout for optimal memory access patterns
///
/// This class stores genomes and fitness values in separate containers to enable:
/// - Better cache efficiency for fitness-only operations
/// - Vectorization-friendly memory layout
/// - Custom memory allocation strategies via PMR
/// - Pre-allocation to avoid reallocations during evolution
///
/// @tparam GenomeT The genome type (e.g., std::vector<int> for permutation problems)
template <typename GenomeT>
class Population {
  private:
    std::pmr::vector<GenomeT> genomes_;
    std::pmr::vector<Fitness> fitness_;

  public:
    /// Construct population with specified capacity and optional custom memory resource
    ///
    /// @param capacity Maximum number of individuals to store
    /// @param resource Custom memory resource for allocation (default: global default)
    explicit Population(std::size_t capacity,
                        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : genomes_(std::pmr::polymorphic_allocator<GenomeT>(resource)),
          fitness_(std::pmr::polymorphic_allocator<Fitness>(resource)) {
        // Pre-allocate memory to avoid reallocations during evolution
        genomes_.reserve(capacity);
        fitness_.reserve(capacity);
    }

    /// Get the maximum capacity of this population
    [[nodiscard]] std::size_t capacity() const noexcept { return genomes_.capacity(); }

    /// Get the current number of individuals in the population
    [[nodiscard]] std::size_t size() const noexcept { return genomes_.size(); }

    /// Check if the population is empty
    [[nodiscard]] bool empty() const noexcept { return genomes_.empty(); }

    /// Add an individual to the population
    ///
    /// @param genome The genome to add
    /// @param fitness The fitness value for this individual
    void push_back(const GenomeT& genome, Fitness fitness) {
        genomes_.push_back(genome);
        fitness_.push_back(fitness);
    }

    /// Add an individual to the population (move version)
    ///
    /// @param genome The genome to add (will be moved)
    /// @param fitness The fitness value for this individual
    void push_back(GenomeT&& genome, Fitness fitness) {
        genomes_.push_back(std::move(genome));
        fitness_.push_back(fitness);
    }

    /// Get reference to genome at specified index
    ///
    /// @param index Index of the individual (0-based)
    /// @return Reference to the genome
    [[nodiscard]] GenomeT& genome(std::size_t index) { return genomes_[index]; }

    /// Get const reference to genome at specified index
    ///
    /// @param index Index of the individual (0-based)
    /// @return Const reference to the genome
    [[nodiscard]] const GenomeT& genome(std::size_t index) const { return genomes_[index]; }

    /// Get const reference to fitness value at specified index
    ///
    /// @param index Index of the individual (0-based)
    /// @return Const reference to the fitness value
    [[nodiscard]] const Fitness& fitness(std::size_t index) const { return fitness_[index]; }

    /// Get mutable reference to fitness value at specified index
    ///
    /// @param index Index of the individual (0-based)
    /// @return Reference to the fitness value
    [[nodiscard]] Fitness& fitness(std::size_t index) { return fitness_[index]; }

    /// Get span over all genomes for batch operations and vectorization
    ///
    /// @return Span covering all genome data
    [[nodiscard]] std::span<GenomeT> genomes() { return std::span(genomes_); }

    /// Get const span over all genomes for batch operations and vectorization
    ///
    /// @return Const span covering all genome data
    [[nodiscard]] std::span<const GenomeT> genomes() const { return std::span(genomes_); }

    /// Get span over all fitness values for batch operations and vectorization
    ///
    /// @return Span covering all fitness data
    [[nodiscard]] std::span<Fitness> fitness_values() { return std::span(fitness_); }

    /// Get const span over all fitness values for batch operations and vectorization
    ///
    /// @return Const span covering all fitness data
    [[nodiscard]] std::span<const Fitness> fitness_values() const { return std::span(fitness_); }

    /// Clear all individuals from the population
    void clear() noexcept {
        genomes_.clear();
        fitness_.clear();
    }

    /// Resize population to specified size
    ///
    /// @param new_size New size for the population
    void resize(std::size_t new_size) {
        genomes_.resize(new_size);
        fitness_.resize(new_size);
    }

    /// Get the memory resource used by this population
    ///
    /// @return Pointer to the memory resource
    [[nodiscard]] std::pmr::memory_resource* get_memory_resource() const noexcept {
        return genomes_.get_allocator().resource();
    }

    // Iterator support removed - use index-based access with size() or spans for batch operations
};

} // namespace evolab::core
