#include "ope/version.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string_view>

using std::count;
using std::string_view;

TEST(Version, IsSemanticVersion) {
    const string_view v = ope::version();

    ASSERT_FALSE(v.empty());
    EXPECT_EQ(count(v.begin(), v.end(), '.'), 2) << "expected MAJOR.MINOR.PATCH, got " << v;
}
