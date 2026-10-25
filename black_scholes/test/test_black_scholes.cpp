#include "ope/black_scholes.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

using std::exp;
using std::invalid_argument;
using std::max;
using std::sqrt;
using std::vector;

namespace {

using ope::d1d2;
using ope::OptionInputs;
using ope::OptionType;
using ope::price;

// Hull Ch 15 worked example.
OptionInputs hull_example() { return {OptionType::Call, 42.0, 40.0, 0.5, 0.10, 0.20}; }

OptionInputs with_type(OptionInputs in, OptionType type) {
    in.type = type;
    return in;
}

double discounted_strike(const OptionInputs& in) {
    return in.strike * exp(-in.rate * in.time_to_expiry);
}

// Calls across moneyness, maturities, rates (incl. negative) and vols.
vector<OptionInputs> grid() {
    vector<OptionInputs> out;
    for (double s : {50.0, 90.0, 100.0, 110.0, 200.0}) {
        for (double t : {0.1, 1.0, 3.0}) {
            for (double r : {-0.01, 0.0, 0.05}) {
                for (double vol : {0.05, 0.2, 0.6}) {
                    out.push_back({OptionType::Call, s, 100.0, t, r, vol});
                }
            }
        }
    }
    return out;
}

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

TEST(Price, MatchesHullExample) {
    const auto call = hull_example();
    const auto put = with_type(call, OptionType::Put);
    EXPECT_NEAR(price(call), 4.76, 5e-3);
    EXPECT_NEAR(price(put), 0.81, 5e-3);
    EXPECT_NEAR(price(call), 4.759422392871535, 1e-12);
    EXPECT_NEAR(price(put), 0.808599372900096, 1e-12);
}

TEST(Price, MatchesAtTheMoneyReference) {
    const OptionInputs call{OptionType::Call, 100.0, 100.0, 1.0, 0.05, 0.20};
    EXPECT_NEAR(price(call), 10.4506, 1e-4);
    EXPECT_NEAR(price(with_type(call, OptionType::Put)), 5.5735, 1e-4);
}

TEST(Price, SatisfiesPutCallParity) {
    for (const auto& call : grid()) {
        const double c = price(call);
        const double p = price(with_type(call, OptionType::Put));
        EXPECT_NEAR(c - p, call.spot - discounted_strike(call), 1e-10)
            << "S=" << call.spot << " T=" << call.time_to_expiry << " r=" << call.rate
            << " vol=" << call.volatility;
    }
}

TEST(Price, StaysWithinNoArbitrageBounds) {
    for (const auto& call : grid()) {
        const double kdf = discounted_strike(call);
        const double c = price(call);
        const double p = price(with_type(call, OptionType::Put));
        EXPECT_GE(c, max(call.spot - kdf, 0.0) - 1e-12) << "S=" << call.spot;
        EXPECT_LE(c, call.spot) << "S=" << call.spot;
        EXPECT_GE(p, max(kdf - call.spot, 0.0) - 1e-12) << "S=" << call.spot;
        EXPECT_LE(p, kdf) << "S=" << call.spot;
    }
}

TEST(Price, IncreasesWithVolatility) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        auto in = with_type(hull_example(), type);
        double previous = 0.0;
        for (double vol : {0.05, 0.1, 0.2, 0.4, 0.8}) {
            in.volatility = vol;
            const double p = price(in);
            EXPECT_GT(p, previous) << "vol=" << vol;
            previous = p;
        }
    }
}

TEST(Price, CallRisesAndPutFallsWithSpot) {
    auto low = hull_example();
    auto high = hull_example();
    low.spot = 38.0;
    high.spot = 46.0;
    EXPECT_LT(price(low), price(high));
    EXPECT_GT(price(with_type(low, OptionType::Put)), price(with_type(high, OptionType::Put)));
}

TEST(Price, ScalesWithSpotAndStrike) {
    auto doubled = hull_example();
    doubled.spot *= 2.0;
    doubled.strike *= 2.0;
    EXPECT_NEAR(price(doubled), 2.0 * price(hull_example()), 1e-12);
}

TEST(Price, AtExpiryEqualsPayoff) {
    for (double s : {35.0, 40.0, 45.0}) {
        auto call = hull_example();
        call.spot = s;
        call.time_to_expiry = 0.0;
        EXPECT_DOUBLE_EQ(price(call), max(s - 40.0, 0.0)) << "S=" << s;
        EXPECT_DOUBLE_EQ(price(with_type(call, OptionType::Put)), max(40.0 - s, 0.0)) << "S=" << s;
    }
}

TEST(Price, ZeroVolatilityEqualsDiscountedPayoff) {
    for (double s : {35.0, 40.0, 45.0}) {
        auto call = hull_example();
        call.spot = s;
        call.volatility = 0.0;
        const double kdf = discounted_strike(call);
        EXPECT_DOUBLE_EQ(price(call), max(s - kdf, 0.0)) << "S=" << s;
        EXPECT_DOUBLE_EQ(price(with_type(call, OptionType::Put)), max(kdf - s, 0.0)) << "S=" << s;
    }
}

TEST(Price, FormulaApproachesLimitsAsVolOrTimeShrink) {
    auto at_expiry = hull_example();
    at_expiry.time_to_expiry = 0.0;
    auto near_expiry = hull_example();
    near_expiry.time_to_expiry = 1e-10;
    EXPECT_NEAR(price(near_expiry), price(at_expiry), 1e-6);

    auto no_vol = hull_example();
    no_vol.volatility = 0.0;
    auto tiny_vol = hull_example();
    tiny_vol.volatility = 1e-8;
    EXPECT_NEAR(price(tiny_vol), price(no_vol), 1e-6);
}

TEST(Price, DeepInAndOutOfTheMoney) {
    const OptionInputs deep_otm{OptionType::Call, 1.0, 1000.0, 1.0, 0.05, 0.20};
    EXPECT_GE(price(deep_otm), 0.0);
    EXPECT_LT(price(deep_otm), 1e-100);

    const OptionInputs deep_itm{OptionType::Call, 1000.0, 1.0, 1.0, 0.05, 0.20};
    EXPECT_NEAR(price(deep_itm), 1000.0 - discounted_strike(deep_itm), 1e-10);
    EXPECT_GE(price(with_type(deep_itm, OptionType::Put)), 0.0);
}

TEST(Price, ThrowsOnInvalidInputs) {
    auto in = hull_example();
    in.volatility = -0.2;
    EXPECT_THROW((void)price(in), invalid_argument);
}
