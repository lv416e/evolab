#pragma once

#include "../core/concepts.hpp"
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

namespace evolab::operators {

/// Partially Mapped Crossover (PMX) for permutations
class PMXCrossover {
public:
    template<core::Problem P>
    std::pair<typename P::GenomeT, typename P::GenomeT> cross(
        const P& problem,
        const typename P::GenomeT& parent1,
        const typename P::GenomeT& parent2,
        std::mt19937& rng) const {
        
        using GenomeT = typename P::GenomeT;
        
        assert(parent1.size() == parent2.size());
        const std::size_t n = parent1.size();
        
        if (n <= 2) {
            return {parent1, parent2};  // Too small for meaningful crossover
        }
        
        // Choose crossover points
        std::uniform_int_distribution<std::size_t> dist(0, n - 1);
        std::size_t point1 = dist(rng);
        std::size_t point2 = dist(rng);
        if (point1 > point2) std::swap(point1, point2);
        
        GenomeT child1 = parent1;
        GenomeT child2 = parent2;
        
        // Create mapping from parent2 to parent1 in the crossover segment
        std::unordered_map<typename P::Gene, typename P::Gene> mapping1, mapping2;
        
        for (std::size_t i = point1; i <= point2; ++i) {
            if (parent1[i] != parent2[i]) {
                mapping1[parent2[i]] = parent1[i];
                mapping2[parent1[i]] = parent2[i];
            }
        }
        
        // Copy crossover segment directly
        for (std::size_t i = point1; i <= point2; ++i) {
            child1[i] = parent2[i];
            child2[i] = parent1[i];
        }
        
        // Fix conflicts outside crossover segment
        auto fix_conflicts = [](GenomeT& child, const std::unordered_map<typename P::Gene, typename P::Gene>& mapping) {
            for (std::size_t i = 0; i < child.size(); ++i) {
                auto current = child[i];
                while (mapping.find(current) != mapping.end()) {
                    current = mapping.at(current);
                }
                child[i] = current;
            }
        };
        
        fix_conflicts(child1, mapping1);
        fix_conflicts(child2, mapping2);
        
        return {std::move(child1), std::move(child2)};
    }
};

/// Order Crossover (OX) for permutations
class OrderCrossover {
public:
    template<core::Problem P>
    std::pair<typename P::GenomeT, typename P::GenomeT> cross(
        const P& problem,
        const typename P::GenomeT& parent1,
        const typename P::GenomeT& parent2,
        std::mt19937& rng) const {
        
        using GenomeT = typename P::GenomeT;
        using Gene = typename P::Gene;
        
        assert(parent1.size() == parent2.size());
        const std::size_t n = parent1.size();
        
        if (n <= 2) {
            return {parent1, parent2};
        }
        
        // Choose crossover points
        std::uniform_int_distribution<std::size_t> dist(0, n - 1);
        std::size_t point1 = dist(rng);
        std::size_t point2 = dist(rng);
        if (point1 > point2) std::swap(point1, point2);
        
        auto create_child = [&](const GenomeT& p1, const GenomeT& p2) {
            GenomeT child(n);
            
            // Copy segment from first parent
            std::unordered_set<Gene> used;
            for (std::size_t i = point1; i <= point2; ++i) {
                child[i] = p1[i];
                used.insert(p1[i]);
            }
            
            // Fill remaining positions with elements from second parent in order
            std::size_t child_pos = (point2 + 1) % n;
            for (std::size_t i = 0; i < n; ++i) {
                std::size_t parent_pos = (point2 + 1 + i) % n;
                Gene gene = p2[parent_pos];
                
                if (used.find(gene) == used.end()) {
                    child[child_pos] = gene;
                    child_pos = (child_pos + 1) % n;
                }
            }
            
            return child;
        };
        
        return {create_child(parent1, parent2), create_child(parent2, parent1)};
    }
};

/// Cycle Crossover (CX) for permutations
class CycleCrossover {
public:
    template<core::Problem P>
    std::pair<typename P::GenomeT, typename P::GenomeT> cross(
        const P& problem,
        const typename P::GenomeT& parent1,
        const typename P::GenomeT& parent2,
        std::mt19937& rng) const {
        
        using GenomeT = typename P::GenomeT;
        using Gene = typename P::Gene;
        
        assert(parent1.size() == parent2.size());
        const std::size_t n = parent1.size();
        
        if (n <= 1) {
            return {parent1, parent2};
        }
        
        GenomeT child1 = parent2;  // Start with parent2
        GenomeT child2 = parent1;  // Start with parent1
        
        std::vector<bool> visited(n, false);
        
        for (std::size_t start = 0; start < n; ++start) {
            if (visited[start]) continue;
            
            // Trace cycle starting from position 'start'
            std::size_t pos = start;
            bool use_parent1 = (std::uniform_int_distribution<int>(0, 1)(rng) == 0);
            
            do {
                visited[pos] = true;
                
                if (use_parent1) {
                    child1[pos] = parent1[pos];
                    child2[pos] = parent2[pos];
                } else {
                    child1[pos] = parent2[pos];
                    child2[pos] = parent1[pos];
                }
                
                // Find next position in cycle
                Gene target = parent2[pos];
                pos = std::find(parent1.begin(), parent1.end(), target) - parent1.begin();
                
            } while (!visited[pos] && pos < n);
        }
        
        return {std::move(child1), std::move(child2)};
    }
};

/// Edge Recombination Crossover for TSP-like problems
class EdgeRecombinationCrossover {
public:
    template<core::Problem P>
    std::pair<typename P::GenomeT, typename P::GenomeT> cross(
        const P& problem,
        const typename P::GenomeT& parent1,
        const typename P::GenomeT& parent2,
        std::mt19937& rng) const {
        
        using GenomeT = typename P::GenomeT;
        using Gene = typename P::Gene;
        
        assert(parent1.size() == parent2.size());
        const std::size_t n = parent1.size();
        
        if (n <= 2) {
            return {parent1, parent2};
        }
        
        auto create_child = [&](const GenomeT& p1, const GenomeT& p2) {
            // Build edge table
            std::unordered_map<Gene, std::unordered_set<Gene>> edges;
            
            // Add edges from both parents
            for (std::size_t i = 0; i < n; ++i) {
                Gene current1 = p1[i];
                Gene next1 = p1[(i + 1) % n];
                Gene prev1 = p1[(i + n - 1) % n];
                
                edges[current1].insert(next1);
                edges[current1].insert(prev1);
                
                Gene current2 = p2[i];
                Gene next2 = p2[(i + 1) % n];
                Gene prev2 = p2[(i + n - 1) % n];
                
                edges[current2].insert(next2);
                edges[current2].insert(prev2);
            }
            
            GenomeT child;
            child.reserve(n);
            
            std::unordered_set<Gene> used;
            
            // Start with random city
            std::uniform_int_distribution<std::size_t> start_dist(0, n - 1);
            Gene current = p1[start_dist(rng)];
            child.push_back(current);
            used.insert(current);
            
            while (child.size() < n) {
                // Remove current city from all edge lists
                for (auto& [city, adj] : edges) {
                    adj.erase(current);
                }
                
                // Find next city
                Gene next;
                bool found = false;
                
                // Look for city with minimum available edges
                int min_edges = INT_MAX;
                std::vector<Gene> candidates;
                
                for (Gene neighbor : edges[current]) {
                    if (used.find(neighbor) == used.end()) {
                        int edge_count = static_cast<int>(edges[neighbor].size());
                        if (edge_count < min_edges) {
                            min_edges = edge_count;
                            candidates.clear();
                            candidates.push_back(neighbor);
                        } else if (edge_count == min_edges) {
                            candidates.push_back(neighbor);
                        }
                    }
                }
                
                if (!candidates.empty()) {
                    std::uniform_int_distribution<std::size_t> cand_dist(0, candidates.size() - 1);
                    next = candidates[cand_dist(rng)];
                    found = true;
                }
                
                // If no connected city available, choose random unused city
                if (!found) {
                    std::vector<Gene> unused;
                    for (const auto& city : p1) {
                        if (used.find(city) == used.end()) {
                            unused.push_back(city);
                        }
                    }
                    
                    if (!unused.empty()) {
                        std::uniform_int_distribution<std::size_t> unused_dist(0, unused.size() - 1);
                        next = unused[unused_dist(rng)];
                    }
                }
                
                child.push_back(next);
                used.insert(next);
                current = next;
            }
            
            return child;
        };
        
        return {create_child(parent1, parent2), create_child(parent2, parent1)};
    }
};

/// Uniform Crossover for any representation
class UniformCrossover {
    double probability_;
    
public:
    explicit UniformCrossover(double prob = 0.5) : probability_(prob) {
        assert(prob >= 0.0 && prob <= 1.0);
    }
    
    template<core::Problem P>
    std::pair<typename P::GenomeT, typename P::GenomeT> cross(
        const P& problem,
        const typename P::GenomeT& parent1,
        const typename P::GenomeT& parent2,
        std::mt19937& rng) const {
        
        using GenomeT = typename P::GenomeT;
        
        assert(parent1.size() == parent2.size());
        
        GenomeT child1 = parent1;
        GenomeT child2 = parent2;
        
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        
        for (std::size_t i = 0; i < parent1.size(); ++i) {
            if (dist(rng) < probability_) {
                std::swap(child1[i], child2[i]);
            }
        }
        
        return {std::move(child1), std::move(child2)};
    }
    
    double probability() const { return probability_; }
};

} // namespace evolab::operators