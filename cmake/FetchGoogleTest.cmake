# Downloads GoogleTest once for the whole project; every module's tests link
# GTest::gtest_main from here.
#
# Pinned release + checksum so every machine builds against the same GoogleTest.
# SYSTEM keeps our strict warnings from firing inside GoogleTest's headers.
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SYSTEM
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# CMake's built-in module: provides gtest_discover_tests().
include(GoogleTest)
