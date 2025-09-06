#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <evolab/evolab.hpp>
#include <nlohmann/json.hpp>

using namespace evolab;

// Default values for random TSP instance fallback
namespace {
constexpr int DEFAULT_RANDOM_CITIES = 100;
constexpr double DEFAULT_MAX_COORD = 1000.0;
} // namespace

/// Command-line arguments structure
/// Wraps both the configuration and runtime options
struct CLIConfig {
    std::string instance_file;
    std::string config_file;
    std::string algorithm = "basic";
    std::size_t population = 256;
    std::size_t generations = 1000;
    double crossover_prob = 0.9;
    double mutation_prob = 0.1;
    std::uint64_t seed = 1;
    bool verbose = false;
    std::string output_file;
    bool json_output = false;
    std::string json_file;

    // Track which values were explicitly set via command line
    bool has_population_override = false;
    bool has_generations_override = false;
    bool has_crossover_override = false;
    bool has_mutation_override = false;
    bool has_seed_override = false;

    // Convert CLI overrides to configuration overrides
    config::ConfigOverrides to_overrides() const {
        config::ConfigOverrides overrides;
        // Only set overrides if explicitly provided via command line
        if (has_population_override) {
            overrides.population_size = population;
        }
        if (has_generations_override) {
            overrides.max_generations = generations;
        }
        if (has_crossover_override) {
            overrides.crossover_probability = crossover_prob;
        }
        if (has_mutation_override) {
            overrides.mutation_probability = mutation_prob;
        }
        if (has_seed_override) {
            overrides.seed = seed;
        }
        return overrides;
    }
};

/// Get git commit hash (from CMake build)
std::string get_git_hash() {
#ifdef GIT_HASH
    return GIT_HASH;
#else
    return "unknown";
#endif
}

/// Get hostname (from CMake build)
std::string get_hostname() {
#ifdef BUILD_HOSTNAME
    return BUILD_HOSTNAME;
#else
    return "unknown";
#endif
}

/// Get build configuration info
std::string get_build_config() {
#ifdef NDEBUG
    std::string mode = "Release";
#else
    std::string mode = "Debug";
#endif

#ifdef __clang__
    std::string compiler =
        "Clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__);
#elif defined(__GNUC__)
    std::string compiler = "GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
#elif defined(_MSC_VER)
    std::string compiler = "MSVC " + std::to_string(_MSC_VER);
#else
    std::string compiler = "Unknown";
#endif

    return mode + " (" + compiler + ")";
}

/// Print usage information
void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -h, --help              Show this help message\n"
              << "  --config FILE           Load configuration from TOML file\n"
              << "  -i, --instance FILE     TSP instance file (random if not specified)\n"
              << "  -a, --algorithm ALGO    Algorithm: basic, advanced (default: basic)\n"
              << "  -p, --population SIZE   Population size (default: 256)\n"
              << "  -g, --generations NUM   Max generations (default: 1000)\n"
              << "  -c, --crossover PROB    Crossover probability (default: 0.9)\n"
              << "  -m, --mutation PROB     Mutation probability (default: 0.1)\n"
              << "  -s, --seed SEED         Random seed (default: 1)\n"
              << "  -v, --verbose           Verbose output\n"
              << "  -o, --output FILE       Output file for best tour\n"
              << "  --json                  Enable JSON output format\n"
              << "  --json-file FILE        Write JSON results to file\n"
              << "\nExamples:\n"
              << "  " << program_name << " --config config/basic.toml --instance data/pr76.tsp\n"
              << "  " << program_name << " --algorithm advanced --population 512\n"
              << "  " << program_name << " --verbose --output solution.tour\n"
              << "  " << program_name << " --json --json-file results.json\n";
}

