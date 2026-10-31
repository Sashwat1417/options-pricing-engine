# Monte Carlo building blocks: stock path simulation. Used by Layer 3 (hedging) and Layer 2.

add_library(ope_monte_carlo
    main/src/gbm.cpp
)
add_library(ope::monte_carlo ALIAS ope_monte_carlo)

target_include_directories(ope_monte_carlo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/main/include>
)

target_compile_features(ope_monte_carlo PUBLIC cxx_std_17)

ope_set_warnings(ope_monte_carlo)
ope_enable_sanitizers(ope_monte_carlo)

if(OPE_BUILD_TESTS)
    add_executable(monte_carlo_tests
        test/test_gbm.cpp
    )

    target_link_libraries(monte_carlo_tests
        PRIVATE ope::monte_carlo ope::black_scholes GTest::gtest_main)

    ope_set_warnings(monte_carlo_tests)
    ope_enable_sanitizers(monte_carlo_tests)

    gtest_discover_tests(monte_carlo_tests)
endif()
