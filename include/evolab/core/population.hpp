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
    ///
    /// @return Minimum capacity of both containers (actual usable pair slots)
    [[nodiscard]] std::size_t capacity() const noexcept {
        const auto gcap = genomes_.capacity();
        const auto fcap = fitness_.capacity();
        return gcap < fcap ? gcap : fcap;
    }

    /// Get the current number of individuals in the population
    [[nodiscard]] std::size_t size() const noexcept { return genomes_.size(); }

    /// Check if the population is empty
    [[nodiscard]] bool empty() const noexcept { return genomes_.empty(); }

    /// Reserve capacity for both arrays to avoid reallocations and keep capacities in sync
    ///
    /// @param new_cap Minimum capacity to reserve for both genome and fitness containers
    void reserve(std::size_t new_cap) {
        genomes_.reserve(new_cap);
        fitness_.reserve(new_cap);
    }

    /// Add an individual to the population
    ///
    /// @param genome The genome to add
    /// @param fitness The fitness value for this individual
    /// @throws std::exception Strong exception safety guarantee
    void push_back(const GenomeT& genome, Fitness fitness) {
        genomes_.push_back(genome);
        try {
            fitness_.push_back(fitness);
        } catch (...) {
            genomes_.pop_back(); // Restore invariant if fitness insertion fails
            throw;
        }
    }

    /// Add an individual to the population (move version)
    ///
    /// @param genome The genome to add (will be moved)
    /// @param fitness The fitness value for this individual
    /// @throws std::exception Strong exception safety guarantee
    void push_back(GenomeT&& genome, Fitness fitness) {
        genomes_.push_back(std::move(genome));
        try {
            fitness_.push_back(fitness);
        } catch (...) {
            genomes_.pop_back(); // Restore invariant if fitness insertion fails
            throw;
        }
    }

    /// Get reference to genome at specified index
    ///
    /// @param index Index of the individual (0-based)
    /// @return Reference to the genome
    [[nodiscard]] GenomeT& genome(std::size_t index) noexcept { return genomes_[index]; }

    /// Get const reference to genome at specified index
    ///
    /// @param index Index of the individual (0-based)
    /// @return Const reference to the genome
    [[nodiscard]] const GenomeT& genome(std::size_t index) const noexcept {
        return genomes_[index];
    }

    /// Get const reference to fitness value at specified index
    ///
    /// @param index Index of the individual (0-based)
    /// @return Const reference to the fitness value
    [[nodiscard]] const Fitness& fitness(std::size_t index) const noexcept {
        return fitness_[index];
    }

    /// Get mutable reference to fitness value at specified index
    ///
    /// @param index Index of the individual (0-based)
    /// @return Reference to the fitness value
    [[nodiscard]] Fitness& fitness(std::size_t index) noexcept { return fitness_[index]; }

    /// Get const reference to genome at specified index (with bounds checking)
    ///
    /// @param index Index of the individual (0-based)
    /// @return Const reference to the genome
    /// @throws std::out_of_range if index is out of bounds
    [[nodiscard]] const GenomeT& at_genome(std::size_t index) const { return genomes_.at(index); }

    /// Get mutable reference to genome at specified index (with bounds checking)
    ///
    /// @param index Index of the individual (0-based)
    /// @return Reference to the genome
    /// @throws std::out_of_range if index is out of bounds
    [[nodiscard]] GenomeT& at_genome(std::size_t index) { return genomes_.at(index); }

    /// Get const reference to fitness at specified index (with bounds checking)
    ///
    /// @param index Index of the individual (0-based)
    /// @return Const reference to the fitness value
    /// @throws std::out_of_range if index is out of bounds
    [[nodiscard]] const Fitness& at_fitness(std::size_t index) const { return fitness_.at(index); }

    /// Get mutable reference to fitness at specified index (with bounds checking)
    ///
    /// @param index Index of the individual (0-based)
    /// @return Reference to the fitness value
    /// @throws std::out_of_range if index is out of bounds
    [[nodiscard]] Fitness& at_fitness(std::size_t index) { return fitness_.at(index); }

    /// Get span over all genomes for batch operations and vectorization
    ///
    /// @return Span covering all genome data
    [[nodiscard]] std::span<GenomeT> genomes() { return {genomes_.data(), genomes_.size()}; }

    /// Get const span over all genomes for batch operations and vectorization
    ///
    /// @return Const span covering all genome data
    [[nodiscard]] std::span<const GenomeT> genomes() const {
        return {genomes_.data(), genomes_.size()};
    }

    /// Get span over all fitness values for batch operations and vectorization
    ///
    /// @return Span covering all fitness data
    [[nodiscard]] std::span<Fitness> fitness_values() { return {fitness_.data(), fitness_.size()}; }

    /// Get const span over all fitness values for batch operations and vectorization
    ///
    /// @return Const span covering all fitness data
    [[nodiscard]] std::span<const Fitness> fitness_values() const {
        return {fitness_.data(), fitness_.size()};
    }

    /// Clear all individuals from the population
    void clear() noexcept {
        genomes_.clear();
        fitness_.clear();
    }

    /// Resize population to specified size
    ///
    /// @param new_size New size for the population
    /// @throws std::exception Strong exception safety guarantee
    void resize(std::size_t new_size) {
        const auto old_size = genomes_.size();
        genomes_.resize(new_size);
        try {
            fitness_.resize(new_size);
        } catch (...) {
            genomes_.resize(old_size); // Restore original size if fitness resize fails
            throw;
        }
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
