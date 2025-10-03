#pragma once

/// @file tsp.hpp
/// @brief Traveling Salesman Problem implementation with modern C++23 optimizations
///
/// This header provides a high-performance TSP class supporting TSPLIB instances,
/// candidate lists, and various distance calculation methods. Designed for research-grade
/// metaheuristics with focus on cache efficiency and algorithmic performance.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <vector>

// EvoLab dependencies - core concepts and supporting utilities
#include <evolab/core/concepts.hpp>        // Type constraints for problem interface
#include <evolab/io/tsplib.hpp>            // TSPLIB file format support
#include <evolab/utils/candidate_list.hpp> // Performance optimization for local search
#include <evolab/utils/compiler_hints.hpp> // Branch prediction hints
#include <evolab/utils/distance_cache.hpp> // Distance lookup cache

namespace evolab::problems {

/// Traveling Salesman Problem implementation
class TSP {
  public:
    using Gene = int;
    using GenomeT = std::vector<int>;

  private:
    int n_;
    std::vector<double> distances_; // Row-major: dist[i*n + j]
    mutable std::optional<utils::CandidateList> candidate_list_;
    mutable utils::DistanceCache<double> distance_cache_; // Cache for local search

  public:
    TSP() = default;

    /// Construct TSP with distance matrix
    TSP(int n, std::vector<double> distances) : n_(n), distances_(std::move(distances)) {
        assert(distances_.size() == static_cast<std::size_t>(n * n));
    }

    /// Construct TSP from city coordinates (Euclidean distances)
    TSP(const std::vector<std::pair<double, double>>& cities)
        : n_(static_cast<int>(cities.size())), distances_(cities.size() * cities.size()) {

        for (int i = 0; i < n_; ++i) {
            for (int j = 0; j < n_; ++j) {
                if (i == j) {
                    distances_[i * n_ + j] = 0.0;
                } else {
                    double dx = cities[i].first - cities[j].first;
                    double dy = cities[i].second - cities[j].second;
                    distances_[i * n_ + j] = std::sqrt(dx * dx + dy * dy);
                }
            }
        }
    }

    /// Create TSP from TSPLIB instance
    /// Factory method that creates a TSP problem from a parsed TSPLIB instance
    /// This enables integration with the standard TSPLIB test suite
    static TSP from_tsplib(const io::TSPInstance& instance) {
        // Validate that this is actually a TSP problem (not ATSP, HCP, or SOP)
        if (instance.type != io::TSPType::TSP) {
            std::string type_name;
            switch (instance.type) {
            case io::TSPType::ATSP:
                type_name = "ATSP (Asymmetric TSP)";
                break;
            case io::TSPType::HCP:
                type_name = "HCP (Hamiltonian Cycle Problem)";
                break;
            case io::TSPType::SOP:
                type_name = "SOP (Sequential Ordering Problem)";
                break;
            default:
                type_name = "Unknown";
                break;
            }
            throw io::TSPLIBDataError(
                "Invalid problem type for TSP solver. Expected TSP but got: " + type_name);
        }

        // Validate dimension
        if (instance.dimension <= 0) {
            throw io::TSPLIBDataError("Invalid TSP dimension: " +
                                      std::to_string(instance.dimension));
        }

        // Validate that we have either coordinates or explicit distance matrix
        if (instance.edge_weight_type != io::EdgeWeightType::EXPLICIT &&
            instance.node_coords.empty()) {
            throw io::TSPLIBDataError(
                "TSP instance has neither node coordinates nor explicit distance matrix");
        }

        // Get distance matrix and create TSP instance
        auto distances = instance.get_full_distance_matrix();
        return TSP(instance.dimension, std::move(distances));
    }

    /// Get distance between two cities
    double distance(int i, int j) const noexcept {
        assert(i >= 0 && i < n_ && j >= 0 && j < n_);
        return distances_[i * n_ + j];
    }

    /// Get distance with cache (for local search hot paths)
    /// Significantly reduces memory latency in tight loops
    /// Canonicalizes indices for symmetric TSP to improve cache hit rate
    double cached_distance(int i, int j) const noexcept {
        // Canonicalize indices for symmetric TSP: ensure i <= j
        // This doubles the cache hit rate by treating (i,j) and (j,i) as same entry
        if (i > j)
            std::swap(i, j);

        double value;
        if (EVOLAB_LIKELY(distance_cache_.try_get(i, j, value))) {
            return value;
        }
        value = distances_[i * n_ + j];
        distance_cache_.put(i, j, value);
        return value;
    }

    /// Get problem size (number of cities)
    std::size_t size() const noexcept { return n_; }

    /// Evaluate a tour (lower is better)
    core::Fitness evaluate(const GenomeT& tour) const {
        assert(static_cast<int>(tour.size()) == n_);

        double total_distance = 0.0;
        for (int i = 0; i < n_; ++i) {
            int from = tour[i];
            int to = tour[(i + 1) % n_];
            total_distance += distance(from, to);
        }

        return core::Fitness{total_distance};
    }

