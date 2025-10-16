#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <concepts>
#include <functional>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <evolab/core/concepts.hpp>

namespace evolab::schedulers {

// Thread-local random number generator for default scheduler parameters
// Uses std::seed_seq to combine entropy from multiple sources (random_device +
// high_resolution_clock) to avoid seed collision across threads, especially on platforms where
// random_device may be deterministic
inline std::mt19937& get_thread_rng() {
    static thread_local std::mt19937 gen = [] {
        std::random_device rd;
        auto clock_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        // Split 64-bit clock_seed into two 32-bit values for better entropy distribution
        std::seed_seq ssq{rd(), static_cast<unsigned int>(clock_seed),
                          static_cast<unsigned int>(clock_seed >> 32)};
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
//
// This MAB scheduler implementation requires comprehensive empirical validation to confirm
// research-grade quality and scientific rigor. The implementation is functionally correct
// and unit-tested, but lacks the experimental validation necessary for research publications.
//
// Required validation experiments:
//
// 1. Convergence Validation:
//    - Verify MAB learns operator quality over extended runs (1000+ generations)
//    - Test with operators of known quality differences (e.g., Lin-Kernighan vs 2-opt vs random)
//    - Measure: generations until best operator selected >80% of time
//    - Expected: Convergence within 100-200 generations for clear quality differences
//
// 2. Performance Overhead Characterization:
//    - Quantify MAB selection cost vs operator execution cost
//    - Benchmark: UCB selection time, Thompson sampling time
//    - Test problem sizes: 50, 100, 200, 500, 1000 cities (TSP)
//    - Expected: <1% overhead for typical local search operations (>10ms execution)
//    - Document when overhead becomes significant (very fast operators <1ms)
//
// 3. Solution Quality Comparison:
//    - Compare final solution quality: MAB vs random selection vs round-robin vs best-only
//    - Statistical rigor: 50+ independent runs per configuration
//    - Significance testing: Mann-Whitney U test, effect size (Cohen's d)
//    - Expected: MAB should match or exceed fixed strategies, especially for heterogeneous
//    operators
//
// 4. Multi-Operator Scenarios:
//    - Test with 3-5 operators of varying quality and computational cost
//    - Validate UCB exploration constant sensitivity (test c=0.5, 1.0, 2.0, 4.0)
//    - Validate Thompson Sampling reward threshold sensitivity (test multiple thresholds)
//    - Measure operator selection distribution over time (should favor better operators)
//
// 5. Real-World GA Integration:
//    - Run complete memetic GA with MAB for 50+ independent runs
//    - Test problems: TSP instances from TSPLIB (att48, eil76, kroA100, lin318, pcb442)
//    - Compare with literature baselines (fixed operator strategies, other adaptive methods)
//    - Document: best fitness, mean fitness, std deviation, convergence curves
//
// 6. Research Publication Readiness:
//    - Reproducible experimental setup (document all parameters, seeds, hardware)
//    - Statistical analysis with confidence intervals
//    - Comparison table with literature baselines
//    - Performance profiles and convergence plots
//    - Technical report or paper draft with methodology and results
//
// Acceptance Criteria:
// - All validation experiments implemented and documented
// - Results demonstrate MAB provides value (matches or exceeds baselines)
// - Statistical significance confirmed where applicable
// - Reproducible benchmarks with fixed seeds in benchmarks/ directory
// - Ready for submission to research venues (EA conferences/journals)
//
// Priority: HIGH - Blocks v1.0 release and research publications
// Estimated Effort: 2-3 days of focused experimental work + analysis + documentation
// See: https://github.com/lv416e/evolab/issues/33
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

// TODO(refactor): AdaptiveOperatorSelector and AdaptiveLocalSearchSelector share ~110 lines of
// nearly identical code (constructor validation, reporting, getters, reset logic). The duplication
// threshold has been reached. A templated base class or CRTP pattern could eliminate most of this
// without complex metaprogramming - the main challenge is abstracting the different return types
// (pair vs Fitness) and parameter lists in the apply methods. This refactoring should be
// prioritized before adding a third selector type to avoid further code multiplication.
// See: https://github.com/lv416e/evolab/issues/32

/// @brief Adaptive crossover operator selector using multi-armed bandit algorithms
///
/// This class enables automatic selection of the best-performing crossover operator
/// based on historical performance using UCB or Thompson Sampling schedulers.
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
/// @tparam Problem The optimization problem type (must satisfy Problem concept)
template <typename SchedulerType, typename Problem>
class AdaptiveOperatorSelector {
  private:
    using CrossoverFn =
        std::function<std::pair<typename Problem::GenomeT, typename Problem::GenomeT>(
            const Problem&, const typename Problem::GenomeT&, const typename Problem::GenomeT&,
            std::mt19937&)>;

    SchedulerType scheduler_;
    std::vector<CrossoverFn> operators_;
    std::vector<std::string> operator_names_;
    int current_selection_;
    double last_fitness_improvement_;
    double last_execution_time_;
    bool tracking_improvement_;

  public:
    template <typename... Args>
    explicit AdaptiveOperatorSelector(size_t num_operators, Args&&... args)
        : scheduler_(num_operators, std::forward<Args>(args)...), current_selection_(-1),
          last_fitness_improvement_(0.0), last_execution_time_(0.0), tracking_improvement_(false) {
        if (num_operators == 0) {
            throw std::invalid_argument(
                "AdaptiveOperatorSelector must be configured with at least one operator.");
        }
        operators_.reserve(num_operators);
        operator_names_.reserve(num_operators);
    }

    template <core::CrossoverOperator<Problem> OpType>
    void add_operator(OpType op, std::string name) {
        if (operators_.size() >= scheduler_.get_stats().size()) {
            throw std::logic_error(
                "Cannot add more crossover operators than the number specified in the "
                "selector's constructor. Maximum allowed: " +
                std::to_string(scheduler_.get_stats().size()) + ", current: " +
                std::to_string(operators_.size()) + ". Extra operators will never be selected.");
        }
        operator_names_.emplace_back(std::move(name));
        operators_.emplace_back(
            [op = std::move(op)](const Problem& problem, const typename Problem::GenomeT& parent1,
                                 const typename Problem::GenomeT& parent2, std::mt19937& rng) {
                return op.cross(problem, parent1, parent2, rng);
            });
    }

    std::pair<typename Problem::GenomeT, typename Problem::GenomeT>
    apply_crossover(const Problem& problem, const typename Problem::GenomeT& parent1,
                    const typename Problem::GenomeT& parent2, std::mt19937& rng) {
        if (operators_.empty()) {
            throw std::logic_error("Cannot apply crossover: no operators have been added.");
        }

        if (tracking_improvement_) {
            throw std::logic_error(
                "apply_crossover called again before report_fitness_improvement was called for the "
                "previous operation.");
        }

        current_selection_ = scheduler_.select_operator();

        if (current_selection_ < 0 || current_selection_ >= static_cast<int>(operators_.size())) {
            throw std::out_of_range(
                "Selected crossover operator index " + std::to_string(current_selection_) +
                " is out of bounds. This can happen if the number of "
                "operators added via add_operator() does not match the num_operators argument in "
                "the constructor. Expected " +
                std::to_string(scheduler_.get_stats().size()) + " operators, but only " +
                std::to_string(operators_.size()) + " were added.");
        }

        auto start_time = std::chrono::steady_clock::now();
        auto result = operators_[current_selection_](problem, parent1, parent2, rng);
        auto end_time = std::chrono::steady_clock::now();
        last_execution_time_ = std::chrono::duration<double>(end_time - start_time).count();

        tracking_improvement_ = true;
        return result;
    }

    void report_fitness_improvement(double improvement) {
        if (tracking_improvement_ && current_selection_ >= 0) {
            last_fitness_improvement_ = improvement;
            scheduler_.update_reward(current_selection_, improvement);
            tracking_improvement_ = false;
        }
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
    /// @param old_fitness Fitness value before crossover operation
    /// @param new_fitness Fitness value after crossover operation
    ///
    /// @example
    /// @code
    /// // For minimization problems (TSP):
    /// selector.report_fitness_change(100.0, 90.0);   // improvement = 10.0 (better)
    /// selector.report_fitness_change(90.0, 100.0);   // improvement = -10.0 (worse)
    /// @endcode
    void report_fitness_change(double old_fitness, double new_fitness) {
        double improvement = old_fitness - new_fitness; // Minimization: lower is better
        report_fitness_improvement(improvement);
    }

    const std::vector<OperatorStats>& get_operator_stats() const { return scheduler_.get_stats(); }

    const std::vector<std::string>& get_operator_names() const { return operator_names_; }

    void reset_stats() {
        scheduler_.reset();
        current_selection_ = -1;
        last_fitness_improvement_ = 0.0;
        last_execution_time_ = 0.0;
        tracking_improvement_ = false;
    }

    size_t get_operator_count() const { return operators_.size(); }

    int get_last_selection() const { return current_selection_; }
    double get_last_improvement() const { return last_fitness_improvement_; }
    double get_last_execution_time() const { return last_execution_time_; }
};

template <typename Problem>
using UCBOperatorSelector = AdaptiveOperatorSelector<UCBScheduler, Problem>;

template <typename Problem>
using ThompsonOperatorSelector = AdaptiveOperatorSelector<ThompsonSamplingScheduler, Problem>;

/// @brief Adaptive local search operator selector using multi-armed bandit algorithms
///
/// This class enables automatic selection of the best-performing local search operator
/// based on historical performance using UCB or Thompson Sampling schedulers.
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
/// @tparam Problem The optimization problem type (must satisfy Problem concept)
template <typename SchedulerType, typename Problem>
class AdaptiveLocalSearchSelector {
  private:
    using LocalSearchFn =
        std::function<core::Fitness(const Problem&, typename Problem::GenomeT&, std::mt19937&)>;

    SchedulerType scheduler_;
    std::vector<LocalSearchFn> operators_;
    std::vector<std::string> operator_names_;
    int current_selection_;
    double last_fitness_improvement_;
    double last_execution_time_;
    bool tracking_improvement_;

  public:
    template <typename... Args>
    explicit AdaptiveLocalSearchSelector(size_t num_operators, Args&&... args)
        : scheduler_(num_operators, std::forward<Args>(args)...), current_selection_(-1),
          last_fitness_improvement_(0.0), last_execution_time_(0.0), tracking_improvement_(false) {
        if (num_operators == 0) {
            throw std::invalid_argument(
                "AdaptiveLocalSearchSelector must be configured with at least one operator.");
        }
        operators_.reserve(num_operators);
        operator_names_.reserve(num_operators);
    }

    // TODO(design): Consider relaxing LocalSearchOperator concept to support
    // stateful algorithms (e.g., Tabu Search) by accepting non-const operators.
    // This would require making the lambda mutable:
    //   [op = std::move(op)](...) mutable { return op.improve(...); }
    // The concept in core/concepts.hpp would need to accept non-const L&.
    // This would improve extensibility for stateful local search algorithms.
    template <core::LocalSearchOperator<Problem> OpType>
    void add_operator(OpType op, std::string name) {
        if (operators_.size() >= scheduler_.get_stats().size()) {
            throw std::logic_error(
                "Cannot add more local search operators than the number specified in the "
                "selector's constructor. Maximum allowed: " +
                std::to_string(scheduler_.get_stats().size()) + ", current: " +
                std::to_string(operators_.size()) + ". Extra operators will never be selected.");
        }
        operator_names_.emplace_back(std::move(name));
        operators_.emplace_back(
            [op = std::move(op)](const Problem& problem, typename Problem::GenomeT& genome,
                                 std::mt19937& rng) { return op.improve(problem, genome, rng); });
    }

    core::Fitness apply_local_search(const Problem& problem, typename Problem::GenomeT& genome,
                                     std::mt19937& rng) {
        if (operators_.empty()) {
            throw std::logic_error("Cannot apply local search: no operators have been added.");
        }

        if (tracking_improvement_) {
            throw std::logic_error(
                "apply_local_search called again before report_fitness_improvement was called for "
                "the previous operation.");
        }

        current_selection_ = scheduler_.select_operator();

        if (current_selection_ < 0 || current_selection_ >= static_cast<int>(operators_.size())) {
            throw std::out_of_range(
                "Selected local search operator index " + std::to_string(current_selection_) +
                " is out of bounds. This can happen if the number of "
                "operators added via add_operator() does not match the num_operators argument in "
                "the constructor. Expected " +
                std::to_string(scheduler_.get_stats().size()) + " operators, but only " +
                std::to_string(operators_.size()) + " were added.");
        }

        auto start_time = std::chrono::steady_clock::now();
        core::Fitness result = operators_[current_selection_](problem, genome, rng);

        auto end_time = std::chrono::steady_clock::now();
        last_execution_time_ = std::chrono::duration<double>(end_time - start_time).count();

        tracking_improvement_ = true;
        return result;
    }

    void report_fitness_improvement(double improvement) {
        if (tracking_improvement_ && current_selection_ >= 0) {
            last_fitness_improvement_ = improvement;
            scheduler_.update_reward(current_selection_, improvement);
            tracking_improvement_ = false;
        }
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
    /// @param old_fitness Fitness value before local search operation
    /// @param new_fitness Fitness value after local search operation
    ///
    /// @example
    /// @code
    /// // For minimization problems (TSP):
    /// selector.report_fitness_change(100.0, 90.0);   // improvement = 10.0 (better)
    /// selector.report_fitness_change(90.0, 100.0);   // improvement = -10.0 (worse)
    /// @endcode
    void report_fitness_change(double old_fitness, double new_fitness) {
        double improvement = old_fitness - new_fitness; // Minimization: lower is better
        report_fitness_improvement(improvement);
    }

    const std::vector<OperatorStats>& get_operator_stats() const { return scheduler_.get_stats(); }

    const std::vector<std::string>& get_operator_names() const { return operator_names_; }

    void reset_stats() {
        scheduler_.reset();
        current_selection_ = -1;
        last_fitness_improvement_ = 0.0;
        last_execution_time_ = 0.0;
        tracking_improvement_ = false;
    }

    size_t get_operator_count() const { return operators_.size(); }

    int get_last_selection() const { return current_selection_; }
    double get_last_improvement() const { return last_fitness_improvement_; }
    double get_last_execution_time() const { return last_execution_time_; }
};

template <typename Problem>
using UCBLocalSearchSelector = AdaptiveLocalSearchSelector<UCBScheduler, Problem>;

template <typename Problem>
using ThompsonLocalSearchSelector = AdaptiveLocalSearchSelector<ThompsonSamplingScheduler, Problem>;

} // namespace evolab::schedulers
