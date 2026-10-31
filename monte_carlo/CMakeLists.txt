# Monte Carlo: stock path simulation (Layers 2 and 3) and the Layer 2 pricer.

add_library(ope_monte_carlo
    main/src/gbm.cpp
    main/src/mc_pricer.cpp
)
add_library(ope::monte_carlo ALIAS ope_monte_carlo)

target_include_directories(ope_monte_carlo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/main/include>
)

target_link_libraries(ope_monte_carlo PUBLIC ope::black_scholes)

target_compile_features(ope_monte_carlo PUBLIC cxx_std_17)

ope_set_warnings(ope_monte_carlo)
ope_enable_sanitizers(ope_monte_carlo)

if(OPE_BUILD_TESTS)
    add_executable(monte_carlo_tests
        test/test_gbm.cpp
        test/test_mc_pricer.cpp
    )

    target_link_libraries(monte_carlo_tests
        PRIVATE ope::monte_carlo ope::black_scholes GTest::gtest_main)

    ope_set_warnings(monte_carlo_tests)
    ope_enable_sanitizers(monte_carlo_tests)

    gtest_discover_tests(monte_carlo_tests)
endif()
