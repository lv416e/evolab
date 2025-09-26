#pragma once

/// @file selection.hpp
/// @brief Selection operators for evolutionary algorithms with modern C++23 features
///
/// This header implements various selection strategies including tournament, roulette wheel,
/// and rank-based selection. All operators use concepts for compile-time type validation.

#include <algorithm>
#include <cassert>
#include <exception> // for std::terminate in fallback
#include <numeric>
#include <random>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

// EvoLab core concepts - fundamental type requirements for selection operators
#include <evolab/core/concepts.hpp>

namespace evolab::operators {

/// @private
namespace detail {
/// Portable unreachable code marker using C++23 std::unreachable when available
[[noreturn]] inline void unreachable() noexcept {
#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
    std::unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#else
    // Fallback: terminate to maintain defined behavior
    std::terminate();
#endif
}
} // namespace detail

/// Tournament selection operator implementing competitive selection strategy
///
/// Tournament selection works by randomly selecting k individuals from the population
/// and choosing the best (lowest fitness for minimization problems) among them.
/// This creates selection pressure while maintaining diversity in the population.
///
/// ## API Compatibility (v2.0+)
/// This class implements the modern SelectionOperator concept using `std::span<const Fitness>`.
/// For migration from v1.x implementations, see SelectionOperator concept documentation.
///
/// ## Algorithm:
/// 1. Randomly select `tournament_size` individuals from the fitness span
/// 2. Return the index of the individual with the best (lowest) fitness
/// 3. Handle edge cases: single individual tournaments, empty populations
///
/// ## Performance Characteristics:
/// - Time Complexity: O(tournament_size)
/// - Space Complexity: O(1)
/// - Cache Friendly: Sequential access to fitness values
///
/// ## Example Usage:
/// ```cpp
/// // Create tournament selector with size 4
/// TournamentSelection selector(4);
///
/// // Use with fitness span (v2.0+ API)
/// std::vector<Fitness> fitnesses = {Fitness{10.0}, Fitness{5.0}, Fitness{15.0}};
/// std::mt19937 rng(42);
/// std::size_t selected = selector.select(fitnesses, rng);
///
/// // Integration with GeneticAlgorithm
/// auto ga = make_ga(TournamentSelection(3), crossover, mutation);
/// ```
///
/// @see SelectionOperator concept for interface requirements
/// @see RouletteWheelSelection, RankSelection for alternative strategies
/// @since v2.0.0 (modernized API)
class TournamentSelection {
    std::size_t tournament_size_;

  public:
    explicit TournamentSelection(std::size_t tournament_size = 4)
        // Explicitly specify template parameter for clarity in constructor context
        : tournament_size_(std::max<std::size_t>(1, tournament_size)) {
        // Constructor parameter type is already guaranteed by signature
    }

