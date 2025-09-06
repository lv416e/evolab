# EvoLab Configuration Examples

This directory contains example TOML configuration files for different EvoLab usage scenarios.

## Configuration Files

### `basic.toml`
- **Purpose**: Quick experimentation and learning
- **Population**: 100 individuals  
- **Operators**: Simple PMX crossover, swap mutation
- **Runtime**: ~5 minutes typical
- **Best for**: Small problems, initial testing

### `advanced.toml`  
- **Purpose**: Balanced performance and quality
- **Population**: 256 individuals
- **Operators**: EAX crossover with 2-opt local search
- **Runtime**: ~15 minutes typical  
- **Best for**: Medium-sized problems, research validation

### `high_performance.toml`
- **Purpose**: Competitive solution quality
- **Population**: 512 individuals
- **Operators**: EAX + adaptive mutation + Lin-Kernighan
- **Features**: Multi-Armed Bandit scheduling, parallelization
- **Runtime**: ~60 minutes typical
- **Best for**: Large problems, benchmarking

### `experimental.toml`
- **Purpose**: Algorithm development and research
- **Population**: 200 individuals  
- **Features**: UCB scheduler, diversity maintenance, operator tracking
- **Runtime**: ~30 minutes typical
- **Best for**: Research experiments, parameter studies

## Usage

```bash
# Use a specific configuration
./build/apps/evolab-tsp --config configs/advanced.toml --instance data/tsplib/burma14.tsp

# Override specific parameters
./build/apps/evolab-tsp --config configs/basic.toml --population 200 --seed 2023
```

## Configuration Structure

```toml
[ga]              # Core genetic algorithm parameters
[operators]       # Crossover, mutation, selection settings  
[local_search]    # Local optimization configuration
[scheduler]       # Multi-Armed Bandit operator selection
[diversity]       # Population diversity maintenance
[parallel]        # Parallelization settings
[termination]     # Stopping criteria
[logging]         # Output and monitoring options
```

See the full specification in `.kiro/specs/evolab/requirements.md` for detailed parameter descriptions.