    /// Generate a random valid tour
    GenomeT random_genome(std::mt19937& rng) const {
        GenomeT tour(n_);
        std::iota(tour.begin(), tour.end(), 0);
        std::shuffle(tour.begin(), tour.end(), rng);
        return tour;
    }

    /// Create identity permutation (0, 1, 2, ..., n-1)
    GenomeT identity_genome() const {
        GenomeT tour(n_);
        std::iota(tour.begin(), tour.end(), 0);
        return tour;
    }

    /// Check if a tour is valid (proper permutation)
    bool is_valid_tour(const GenomeT& tour) const {
        if (static_cast<int>(tour.size()) != n_)
            return false;

        std::vector<bool> visited(n_, false);
        for (int city : tour) {
            if (city < 0 || city >= n_ || visited[city]) {
                return false;
            }
            visited[city] = true;
        }
        return true;
    }

    /// Get number of cities
    int num_cities() const noexcept { return n_; }

    /// Get raw distance matrix (for advanced algorithms)
    const std::vector<double>& distance_matrix() const noexcept { return distances_; }

    /// Calculate 2-opt gain for edge swap (for local search)
    double two_opt_gain(const GenomeT& tour, int i, int j) const {
        assert(i >= 0 && i < n_ && j >= 0 && j < n_ && i != j);

        // Ensure i < j for consistency
        if (i > j)
            std::swap(i, j);

        // Current edges: (tour[i], tour[i+1]) and (tour[j], tour[(j+1)%n])
        // New edges after 2-opt: (tour[i], tour[j]) and (tour[i+1], tour[(j+1)%n])

        int city_i = tour[i];
        int city_i_next = tour[(i + 1) % n_];
        int city_j = tour[j];
        int city_j_next = tour[(j + 1) % n_];

        double old_distance = distance(city_i, city_i_next) + distance(city_j, city_j_next);
        double new_distance = distance(city_i, city_j) + distance(city_i_next, city_j_next);

        // Positive gain means improvement
        return old_distance - new_distance;
    }

    /// Optimized 2-opt gain with caching and branch prediction hints
    /// Uses distance cache to reduce memory latency in hot loops
    EVOLAB_FORCE_INLINE double two_opt_gain_cached(const GenomeT& tour, int i,
                                                   int j) const noexcept {
        // Ensure i < j for consistency
        if (EVOLAB_UNLIKELY(i > j)) {
            std::swap(i, j);
        }

        const int city_i = tour[i];
        const int city_i_next = tour[(i + 1) % n_];
        const int city_j = tour[j];
        const int city_j_next = tour[(j + 1) % n_];

        // Use cached distances for reduced memory latency
        const double old_dist =
            cached_distance(city_i, city_i_next) + cached_distance(city_j, city_j_next);
        const double new_dist =
            cached_distance(city_i, city_j) + cached_distance(city_i_next, city_j_next);

        return old_dist - new_dist;
    }

    /// Apply 2-opt move
    void apply_two_opt(GenomeT& tour, int i, int j) const {
        if (i > j)
            std::swap(i, j);
        std::reverse(tour.begin() + i + 1, tour.begin() + j + 1);
    }

    /// Clear distance cache (call before starting new local search)
    void clear_distance_cache() const noexcept { distance_cache_.clear(); }

    /// Get cache hit rate for performance monitoring
    double cache_hit_rate() const noexcept { return distance_cache_.hit_rate(); }

    /// Get cache statistics (hits, misses)
    std::pair<std::size_t, std::size_t> cache_stats() const noexcept {
        return distance_cache_.stats();
    }

    /// Create candidate list for efficient local search
    /// @param k Number of nearest neighbors to maintain per city
    void create_candidate_list(int k = 20) const;

    /// Get candidate list (creates it if needed)
    const utils::CandidateList* get_candidate_list(int k = 20) const;

    /// Check if candidate list exists
    bool has_candidate_list() const { return candidate_list_.has_value(); }

    /// Convert distance matrix to 2D format for candidate list creation
    std::vector<std::vector<double>> get_distance_matrix_2d() const {
        std::vector<std::vector<double>> matrix_2d(n_, std::vector<double>(n_));
        for (int i = 0; i < n_; ++i) {
            for (int j = 0; j < n_; ++j) {
                matrix_2d[i][j] = distances_[i * n_ + j];
            }
        }
        return matrix_2d;
    }
};

// Implementations for candidate list methods
inline void TSP::create_candidate_list(int k) const {
    auto matrix_2d = get_distance_matrix_2d();
    candidate_list_ = utils::CandidateList(matrix_2d, k);
}

inline const utils::CandidateList* TSP::get_candidate_list(int k) const {
    if (!candidate_list_.has_value() || candidate_list_->k() != k) {
        create_candidate_list(k);
    }
    return &candidate_list_.value();
}

/// Create random TSP instance
inline TSP create_random_tsp(int n, double max_coord = 1000.0, std::uint64_t seed = 1) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(0.0, max_coord);

    std::vector<std::pair<double, double>> cities;
    cities.reserve(n);

    for (int i = 0; i < n; ++i) {
        cities.emplace_back(dist(rng), dist(rng));
    }

    return TSP(cities);
}

} // namespace evolab::problems