/// Parse command line arguments
CLIConfig parse_args(int argc, char** argv) {
    CLIConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (arg == "--config" && i + 1 < argc) {
            config.config_file = argv[++i];
        } else if ((arg == "-i" || arg == "--instance") && i + 1 < argc) {
            config.instance_file = argv[++i];
        } else if ((arg == "-a" || arg == "--algorithm") && i + 1 < argc) {
            config.algorithm = argv[++i];
        } else if ((arg == "-p" || arg == "--population") && i + 1 < argc) {
            config.population = std::stoull(argv[++i]);
            config.has_population_override = true;
        } else if ((arg == "-g" || arg == "--generations") && i + 1 < argc) {
            config.generations = std::stoull(argv[++i]);
            config.has_generations_override = true;
        } else if ((arg == "-c" || arg == "--crossover") && i + 1 < argc) {
            config.crossover_prob = std::stod(argv[++i]);
            config.has_crossover_override = true;
        } else if ((arg == "-m" || arg == "--mutation") && i + 1 < argc) {
            config.mutation_prob = std::stod(argv[++i]);
            config.has_mutation_override = true;
        } else if ((arg == "-s" || arg == "--seed") && i + 1 < argc) {
            config.seed = std::stoull(argv[++i]);
            config.has_seed_override = true;
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            config.output_file = argv[++i];
        } else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        } else if (arg == "--json") {
            config.json_output = true;
        } else if (arg == "--json-file" && i + 1 < argc) {
            config.json_file = argv[++i];
            config.json_output = true;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            std::exit(1);
        }
    }

    return config;
}

/// Create TSP problem from CLI config
problems::TSP create_problem(const CLIConfig& cli_config, std::uint64_t seed) {
    if (cli_config.instance_file.empty()) {
        // Create random TSP instance
        if (!cli_config.json_output) {
            std::cout << "Creating random TSP instance with " << DEFAULT_RANDOM_CITIES
                      << " cities...\n";
        }
        return problems::create_random_tsp(DEFAULT_RANDOM_CITIES, DEFAULT_MAX_COORD, seed);
    } else {
        // Load TSPLIB instance
        if (!cli_config.json_output) {
            std::cout << "Loading TSPLIB instance: " << cli_config.instance_file << "\n";
        }
        try {
            io::TSPLIBParser parser;
            auto instance = parser.parse_file(cli_config.instance_file);
            if (!cli_config.json_output) {
                std::cout << "Loaded: " << instance.name << " (" << instance.dimension
                          << " cities)\n";
                if (!instance.comment.empty()) {
                    std::cout << "Comment: " << instance.comment << "\n";
                }
            }
            return problems::TSP::from_tsplib(instance);
        } catch (const std::exception& e) {
            if (!cli_config.json_output) {
                std::cerr << "Failed to load TSPLIB file: " << e.what() << "\n";
                std::cerr << "Using random instance instead.\n";
            }
            return problems::create_random_tsp(DEFAULT_RANDOM_CITIES, DEFAULT_MAX_COORD, seed);
        }
    }
}

/// Write tour to file
void write_tour(const std::string& filename, const problems::TSP::GenomeT& tour, double fitness,
                bool json_output = false) {
    std::ofstream file(filename);
    if (!file) {
        if (!json_output) {
            std::cerr << "Could not open output file: " << filename << "\n";
        }
        return;
    }

    file << "# TSP Tour - Fitness: " << fitness << "\n";
    file << "# Tour length: " << tour.size() << "\n";

    for (std::size_t i = 0; i < tour.size(); ++i) {
        file << tour[i];
        if (i < tour.size() - 1)
            file << " ";
    }
    file << "\n";

    if (!json_output) {
        std::cout << "Tour written to: " << filename << "\n";
    }
}

