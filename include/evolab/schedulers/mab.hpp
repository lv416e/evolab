#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <concepts>
#include <functional>
#include <memory>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace evolab::schedulers {

template <typename T, typename Problem>
concept CrossoverOperator =
    requires(T op, Problem problem, typename Problem::GenomeT genome, std::mt19937& rng) {
        {
            op.cross(problem, genome, genome, rng)
        } -> std::convertible_to<std::pair<typename Problem::GenomeT, typename Problem::GenomeT>>;
    };

template <typename T, typename Problem>
concept LocalSearchOperator =
    requires(T op, const Problem& problem, typename Problem::GenomeT& genome, std::mt19937& rng) {
        { op.improve(problem, genome, rng) } -> std::convertible_to<core::Fitness>;
    };

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
    /**
           * @brief Construct a UCB-based scheduler for a fixed number of operators.
           *
           * Initializes per-operator statistics, sets the exploration constant used in
           * UCB score computation, and binds the RNG to use for tie-breaking.
           *
           * @param num_operators Number of operators to manage; allocates internal stats for each.
           * @param exploration_constant Exploration weight applied to the UCB exploration term.
           * @param rng Random number generator used for random tie-breaking between equal UCB scores.
           */
          explicit UCBScheduler(size_t num_operators, double exploration_constant = 2.0,
                          std::mt19937& rng = get_thread_rng())
        : stats_(num_operators), exploration_constant_(exploration_constant), total_selections_(0),
          rng_(rng) {}

    /**
     * @brief Selects an operator index using the Upper Confidence Bound (UCB) policy.
     *
     * Balances exploitation (operator average reward) and exploration; operators with zero prior
     * selections are prioritized to guarantee initial exploration. Ties among equally-scored
     * operators are broken uniformly at random using the scheduler's RNG. This call also
     * increments the scheduler's internal total selection count.
     *
     * @return int Index of the selected operator.
     *
     * @throws std::logic_error if no operators are configured.
     */
    int select_operator() {
        if (stats_.empty()) {
            throw std::logic_error("UCBScheduler: no operators configured");
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

  private:
    static std::mt19937& get_thread_rng() {
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        return gen;
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
    /**
           * @brief Construct a ThompsonSamplingScheduler managing a fixed number of operators.
           *
           * Initializes per-operator beta distributions and statistics, sets the reward
           * threshold used to determine successes, and uses the provided RNG for sampling.
           *
           * @param num_operators Number of operators to manage.
           * @param reward_threshold Threshold above which an observed reward is treated as a success.
           * @param rng Reference to the random number generator used for sampling distributions.
           */
          explicit ThompsonSamplingScheduler(size_t num_operators, double reward_threshold = 0.0,
                                       std::mt19937& rng = get_thread_rng())
        : distributions_(num_operators), stats_(num_operators), rng_(rng),
          reward_threshold_(reward_threshold) {}

    /**
     * Selects an operator by sampling each operator's Beta distribution and choosing the operator
     * with the largest sampled value.
     *
     * @return int Index (0-based) of the operator with the highest sampled value.
     *
     * @throws std::logic_error If no operators are configured.
     */
    int select_operator() {
        if (distributions_.empty()) {
            throw std::logic_error("ThompsonSamplingScheduler: no operators configured");
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

  private:
    static std::mt19937& get_thread_rng() {
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        return gen;
    }
};

template <typename SchedulerType, typename Problem>
class AdaptiveOperatorSelector {
  private:
    SchedulerType scheduler_;
    std::vector<std::function<std::pair<typename Problem::GenomeT, typename Problem::GenomeT>(
        const Problem&, const typename Problem::GenomeT&, const typename Problem::GenomeT&,
        std::mt19937&)>>
        operators_;
    std::vector<std::string> operator_names_;
    int current_selection_;
    double last_fitness_improvement_;
    bool tracking_improvement_;

  public:
    template <typename... Args>
    explicit AdaptiveOperatorSelector(size_t num_operators, Args&&... args)
        : scheduler_(num_operators, std::forward<Args>(args)...), current_selection_(-1),
          last_fitness_improvement_(0.0), tracking_improvement_(false) {
        operators_.reserve(num_operators);
        operator_names_.reserve(num_operators);
    }

    template <CrossoverOperator<Problem> OpType>
    void add_operator(const OpType& op, const std::string& name) {
        operator_names_.push_back(name);
        operators_.emplace_back(
            [op](const Problem& problem, const typename Problem::GenomeT& parent1,
                 const typename Problem::GenomeT& parent2,
                 std::mt19937& rng) { return op.cross(problem, parent1, parent2, rng); });
    }

    std::pair<typename Problem::GenomeT, typename Problem::GenomeT>
    apply_crossover(const Problem& problem, const typename Problem::GenomeT& parent1,
                    const typename Problem::GenomeT& parent2, std::mt19937& rng) {
        current_selection_ = scheduler_.select_operator();
        tracking_improvement_ = true;

        if (current_selection_ >= 0 && current_selection_ < static_cast<int>(operators_.size())) {
            return operators_[current_selection_](problem, parent1, parent2, rng);
        }

        return {parent1, parent2};
    }

    void report_fitness_improvement(double improvement) {
        if (tracking_improvement_ && current_selection_ >= 0) {
            last_fitness_improvement_ = improvement;
            scheduler_.update_reward(current_selection_, improvement);
            tracking_improvement_ = false;
        }
    }

    void report_fitness_change(double old_fitness, double new_fitness) {
        double improvement = old_fitness - new_fitness; // Assuming minimization
        report_fitness_improvement(improvement);
    }

    const std::vector<OperatorStats>& get_operator_stats() const { return scheduler_.get_stats(); }

    const std::vector<std::string>& get_operator_names() const { return operator_names_; }

    void reset_stats() {
        scheduler_.reset();
        current_selection_ = -1;
        last_fitness_improvement_ = 0.0;
        tracking_improvement_ = false;
    }

    size_t get_operator_count() const { return operators_.size(); }

    int get_last_selection() const { return current_selection_; }
    double get_last_improvement() const { return last_fitness_improvement_; }
};

template <typename Problem>
using UCBOperatorSelector = AdaptiveOperatorSelector<UCBScheduler, Problem>;

template <typename Problem>
using ThompsonOperatorSelector = AdaptiveOperatorSelector<ThompsonSamplingScheduler, Problem>;

template <typename SchedulerType, typename Problem>
class AdaptiveLocalSearchSelector {
  private:
    SchedulerType scheduler_;
    std::vector<
        std::function<core::Fitness(const Problem&, typename Problem::GenomeT&, std::mt19937&)>>
        operators_;
    std::vector<std::string> operator_names_;
    int current_selection_;
    double last_fitness_improvement_;
    double last_execution_time_;
    bool tracking_improvement_;

  public:
    template <typename... Args>
    /**
     * @brief Construct an adaptive local-search selector and prepare internal storage.
     *
     * Initializes the underlying scheduler with num_operators and forwards any additional arguments to the scheduler's constructor.
     *
     * @param num_operators Number of local-search operators to manage; used to initialize the scheduler and reserve internal containers.
     * @tparam Args Variadic types forwarded to the scheduler constructor.
     */
    explicit AdaptiveLocalSearchSelector(size_t num_operators, Args&&... args)
        : scheduler_(num_operators, std::forward<Args>(args)...), current_selection_(-1),
          last_fitness_improvement_(0.0), last_execution_time_(0.0), tracking_improvement_(false) {
        operators_.reserve(num_operators);
        operator_names_.reserve(num_operators);
    }

    template <LocalSearchOperator<Problem> OpType>
    /**
     * @brief Register a local search operator with the selector.
     *
     * Adds the given operator and its name to the internal lists so the scheduler
     * can select and invoke it via op.improve(problem, genome, rng).
     *
     * @tparam OpType Type of the operator; must satisfy LocalSearchOperator<Problem>.
     * @param op Local search operator whose `improve(const Problem&, Problem::GenomeT&, std::mt19937&)`
     *           will be called when selected.
     * @param name Human-readable name for the operator.
     *
     * @throws std::logic_error If adding this operator would exceed the number of operators
     *                         configured in the selector (such operators would never be selected).
     */
    void add_operator(OpType op, const std::string& name) {
        if (operators_.size() >= scheduler_.get_stats().size()) {
            throw std::logic_error(
                "Cannot add more local search operators than the number specified in the "
                "selector's constructor. Extra operators will never be selected.");
        }
        operator_names_.push_back(name);
        operators_.emplace_back([op = std::move(op)](const Problem& problem,
                                                     typename Problem::GenomeT& genome,
                                                     std::mt19937& rng) mutable {
            return op.improve(problem, genome, rng);
        });
    }

    /**
     * @brief Selects a local-search operator, applies it to the provided genome, and records the execution time and selection for later reward reporting.
     *
     * The chosen operator is executed and may modify `genome` in-place. Execution duration is stored in `last_execution_time_` and the selection is recorded so a subsequent call to report_fitness_improvement/report_fitness_change can attribute rewards to the operator.
     *
     * @param problem Problem instance used by the local-search operator.
     * @param genome Genome to be improved; may be modified by the operator.
     * @param rng Random number generator used by the operator.
     *
     * @return core::Fitness The fitness value returned by the executed local-search operator.
     *
     * @throws std::logic_error If no operators have been added.
     * @throws std::out_of_range If the scheduler selects an operator index outside the range of added operators.
     */
    core::Fitness apply_local_search(const Problem& problem, typename Problem::GenomeT& genome,
                                     std::mt19937& rng) {
        if (operators_.empty()) {
            throw std::logic_error("Cannot apply local search: no operators have been added.");
        }

        current_selection_ = scheduler_.select_operator();
        tracking_improvement_ = true;

        auto start_time = std::chrono::high_resolution_clock::now();

        if (current_selection_ < 0 || current_selection_ >= static_cast<int>(operators_.size())) {
            throw std::out_of_range(
                "Selected operator index is out of bounds. This can happen if the number of "
                "operators added via add_operator() does not match the num_operators argument in "
                "the constructor.");
        }
        core::Fitness result = operators_[current_selection_](problem, genome, rng);

        auto end_time = std::chrono::high_resolution_clock::now();
        last_execution_time_ = std::chrono::duration<double>(end_time - start_time).count();

        return result;
    }

    /**
     * @brief Record and report a tracked fitness improvement for the most recently selected operator.
     *
     * If an improvement is currently being tracked and a valid operator was selected, stores the
     * provided improvement value, forwards it to the scheduler as the operator's reward, and stops
     * tracking further improvements for that selection.
     *
     * @param improvement Amount of fitness improvement computed as (old_fitness - new_fitness); a
     *                    positive value indicates that fitness improved.
     */
    void report_fitness_improvement(double improvement) {
        if (tracking_improvement_ && current_selection_ >= 0) {
            last_fitness_improvement_ = improvement;
            scheduler_.update_reward(current_selection_, improvement);
            tracking_improvement_ = false;
        }
    }

    /**
     * @brief Compute the fitness improvement between two evaluations and forward it for operator scoring.
     *
     * The improvement is calculated as `old_fitness - new_fitness` and propagated to the selector's
     * reporting mechanism so the scheduler can update operator rewards.
     *
     * @param old_fitness Fitness value before the change.
     * @param new_fitness Fitness value after the change.
     */
    void report_fitness_change(double old_fitness, double new_fitness) {
        double improvement = old_fitness - new_fitness;
        report_fitness_improvement(improvement);
    }

    /**
 * @brief Access the current operator statistics maintained by the scheduler.
 *
 * @return const std::vector<OperatorStats>& Const reference to the vector of per-operator statistics, each containing metrics such as `total_reward`, `selection_count`, `avg_reward`, `success_rate`, and `success_count`.
 */
const std::vector<OperatorStats>& get_operator_stats() const { return scheduler_.get_stats(); }

    /**
 * @brief Get the names of registered operators in insertion order.
 *
 * @return const std::vector<std::string>& A reference to the vector holding operator names, ordered by when they were added.
 */
const std::vector<std::string>& get_operator_names() const { return operator_names_; }

    /**
     * @brief Reset scheduler statistics and clear last-selection tracking.
     *
     * Resets the underlying scheduler's statistics and restores selection/tracking
     * state (current selection index, last recorded fitness improvement,
     * last execution time, and the improvement-tracking flag) to their initial values.
     */
    void reset_stats() {
        scheduler_.reset();
        current_selection_ = -1;
        last_fitness_improvement_ = 0.0;
        last_execution_time_ = 0.0;
        tracking_improvement_ = false;
    }

    /**
 * @brief Retrieves the number of registered operators.
 *
 * @return size_t The number of operators currently stored.
 */
size_t get_operator_count() const { return operators_.size(); }

    /**
 * @brief Retrieve the index of the last selected operator.
 *
 * @return int Index of the last selected operator, or -1 if no operator has been selected.
 */
int get_last_selection() const { return current_selection_; }
    /**
 * @brief Retrieves the last recorded fitness improvement from the most recent operator execution.
 *
 * @return double Last recorded fitness improvement; 0.0 if no improvement has been recorded yet.
 */
double get_last_improvement() const { return last_fitness_improvement_; }
    /**
 * @brief Retrieves the elapsed time of the last executed local search operator.
 *
 * @return double Elapsed time in seconds for the last operator execution; 0.0 if no execution has occurred.
 */
double get_last_execution_time() const { return last_execution_time_; }
};

template <typename Problem>
using UCBLocalSearchSelector = AdaptiveLocalSearchSelector<UCBScheduler, Problem>;

template <typename Problem>
using ThompsonLocalSearchSelector = AdaptiveLocalSearchSelector<ThompsonSamplingScheduler, Problem>;

} // namespace evolab::schedulers