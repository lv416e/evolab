#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <toml.hpp>

namespace evolab::core {
struct GAConfig; // Forward declaration
}

namespace evolab::config {

/// Custom exception for configuration validation errors
class ConfigValidationError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

/// GA core configuration parameters
struct GAConfig {
    std::size_t population_size = 256;  // Default population size for reasonable performance
    std::size_t max_generations = 1000; // Default generation limit for convergence
    double elite_rate = 0.02;           // Preserve top 2% by default for elitism
    std::uint64_t seed = 1;             // Reproducible seed by default
};

/// Crossover operator configuration
struct CrossoverConfig {
    std::string type = "PMX"; // Default to PMX for permutation problems
    double probability = 0.8; // High crossover probability for exploration
};

/// Mutation operator configuration
struct MutationConfig {
    std::string type = "swap"; // Simple swap mutation as default
    double probability = 0.1;  // Low mutation for stability
};

/// Selection operator configuration
struct SelectionConfig {
    std::string type = "tournament"; // Tournament selection for balance
    std::size_t tournament_size = 3; // Small tournament for moderate pressure
    double selection_pressure = 1.5; // For ranking selection
};

/// Combined operators configuration
struct OperatorsConfig {
    CrossoverConfig crossover;
    MutationConfig mutation;
    SelectionConfig selection;
};

/// Local search configuration for memetic algorithms
struct LocalSearchConfig {
    bool enabled = false;                 // Disabled by default for basic GA
    std::string type = "2-opt";           // 2-opt as standard local search
    std::size_t max_iterations = 100;     // Iteration limit to control runtime
    double probability = 0.3;             // Apply to 30% of population
    std::size_t candidate_list_size = 40; // K-nearest neighbors for efficiency
};

/// Multi-Armed Bandit scheduler configuration
struct SchedulerConfig {
    bool enabled = false;               // Disabled by default
    std::string type = "thompson";      // Thompson sampling as default
    std::vector<std::string> operators; // List of operators to schedule
    std::size_t window_size = 100;      // Performance tracking window
    double exploration_rate = 2.0;      // UCB exploration parameter
};

/// Termination criteria configuration
struct TerminationConfig {
    std::size_t max_generations = 1000;       // Generation limit
    double time_limit_minutes = 60.0;         // Time limit in minutes
    std::size_t stagnation_generations = 100; // No improvement threshold
    double target_fitness = 0.0;              // Target fitness (problem-specific)
};

/// Logging and output configuration
struct LoggingConfig {
    std::size_t log_interval = 50;     // Log every N generations
    bool verbose = false;              // Detailed output disabled by default
    bool track_diversity = false;      // Diversity tracking disabled by default
    bool save_evolution_curve = false; // CSV output disabled by default
};

/// Parallel execution configuration
struct ParallelConfig {
    bool enabled = false;        // Sequential by default
    std::size_t threads = 0;     // 0 = auto-detect
    std::size_t chunk_size = 64; // Work chunk size for load balancing
};

/// Population diversity maintenance configuration
struct DiversityConfig {
    bool enabled = false;                  // Diversity tracking disabled by default
    double minimum_diversity = 0.1;        // Minimum required population diversity
    double restart_threshold = 0.05;       // Diversity threshold for population restart
    std::size_t measurement_interval = 10; // Measure diversity every N generations
};

/// Command-line override structure
/// Contains optional overrides for configuration parameters
struct ConfigOverrides {
    std::optional<std::size_t> population_size;
    std::optional<std::size_t> max_generations;
    std::optional<double> crossover_probability;
    std::optional<double> mutation_probability;
    std::optional<std::uint64_t> seed;
    std::optional<std::string> algorithm; // For selecting preset configurations
};

/// Complete configuration structure
struct Config {
    GAConfig ga;
    OperatorsConfig operators;
    LocalSearchConfig local_search;
    SchedulerConfig scheduler;
    TerminationConfig termination;
    LoggingConfig logging;
    ParallelConfig parallel;
    DiversityConfig diversity;

    /// Load configuration from TOML file
    /// Validates all parameters and applies defaults for missing values
    static Config from_file(const std::string& filepath);

    /// Load configuration from TOML string
    static Config from_string(const std::string& toml_string);

    /// Validate configuration parameters
    /// Throws ConfigValidationError if any parameter is invalid
    void validate() const;

    /// Merge with another config (other takes precedence)
    /// Useful for command-line overrides
    void merge(const Config& other);

