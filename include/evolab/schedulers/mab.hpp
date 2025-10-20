#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <concepts>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <evolab/core/concepts.hpp>

namespace evolab::schedulers {

// Thread-local random number generator for default scheduler parameters
// Uses std::seed_seq to combine entropy from multiple sources (random_device +
// high_resolution_clock + thread_id) to avoid seed collision across threads, especially on
// platforms where random_device may be deterministic
inline std::mt19937& get_thread_rng() {
    static thread_local std::mt19937 gen = [] {
        std::random_device rd;
        auto clock_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto thread_id_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());

        // Combine entropy from random_device, high-res clock, and thread ID
        std::seed_seq ssq{rd(), static_cast<unsigned int>(clock_seed),
                          static_cast<unsigned int>(clock_seed >> 32),
                          static_cast<unsigned int>(thread_id_hash)};
        return std::mt19937(ssq);
    }();
    return gen;
}

// TODO(performance): Aggregate the per-invocation timing (already tracked in selectors
// via last_execution_time_) into OperatorStats for cost-benefit analysis. Consider adding:
// - double total_execution_time: cumulative time spent on this operator
// - double avg_execution_time: average execution time per selection
// This would enable direct comparison of performance vs. reward trade-offs
// across operators in the adaptive selection process.

// TODO(validation): Empirical validation required before v1.0 release
// This MAB scheduler implementation is functionally correct and unit-tested, but requires
// comprehensive empirical validation for research-grade quality. See detailed validation
// plan in Issue #33: https://github.com/lv416e/evolab/issues/33
// Priority: HIGH - Blocks v1.0 release and research publications
//
struct OperatorStats {
    double total_reward = 0.0;
    size_t selection_count = 0;
    double avg_reward = 0.0;
    double success_rate = 0.0;
    size_t success_count = 0;

    void update_reward(double reward) {
        total_reward += reward;
        selection_count++;
        avg_reward = total_reward / selection_count;
        if (reward > 0.0) {
            success_count++;
        }
        success_rate = static_cast<double>(success_count) / selection_count;
    }

    void reset() {
        total_reward = 0.0;
        selection_count = 0;
        avg_reward = 0.0;
        success_rate = 0.0;
        success_count = 0;
    }
};

/// @brief Operator traits for crossover operators in adaptive selector
///
/// Defines the interface and type requirements for crossover operators,
/// enabling policy-based design for AdaptiveSelector.
///
/// @tparam Problem The optimization problem type
template <typename Problem>
struct CrossoverOperatorTraits {
    using GenomeT = typename Problem::GenomeT;
    using ResultType = std::pair<GenomeT, GenomeT>;
    using OperatorFn =
        std::function<ResultType(const Problem&, const GenomeT&, const GenomeT&, std::mt19937&)>;

    static constexpr const char* selector_type_name = "crossover";

    /// @brief Wraps a crossover operator into a type-erased function object
    ///
    /// @tparam OpType Crossover operator type (must satisfy CrossoverOperator concept)
    /// @param op Crossover operator to wrap
    /// @return Type-erased function object compatible with OperatorFn signature
    template <typename OpType>
        requires core::CrossoverOperator<OpType, Problem>
    static OperatorFn wrap_operator(OpType&& op) {
        return [op = std::forward<OpType>(op)](const Problem& problem, const GenomeT& parent1,
                                               const GenomeT& parent2, std::mt19937& rng) {
            return op.cross(problem, parent1, parent2, rng);
        };
    }
};

/// @brief Operator traits for local search operators in adaptive selector
///
/// Defines the interface and type requirements for local search operators,
/// enabling policy-based design for AdaptiveSelector.
///
/// @tparam Problem The optimization problem type
template <typename Problem>
struct LocalSearchOperatorTraits {
    using GenomeT = typename Problem::GenomeT;
    using ResultType = core::Fitness;
    using OperatorFn = std::function<ResultType(const Problem&, GenomeT&, std::mt19937&)>;

    static constexpr const char* selector_type_name = "local search";

    /// @brief Wraps a local search operator into a type-erased function object
    ///
    /// @tparam OpType Local search operator type (must satisfy LocalSearchOperator concept)
    /// @param op Local search operator to wrap
    /// @return Type-erased function object compatible with OperatorFn signature
    template <typename OpType>
        requires core::LocalSearchOperator<OpType, Problem>
    static OperatorFn wrap_operator(OpType&& op) {
        return [op = std::forward<OpType>(op)](const Problem& problem, GenomeT& genome,
                                               std::mt19937& rng) {
            return op.improve(problem, genome, rng);
        };
    }
};

