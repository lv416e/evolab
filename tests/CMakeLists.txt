# Tests for EvoLab

# Simple header-only testing - no external dependencies for now
add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE evolab)
target_compile_features(test_core PRIVATE cxx_std_23)

add_executable(test_tsp test_tsp.cpp)
target_link_libraries(test_tsp PRIVATE evolab)
target_compile_features(test_tsp PRIVATE cxx_std_23)

add_executable(test_operators test_operators.cpp)
target_link_libraries(test_operators PRIVATE evolab)
target_compile_features(test_operators PRIVATE cxx_std_23)

add_executable(test_schedulers test_schedulers.cpp)
target_link_libraries(test_schedulers PRIVATE evolab)
target_compile_features(test_schedulers PRIVATE cxx_std_23)

add_executable(test_tsplib test_tsplib.cpp)
target_link_libraries(test_tsplib PRIVATE evolab)
target_compile_features(test_tsplib PRIVATE cxx_std_23)

add_executable(test_config test_config.cpp)
target_link_libraries(test_config PRIVATE evolab)
target_compile_features(test_config PRIVATE cxx_std_23)

add_executable(test_config_integration test_config_integration.cpp)
target_link_libraries(test_config_integration PRIVATE evolab)
target_compile_features(test_config_integration PRIVATE cxx_std_23)

add_executable(test_candidate_list test_candidate_list.cpp)
target_link_libraries(test_candidate_list PRIVATE evolab)
target_compile_features(test_candidate_list PRIVATE cxx_std_23)

add_executable(test_delta_evaluation test_delta_evaluation.cpp)
target_link_libraries(test_delta_evaluation PRIVATE evolab)
target_compile_features(test_delta_evaluation PRIVATE cxx_std_23)

add_executable(test_lk test_lk.cpp)
target_link_libraries(test_lk PRIVATE evolab)
target_compile_features(test_lk PRIVATE cxx_std_23)

add_executable(test_lk_integration test_lk_integration.cpp)
target_link_libraries(test_lk_integration PRIVATE evolab)
target_compile_features(test_lk_integration PRIVATE cxx_std_23)

# Register candidate list tests with CTest
add_test(NAME CandidateListTests COMMAND test_candidate_list)
set_tests_properties(CandidateListTests PROPERTIES LABELS "unit;tsp;candidate-list")

# Register delta evaluation tests with CTest
add_test(NAME DeltaEvaluationTests COMMAND test_delta_evaluation)
set_tests_properties(DeltaEvaluationTests PROPERTIES LABELS "unit;tsp;performance")

# Register Lin-Kernighan tests with CTest
add_test(NAME LinKernighanTests COMMAND test_lk)
set_tests_properties(LinKernighanTests PROPERTIES LABELS "unit;local-search;lk")

# Register Lin-Kernighan integration tests with CTest
add_test(NAME LinKernighanIntegrationTests COMMAND test_lk_integration)
set_tests_properties(LinKernighanIntegrationTests PROPERTIES LABELS "integration;local-search;lk;memetic")

# Conditional TBB parallel tests
if(TBB_FOUND)
    add_executable(test_parallel test_parallel.cpp)
    target_link_libraries(test_parallel PRIVATE evolab)
    target_compile_features(test_parallel PRIVATE cxx_std_23)

    # Register parallel test with CTest
    add_test(NAME ParallelTests COMMAND test_parallel)
    # Helpful labels for filtering in CI dashboards
    set_tests_properties(ParallelTests PROPERTIES LABELS "parallel;TBB")
else()
    message(STATUS "TBB not found: parallel tests disabled (ParallelTests not built)")
endif()

# NUMA allocator tests (always built, uses optional NUMA support)
add_executable(test_numa test_numa.cpp)
target_link_libraries(test_numa PRIVATE evolab)
target_compile_features(test_numa PRIVATE cxx_std_23)

# Register core tests with CTest
add_test(NAME CoreTests COMMAND test_core)
add_test(NAME TSPTests COMMAND test_tsp)
add_test(NAME OperatorTests COMMAND test_operators)
add_test(NAME SchedulerTests COMMAND test_schedulers)
add_test(NAME TSPLIBTests COMMAND test_tsplib)
add_test(NAME ConfigTests COMMAND test_config)
add_test(NAME ConfigIntegrationTests COMMAND test_config_integration)
add_test(NAME NumaTests COMMAND test_numa)

# Add labels to tests for filtering in CI
set_tests_properties(CoreTests PROPERTIES LABELS "unit;core")
set_tests_properties(TSPTests PROPERTIES LABELS "unit;tsp")
set_tests_properties(OperatorTests PROPERTIES LABELS "unit;operators")
set_tests_properties(SchedulerTests PROPERTIES LABELS "unit;schedulers")
set_tests_properties(TSPLIBTests PROPERTIES LABELS "integration;tsplib")
set_tests_properties(ConfigTests PROPERTIES LABELS "unit;config")
set_tests_properties(ConfigIntegrationTests PROPERTIES LABELS "integration;config")

# Check for NUMA support
find_path(NUMA_INCLUDE_DIR numa.h)
find_library(NUMA_LIBRARY numa)

if(NUMA_INCLUDE_DIR AND NUMA_LIBRARY)
    message(STATUS "NUMA support found: enabling NUMA optimizations")
    target_compile_definitions(test_numa PRIVATE EVOLAB_NUMA_SUPPORT)
    target_include_directories(test_numa PRIVATE ${NUMA_INCLUDE_DIR})
    target_link_libraries(test_numa PRIVATE ${NUMA_LIBRARY})
    set_tests_properties(NumaTests PROPERTIES LABELS "numa;performance")
else()
    message(STATUS "NUMA support not found: NUMA allocator will use fallback mode")
    set_tests_properties(NumaTests PROPERTIES LABELS "numa;fallback")
endif()