    /// Select an individual from the population using tournament selection
    ///
    /// Performs tournament selection by randomly choosing `tournament_size` individuals
    /// from the fitness span and returning the index of the best (lowest fitness) individual.
    /// This method implements the core selection pressure mechanism for genetic algorithms.
    ///
    /// ## Algorithm Details:
    /// 1. Randomly select the first tournament participant
    /// 2. For each remaining tournament slot:
    ///    - Randomly select another participant
    ///    - Compare fitness with current best
    /// 3. Return index of participant with lowest fitness value
    ///
    /// ## Performance Characteristics:
    /// - **Time Complexity**: O(tournament_size) - linear in tournament size
    /// - **Space Complexity**: O(1) - constant additional memory
    /// - **Memory Access**: Random access to fitness span (cache-friendly for small tournaments)
    /// - **RNG Calls**: Exactly `tournament_size` random number generations
    ///
    /// ## Thread Safety:
    /// This method is thread-safe for concurrent reads of the same fitness span,
    /// but requires separate RNG instances per thread to avoid data races.
    ///
    /// @param fitnesses Span of fitness values for all individuals in the population.
    ///                  Must be non-empty and contain valid Fitness objects.
    ///                  The span provides zero-copy access to fitness data.
    /// @param rng Reference to MT19937 random number generator.
    ///            Generator state will be modified by this call.
    ///            Use thread_local generators for parallel execution.
    ///
    /// @return Index of the selected individual (0 <= index < fitnesses.size()).
    ///         Returns 0 immediately if population contains only one individual.
    ///
    /// @pre fitnesses.size() >= 1 (enforced by assertion)
    /// @post 0 <= return_value < fitnesses.size()
    ///
    /// @throws None This method is noexcept in practice but may assert on invalid input
    ///
    /// ## Example Usage:
    /// ```cpp
    /// std::vector<Fitness> population_fitness = {
    ///     Fitness{10.5}, Fitness{5.2}, Fitness{8.1}, Fitness{12.0}
    /// };
    /// std::mt19937 rng(std::random_device{}());
    /// TournamentSelection selector(3);  // Tournament size 3
    ///
    /// std::size_t selected = selector.select(population_fitness, rng);
    /// // 'selected' is index of individual with best fitness among 3 random contestants
    /// ```
    ///
    /// @see SelectionOperator concept for interface requirements
    /// @see tournament_size() for accessing the tournament size parameter
    /// @since v2.0.0 (span-based interface)
    std::size_t select(std::span<const core::Fitness> fitnesses, std::mt19937& rng) const {

        assert(!fitnesses.empty());
        // Precondition enforced by assert; no runtime branch needed
        if (fitnesses.size() == 1) {
            return 0;
        }

        std::uniform_int_distribution<std::size_t> dist(0, fitnesses.size() - 1);

        std::size_t best_idx = dist(rng);
        core::Fitness best_fitness = fitnesses[best_idx];

        for (std::size_t i = 1; i < tournament_size_; ++i) {
            std::size_t candidate = dist(rng);
            if (fitnesses[candidate] < best_fitness) {
                best_idx = candidate;
                best_fitness = fitnesses[candidate];
            }
        }

        return best_idx;
    }

