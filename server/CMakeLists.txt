# REST API for Layer 1. Builds the `ope-server` executable.
# Layered like Order Book's order-service: api -> service -> ope::black_scholes,
# with request/ and response/ DTOs at the edges.

add_library(ope_server_core
    main/src/api/PricingController.cpp
    main/src/request/IvRequest.cpp
    main/src/request/JsonReader.cpp
    main/src/request/PriceRequest.cpp
    main/src/request/SweepRequest.cpp
    main/src/response/SweepResponse.cpp
    main/src/service/PricingService.cpp
)

target_include_directories(ope_server_core PUBLIC main/src)

target_link_libraries(ope_server_core
    PUBLIC
        ope::black_scholes
        httplib::httplib
        nlohmann_json::nlohmann_json
)

ope_set_warnings(ope_server_core)
ope_enable_sanitizers(ope_server_core)

add_executable(ope_server
    main/src/main.cpp
)
set_target_properties(ope_server PROPERTIES OUTPUT_NAME ope-server)

target_link_libraries(ope_server PRIVATE ope_server_core)

ope_set_warnings(ope_server)
ope_enable_sanitizers(ope_server)

if(OPE_BUILD_TESTS)
    add_executable(server_tests
        test/request/RequestParsingTest.cpp
        test/service/PricingServiceTest.cpp
    )

    target_link_libraries(server_tests PRIVATE ope_server_core GTest::gtest_main)

    ope_set_warnings(server_tests)
    ope_enable_sanitizers(server_tests)

    gtest_discover_tests(server_tests)
endif()
