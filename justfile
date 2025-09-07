# EvoLab C++23 Metaheuristics Research Platform
# Modern justfile for development workflow automation with CMake Presets
# https://just.systems/

# Configuration variables - CMake Presets integration
preset := env_var_or_default('PRESET', 'default')
parallel_jobs := env_var_or_default('PARALLEL_JOBS', `nproc`)

# Default recipe - show available commands
default:
    @echo "🚀 EvoLab C++23 Development Commands"
    @echo "=================================="
    @just --list

# 🔧 Configure and build the project using CMake Presets
build preset=preset:
    @echo "🔧 Configuring EvoLab with C++23 ({{preset}} preset)..."
    cmake --preset {{preset}}
    @echo "🏗️ Building with {{parallel_jobs}} parallel jobs..."
    cmake --build --preset {{preset}} --parallel {{parallel_jobs}}

# 🔧 CMake workflow execution (configure + build + test)
workflow preset=preset:
    @echo "🚀 Running complete workflow with {{preset}} preset..."
    cmake --workflow --preset {{preset}}

# 🧪 Run all tests using CMake Presets
test preset=preset: (build preset)
    @echo "🧪 Running test suite with {{preset}} preset..."
    ctest --preset {{preset}}

# 🎯 Run specific test suites  
test-core preset=preset: (build preset)
    @echo "🎯 Running core algorithm tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "CoreTests" --verbose

test-tsp preset=preset: (build preset)
    @echo "🎯 Running TSP problem tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "TSPTests" --verbose

test-operators preset=preset: (build preset) 
    @echo "🎯 Running genetic operator tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "OperatorTests" --verbose

test-schedulers preset=preset: (build preset)
    @echo "🚀 Running scheduler tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "SchedulerTests" --verbose

test-tsplib preset=preset: (build preset)
    @echo "📚 Running TSPLIB integration tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "TSPLIBTests" --verbose

test-config preset=preset: (build preset)
    @echo "⚙️ Running configuration tests with {{preset}} preset..."
    ctest --preset {{preset}} -R "ConfigTests|ConfigIntegrationTests" --verbose

# 🎮 Build and run TSP application
run preset=preset *ARGS: (build preset)
    @echo "🎮 Running TSP application with {{preset}} preset..."
    {{build_dir}}/apps/evolab-tsp {{ARGS}}

# Helper function to get build directory for preset - use in variables
build_dir := if preset == "debug" { "build/debug" } else if preset == "release" { "build/release" } else if preset == "ninja-release" { "build/ninja-release" } else { "build" }

# 🏃‍♀️ Quick development workflow with default preset
dev: clean (workflow preset) format lint
    @echo "✅ Development cycle completed successfully!"

# 🐛 Debug build with sanitizers using debug preset
debug:
    @echo "🐛 Creating debug build with sanitizers..."
    just workflow debug

# 🚀 Optimized release build using release preset
release:
    @echo "🚀 Creating optimized release build..."
    just workflow release

# ⚡ Ultra-fast build using Ninja generator
ninja:
    @echo "⚡ Creating ultra-fast Ninja build..."
    just workflow ninja-release

# 🎨 Code formatting with clang-format
format:
    @echo "🎨 Formatting C++ code..."
    find include tests apps -name "*.hpp" -o -name "*.cpp" | xargs clang-format -i

# 🔍 Check code formatting
check-format:
    @echo "🔍 Checking code formatting..."
    find include tests apps -name "*.hpp" -o -name "*.cpp" | xargs clang-format --dry-run --Werror

# 📝 Static analysis with clang-tidy
lint preset=preset: (build preset)
    @echo "📝 Running clang-tidy static analysis with {{preset}} preset..."
    cd {{build_dir}} && \
    find ../include ../tests ../apps -name "*.cpp" | \
    xargs clang-tidy -p . --config-file=../.clang-tidy || echo "clang-tidy completed with warnings"

# 🧹 Clean build artifacts
clean:
    @echo "🧹 Cleaning all build directories..."
    rm -rf build build/debug build/release build/ninja-release

clean-preset preset=preset:
    @echo "🧹 Cleaning {{preset}} build artifacts..."
    rm -rf {{build_dir}}

clean-all: clean
    @echo "🗑️ Deep cleaning all generated files..."
    find . -name "*.orig" -delete
    find . -name "compile_commands.json" -delete

# 📋 Project information and diagnostics
info:
    @echo "📋 EvoLab Project Information"
    @echo "============================"
    @echo "Active preset: {{preset}}"
    @echo "Parallel jobs: {{parallel_jobs}}"
    @echo ""
    @echo "🔧 Tool Versions:"
    @echo "CMake: `cmake --version | head -1`"
    @echo "C++ Compiler: `clang++ --version | head -1`"
    @echo "System: `uname -sm`"
    @echo ""
    @echo "📋 Available CMake Presets:"
    cmake --list-presets=all
    @echo ""
    @echo "📁 Build Status:"
    @if [ -d "build" ]; then echo "  build: Configured"; else echo "  build: Not configured"; fi
    @if [ -d "build/debug" ]; then echo "  build/debug: Configured"; else echo "  build/debug: Not configured"; fi  
    @if [ -d "build/release" ]; then echo "  build/release: Configured"; else echo "  build/release: Not configured"; fi
    @if [ -d "build/ninja-release" ]; then echo "  build/ninja-release: Configured"; else echo "  build/ninja-release: Not configured"; fi

# 🆘 Help command with usage examples
help:
    @echo "🆘 EvoLab Development Help"
    @echo "========================"
    @echo ""
    @echo "🏁 Quick Start (CMake Presets):"
    @echo "  just build           # Build with default preset"
    @echo "  just test            # Test with default preset"
    @echo "  just workflow        # Complete workflow (configure+build+test)"
    @echo "  just dev             # Full development cycle"
    @echo ""
    @echo "🎯 Preset-specific Commands:"
    @echo "  just build debug     # Debug build with sanitizers"
    @echo "  just test release    # Test optimized release build"
    @echo "  just workflow ninja-release  # Ultra-fast Ninja workflow"
    @echo ""
    @echo "🔧 Build Variants:"
    @echo "  just debug           # Debug workflow (sanitizers + verbose tests)"
    @echo "  just release         # Release workflow (O3 optimizations)"
    @echo "  just ninja           # Ninja workflow (fastest builds)"
    @echo ""
    @echo "🎮 Running Applications:"
    @echo "  just run --help      # Show TSP app options (default preset)"
    @echo "  just run release --problem random --cities 20  # Run with release preset"
    @echo ""
    @echo "🧪 Testing Options:"
    @echo "  just test-schedulers debug  # Test schedulers with debug preset"
    @echo "  just test-tsp release       # Test TSP with release preset"
    @echo ""
    @echo "⚙️ Environment Variables:"
    @echo "  PRESET=debug just build     # Override default preset"
    @echo "  PARALLEL_JOBS=16 just test  # Override parallel jobs"
    @echo ""
    @echo "📋 Information:"
    @echo "  just info            # Show project and preset information"
    @echo "  cmake --list-presets # List all available presets"