    /// Export configuration to TOML string
    std::string to_toml() const;

    /// Convert to core::GAConfig for GA execution
    /// Integrates termination and logging settings
    core::GAConfig to_ga_config() const;

    /// Apply command-line overrides to configuration
    /// Overrides take precedence over loaded values
    void apply_overrides(const ConfigOverrides& overrides);

  private:
    /// Parse GA configuration from TOML table
    static GAConfig parse_ga(const toml::value& data);

    /// Parse operators configuration from TOML table
    static OperatorsConfig parse_operators(const toml::value& data);

    /// Parse local search configuration from TOML table
    static LocalSearchConfig parse_local_search(const toml::value& data);

    /// Parse scheduler configuration from TOML table
    static SchedulerConfig parse_scheduler(const toml::value& data);

    /// Parse termination configuration from TOML table
    static TerminationConfig parse_termination(const toml::value& data);

    /// Parse logging configuration from TOML table
    static LoggingConfig parse_logging(const toml::value& data);

    /// Parse parallel configuration from TOML table
    static ParallelConfig parse_parallel(const toml::value& data);

    /// Parse diversity configuration from TOML table
    static DiversityConfig parse_diversity(const toml::value& data);
};

// Implementation of Config methods

inline Config Config::from_file(const std::string& filepath) {
    const auto data = toml::parse(filepath);
    Config config;

    // Parse each section if it exists, otherwise use defaults
    if (data.contains("ga")) {
        config.ga = parse_ga(data);
    }

    if (data.contains("operators")) {
        config.operators = parse_operators(data);
    }

    if (data.contains("local_search")) {
        config.local_search = parse_local_search(data);
    }

    if (data.contains("scheduler")) {
        config.scheduler = parse_scheduler(data);
    }

    if (data.contains("termination")) {
        config.termination = parse_termination(data);
    }

    if (data.contains("logging")) {
        config.logging = parse_logging(data);
    }

    if (data.contains("parallel")) {
        config.parallel = parse_parallel(data);
    }

    if (data.contains("diversity")) {
        config.diversity = parse_diversity(data);
    }

    // Validate the complete configuration
    config.validate();
    return config;
}

inline Config Config::from_string(const std::string& toml_string) {
    std::istringstream iss(toml_string);
    const auto data = toml::parse(iss, "config_string");
    Config config;

    if (data.contains("ga")) {
        config.ga = parse_ga(data);
    }

    if (data.contains("operators")) {
        config.operators = parse_operators(data);
    }

    if (data.contains("local_search")) {
        config.local_search = parse_local_search(data);
    }

    if (data.contains("scheduler")) {
        config.scheduler = parse_scheduler(data);
    }

    if (data.contains("termination")) {
        config.termination = parse_termination(data);
    }

    if (data.contains("logging")) {
        config.logging = parse_logging(data);
    }

    if (data.contains("parallel")) {
        config.parallel = parse_parallel(data);
    }

    config.validate();
    return config;
}

inline void Config::validate() const {
    // Validate GA configuration
    if (ga.population_size == 0) {
        throw ConfigValidationError("Population size must be positive");
    }

    if (ga.elite_rate < 0.0 || ga.elite_rate > 1.0) {
        throw ConfigValidationError("Elite rate must be in [0,1]");
    }

    // Validate operator probabilities
    if (operators.crossover.probability < 0.0 || operators.crossover.probability > 1.0) {
        throw ConfigValidationError("Crossover probability must be in [0,1]");
    }

    if (operators.mutation.probability < 0.0 || operators.mutation.probability > 1.0) {
        throw ConfigValidationError("Mutation probability must be in [0,1]");
    }

    if (operators.selection.tournament_size == 0) {
        throw ConfigValidationError("Tournament size must be positive");
    }

    // Validate local search configuration
    if (local_search.probability < 0.0 || local_search.probability > 1.0) {
        throw ConfigValidationError("Local search probability must be in [0,1]");
    }

    if (local_search.enabled && local_search.max_iterations == 0) {
        throw ConfigValidationError("Local search iterations must be positive when enabled");
    }

    // Validate scheduler configuration
    if (scheduler.enabled && scheduler.operators.empty()) {
        throw ConfigValidationError("Scheduler requires at least one operator");
    }

    if (scheduler.window_size == 0) {
        throw ConfigValidationError("Scheduler window size must be positive");
    }

    // Validate termination criteria
    if (termination.time_limit_minutes < 0.0) {
        throw ConfigValidationError("Time limit cannot be negative");
    }

    // Validate parallel configuration
    // threads = 0 is valid (auto-detect), but negative is not
    if (parallel.chunk_size == 0) {
        throw ConfigValidationError("Parallel chunk size must be positive");
    }
}

inline GAConfig Config::parse_ga(const toml::value& data) {
    GAConfig ga;
    const auto& ga_table = toml::find(data, "ga");

    if (ga_table.contains("population_size")) {
        ga.population_size = toml::find<std::size_t>(ga_table, "population_size");
    }

    if (ga_table.contains("max_generations")) {
        ga.max_generations = toml::find<std::size_t>(ga_table, "max_generations");
    }

    if (ga_table.contains("elite_rate")) {
        ga.elite_rate = toml::find<double>(ga_table, "elite_rate");
    }

    if (ga_table.contains("seed")) {
        ga.seed = toml::find<std::uint64_t>(ga_table, "seed");
    }

    return ga;
}

inline OperatorsConfig Config::parse_operators(const toml::value& data) {
    OperatorsConfig ops;
    const auto& ops_table = toml::find(data, "operators");

    // Parse crossover configuration
    if (ops_table.contains("crossover")) {
        const auto& crossover = toml::find(ops_table, "crossover");
        if (crossover.contains("type")) {
            ops.crossover.type = toml::find<std::string>(crossover, "type");
        }
        if (crossover.contains("probability")) {
            ops.crossover.probability = toml::find<double>(crossover, "probability");
        }
    }

    // Parse mutation configuration
    if (ops_table.contains("mutation")) {
        const auto& mutation = toml::find(ops_table, "mutation");
        if (mutation.contains("type")) {
            ops.mutation.type = toml::find<std::string>(mutation, "type");
        }
        if (mutation.contains("probability")) {
            ops.mutation.probability = toml::find<double>(mutation, "probability");
        }
    }

    // Parse selection configuration
    if (ops_table.contains("selection")) {
        const auto& selection = toml::find(ops_table, "selection");
        if (selection.contains("type")) {
            ops.selection.type = toml::find<std::string>(selection, "type");
        }
        if (selection.contains("tournament_size")) {
            ops.selection.tournament_size = toml::find<std::size_t>(selection, "tournament_size");
        }
        if (selection.contains("selection_pressure")) {
            ops.selection.selection_pressure = toml::find<double>(selection, "selection_pressure");
        }
    }

    return ops;
}

inline LocalSearchConfig Config::parse_local_search(const toml::value& data) {
    LocalSearchConfig ls;
    const auto& ls_table = toml::find(data, "local_search");

    if (ls_table.contains("enabled")) {
        ls.enabled = toml::find<bool>(ls_table, "enabled");
    }

    if (ls_table.contains("type")) {
        ls.type = toml::find<std::string>(ls_table, "type");
    }

    if (ls_table.contains("max_iterations")) {
        ls.max_iterations = toml::find<std::size_t>(ls_table, "max_iterations");
    }

    if (ls_table.contains("probability")) {
        ls.probability = toml::find<double>(ls_table, "probability");
    }

    if (ls_table.contains("candidate_list_size")) {
        ls.candidate_list_size = toml::find<std::size_t>(ls_table, "candidate_list_size");
    }

    return ls;
}

inline SchedulerConfig Config::parse_scheduler(const toml::value& data) {
    SchedulerConfig sched;
    const auto& sched_table = toml::find(data, "scheduler");

    if (sched_table.contains("enabled")) {
        sched.enabled = toml::find<bool>(sched_table, "enabled");
    }

    if (sched_table.contains("type")) {
        sched.type = toml::find<std::string>(sched_table, "type");
    }

    if (sched_table.contains("operators")) {
        sched.operators = toml::find<std::vector<std::string>>(sched_table, "operators");
    }

    if (sched_table.contains("window_size")) {
        sched.window_size = toml::find<std::size_t>(sched_table, "window_size");
    }

    if (sched_table.contains("exploration_rate")) {
        sched.exploration_rate = toml::find<double>(sched_table, "exploration_rate");
    }

    return sched;
}

inline TerminationConfig Config::parse_termination(const toml::value& data) {
    TerminationConfig term;
    const auto& term_table = toml::find(data, "termination");

    if (term_table.contains("max_generations")) {
        term.max_generations = toml::find<std::size_t>(term_table, "max_generations");
    }

    if (term_table.contains("time_limit_minutes")) {
        // Handle both integer and floating-point values for time limit
        const auto& time_val = term_table.at("time_limit_minutes");
        if (time_val.is_integer()) {
            term.time_limit_minutes =
                static_cast<double>(toml::find<std::int64_t>(term_table, "time_limit_minutes"));
        } else {
            term.time_limit_minutes = toml::find<double>(term_table, "time_limit_minutes");
        }
    }

    if (term_table.contains("stagnation_generations")) {
        term.stagnation_generations = toml::find<std::size_t>(term_table, "stagnation_generations");
    }

    if (term_table.contains("target_fitness")) {
        // Handle both integer and floating-point values for target fitness
        const auto& fitness_val = term_table.at("target_fitness");
        if (fitness_val.is_integer()) {
            term.target_fitness =
                static_cast<double>(toml::find<std::int64_t>(term_table, "target_fitness"));
        } else {
            term.target_fitness = toml::find<double>(term_table, "target_fitness");
        }
    }

    return term;
}

inline LoggingConfig Config::parse_logging(const toml::value& data) {
    LoggingConfig log;
    const auto& log_table = toml::find(data, "logging");

    if (log_table.contains("log_interval")) {
        log.log_interval = toml::find<std::size_t>(log_table, "log_interval");
    }

    if (log_table.contains("verbose")) {
        log.verbose = toml::find<bool>(log_table, "verbose");
    }

    if (log_table.contains("track_diversity")) {
        log.track_diversity = toml::find<bool>(log_table, "track_diversity");
    }

    if (log_table.contains("save_evolution_curve")) {
        log.save_evolution_curve = toml::find<bool>(log_table, "save_evolution_curve");
    }

    return log;
}

inline ParallelConfig Config::parse_parallel(const toml::value& data) {
    ParallelConfig par;
    const auto& par_table = toml::find(data, "parallel");

    if (par_table.contains("enabled")) {
        par.enabled = toml::find<bool>(par_table, "enabled");
    }

    if (par_table.contains("threads")) {
        par.threads = toml::find<std::size_t>(par_table, "threads");
    }

    if (par_table.contains("chunk_size")) {
        par.chunk_size = toml::find<std::size_t>(par_table, "chunk_size");
    }

    return par;
}

inline DiversityConfig Config::parse_diversity(const toml::value& data) {
    DiversityConfig div;
    const auto& div_table = toml::find(data, "diversity");

    if (div_table.contains("enabled")) {
        div.enabled = toml::find<bool>(div_table, "enabled");
    }

    if (div_table.contains("minimum_diversity")) {
        div.minimum_diversity = toml::find<double>(div_table, "minimum_diversity");
    }

    if (div_table.contains("restart_threshold")) {
        div.restart_threshold = toml::find<double>(div_table, "restart_threshold");
    }

    if (div_table.contains("measurement_interval")) {
        div.measurement_interval = toml::find<std::size_t>(div_table, "measurement_interval");
    }

    return div;
}

inline void Config::merge(const Config& other) {
    // Simple merge strategy: other config values override this config
    // In practice, this could be more sophisticated
    // For now, we just copy the other config
    *this = other;
}

inline std::string Config::to_toml() const {
    toml::value root;

    // GA section
    toml::value ga_table;
    ga_table["population_size"] = ga.population_size;
    ga_table["max_generations"] = ga.max_generations;
    ga_table["elite_rate"] = ga.elite_rate;
    ga_table["seed"] = ga.seed;
    root["ga"] = ga_table;

    // Operators section
    toml::value ops_table;
    toml::value crossover_table;
    crossover_table["type"] = operators.crossover.type;
    crossover_table["probability"] = operators.crossover.probability;
    ops_table["crossover"] = crossover_table;

    toml::value mutation_table;
    mutation_table["type"] = operators.mutation.type;
    mutation_table["probability"] = operators.mutation.probability;
    ops_table["mutation"] = mutation_table;

    toml::value selection_table;
    selection_table["type"] = operators.selection.type;
    selection_table["tournament_size"] = operators.selection.tournament_size;
    selection_table["selection_pressure"] = operators.selection.selection_pressure;
    ops_table["selection"] = selection_table;
    root["operators"] = ops_table;

    // Local search section
    toml::value ls_table;
    ls_table["enabled"] = local_search.enabled;
    ls_table["type"] = local_search.type;
    ls_table["max_iterations"] = local_search.max_iterations;
    ls_table["probability"] = local_search.probability;
    ls_table["candidate_list_size"] = local_search.candidate_list_size;
    root["local_search"] = ls_table;

    // Scheduler section
    toml::value sched_table;
    sched_table["enabled"] = scheduler.enabled;
    sched_table["type"] = scheduler.type;
    sched_table["operators"] = scheduler.operators;
    sched_table["window_size"] = scheduler.window_size;
    sched_table["exploration_rate"] = scheduler.exploration_rate;
    root["scheduler"] = sched_table;

    // Termination section
    toml::value term_table;
    term_table["max_generations"] = termination.max_generations;
    term_table["time_limit_minutes"] = termination.time_limit_minutes;
    term_table["stagnation_generations"] = termination.stagnation_generations;
    term_table["target_fitness"] = termination.target_fitness;
    root["termination"] = term_table;

    // Logging section
    toml::value log_table;
    log_table["log_interval"] = logging.log_interval;
    log_table["verbose"] = logging.verbose;
    log_table["track_diversity"] = logging.track_diversity;
    log_table["save_evolution_curve"] = logging.save_evolution_curve;
    root["logging"] = log_table;

    // Parallel section
    toml::value par_table;
    par_table["enabled"] = parallel.enabled;
    par_table["threads"] = parallel.threads;
    par_table["chunk_size"] = parallel.chunk_size;
    root["parallel"] = par_table;

    // Diversity section
    toml::value div_table;
    div_table["enabled"] = diversity.enabled;
    div_table["minimum_diversity"] = diversity.minimum_diversity;
    div_table["restart_threshold"] = diversity.restart_threshold;
    div_table["measurement_interval"] = diversity.measurement_interval;
    root["diversity"] = div_table;

    std::stringstream ss;
    ss << toml::format(root);
    return ss.str();
}

inline void Config::apply_overrides(const ConfigOverrides& overrides) {
    // Apply overrides only if they are set
    if (overrides.population_size.has_value()) {
        ga.population_size = overrides.population_size.value();
    }

    if (overrides.max_generations.has_value()) {
        ga.max_generations = overrides.max_generations.value();
        termination.max_generations = overrides.max_generations.value();
    }

    if (overrides.crossover_probability.has_value()) {
        operators.crossover.probability = overrides.crossover_probability.value();
    }

    if (overrides.mutation_probability.has_value()) {
        operators.mutation.probability = overrides.mutation_probability.value();
    }

    if (overrides.seed.has_value()) {
        ga.seed = overrides.seed.value();
    }

    // Re-validate after applying overrides
    validate();
}

} // namespace evolab::config

