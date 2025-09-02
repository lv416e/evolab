# CLI Applications

add_executable(evolab-tsp tsp_main.cpp)
target_link_libraries(evolab-tsp PRIVATE evolab)
target_compile_features(evolab-tsp PRIVATE cxx_std_23)

# Install applications
install(TARGETS evolab-tsp
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