    /// Get the tournament size parameter
    ///
    /// @return Number of individuals participating in each tournament.
    ///         Always ≥ 1 due to constructor validation.
    std::size_t tournament_size() const { return tournament_size_; }
};

/// Roulette wheel selection operator with fitness-proportionate selection probability
///
/// Roulette wheel selection assigns selection probability proportional to fitness quality.
/// Since this implementation assumes minimization problems, fitness values are inverted
/// to create positive weights where better (lower) fitness values get higher selection probability.
///
/// ## API Compatibility (v2.0+)
/// This class implements the modern SelectionOperator concept using `std::span<const Fitness>`.
/// For migration from v1.x implementations, see SelectionOperator concept documentation.
///
/// ## Algorithm:
/// 1. Calculate fitness range (max - min) to handle negative fitness values
/// 2. Convert minimization fitness to maximization weights: weight = (max_fitness - fitness + 1)
/// 3. Calculate total weight sum for normalization
/// 4. Generate random target value in [0, total_weight)
/// 5. Find individual whose cumulative weight exceeds target
///
/// ## Performance Characteristics:
/// - Time Complexity: O(n) where n = population size
/// - Space Complexity: O(1) - no intermediate storage of weights
/// - Memory Access: Two sequential passes through fitness data
/// - Cache Efficiency: Excellent due to sequential access pattern
///
/// ## Edge Case Handling:
/// - **Uniform fitness**: When all fitness values are equal (range = 0), uniform selection
/// - **Single individual**: Returns index 0 immediately
/// - **Negative fitness**: Properly handled by range-based weight calculation
///
/// ## Example Usage:
/// ```cpp
/// RouletteWheelSelection selector;
/// std::vector<Fitness> fitnesses = {Fitness{10.0}, Fitness{5.0}, Fitness{15.0}};
/// std::mt19937 rng(42);
/// std::size_t selected = selector.select(fitnesses, rng);
/// // Individual with fitness 5.0 has highest selection probability
/// ```
///
/// @see SelectionOperator concept for interface requirements
/// @see TournamentSelection, RankSelection for alternative strategies
/// @since v2.0.0 (modernized API)
class RouletteWheelSelection {
  public:
    /// Select an individual using fitness-proportionate roulette wheel selection
    ///
    /// Implements roulette wheel selection where selection probability is proportional
    /// to fitness quality. For minimization problems, fitness values are inverted so
    /// that better (lower) fitness values receive higher selection probability.
    ///
    /// ## Algorithm Details:
    /// 1. **Weight Calculation**: Convert minimization fitness to positive weights
    ///    - Find min and max fitness in population
    ///    - Calculate weight = (max_fitness - individual_fitness + 1.0)
    ///    - Handle uniform fitness case with equal weights
    /// 2. **Selection Process**: Use cumulative weight distribution
    ///    - Calculate total weight sum
    ///    - Generate random target in [0, total_weight)
    ///    - Return first individual whose cumulative weight ≥ target
    ///
    /// ## Performance Optimization:
    /// - **Memory Efficient**: No intermediate weight storage (O(1) space)
    /// - **Cache Friendly**: Sequential access pattern through fitness data
    /// - **Minimal Allocation**: Uses std::minmax_element to find range in single pass
    ///
    /// @param fitnesses Span of fitness values for all individuals in the population.
    ///                  Must be non-empty. Values can be negative or positive.
    /// @param rng Reference to MT19937 random number generator for weight selection.
    ///
    /// @return Index of the selected individual (0 <= index < fitnesses.size()).
    ///         Selection probability is inversely proportional to fitness value.
    ///
    /// @pre fitnesses.size() >= 1 (enforced by assertion)
    /// @post 0 <= return_value < fitnesses.size()
    ///
    /// @throws None This method is noexcept in practice but may assert on invalid input
    ///
    /// ## Mathematical Foundation:
    /// For minimization problem with fitness values [f₁, f₂, ..., fₙ]:
    /// - Weight: wᵢ = (max(f) - fᵢ + 1)
    /// - Probability: P(i) = wᵢ / Σwⱼ
    /// - Better fitness (lower fᵢ) → higher weight → higher selection probability
    ///
    /// ## Thread Safety:
    /// Thread-safe for concurrent reads of fitness span with separate RNG instances.
    ///
    /// @see SelectionOperator concept for interface requirements
    /// @since v2.0.0 (span-based interface)
    std::size_t select(std::span<const core::Fitness> fitnesses, std::mt19937& rng) const {

        assert(!fitnesses.empty());
        if (fitnesses.size() == 1) {
            return 0;
        }

        // Convert to positive weights (assume minimization, so invert)
        const auto [min_it, max_it] =
            std::minmax_element(fitnesses.begin(), fitnesses.end(),
                                [](const auto& a, const auto& b) { return a.value < b.value; });
        const double min_fitness = min_it->value;
        const double max_fitness = max_it->value;
        double range = max_fitness - min_fitness;

        // First pass: calculate total weight (avoiding heap allocation)
        double total_weight = 0.0;
        for (const auto& fitness : fitnesses) {
            // Convert minimization to maximization weights
            double weight = (range == 0.0) ? 1.0 : (max_fitness - fitness.value + 1.0);
            total_weight += weight;
        }

        std::uniform_real_distribution<double> dist(0.0, total_weight);
        const double target = dist(rng);

        // Second pass: find target bucket without storing weights
        double cumulative = 0.0;
        for (std::size_t i = 0; i < fitnesses.size(); ++i) {
            const double weight = (range == 0.0) ? 1.0 : (max_fitness - fitnesses[i].value + 1.0);
            cumulative += weight;
            if (cumulative >= target) {
                return i;
            }
        }

        // Mathematically unreachable due to cumulative probability reaching total_weight
        detail::unreachable();
    }
};

/// Rank-based selection operator with configurable selection pressure
///
/// Rank selection assigns selection probability based on fitness rank rather than
/// absolute fitness values. This approach provides more consistent selection pressure
/// regardless of fitness value distribution and prevents premature convergence when
/// fitness differences are very large.
///
/// ## API Compatibility (v2.0+)
/// This class implements the modern SelectionOperator concept using `std::span<const Fitness>`.
/// For migration from v1.x implementations, see SelectionOperator concept documentation.
///
/// ## Algorithm:
/// 1. Sort individuals by fitness (best = rank 0, worst = rank n-1)
/// 2. Calculate rank-based probability using linear ranking formula
/// 3. Select individual using cumulative probability distribution
///
/// ## Selection Pressure Control:
/// The selection pressure parameter controls the bias toward better individuals:
/// - **pressure = 1.0**: Uniform selection (no pressure)
/// - **pressure = 1.5**: Moderate pressure (default, good balance)
/// - **pressure = 2.0**: Maximum pressure (strong bias toward best)
///
/// ## Mathematical Formula:
/// For individual at rank r (0 = best, n-1 = worst):
/// ```
/// P(r) = (2 - s)/n + (2*(s - 1)*(n - r - 1))/(n*(n - 1))
/// ```
/// where s = selection_pressure, n = population_size
///
/// ## Performance Characteristics:
/// - Time Complexity: O(n log n) due to sorting requirement
/// - Space Complexity: O(n) for index array and probability storage
/// - Memory Access: Two passes - sorting, then probability calculation
///
/// ## Advantages:
/// - **Scale Invariant**: Works regardless of fitness value range
/// - **Prevents Stagnation**: Maintains diversity even with large fitness differences
/// - **Configurable Pressure**: Tunable selection strength
/// - **Robust**: Handles negative, zero, or very large fitness values
///
/// ## Example Usage:
/// ```cpp
/// RankSelection selector(1.8);  // High selection pressure
/// std::vector<Fitness> fitnesses = {Fitness{100.0}, Fitness{5.0}, Fitness{50.0}};
/// std::mt19937 rng(42);
/// std::size_t selected = selector.select(fitnesses, rng);
/// // Selection based on rank, not absolute fitness values
/// ```
///
/// @see SelectionOperator concept for interface requirements
/// @see TournamentSelection, RouletteWheelSelection for alternative strategies
/// @since v2.0.0 (modernized API)
class RankSelection {
    double selection_pressure_; ///< Selection pressure: 1.0 = uniform, 2.0 = maximum bias