// Implementation that depends on core::GAConfig
// Must be after namespace closing to access evolab::core
#include <evolab/core/ga.hpp>

namespace evolab::config {

inline core::GAConfig Config::to_ga_config() const {
    core::GAConfig ga_config;

    // Direct mappings from GA config
    ga_config.population_size = ga.population_size;
    ga_config.max_generations = ga.max_generations;
    ga_config.elite_ratio = ga.elite_rate; // Note: different name in core
    ga_config.seed = ga.seed;

    // Operator probabilities
    ga_config.crossover_prob = operators.crossover.probability;
    ga_config.mutation_prob = operators.mutation.probability;

    // Termination settings
    ga_config.max_generations = termination.max_generations;
    ga_config.stagnation_limit = termination.stagnation_generations;

    // Convert time limit from minutes to milliseconds
    ga_config.time_limit =
        std::chrono::milliseconds(static_cast<long>(termination.time_limit_minutes * 60 * 1000));

    // Logging settings
    ga_config.log_interval = logging.log_interval;
    ga_config.enable_diversity_tracking = diversity.enabled;

    // Diversity parameters from configuration
    if (diversity.enabled) {
        ga_config.diversity_threshold = diversity.restart_threshold;
        ga_config.diversity_max_samples = diversity.measurement_interval;
    }

    // Legacy support: also enable diversity tracking if logging.track_diversity is set
    if (logging.track_diversity && !diversity.enabled) {
        ga_config.enable_diversity_tracking = true;
        ga_config.diversity_threshold = 0.01; // Fallback default
        ga_config.diversity_max_samples = 50; // Fallback default
    }

    return ga_config;
}

} // namespace evolab::config