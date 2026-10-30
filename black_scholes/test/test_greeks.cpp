#include "ope/greeks.hpp"

#include "ope/black_scholes.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

using std::exp;
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

OptionInputs with_expiry(OptionInputs in, double t) {
    in.time_to_expiry = t;
    return in;
}

// θ = ∂V/∂t = −∂V/∂T: time passing shrinks T.
double finite_difference_theta(const OptionInputs& in) {
    const double h = 1e-5;
    const double t = in.time_to_expiry;
    return -(ope::price(with_expiry(in, t + h)) - ope::price(with_expiry(in, t - h))) / (2.0 * h);
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

TEST(Theta, MatchesHullCh19Example) {
    EXPECT_NEAR(greeks(hull_ch19()).theta, -4.31, 5e-3);
    EXPECT_NEAR(greeks(hull_ch19()).theta, -4.305389964546104, 1e-11);
    EXPECT_NEAR(greeks(hull_ch19(OptionType::Put)).theta, -1.853005672196868, 1e-11);
}

TEST(Theta, IsRateOfChangeAsTimePasses) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        for (double s : {30.0, 45.0, 50.0, 55.0, 80.0}) {
            for (double t : {0.05, 0.5, 2.0}) {
                for (double vol : {0.1, 0.3, 0.8}) {
                    const OptionInputs in{type, s, 50.0, t, 0.05, vol};
                    EXPECT_NEAR(greeks(in).theta, finite_difference_theta(in), 1e-4)
                        << (type == OptionType::Call ? "call" : "put") << " S=" << s
                        << " T=" << t << " vol=" << vol;
                }
            }
        }
    }
}

TEST(Theta, CallMinusPutIsInterestOnStrike) {
    for (double s : {30.0, 49.0, 50.0, 70.0}) {
        const auto call = with_spot(hull_ch19(OptionType::Call), s);
        const auto put = with_spot(hull_ch19(OptionType::Put), s);
        const double expected = -call.rate * call.strike * exp(-call.rate * call.time_to_expiry);
        EXPECT_NEAR(greeks(call).theta - greeks(put).theta, expected, 1e-12) << "S=" << s;
    }
}

TEST(Theta, LongCallDecaysNearTheMoney) {
    EXPECT_LT(greeks(hull_ch19()).theta, 0.0);
    const double at_the_money = greeks(with_spot(hull_ch19(), 50.0)).theta;
    const double far_out = greeks(with_spot(hull_ch19(), 30.0)).theta;
    EXPECT_LT(at_the_money, far_out);
}

TEST(Theta, DeepOutOfTheMoneyIsNearZero) {
    EXPECT_NEAR(greeks(with_spot(hull_ch19(), 5.0)).theta, 0.0, 1e-12);
    EXPECT_NEAR(greeks(with_spot(hull_ch19(OptionType::Put), 200.0)).theta, 0.0, 1e-12);
}

TEST(Theta, IsInterestOnStrikeAtZeroVol) {
    auto itm_call = with_spot(hull_ch19(), 55.0);
    itm_call.volatility = 0.0;
    const double kdf = itm_call.strike * exp(-itm_call.rate * itm_call.time_to_expiry);
    EXPECT_DOUBLE_EQ(greeks(itm_call).theta, -itm_call.rate * kdf);
    EXPECT_NEAR(greeks(itm_call).theta, finite_difference_theta(itm_call), 1e-6);

    auto otm_call = with_spot(itm_call, 45.0);
    EXPECT_DOUBLE_EQ(greeks(otm_call).theta, 0.0);

    auto itm_put = otm_call;
    itm_put.type = OptionType::Put;
    EXPECT_DOUBLE_EQ(greeks(itm_put).theta, itm_put.rate * kdf);
}
