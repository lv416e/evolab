#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace evolab {
namespace utils {

/// Candidate list for efficient nearest neighbor queries in TSP instances
class CandidateList {
  public:
    /// Create candidate list with k nearest neighbors for each city
    /// @param distance_matrix Row-major distance matrix
    /// @param k Number of nearest neighbors to maintain
    explicit CandidateList(const std::vector<std::vector<double>>& distance_matrix, int k)
        : n_(distance_matrix.size()), k_(k), candidates_(n_) {

        if (k_ <= 0 || k_ >= static_cast<int>(n_)) {
            k_ = static_cast<int>(n_) - 1; // Use all other cities if k is invalid
        }

        build_candidate_lists(distance_matrix);
    }

    /// Get k nearest neighbors for a given city
    /// @param city City index
    /// @return Vector of nearest neighbor indices sorted by distance
    const std::vector<int>& get_candidates(int city) const { return candidates_[city]; }

    /// Get number of cities
    int size() const { return static_cast<int>(n_); }

    /// Get k value (number of candidates per city)
    int k() const { return k_; }

    /// Check if two cities are candidates of each other
    bool are_mutual_candidates(int city1, int city2) const {
        const auto& candidates1 = candidates_[city1];
        const auto& candidates2 = candidates_[city2];

        bool city1_has_city2 =
            std::find(candidates1.begin(), candidates1.end(), city2) != candidates1.end();
        bool city2_has_city1 =
            std::find(candidates2.begin(), candidates2.end(), city1) != candidates2.end();

        return city1_has_city2 || city2_has_city1;
    }

    /// Get all candidate pairs for efficient iteration
    std::vector<std::pair<int, int>> get_all_candidate_pairs() const {
        std::vector<std::pair<int, int>> pairs;
        pairs.reserve(n_ * k_);

        for (int i = 0; i < static_cast<int>(n_); ++i) {
            for (int j : candidates_[i]) {
                if (i < j) { // Avoid duplicates by ensuring i < j
                    pairs.emplace_back(i, j);
                }
            }
        }

        return pairs;
    }

  private:
    void build_candidate_lists(const std::vector<std::vector<double>>& distance_matrix) {
        for (std::size_t i = 0; i < n_; ++i) {
            // Create vector of (distance, city_index) pairs
            std::vector<std::pair<double, int>> distances;
            distances.reserve(n_ - 1);

            for (std::size_t j = 0; j < n_; ++j) {
                if (i != j) {
                    distances.emplace_back(distance_matrix[i][j], static_cast<int>(j));
                }
            }

            // Sort by distance (ascending)
            std::sort(distances.begin(), distances.end());

            // Take k nearest neighbors
            candidates_[i].reserve(k_);
            for (int idx = 0; idx < std::min(k_, static_cast<int>(distances.size())); ++idx) {
                candidates_[i].push_back(distances[idx].second);
            }
        }
    }

    std::size_t n_;                            // Number of cities
    int k_;                                    // Number of candidates per city
    std::vector<std::vector<int>> candidates_; // Candidate lists for each city
};

/// Factory function to create candidate list with automatic k selection
/// @param distance_matrix Distance matrix
/// @param k_factor Multiplier for automatic k selection (k = k_factor * log(n))
inline CandidateList make_candidate_list(const std::vector<std::vector<double>>& distance_matrix,
                                         double k_factor = 2.0) {
    int n = static_cast<int>(distance_matrix.size());
    int k = std::max(5, static_cast<int>(k_factor * std::log(n))); // Minimum k=5
    k = std::min(k, n - 1);                                        // Maximum k=n-1
    return CandidateList(distance_matrix, k);
}

} // namespace utils
} // namespace evolab