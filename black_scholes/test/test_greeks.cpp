#include "ope/greeks.hpp"

#include "ope/black_scholes.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using std::invalid_argument;

namespace {

using ope::greeks;
using ope::OptionInputs;
using ope::OptionType;

// Hull Ch 19 example: S=49, K=50, T=20 weeks, r=5%, σ=20%.
OptionInputs hull_ch19(OptionType type = OptionType::Call) {
    return {type, 49.0, 50.0, 0.3846, 0.05, 0.20};
}

OptionInputs with_spot(OptionInputs in, double spot) {
    in.spot = spot;
    return in;
}

// Δ as the literal slope of the price curve.
double finite_difference_delta(const OptionInputs& in) {
    const double h = 1e-4 * in.spot;
    return (ope::price(with_spot(in, in.spot + h)) - ope::price(with_spot(in, in.spot - h))) /
           (2.0 * h);
}

}  // namespace

TEST(Delta, MatchesHullCh19Example) {
    EXPECT_NEAR(greeks(hull_ch19()).delta, 0.522, 5e-4);
    EXPECT_NEAR(greeks(hull_ch19()).delta, 0.5216016339715761, 1e-12);
    EXPECT_NEAR(greeks(hull_ch19(OptionType::Put)).delta, -0.4783983660284239, 1e-12);
}

TEST(Delta, IsSlopeOfPriceCurve) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        for (double s : {30.0, 45.0, 50.0, 55.0, 80.0}) {
            for (double t : {0.05, 0.5, 2.0}) {
                for (double vol : {0.1, 0.3, 0.8}) {
                    const OptionInputs in{type, s, 50.0, t, 0.05, vol};
                    EXPECT_NEAR(greeks(in).delta, finite_difference_delta(in), 1e-6)
                        << (type == OptionType::Call ? "call" : "put") << " S=" << s
                        << " T=" << t << " vol=" << vol;
                }
            }
        }
    }
}

TEST(Delta, CallMinusPutIsOne) {
    for (double s : {30.0, 49.0, 50.0, 70.0}) {
        const double call = greeks(with_spot(hull_ch19(OptionType::Call), s)).delta;
        const double put = greeks(with_spot(hull_ch19(OptionType::Put), s)).delta;
        EXPECT_NEAR(call - put, 1.0, 1e-15) << "S=" << s;
    }
}

TEST(Delta, StaysWithinBounds) {
    for (double s = 5.0; s <= 150.0; s += 5.0) {
        const double call = greeks(with_spot(hull_ch19(OptionType::Call), s)).delta;
        const double put = greeks(with_spot(hull_ch19(OptionType::Put), s)).delta;
        EXPECT_GE(call, 0.0) << "S=" << s;
        EXPECT_LE(call, 1.0) << "S=" << s;
        EXPECT_GE(put, -1.0) << "S=" << s;
        EXPECT_LE(put, 0.0) << "S=" << s;
    }
}

TEST(Delta, RisesWithSpot) {
    double previous = -1.0;
    for (double s = 30.0; s <= 70.0; s += 2.0) {
        const double d = greeks(with_spot(hull_ch19(), s)).delta;
        EXPECT_GT(d, previous) << "S=" << s;
        previous = d;
    }
}

TEST(Delta, DeepInAndOutOfTheMoney) {
    EXPECT_NEAR(greeks(with_spot(hull_ch19(), 200.0)).delta, 1.0, 1e-12);
    EXPECT_NEAR(greeks(with_spot(hull_ch19(), 5.0)).delta, 0.0, 1e-12);
    EXPECT_NEAR(greeks(with_spot(hull_ch19(OptionType::Put), 200.0)).delta, 0.0, 1e-12);
    EXPECT_NEAR(greeks(with_spot(hull_ch19(OptionType::Put), 5.0)).delta, -1.0, 1e-12);
}

TEST(Delta, IsStepAtExpiryAndZeroVol) {
    for (auto field : {0, 1}) {
        auto itm = with_spot(hull_ch19(), 55.0);
        auto otm = with_spot(hull_ch19(), 45.0);
        (field == 0 ? itm.time_to_expiry : itm.volatility) = 0.0;
        (field == 0 ? otm.time_to_expiry : otm.volatility) = 0.0;
        EXPECT_DOUBLE_EQ(greeks(itm).delta, 1.0);
        EXPECT_DOUBLE_EQ(greeks(otm).delta, 0.0);
        itm.type = OptionType::Put;
        otm.type = OptionType::Put;
        EXPECT_DOUBLE_EQ(greeks(itm).delta, 0.0);
        EXPECT_DOUBLE_EQ(greeks(otm).delta, -1.0);
    }
}

TEST(Delta, ThrowsOnInvalidInputs) {
    auto in = hull_ch19();
    in.spot = -1.0;
    EXPECT_THROW((void)greeks(in), invalid_argument);
}