  public:
    /// Construct rank selection with specified selection pressure
    ///
    /// @param pressure Selection pressure controlling bias toward better individuals.
    ///                 Range: [1.0, 2.0] where 1.0 = uniform, 2.0 = maximum pressure.
    ///                 Default: 1.5 (balanced pressure).
    /// @pre pressure >= 1.0 && pressure <= 2.0 (enforced by assertion)
    explicit RankSelection(double pressure = 1.5) : selection_pressure_(pressure) {
        assert(pressure >= 1.0 && pressure <= 2.0);
    }

    /// Select an individual using rank-based selection with linear probability assignment
    ///
    /// Implements rank selection where individuals are selected based on their fitness
    /// rank rather than absolute fitness values. This provides scale-invariant selection
    /// pressure and prevents issues with very large or very small fitness differences.
    ///
    /// ## Algorithm Details:
    /// 1. **Ranking Phase**: Sort population by fitness (O(n log n))
    ///    - Create index array [0, 1, 2, ..., n-1]
    ///    - Sort indices by corresponding fitness values
    ///    - Best individual gets rank 0, worst gets rank n-1
    /// 2. **Probability Calculation**: Apply linear ranking formula
    ///    - P(rank) = (2-s)/n + 2*s*(n-rank-1)/(n*(n-1))
    ///    - Higher pressure → stronger bias toward better ranks
    /// 3. **Selection**: Use roulette wheel on rank-based probabilities
    ///
    /// ## Edge Case Handling:
    /// - **Single individual**: Returns index 0 immediately
    /// - **Uniform fitness**: Ranking still provides selection, unlike roulette wheel
    /// - **Extreme fitness differences**: Rank-based approach maintains fair selection
    ///
    /// ## Performance Considerations:
    /// - **Sorting Cost**: O(n log n) makes this slower than tournament/roulette for large
    /// populations
    /// - **Memory Usage**: O(n) for indices and probabilities
    /// - **Cache Efficiency**: Good locality during probability calculation phase
    ///
    /// @param fitnesses Span of fitness values for all individuals in the population.
    ///                  Must be non-empty. Can handle any fitness value range.
    /// @param rng Reference to MT19937 random number generator for selection.
    ///
    /// @return Index of the selected individual (0 <= index < fitnesses.size()).
    ///         Selection probability based on fitness rank, not absolute values.
    ///
    /// @pre fitnesses.size() >= 1 (enforced by assertion)
    /// @post 0 <= return_value < fitnesses.size()
    ///
    /// @throws None This method is noexcept in practice but may assert on invalid input
    ///
    /// ## Selection Pressure Examples:
    /// For 4-individual population (ranks 0,1,2,3):
    /// - pressure=1.0: P=[0.25, 0.25, 0.25, 0.25] (uniform)
    /// - pressure=1.5: P≈[0.375, 0.292, 0.208, 0.125] (moderate bias)
    /// - pressure=2.0: P=[0.50, 0.33, 0.17, 0.00] (strong bias)
    ///
    /// @see selection_pressure() for accessing the pressure parameter
    /// @see SelectionOperator concept for interface requirements
    /// @since v2.0.0 (span-based interface)
    std::size_t select(std::span<const core::Fitness> fitnesses, std::mt19937& rng) const {

        assert(!fitnesses.empty());
        if (fitnesses.size() == 1) {
            return 0;
        }

        // Create ranking using modern algorithms
        std::vector<std::size_t> indices(fitnesses.size());
        std::iota(indices.begin(), indices.end(), 0);

        // Sort by fitness (best first for minimization) using ranges algorithm
        std::ranges::sort(
            indices, [&](std::size_t a, std::size_t b) { return fitnesses[a] < fitnesses[b]; });

        // Select using on-the-fly probability calculation (no intermediate storage)
        const double n = static_cast<double>(fitnesses.size());
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        const double target = dist(rng);

        double cumulative = 0.0;
        for (std::size_t rank = 0; rank < fitnesses.size(); ++rank) {
            // Rank 0 is best, rank n-1 is worst
            // Formula: pressure=1.0 yields uniform selection, pressure=2.0 maximum bias
            const double prob =
                (2.0 - selection_pressure_) / n +
                (2.0 * (selection_pressure_ - 1.0) * (n - rank - 1)) / (n * (n - 1));
            cumulative += prob;
            if (cumulative >= target) {
                return indices[rank];
            }
        }

        // Mathematically unreachable due to cumulative probability reaching total_prob
        detail::unreachable();
    }