class UCBScheduler {
  private:
    std::vector<OperatorStats> stats_;
    double exploration_constant_;
    size_t total_selections_;
    std::mt19937& rng_;

  public:
    explicit UCBScheduler(size_t num_operators, double exploration_constant = 2.0,
                          std::mt19937& rng = get_thread_rng())
        : stats_(num_operators), exploration_constant_(exploration_constant), total_selections_(0),
          rng_(rng) {}

    int select_operator() {
        if (stats_.empty()) {
            throw std::runtime_error("UCBScheduler: no operators configured");
        }

        total_selections_++;

        std::vector<size_t> best_operators;
        double best_ucb = -std::numeric_limits<double>::infinity();

        for (size_t i = 0; i < stats_.size(); ++i) {
            double ucb_value;
            if (stats_[i].selection_count == 0) {
                ucb_value = std::numeric_limits<double>::infinity();
            } else {
                double exploitation = stats_[i].avg_reward;
                double exploration = exploration_constant_ * std::sqrt(std::log(total_selections_) /
                                                                       stats_[i].selection_count);
                ucb_value = exploitation + exploration;
            }

            if (ucb_value > best_ucb) {
                best_ucb = ucb_value;
                best_operators.clear();
                best_operators.push_back(i);
            } else if (ucb_value == best_ucb) {
                best_operators.push_back(i);
            }
        }

        // Random tie-breaking
        if (best_operators.size() > 1) {
            std::uniform_int_distribution<size_t> dist(0, best_operators.size() - 1);
            return static_cast<int>(best_operators[dist(rng_)]);
        }

        return static_cast<int>(best_operators[0]);
    }

    void update_reward(int operator_id, double reward) {
        if (operator_id >= 0 && operator_id < static_cast<int>(stats_.size())) {
            stats_[operator_id].update_reward(reward);
        }
    }

    const std::vector<OperatorStats>& get_stats() const { return stats_; }

    void reset() {
        for (auto& stat : stats_) {
            stat.reset();
        }
        total_selections_ = 0;
    }
};

class ThompsonSamplingScheduler {
  private:
    struct BetaDistribution {
        double alpha = 1.0;
        double beta = 1.0;

        void update_success() { alpha += 1.0; }
        void update_failure() { beta += 1.0; }

        double sample(std::mt19937& rng) const {
            std::gamma_distribution<double> gamma_alpha(alpha, 1.0);
            std::gamma_distribution<double> gamma_beta(beta, 1.0);

            double x = gamma_alpha(rng);
            double y = gamma_beta(rng);

            return x / (x + y);
        }

        void reset() {
            alpha = 1.0;
            beta = 1.0;
        }
    };

    std::vector<BetaDistribution> distributions_;
    std::vector<OperatorStats> stats_;
    std::mt19937& rng_;
    double reward_threshold_;

  public:
    explicit ThompsonSamplingScheduler(size_t num_operators, double reward_threshold = 0.0,
                                       std::mt19937& rng = get_thread_rng())
        : distributions_(num_operators), stats_(num_operators), rng_(rng),
          reward_threshold_(reward_threshold) {}

    int select_operator() {
        if (distributions_.empty()) {
            throw std::runtime_error("ThompsonSamplingScheduler: no operators configured");
        }

        std::vector<double> samples(distributions_.size());

        for (size_t i = 0; i < distributions_.size(); ++i) {
            samples[i] = distributions_[i].sample(rng_);
        }

        return static_cast<int>(
            std::distance(samples.begin(), std::max_element(samples.begin(), samples.end())));
    }

    void update_reward(int operator_id, double reward) {
        if (operator_id >= 0 && operator_id < static_cast<int>(distributions_.size())) {
            // Update basic stats but override success counting
            stats_[operator_id].total_reward += reward;
            stats_[operator_id].selection_count++;
            stats_[operator_id].avg_reward =
                stats_[operator_id].total_reward / stats_[operator_id].selection_count;

            // Use threshold-based success counting for Thompson sampling
            if (reward > reward_threshold_) {
                stats_[operator_id].success_count++;
                distributions_[operator_id].update_success();
            } else {
                distributions_[operator_id].update_failure();
            }

            stats_[operator_id].success_rate =
                static_cast<double>(stats_[operator_id].success_count) /
                stats_[operator_id].selection_count;
        }
    }

