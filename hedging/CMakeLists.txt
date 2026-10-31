# Layer 3: delta-hedging simulator.

add_library(ope_hedging
    main/src/hedging.cpp
)
add_library(ope::hedging ALIAS ope_hedging)

target_include_directories(ope_hedging
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/main/include>
)

target_link_libraries(ope_hedging PUBLIC ope::black_scholes PRIVATE ope::monte_carlo)
target_compile_features(ope_hedging PUBLIC cxx_std_17)

ope_set_warnings(ope_hedging)
ope_enable_sanitizers(ope_hedging)

if(OPE_BUILD_TESTS)
    add_executable(hedging_tests
        test/test_hedging.cpp
    )

    target_link_libraries(hedging_tests PRIVATE ope::hedging ope::monte_carlo GTest::gtest_main)

    ope_set_warnings(hedging_tests)
    ope_enable_sanitizers(hedging_tests)

    gtest_discover_tests(hedging_tests)
endif()
