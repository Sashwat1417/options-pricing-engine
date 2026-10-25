# Command-line tool for Layer 1. Builds the `ope` executable.

add_executable(ope_cli
    main/src/main.cpp
)
set_target_properties(ope_cli PROPERTIES OUTPUT_NAME ope)

target_link_libraries(ope_cli PRIVATE ope::black_scholes CLI11::CLI11)

ope_set_warnings(ope_cli)
ope_enable_sanitizers(ope_cli)

if(OPE_BUILD_TESTS)
    add_subdirectory(test)
endif()