    const std::vector<OperatorStats>& get_stats() const { return stats_; }

    void reset() {
        for (auto& dist : distributions_) {
            dist.reset();
        }
        for (auto& stat : stats_) {
            stat.reset();
        }
    }

    void set_reward_threshold(double threshold) { reward_threshold_ = threshold; }
    double get_reward_threshold() const { return reward_threshold_; }
};

/// @brief Concept for operator types that can be wrapped by a traits policy
///
/// This concept validates that an operator type can be wrapped by the traits'
/// wrap_operator function and produces a compatible OperatorFn type.
///
/// @tparam Op The operator type to check
/// @tparam Tr The traits type (CrossoverOperatorTraits or LocalSearchOperatorTraits)
template <typename Op, typename Tr>
concept WrappableBy = requires(Op&& o) {
    {
        Tr::template wrap_operator<Op>(std::forward<Op>(o))
    } -> std::convertible_to<typename Tr::OperatorFn>;
};

/// @brief Unified adaptive operator selector using policy-based design
///
/// This class provides a type-safe, unified implementation for both crossover
/// and local search operator selection using multi-armed bandit algorithms.
/// The policy-based design with operator traits eliminates ~110 lines of code
/// duplication while maintaining full type safety through C++20 concepts.
///
/// @warning NOT THREAD-SAFE: This class maintains mutable state (current_selection_,
///          tracking_improvement_, last_fitness_improvement_, last_execution_time_) and
///          is NOT safe for concurrent access from multiple threads. Sharing a selector
///          across threads will cause race conditions leading to corrupted MAB learning
///          and potentially incorrect research results.
///
/// @note For parallel GAs (e.g., Island Model, parallel populations): Create one
///       selector instance per thread/island. Each thread must have its own independent
///       selector to ensure correct learning and avoid data races.
///
/// @tparam SchedulerType The MAB scheduler type (UCBScheduler or ThompsonSamplingScheduler)
/// @tparam Problem The optimization problem type
/// @tparam Traits Operator traits (CrossoverOperatorTraits or LocalSearchOperatorTraits)
template <typename SchedulerType, typename Problem, typename Traits>
class AdaptiveSelector {
  private:
    using GenomeT = typename Problem::GenomeT;
    using OperatorFn = typename Traits::OperatorFn;

    SchedulerType scheduler_;
    std::vector<OperatorFn> operators_;
    std::vector<std::string> operator_names_;
    int current_selection_;
    double last_fitness_improvement_;
    double last_execution_time_;
    bool tracking_improvement_;

    /// @brief Internal implementation for applying operators with timing and tracking
    ///
    /// Uses variadic templates to support both crossover (3 genome args) and
    /// local search (1 genome arg) signatures without code duplication.
    ///
    /// @tparam Args Variadic arguments forwarded to the operator function
    /// @param problem The optimization problem instance
    /// @param args Additional arguments (parent genomes for crossover, or genome for local search)
    /// @return Result of operator application (depends on Traits::ResultType)
    template <typename... Args>
    auto apply_operator_impl(const Problem& problem, Args&&... args) {
        if (operators_.empty()) {
            throw std::logic_error(
                std::format("Cannot apply {} operator: no operators have been added.",
                            Traits::selector_type_name));
        }

        if (tracking_improvement_) {
            throw std::logic_error(std::format(
                "apply method called again before report_fitness_improvement was called for the "
                "previous {} operation.",
                Traits::selector_type_name));
        }

        current_selection_ = scheduler_.select_operator();

        if (current_selection_ < 0 || current_selection_ >= static_cast<int>(operators_.size())) {
            throw std::out_of_range(std::format(
                "Selected {} operator index {} is out of bounds. This can happen if the number of "
                "operators added via add_operator() does not match the num_operators argument in "
                "the constructor. Expected {} operators, but only {} were added.",
                Traits::selector_type_name, current_selection_, scheduler_.get_stats().size(),
                operators_.size()));
        }

        auto start_time = std::chrono::steady_clock::now();
        auto result = operators_[current_selection_](problem, std::forward<Args>(args)...);
        auto end_time = std::chrono::steady_clock::now();
        last_execution_time_ = std::chrono::duration<double>(end_time - start_time).count();

        tracking_improvement_ = true;
        return result;
    }

