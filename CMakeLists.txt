cmake_minimum_required(VERSION 3.25)

project(options_pricing_engine
    VERSION 0.1.0
    DESCRIPTION "Options pricing engine: Black-Scholes, Monte Carlo, delta hedging"
    LANGUAGES CXX
)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

option(OPE_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
option(OPE_BUILD_CLI "Build the ope command-line tool" ${PROJECT_IS_TOP_LEVEL})
option(OPE_BUILD_SERVER "Build the ope-server REST API" ${PROJECT_IS_TOP_LEVEL})
option(OPE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(OPE_ENABLE_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)

if(OPE_BUILD_TESTS)
    enable_testing()
    include(FetchGoogleTest)
endif()

add_subdirectory(black_scholes)  # Layer 1

if(OPE_BUILD_CLI)
    include(FetchCLI11)
    add_subdirectory(cli)
endif()

if(OPE_BUILD_SERVER)
    include(FetchServerDeps)
    add_subdirectory(server)
endif()
