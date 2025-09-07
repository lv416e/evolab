# EvoLab C++23 Metaheuristics Research Platform
# Modern justfile for development workflow automation
# https://just.systems/

# Configuration variables - override with environment variables  
build_dir := env_var_or_default('BUILD_DIR', 'build')
build_type := env_var_or_default('BUILD_TYPE', 'Release')
parallel_jobs := env_var_or_default('PARALLEL_JOBS', `nproc`)

# Compiler settings
cc := env_var_or_default('CC', 'clang')
cxx := env_var_or_default('CXX', 'clang++')

# Export variables for cmake
export CC := cc
export CXX := cxx

# Default recipe - show available commands
default:
    @echo "🚀 EvoLab C++23 Development Commands"
    @echo "=================================="
    @just --list

# 🔧 Configure and build the project  
build:
    @echo "🔧 Configuring EvoLab with C++23 ({{build_type}} build)..."
    @echo "   Build directory: {{build_dir}}"
    @echo "   Parallel jobs: {{parallel_jobs}}"
    cmake -S . -B {{build_dir}} \
        -DCMAKE_BUILD_TYPE={{build_type}} \
        -DCMAKE_CXX_STANDARD=23 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    @echo "🏗️ Building with {{parallel_jobs}} parallel jobs..."
    cmake --build {{build_dir}} --parallel {{parallel_jobs}}

# 🧪 Run all tests with colored output
test: build
    @echo "🧪 Running all test suites..."
    cd {{build_dir}} && ctest --parallel {{parallel_jobs}} --output-on-failure --progress

# 🎯 Run specific test suites
test-core: build
    @echo "🎯 Running core algorithm tests..."
    cd {{build_dir}} && ctest -R "CoreTests" --verbose

test-tsp: build
    @echo "🎯 Running TSP problem tests..."
    cd {{build_dir}} && ctest -R "TSPTests" --verbose

test-operators: build
    @echo "🎯 Running genetic operator tests..."
    cd {{build_dir}} && ctest -R "OperatorTests" --verbose

test-schedulers: build
    @echo "🚀 Running scheduler tests..."
    cd {{build_dir}} && ctest -R "SchedulerTests" --verbose

test-tsplib: build
    @echo "📚 Running TSPLIB integration tests..."
    cd {{build_dir}} && ctest -R "TSPLIBTests" --verbose

test-config: build
    @echo "⚙️ Running configuration tests..."
    cd {{build_dir}} && ctest -R "ConfigTests|ConfigIntegrationTests" --verbose

# 🎮 Build and run TSP application
run *ARGS: build
    @echo "🎮 Running TSP application..."
    {{build_dir}}/apps/evolab-tsp {{ARGS}}

# 🏃‍♀️ Quick development workflow
dev: clean build test format lint
    @echo "✅ Development cycle completed successfully!"

# 🐛 Debug build with sanitizers
debug:
    @echo "🐛 Creating debug build with sanitizers..."
    BUILD_TYPE=Debug CMAKE_CXX_FLAGS="-fsanitize=address,undefined -g3 -O0" just build

# 🚀 Optimized release build  
release:
    @echo "🚀 Creating optimized release build..."
    BUILD_TYPE=Release just build

# 🎨 Code formatting with clang-format
format:
    @echo "🎨 Formatting C++ code..."
    find include tests apps -name "*.hpp" -o -name "*.cpp" | xargs clang-format -i

# 🔍 Check code formatting
check-format:
    @echo "🔍 Checking code formatting..."
    find include tests apps -name "*.hpp" -o -name "*.cpp" | xargs clang-format --dry-run --Werror

# 📝 Static analysis with clang-tidy
lint: build
    @echo "📝 Running clang-tidy static analysis..."
    cd {{build_dir}} && \
    find ../include ../tests ../apps -name "*.cpp" | \
    xargs clang-tidy -p . --config-file=../.clang-tidy || echo "clang-tidy completed with warnings"

# 🧹 Clean build artifacts
clean:
    @echo "🧹 Cleaning build artifacts..."
    rm -rf {{build_dir}}

clean-all: clean
    @echo "🗑️ Deep cleaning all generated files..."
    find . -name "*.orig" -delete
    find . -name "compile_commands.json" -delete

# 📋 Project information and diagnostics
info:
    @echo "📋 EvoLab Project Information"
    @echo "============================"
    @echo "Build directory: {{build_dir}}"
    @echo "Build type: {{build_type}}"
    @echo "Parallel jobs: {{parallel_jobs}}"
    @echo "C compiler: {{cc}}"
    @echo "C++ compiler: {{cxx}}"
    @echo ""
    @echo "🔧 Tool Versions:"
    @echo "CMake: `cmake --version | head -1`"
    @echo "C++ Compiler: `{{cxx}} --version | head -1`"
    @echo "System: `uname -sm`"
    @echo ""
    @if [ -d "{{build_dir}}" ]; then \
        echo "📁 Build Status: Configured"; \
        echo "Build files: `find {{build_dir}} -name "*.ninja" -o -name "Makefile" | wc -l` files"; \
    else \
        echo "📁 Build Status: Not configured"; \
    fi

# 🆘 Help command with usage examples
help:
    @echo "🆘 EvoLab Development Help"
    @echo "========================"
    @echo ""
    @echo "🏁 Quick Start:"
    @echo "  just build           # Build the project"
    @echo "  just test            # Run all tests"
    @echo "  just dev             # Full development cycle"
    @echo ""
    @echo "🎯 Specific Testing:"
    @echo "  just test-schedulers # Test scheduler algorithms"
    @echo "  just test-tsp        # Test TSP implementations"
    @echo "  just test-tsplib     # Test TSPLIB integration"
    @echo "  just test-config     # Test configuration system"
    @echo ""
    @echo "🎮 Running Applications:"
    @echo "  just run --help      # Show TSP app options"
    @echo "  just run --problem random --cities 20"
    @echo ""
    @echo "🏗️ Build Variants:"
    @echo "  just debug           # Debug build with sanitizers"
    @echo "  just release         # Optimized release build"
    @echo ""
    @echo "⚙️ Configuration:"
    @echo "  BUILD_TYPE=Debug just build"
    @echo "  PARALLEL_JOBS=16 just test"