/// Write JSON output with full metadata
void write_json_output(const auto& result, const CLIConfig& cli_config, const config::Config& cfg,
                       const problems::TSP& tsp, double runtime, const std::string& filename = "") {
    using json = nlohmann::json;

    // Create JSON object
    json output;

    // Metadata section
    output["metadata"] = {{"version", VERSION},
                          {"git_hash", get_git_hash()},
                          {"hostname", get_hostname()},
                          {"build_config", get_build_config()},
                          {"timestamp", std::time(nullptr)},
                          {"runtime_seconds", runtime}};

    // Configuration section
    output["configuration"] = {{"instance_file", cli_config.instance_file},
                               {"algorithm", cli_config.algorithm},
                               {"population_size", cfg.ga.population_size},
                               {"max_generations", cfg.ga.max_generations},
                               {"crossover_probability", cfg.operators.crossover.probability},
                               {"mutation_probability", cfg.operators.mutation.probability},
                               {"seed", cfg.ga.seed}};

    // Problem section
    output["problem"] = {{"type", "TSP"}, {"dimension", tsp.num_cities()}};

    // Results section
    json results;
    results["best_fitness"] = result.best_fitness.value;
    results["generations_used"] = result.generations;
    results["evaluations_performed"] = result.evaluations;
    results["converged"] = result.converged;

    // Best tour (first 10 cities for brevity)
    json tour_sample = json::array();
    auto sample_size = std::min(static_cast<size_t>(10), result.best_genome.size());
    for (size_t i = 0; i < sample_size; ++i) {
        tour_sample.push_back(result.best_genome[i]);
    }
    if (result.best_genome.size() > 10) {
        tour_sample.push_back("...");
    }
    results["best_tour_sample"] = tour_sample;
    results["tour_length"] = result.best_genome.size();
    output["results"] = results;

    // Evolution history section (last 5 entries)
    json history = json::array();
    auto history_start = result.history.size() > 5 ? result.history.size() - 5 : 0;
    for (size_t i = history_start; i < result.history.size(); ++i) {
        const auto& stats = result.history[i];
        history.push_back({{"generation", stats.generation},
                           {"best_fitness", stats.best_fitness.value},
                           {"mean_fitness", stats.mean_fitness.value},
                           {"diversity", stats.diversity},
                           {"elapsed_ms", stats.elapsed_time.count()}});
    }
    output["evolution_history"] = history;

    // Output to file or stdout
    if (!filename.empty()) {
        std::ofstream file(filename);
        if (file) {
            file << output.dump(2); // Pretty print with 2 spaces indentation
            if (!cli_config.json_output) {
                std::cout << "JSON results written to: " << filename << "\n";
            }
        } else {
            std::cerr << "Could not open JSON output file: " << filename << "\n";
        }
    } else {
        std::cout << output.dump(2) << "\n";
    }
}

/// Print statistics
void print_stats(const auto& result, const CLIConfig& cli_config, double runtime) {
    std::cout << "\n=== Results ===\n";
    std::cout << "Best fitness: " << std::fixed << std::setprecision(2) << result.best_fitness.value
              << "\n";
    std::cout << "Generations: " << result.generations << "\n";
    std::cout << "Evaluations: " << result.evaluations << "\n";
    std::cout << "Runtime: " << std::fixed << std::setprecision(3) << runtime << " seconds\n";
    std::cout << "Converged: " << (result.converged ? "Yes" : "No") << "\n";

    if (cli_config.verbose && !result.history.empty()) {
        std::cout << "\n=== Evolution History ===\n";
        std::cout << std::setw(10) << "Gen" << std::setw(15) << "Best" << std::setw(15) << "Mean"
                  << std::setw(15) << "Diversity" << std::setw(12) << "Time(ms)\n";
        std::cout << std::string(67, '-') << "\n";

        for (const auto& stats : result.history) {
            std::cout << std::setw(10) << stats.generation << std::setw(15) << std::fixed
                      << std::setprecision(2) << stats.best_fitness.value << std::setw(15)
                      << std::fixed << std::setprecision(2) << stats.mean_fitness.value
                      << std::setw(15) << std::fixed << std::setprecision(4) << stats.diversity
                      << std::setw(12) << stats.elapsed_time.count() << "\n";
        }
    }
}