    /// Get the selection pressure parameter
    ///
    /// @return Current selection pressure value in range [1.0, 2.0].
    ///         1.0 = uniform selection, 2.0 = maximum bias toward best individuals.
    double selection_pressure() const { return selection_pressure_; }
};

/// Steady-state selection operator for elitist genetic algorithms
///
/// Steady-state selection focuses selection pressure on the best individuals by
/// restricting selection to the top-k performers. This operator is commonly used
/// in elitist strategies where high-quality solutions should dominate reproduction.
///
/// ## API Compatibility (v2.0+)
/// This class implements the modern SelectionOperator concept using `std::span<const Fitness>`.
/// For migration from v1.x implementations, see SelectionOperator concept documentation.
///
/// ## Algorithm:
/// 1. Identify the best `num_best` individuals using partial sort
/// 2. Randomly select one individual from this elite group
/// 3. Return the selected individual's index
///
/// ## Selection Strategy:
/// - **Elite Pool**: Only the best `num_best` individuals are eligible for selection
/// - **Uniform Within Elite**: Equal probability among elite individuals
/// - **Strong Pressure**: Very high selection pressure toward best solutions
///
/// ## Performance Characteristics:
/// - Time Complexity: O(n) with std::nth_element (k = num_best)
/// - Space Complexity: O(n) for index array during partial sorting
/// - Memory Access: Single pass for partial sort, then random access
///
/// ## Use Cases:
/// - **Exploitation Phase**: When converging toward optimal solutions
/// - **Elitist Strategies**: Preserving and propagating best individuals
/// - **Final Refinement**: When population has converged to high-quality region
/// - **Steady-State GA**: Replacing worst individuals with elite offspring
///
/// ## Configuration Guidelines:
/// - **num_best = 1**: Pure elitist selection (highest pressure)
/// - **num_best = 10-20%**: Moderate elitism with some diversity
/// - **num_best ≥ 50%**: Reduced elitism, approaching tournament selection
///
/// ## Example Usage:
/// ```cpp
/// SteadyStateSelection selector(5);  // Select from top 5 individuals
/// std::vector<Fitness> fitnesses = {Fitness{10.0}, Fitness{5.0}, Fitness{15.0}, Fitness{8.0}};
/// std::mt19937 rng(42);
/// std::size_t selected = selector.select(fitnesses, rng);
/// // Selects randomly from individuals with fitness 5.0, 8.0, 10.0 (top 3 available)
/// ```
///
/// @warning High selection pressure can lead to premature convergence.
///          Use with diversity-preserving mechanisms or in final optimization phases.
///
/// @see SelectionOperator concept for interface requirements
/// @see TournamentSelection for variable selection pressure
/// @since v2.0.0 (modernized API)
class SteadyStateSelection {
    std::size_t num_best_; ///< Number of best individuals to consider for selection

