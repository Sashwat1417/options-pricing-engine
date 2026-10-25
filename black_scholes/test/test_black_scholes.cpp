#include "ope/black_scholes.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

using std::invalid_argument;
using std::sqrt;

namespace {

using ope::d1d2;
using ope::OptionInputs;
using ope::OptionType;

// Hull Ch 15 worked example.
OptionInputs hull_example() { return {OptionType::Call, 42.0, 40.0, 0.5, 0.10, 0.20}; }

}  // namespace

TEST(D1D2, MatchesHullExample) {
    const auto d = d1d2(hull_example());
    EXPECT_NEAR(d.d1, 0.7693, 5e-5);
    EXPECT_NEAR(d.d2, 0.6278, 5e-5);
    EXPECT_NEAR(d.d1, 0.7692626281060315, 1e-14);
    EXPECT_NEAR(d.d2, 0.6278412718687220, 1e-14);
}

TEST(D1D2, MatchesAtTheMoneyReference) {
    // S = K, so ln(S/K) = 0: d1 = (0.05 + 0.02) / 0.2 = 0.35, d2 = 0.15.
    const auto d = d1d2({OptionType::Call, 100.0, 100.0, 1.0, 0.05, 0.20});
    EXPECT_NEAR(d.d1, 0.35, 1e-15);
    EXPECT_NEAR(d.d2, 0.15, 1e-15);
}

TEST(D1D2, GapIsVolTimesSqrtT) {
    for (double vol : {0.05, 0.2, 0.8}) {
        for (double t : {0.01, 0.5, 5.0}) {
            auto in = hull_example();
            in.volatility = vol;
            in.time_to_expiry = t;
            const auto d = d1d2(in);
            EXPECT_NEAR(d.d1 - d.d2, vol * sqrt(t), 1e-14) << "vol=" << vol << " t=" << t;
        }
    }
}

TEST(D1D2, IsSameForCallAndPut) {
    auto put = hull_example();
    put.type = OptionType::Put;
    const auto c = d1d2(hull_example());
    const auto p = d1d2(put);
    EXPECT_DOUBLE_EQ(c.d1, p.d1);
    EXPECT_DOUBLE_EQ(c.d2, p.d2);
}

TEST(D1D2, IncreasesWithSpot) {
    auto low = hull_example();
    auto high = hull_example();
    low.spot = 30.0;
    high.spot = 60.0;
    EXPECT_LT(d1d2(low).d1, d1d2(high).d1);
    EXPECT_LT(d1d2(low).d2, d1d2(high).d2);
}

TEST(D1D2, ThrowsWhenVolTimesSqrtTIsZero) {
    auto zero_t = hull_example();
    zero_t.time_to_expiry = 0.0;
    EXPECT_THROW((void)d1d2(zero_t), invalid_argument);

    auto zero_vol = hull_example();
    zero_vol.volatility = 0.0;
    EXPECT_THROW((void)d1d2(zero_vol), invalid_argument);
}

TEST(D1D2, ThrowsOnInvalidInputs) {
    auto in = hull_example();
    in.spot = -1.0;
    EXPECT_THROW((void)d1d2(in), invalid_argument);
}