  public:
    /// @brief Construct adaptive selector with specified number of operators
    ///
    /// @tparam Args Variadic arguments forwarded to the scheduler constructor
    /// @param num_operators Number of operators to be added (must be > 0)
    /// @param args Additional scheduler-specific arguments (e.g., exploration constant for UCB)
    template <typename... Args>
    explicit AdaptiveSelector(size_t num_operators, Args&&... args)
        : scheduler_(num_operators, std::forward<Args>(args)...), current_selection_(-1),
          last_fitness_improvement_(0.0), last_execution_time_(0.0), tracking_improvement_(false) {
        if (num_operators == 0) {
            throw std::invalid_argument(std::format(
                "AdaptiveSelector for {} must be configured with at least one operator.",
                Traits::selector_type_name));
        }
        operators_.reserve(num_operators);
        operator_names_.reserve(num_operators);
    }

    /// @brief Add an operator to the selector
    ///
    /// The operator type is validated against the concept defined in Traits
    /// (CrossoverOperator or LocalSearchOperator) via explicit requires clause.
    ///
    /// @tparam OpType Operator type (must be wrappable by Traits)
    /// @param op Operator instance to add
    /// @param name Human-readable name for the operator
    template <typename OpType>
        requires WrappableBy<OpType, Traits>
    void add_operator(OpType&& op, std::string name) {
        if (operators_.size() >= scheduler_.get_stats().size()) {
            throw std::logic_error(std::format(
                "Cannot add more {} operators than the number specified in the selector's "
                "constructor. "
                "Maximum allowed: {}, current: {}. Extra operators will never be selected.",
                Traits::selector_type_name, scheduler_.get_stats().size(), operators_.size()));
        }
        operator_names_.emplace_back(std::move(name));
        operators_.emplace_back(Traits::wrap_operator(std::forward<OpType>(op)));
    }

    /// @brief Apply crossover operator (only available for CrossoverOperatorTraits)
    ///
    /// @param problem The optimization problem instance
    /// @param parent1 First parent genome
    /// @param parent2 Second parent genome
    /// @param rng Random number generator
    /// @return Pair of offspring genomes
    [[nodiscard]] auto apply_crossover(const Problem& problem, const GenomeT& parent1, const GenomeT& parent2,
                         std::mt19937& rng)
        requires std::same_as<Traits, CrossoverOperatorTraits<Problem>>
    {
        return apply_operator_impl(problem, parent1, parent2, rng);
    }

    /// @brief Apply local search operator (only available for LocalSearchOperatorTraits)
    ///
    /// @param problem The optimization problem instance
    /// @param genome Genome to improve (modified in-place)
    /// @param rng Random number generator
    /// @return Fitness after local search
    [[nodiscard]] auto apply_local_search(const Problem& problem, GenomeT& genome, std::mt19937& rng)
        requires std::same_as<Traits, LocalSearchOperatorTraits<Problem>>
    {
        return apply_operator_impl(problem, genome, rng);
    }

    /// @brief Report fitness improvement for the last operator application
    ///
    /// Must be called after each apply_crossover() or apply_local_search() call
    /// to update the MAB learning statistics.
    ///
    /// @param improvement Fitness improvement value (positive = better, must be finite)
    /// @throws std::invalid_argument if improvement is NaN or infinite
    void report_fitness_improvement(double improvement) {
        if (!std::isfinite(improvement)) {
            throw std::invalid_argument(
                "report_fitness_improvement: improvement must be finite (not NaN or Inf)");
        }
        if (!tracking_improvement_) {
            throw std::logic_error(
                "report_fitness_improvement called without a pending apply_* operation.");
        }
        last_fitness_improvement_ = improvement;
        scheduler_.update_reward(current_selection_, improvement);
        tracking_improvement_ = false;
    }

