# CLI Applications

# Get Git hash at configure time
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_HASH)
    set(GIT_HASH "unknown")
endif()

# Get hostname at configure time
cmake_host_system_information(RESULT BUILD_HOSTNAME QUERY HOSTNAME)
if(NOT BUILD_HOSTNAME)
    set(BUILD_HOSTNAME "unknown")
endif()

add_executable(evolab-tsp tsp_main.cpp)
target_link_libraries(evolab-tsp PRIVATE evolab nlohmann_json::nlohmann_json)
target_compile_features(evolab-tsp PRIVATE cxx_std_23)

# Pass build-time information as compile definitions
target_compile_definitions(evolab-tsp PRIVATE 
    GIT_HASH="${GIT_HASH}"
    BUILD_HOSTNAME="${BUILD_HOSTNAME}"
)

# Install applications
install(TARGETS evolab-tsp
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
