#include "ope/version.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string_view>

TEST(Version, IsSemanticVersion) {
    const std::string_view v = ope::version();

    ASSERT_FALSE(v.empty());
    EXPECT_EQ(std::count(v.begin(), v.end(), '.'), 2) << "expected MAJOR.MINOR.PATCH, got " << v;
}