    /// @brief Report fitness improvement using old and new fitness values
    ///
    /// This is a convenience method for MINIMIZATION problems (TSP, VRP, CVRP, QAP, etc.)
    /// where improvement = old_fitness - new_fitness (lower fitness is better).
    ///
    /// @warning MINIMIZATION PROBLEMS ONLY: This method assumes minimization objectives.
    ///          For maximization problems, you must calculate improvement manually and use
    ///          report_fitness_improvement() directly:
    ///          @code
    ///          double improvement = new_fitness - old_fitness;  // For maximization
    ///          selector.report_fitness_improvement(improvement);
    ///          @endcode
    ///
    /// @param old_fitness Fitness value before operator application
    /// @param new_fitness Fitness value after operator application
    void report_fitness_change(double old_fitness, double new_fitness) {
        if (!std::isfinite(old_fitness) || !std::isfinite(new_fitness)) {
            throw std::invalid_argument(
                "report_fitness_change: fitness values must be finite (not NaN or Inf)");
        }
        double improvement = old_fitness - new_fitness; // Minimization: lower is better
        report_fitness_improvement(improvement);
    }

    /// @brief Get statistics for all operators
    const std::vector<OperatorStats>& get_operator_stats() const { return scheduler_.get_stats(); }

    /// @brief Get names of all operators
    const std::vector<std::string>& get_operator_names() const { return operator_names_; }

    /// @brief Reset all statistics and state
    void reset_stats() {
        scheduler_.reset();
        current_selection_ = -1;
        last_fitness_improvement_ = 0.0;
        last_execution_time_ = 0.0;
        tracking_improvement_ = false;
    }

    /// @brief Get number of operators added
    size_t get_operator_count() const { return operators_.size(); }

    /// @brief Get index of last selected operator
    int get_last_selection() const { return current_selection_; }

    /// @brief Get fitness improvement from last operator application
    double get_last_improvement() const { return last_fitness_improvement_; }

    /// @brief Get execution time of last operator application (in seconds)
    double get_last_execution_time() const { return last_execution_time_; }
};

// ============================================================================
// Type Aliases for API Compatibility
// ============================================================================
// The following type aliases maintain full backward compatibility with existing
// code while leveraging the unified AdaptiveSelector implementation.

/// @brief Type alias for crossover operator selector
///
/// This is a convenience alias that instantiates AdaptiveSelector with
/// CrossoverOperatorTraits. Provides the same API as the previous
/// AdaptiveOperatorSelector class implementation.
///
/// @tparam SchedulerType The MAB scheduler type (UCBScheduler or ThompsonSamplingScheduler)
/// @tparam Problem The optimization problem type
template <typename SchedulerType, typename Problem>
using AdaptiveOperatorSelector =
    AdaptiveSelector<SchedulerType, Problem, CrossoverOperatorTraits<Problem>>;

/// @brief UCB-based crossover operator selector
///
/// Convenience alias for AdaptiveOperatorSelector with UCB scheduler.
template <typename Problem>
using UCBOperatorSelector = AdaptiveOperatorSelector<UCBScheduler, Problem>;

/// @brief Thompson Sampling-based crossover operator selector
///
/// Convenience alias for AdaptiveOperatorSelector with Thompson Sampling scheduler.
template <typename Problem>
using ThompsonOperatorSelector = AdaptiveOperatorSelector<ThompsonSamplingScheduler, Problem>;

/// @brief Type alias for local search operator selector
///
/// This is a convenience alias that instantiates AdaptiveSelector with
/// LocalSearchOperatorTraits. Provides the same API as the previous
/// AdaptiveLocalSearchSelector class implementation.
///
/// @tparam SchedulerType The MAB scheduler type (UCBScheduler or ThompsonSamplingScheduler)
/// @tparam Problem The optimization problem type
template <typename SchedulerType, typename Problem>
using AdaptiveLocalSearchSelector =
    AdaptiveSelector<SchedulerType, Problem, LocalSearchOperatorTraits<Problem>>;

/// @brief UCB-based local search operator selector
///
/// Convenience alias for AdaptiveLocalSearchSelector with UCB scheduler.
template <typename Problem>
using UCBLocalSearchSelector = AdaptiveLocalSearchSelector<UCBScheduler, Problem>;

/// @brief Thompson Sampling-based local search operator selector
///
/// Convenience alias for AdaptiveLocalSearchSelector with Thompson Sampling scheduler.
template <typename Problem>
using ThompsonLocalSearchSelector = AdaptiveLocalSearchSelector<ThompsonSamplingScheduler, Problem>;

} // namespace evolab::schedulers
