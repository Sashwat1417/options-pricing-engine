# Layer 1: Black-Scholes pricer.

add_library(ope_black_scholes
    main/src/black_scholes.cpp
    main/src/option.cpp
    main/src/version.cpp
)
add_library(ope::black_scholes ALIAS ope_black_scholes)

target_include_directories(ope_black_scholes
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/main/include>
)

target_compile_features(ope_black_scholes PUBLIC cxx_std_17)
target_compile_definitions(ope_black_scholes PRIVATE OPE_VERSION_STRING="${PROJECT_VERSION}")

ope_set_warnings(ope_black_scholes)
ope_enable_sanitizers(ope_black_scholes)

if(OPE_BUILD_TESTS)
    add_executable(black_scholes_tests
        test/test_black_scholes.cpp
        test/test_normal.cpp
        test/test_option.cpp
        test/test_version.cpp
    )

    target_link_libraries(black_scholes_tests PRIVATE ope::black_scholes GTest::gtest_main)

    ope_set_warnings(black_scholes_tests)
    ope_enable_sanitizers(black_scholes_tests)

    gtest_discover_tests(black_scholes_tests)
endif()
