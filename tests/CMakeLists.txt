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

# Conditional TBB parallel tests
if(TBB_FOUND)
    add_executable(test_parallel test_parallel.cpp)
    target_link_libraries(test_parallel PRIVATE evolab)
    target_compile_features(test_parallel PRIVATE cxx_std_23)
    
    # Register parallel test with CTest
    add_test(NAME ParallelTests COMMAND test_parallel)
endif()

# Register core tests with CTest
add_test(NAME CoreTests COMMAND test_core)
add_test(NAME TSPTests COMMAND test_tsp)
add_test(NAME OperatorTests COMMAND test_operators)
add_test(NAME SchedulerTests COMMAND test_schedulers)
add_test(NAME TSPLIBTests COMMAND test_tsplib)
add_test(NAME ConfigTests COMMAND test_config)
add_test(NAME ConfigIntegrationTests COMMAND test_config_integration)