  public:
    /// Construct steady-state selection with specified elite pool size
    ///
    /// @param num_best Number of best individuals to include in selection pool.
    ///                 Must be ≥ 1. Values of 0 are automatically corrected to 1.
    ///                 Default: 1 (pure elitist selection).
    explicit SteadyStateSelection(std::size_t num_best = 1)
        : num_best_(std::max(num_best, std::size_t{1})) {}

    /// Select an individual from the elite pool using steady-state selection
    ///
    /// Implements steady-state selection by first identifying the best `num_best`
    /// individuals, then randomly selecting one from this elite group. This provides
    /// very strong selection pressure toward high-quality solutions.
    ///
    /// ## Algorithm Details:
    /// 1. **Elite Identification**: Use std::nth_element to partition top-k individuals
    ///    - Create index array [0, 1, 2, ..., n-1]
    ///    - Partially sort by fitness to identify best k individuals
    ///    - k = min(num_best, population_size) to handle small populations
    /// 2. **Random Selection**: Uniform selection within elite group
    ///    - Generate random index in [0, k)
    ///    - Return corresponding individual index
    ///
    /// ## Performance Optimization:
    /// - **Nth Element**: O(n) complexity using std::nth_element for optimal partitioning
    /// - **Adaptive Pool Size**: Automatically adjusts to population size
    /// - **Single Pass**: Combines identification and selection efficiently
    ///
    /// ## Edge Case Handling:
    /// - **Single individual**: Returns index 0 immediately
    /// - **num_best > population_size**: Uses entire population (equivalent to uniform selection)
    /// - **num_best = 0**: Treated as num_best = 1 for safety
    ///
    /// @param fitnesses Span of fitness values for all individuals in the population.
    ///                  Must be non-empty. Best individuals have lowest fitness values.
    /// @param rng Reference to MT19937 random number generator for elite selection.
    ///
    /// @return Index of the selected individual (0 <= index < fitnesses.size()).
    ///         Selected from the top `num_best` individuals by fitness.
    ///
    /// @pre fitnesses.size() >= 1 (enforced by assertion)
    /// @post 0 <= return_value < fitnesses.size()
    /// @post selected individual is among the best min(num_best, fitnesses.size()) individuals
    ///
    /// @throws None This method is noexcept in practice but may assert on invalid input
    ///
    /// ## Selection Pressure Analysis:
    /// - **Very High**: Only elite individuals can be selected
    /// - **Risk**: Can cause premature convergence if used exclusively
    /// - **Benefit**: Rapid exploitation of high-quality solutions
    /// - **Recommendation**: Combine with diversity-preserving operators
    ///
    /// ## Complexity Comparison:
    /// - std::nth_element: O(n) partitioning to top-k
    /// - Full sort: O(n log n) - unnecessary for this use case
    /// - Tournament: O(tournament_size) - different pressure characteristics
    ///
    /// @see num_best() for accessing the elite pool size parameter
    /// @see SelectionOperator concept for interface requirements
    /// @since v2.0.0 (span-based interface)
    std::size_t select(std::span<const core::Fitness> fitnesses, std::mt19937& rng) const {

        assert(!fitnesses.empty());
        if (fitnesses.size() == 1) {
            return 0;
        }

        std::vector<std::size_t> indices(fitnesses.size());
        std::iota(indices.begin(), indices.end(), 0);

        // Partition to top-k (unsorted) in O(n) instead of O(n log k)
        // num_best_ is guaranteed to be >= 1 by constructor
        const auto k = std::min(num_best_, fitnesses.size());
        std::nth_element(indices.begin(), indices.begin() + (k - 1), indices.end(),
                         [&](std::size_t a, std::size_t b) { return fitnesses[a] < fitnesses[b]; });

        // Randomly select from the best k
        std::uniform_int_distribution<std::size_t> dist(0, k - 1);
        return indices[dist(rng)];
    }

    /// Get the elite pool size parameter
    ///
    /// @return Number of best individuals considered for selection.
    ///         Actual elite pool size is min(num_best, population_size).
    std::size_t num_best() const { return num_best_; }
};

} // namespace evolab::operators
