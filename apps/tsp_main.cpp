#include <evolab/evolab.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>
#include <sstream>

using namespace evolab;

/// Parse command line arguments
struct Config {
    std::string instance_file;
    std::string algorithm = "basic";
    std::size_t population = 256;
    std::size_t generations = 1000;
    double crossover_prob = 0.9;
    double mutation_prob = 0.1;
    std::uint64_t seed = 1;
    bool verbose = false;
    std::string output_file;
};

/// Print usage information
void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -h, --help              Show this help message\n"
              << "  -i, --instance FILE     TSP instance file (random if not specified)\n"
              << "  -a, --algorithm ALGO    Algorithm: basic, advanced (default: basic)\n"
              << "  -p, --population SIZE   Population size (default: 256)\n"
              << "  -g, --generations NUM   Max generations (default: 1000)\n"
              << "  -c, --crossover PROB    Crossover probability (default: 0.9)\n"
              << "  -m, --mutation PROB     Mutation probability (default: 0.1)\n"
              << "  -s, --seed SEED         Random seed (default: 1)\n"
              << "  -v, --verbose           Verbose output\n"
              << "  -o, --output FILE       Output file for best tour\n"
              << "\nExamples:\n"
              << "  " << program_name << " --instance data/pr76.tsp\n"
              << "  " << program_name << " --algorithm advanced --population 512\n"
              << "  " << program_name << " --verbose --output solution.tour\n";
}

/// Parse command line arguments
Config parse_args(int argc, char** argv) {
    Config config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        }
        else if ((arg == "-i" || arg == "--instance") && i + 1 < argc) {
            config.instance_file = argv[++i];
        }
        else if ((arg == "-a" || arg == "--algorithm") && i + 1 < argc) {
            config.algorithm = argv[++i];
        }
        else if ((arg == "-p" || arg == "--population") && i + 1 < argc) {
            config.population = std::stoull(argv[++i]);
        }
        else if ((arg == "-g" || arg == "--generations") && i + 1 < argc) {
            config.generations = std::stoull(argv[++i]);
        }
        else if ((arg == "-c" || arg == "--crossover") && i + 1 < argc) {
            config.crossover_prob = std::stod(argv[++i]);
        }
        else if ((arg == "-m" || arg == "--mutation") && i + 1 < argc) {
            config.mutation_prob = std::stod(argv[++i]);
        }
        else if ((arg == "-s" || arg == "--seed") && i + 1 < argc) {
            config.seed = std::stoull(argv[++i]);
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            config.output_file = argv[++i];
        }
        else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        }
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            std::exit(1);
        }
    }

    return config;
}

/// Create TSP problem from config
problems::TSP create_problem(const Config& config) {
    if (config.instance_file.empty()) {
        // Create random TSP instance
        std::cout << "Creating random TSP instance with 100 cities...\n";
        return problems::create_random_tsp(100, 1000.0, config.seed);
    } else {
        // TODO: Implement TSPLIB file parsing
        std::cerr << "TSPLIB file parsing not yet implemented. Using random instance.\n";
        return problems::create_random_tsp(100, 1000.0, config.seed);
    }
}

/// Write tour to file
void write_tour(const std::string& filename, const problems::TSP::GenomeT& tour, double fitness) {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Could not open output file: " << filename << "\n";
        return;
    }

    file << "# TSP Tour - Fitness: " << fitness << "\n";
    file << "# Tour length: " << tour.size() << "\n";

    for (std::size_t i = 0; i < tour.size(); ++i) {
        file << tour[i];
        if (i < tour.size() - 1) file << " ";
    }
    file << "\n";

    std::cout << "Tour written to: " << filename << "\n";
}

/// Print statistics
void print_stats(const auto& result, const Config& config, double runtime) {
    std::cout << "\n=== Results ===\n";
    std::cout << "Best fitness: " << std::fixed << std::setprecision(2) << result.best_fitness.value << "\n";
    std::cout << "Generations: " << result.generations << "\n";
    std::cout << "Evaluations: " << result.evaluations << "\n";
    std::cout << "Runtime: " << std::fixed << std::setprecision(3) << runtime << " seconds\n";
    std::cout << "Converged: " << (result.converged ? "Yes" : "No") << "\n";

    if (config.verbose && !result.history.empty()) {
        std::cout << "\n=== Evolution History ===\n";
        std::cout << std::setw(10) << "Gen" << std::setw(15) << "Best" << std::setw(15) << "Mean"
                  << std::setw(15) << "Diversity" << std::setw(12) << "Time(ms)\n";
        std::cout << std::string(67, '-') << "\n";

        for (const auto& stats : result.history) {
            std::cout << std::setw(10) << stats.generation
                      << std::setw(15) << std::fixed << std::setprecision(2) << stats.best_fitness.value
                      << std::setw(15) << std::fixed << std::setprecision(2) << stats.mean_fitness.value
                      << std::setw(15) << std::fixed << std::setprecision(4) << stats.diversity
                      << std::setw(12) << stats.elapsed_time.count() << "\n";
        }
    }
}

int main(int argc, char** argv) {
    try {
        auto config = parse_args(argc, argv);

        std::cout << "EvoLab TSP Solver v" << VERSION << "\n";
        std::cout << std::string(30, '=') << "\n";

        // Create problem
        auto tsp = create_problem(config);
        std::cout << "Problem size: " << tsp.num_cities() << " cities\n";

        // Configure GA
        core::GAConfig ga_config{
            .population_size = config.population,
            .max_generations = config.generations,
            .crossover_prob = config.crossover_prob,
            .mutation_prob = config.mutation_prob,
            .seed = config.seed,
            .log_interval = config.verbose ? 50 : 100
        };

        std::cout << "Population: " << ga_config.population_size << "\n";
        std::cout << "Generations: " << ga_config.max_generations << "\n";
        std::cout << "Algorithm: " << config.algorithm << "\n";
        std::cout << "Seed: " << ga_config.seed << "\n\n";

        // Run algorithm
        std::cout << "Starting evolution...\n";
        auto start_time = std::chrono::high_resolution_clock::now();

        auto result = [&]() {
            if (config.algorithm == "advanced") {
                auto ga = factory::make_tsp_ga_advanced();
                return ga.run(tsp, ga_config);
            } else {
                auto ga = factory::make_tsp_ga_basic();
                return ga.run(tsp, ga_config);
            }
        }();

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end_time - start_time).count();

        // Print results
        print_stats(result, config, duration);

        // Write output file if requested
        if (!config.output_file.empty()) {
            write_tour(config.output_file, result.best_genome, result.best_fitness.value);
        }

        // Verify solution
        if (!tsp.is_valid_tour(result.best_genome)) {
            std::cerr << "Warning: Final solution is not a valid tour!\n";
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