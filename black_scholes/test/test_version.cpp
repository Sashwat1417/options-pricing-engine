#include "ope/version.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string_view>

// Smoke test: proves the library builds, links into the test binary, and runs.
TEST(Version, IsSemanticVersion) {
    const std::string_view v = ope::version();

    ASSERT_FALSE(v.empty());
    EXPECT_EQ(std::count(v.begin(), v.end(), '.'), 2) << "expected MAJOR.MINOR.PATCH, got " << v;
}
