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

# Register tests with CTest
add_test(NAME CoreTests COMMAND test_core)
add_test(NAME TSPTests COMMAND test_tsp)
add_test(NAME OperatorTests COMMAND test_operators)
add_test(NAME SchedulerTests COMMAND test_schedulers)
