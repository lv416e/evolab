cmake_minimum_required(VERSION 3.25)
project(EvoLab
    VERSION 0.1.0
    DESCRIPTION "A modern C++23 metaheuristics research platform for TSP/VRP/QAP problems"
    LANGUAGES CXX)

# C++23 standard (experimental support in GCC 12+ and Clang 15+)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native -DNDEBUG)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /DNDEBUG)
    endif()
endif()

# Options
option(EVOLAB_BUILD_TESTS "Build unit tests" ON)
option(EVOLAB_BUILD_BENCHMARKS "Build benchmarks" ON)
option(EVOLAB_USE_TBB "Use oneTBB for parallelization" ON)
option(EVOLAB_USE_OPENMP "Use OpenMP for parallelization" ON)

# Find packages
if(EVOLAB_USE_TBB)
    find_package(TBB QUIET)
    if(TBB_FOUND)
        message(STATUS "Found TBB: ${TBB_VERSION}")
        add_compile_definitions(EVOLAB_HAVE_TBB)
        set(EVOLAB_HAVE_TBB TRUE)  # Set CMake variable for use in conditionals
    endif()
endif()

if(EVOLAB_USE_OPENMP)
    find_package(OpenMP QUIET)
    if(OpenMP_CXX_FOUND)
        message(STATUS "Found OpenMP: ${OpenMP_CXX_VERSION}")
        add_compile_definitions(EVOLAB_HAVE_OPENMP)
    endif()
endif()

# Fetch toml11 library for configuration parsing
# Using FetchContent for modern CMake integration with header-only library
include(FetchContent)

# Configure FetchContent for better SSL/TLS handling and reliability
set(CMAKE_TLS_VERIFY ON)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    toml11
    URL https://github.com/ToruNiina/toml11/archive/refs/tags/v4.2.0.tar.gz
    URL_HASH SHA256=9287971cd4a1a3992ef37e7b95a3972d1ae56410e7f8e3f300727ab1d6c79c2c
    TIMEOUT 30
)
set(TOML11_INSTALL OFF CACHE INTERNAL "")  # Disable toml11 installation to avoid export issues
FetchContent_MakeAvailable(toml11)
message(STATUS "Fetched toml11 library for TOML configuration parsing")

# Fetch nlohmann/json library for JSON generation
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    TIMEOUT 30
)
set(JSON_Install OFF CACHE INTERNAL "")  # Disable installation to avoid export issues
FetchContent_MakeAvailable(nlohmann_json)
message(STATUS "Fetched nlohmann/json library for JSON generation")

# Header-only library
add_library(evolab INTERFACE)
add_library(EvoLab::evolab ALIAS evolab)

target_include_directories(evolab INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(evolab INTERFACE cxx_std_23)

# Link libraries
# Use BUILD_INTERFACE to avoid export issues with FetchContent dependencies
target_link_libraries(evolab INTERFACE 
    $<BUILD_INTERFACE:toml11::toml11>)  # Only during build, not for install

if(TBB_FOUND)
    target_link_libraries(evolab INTERFACE TBB::tbb)
endif()

if(OpenMP_CXX_FOUND)
    target_link_libraries(evolab INTERFACE OpenMP::OpenMP_CXX)
endif()

# Applications
add_subdirectory(apps)

# Tests
if(EVOLAB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarks
if(EVOLAB_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS evolab
    EXPORT EvoLabTargets)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT EvoLabTargets
    FILE EvoLabTargets.cmake
    NAMESPACE EvoLab::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EvoLab)

# Package configuration
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EvoLabConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/EvoLabConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EvoLab)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/EvoLabConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/EvoLabConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/EvoLabConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EvoLab)
