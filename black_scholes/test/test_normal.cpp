#include "ope/normal.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

using std::isnan;
using std::numeric_limits;

namespace {

using ope::norm_cdf;
using ope::norm_pdf;

template <typename F>
void for_each_x(F f) {
    for (int i = -32; i <= 32; ++i) {
        f(0.25 * i);
    }
}

}  // namespace

TEST(NormCdf, IsHalfAtZero) {
    EXPECT_DOUBLE_EQ(norm_cdf(0.0), 0.5);
}

TEST(NormCdf, MatchesReferenceValues) {
    struct Case {
        double x;
        double expected;
    };
    const Case cases[] = {
        {1.0, 0.8413447460685429},   {2.0, 0.9772498680518208},  {3.0, 0.9986501019683699},
        {1.96, 0.9750021048517795},  {-1.96, 0.02499789514822043},
    };
    for (const auto& c : cases) {
        EXPECT_NEAR(norm_cdf(c.x), c.expected, 1e-15) << "x=" << c.x;
    }
}

TEST(NormCdf, MatchesHullTableAtHullExampleD1D2) {
    // Hull Ch 15 example: N(d1), N(d2) from Hull's tables.
    EXPECT_NEAR(norm_cdf(0.7693), 0.7791, 5e-5);
    EXPECT_NEAR(norm_cdf(0.6278), 0.7349, 5e-5);
}

TEST(NormCdf, IsSymmetric) {
    for_each_x([](double x) { EXPECT_NEAR(norm_cdf(x) + norm_cdf(-x), 1.0, 1e-15) << "x=" << x; });
}

TEST(NormCdf, IsNonDecreasingAndBounded) {
    double previous = 0.0;
    for_each_x([&](double x) {
        const double n = norm_cdf(x);
        EXPECT_GE(n, previous) << "x=" << x;
        EXPECT_GE(n, 0.0) << "x=" << x;
        EXPECT_LE(n, 1.0) << "x=" << x;
        previous = n;
    });
}

TEST(NormCdf, StaysAccurateDeepInLeftTail) {
    // 1 + erf would give exactly 0 here.
    const double expected = 7.619853024160593e-24;
    EXPECT_GT(norm_cdf(-10.0), 0.0);
    EXPECT_NEAR(norm_cdf(-10.0) / expected, 1.0, 1e-12);
}

TEST(NormCdf, PropagatesNaN) {
    EXPECT_TRUE(isnan(norm_cdf(numeric_limits<double>::quiet_NaN())));
}

TEST(NormPdf, MatchesReferenceValues) {
    EXPECT_NEAR(norm_pdf(0.0), 0.3989422804014327, 1e-16);
    EXPECT_NEAR(norm_pdf(1.0), 0.24197072451914337, 1e-16);
}

TEST(NormPdf, IsSymmetric) {
    for_each_x([](double x) { EXPECT_DOUBLE_EQ(norm_pdf(x), norm_pdf(-x)) << "x=" << x; });
}

TEST(NormPdf, IsDerivativeOfCdf) {
    const double h = 1e-5;
    for_each_x([h](double x) {
        const double slope = (norm_cdf(x + h) - norm_cdf(x - h)) / (2.0 * h);
        EXPECT_NEAR(slope, norm_pdf(x), 1e-9) << "x=" << x;
    });
}