int main(int argc, char** argv) {
    try {
        auto cli_config = parse_args(argc, argv);

        // Only print header if not in JSON output mode
        if (!cli_config.json_output) {
            std::cout << "EvoLab TSP Solver v" << VERSION << "\n";
            std::cout << std::string(30, '=') << "\n";
        }

        // Load or create configuration
        config::Config cfg;
        if (!cli_config.config_file.empty()) {
            // Load configuration from TOML file
            if (!cli_config.json_output) {
                std::cout << "Loading configuration from: " << cli_config.config_file << "\n";
            }
            cfg = config::Config::from_file(cli_config.config_file);

            // Apply command-line overrides
            cfg.apply_overrides(cli_config.to_overrides());
        } else {
            // Create default configuration from CLI args
            cfg.ga.population_size = cli_config.population;
            cfg.ga.max_generations = cli_config.generations;
            cfg.operators.crossover.probability = cli_config.crossover_prob;
            cfg.operators.mutation.probability = cli_config.mutation_prob;
            cfg.ga.seed = cli_config.seed;
            cfg.termination.max_generations = cli_config.generations;
            cfg.logging.verbose = cli_config.verbose;
            cfg.logging.log_interval = cli_config.verbose ? 50 : 100;
        }

        // Create problem
        auto tsp = create_problem(cli_config, cfg.ga.seed);
        if (!cli_config.json_output) {
            std::cout << "Problem size: " << tsp.num_cities() << " cities\n";
        }

        // Get GA configuration
        auto ga_config = cfg.to_ga_config();

        if (!cli_config.json_output) {
            std::cout << "Population: " << ga_config.population_size << "\n";
            std::cout << "Generations: " << ga_config.max_generations << "\n";
            std::cout << "Algorithm: " << cli_config.algorithm << "\n";
            std::cout << "Seed: " << ga_config.seed << "\n\n";
            std::cout << "Starting evolution...\n";
        }
        auto start_time = std::chrono::high_resolution_clock::now();

        // Run GA based on algorithm selection
        auto result = [&]() {
            if (cli_config.algorithm == "advanced") {
                auto ga = factory::make_tsp_ga_advanced();
                return ga.run(tsp, ga_config);
            } else if (cli_config.algorithm == "config" && !cli_config.config_file.empty()) {
                // Use config-based GA with dynamic operator selection
                const std::string& crossover_type = cfg.operators.crossover.type;

                if (cfg.local_search.enabled) {
                    // Local search enabled - select appropriate crossover with local search
                    if (crossover_type == "EAX") {
                        auto ga = factory::make_tsp_ga_eax_with_local_search_from_config(cfg);
                        return ga.run(tsp, ga_config);
                    } else if (crossover_type == "OX") {
                        auto ga = factory::make_tsp_ga_ox_with_local_search_from_config(cfg);
                        return ga.run(tsp, ga_config);
                    } else {
                        // Default to PMX
                        auto ga = factory::make_tsp_ga_with_local_search_from_config(cfg);
                        return ga.run(tsp, ga_config);
                    }
                } else {
                    // No local search - select appropriate crossover
                    if (crossover_type == "EAX") {
                        auto ga = factory::make_tsp_ga_eax_from_config(cfg);
                        return ga.run(tsp, ga_config);
                    } else if (crossover_type == "OX") {
                        auto ga = factory::make_tsp_ga_ox_from_config(cfg);
                        return ga.run(tsp, ga_config);
                    } else {
                        // Default to PMX
                        auto ga = factory::make_tsp_ga_from_config(cfg);
                        return ga.run(tsp, ga_config);
                    }
                }
            } else {
                auto ga = factory::make_tsp_ga_basic();
                return ga.run(tsp, ga_config);
            }
        }();

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end_time - start_time).count();

        // Output results based on mode
        if (cli_config.json_output) {
            // JSON output mode
            write_json_output(result, cli_config, cfg, tsp, duration, cli_config.json_file);
        } else {
            // Normal console output
            print_stats(result, cli_config, duration);
        }

        // Write tour file if requested (independent of JSON output)
        if (!cli_config.output_file.empty()) {
            write_tour(cli_config.output_file, result.best_genome, result.best_fitness.value,
                       cli_config.json_output);
        }

        // Verify solution
        if (!tsp.is_valid_tour(result.best_genome)) {
            if (!cli_config.json_output) {
                std::cerr << "Warning: Final solution is not a valid tour!\n";
            }
            return 1;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred\n";
        return 1;
    }
}
