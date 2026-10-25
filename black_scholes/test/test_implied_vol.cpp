#include "ope/implied_vol.hpp"

#include "ope/black_scholes.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <stdexcept>

using std::exp;
using std::invalid_argument;
using std::numeric_limits;

namespace {

using ope::implied_vol;
using ope::OptionInputs;
using ope::OptionType;
using ope::price;

// Hull Ch 15 worked example.
OptionInputs hull_example() { return {OptionType::Call, 42.0, 40.0, 0.5, 0.10, 0.20}; }

OptionInputs with_vol(OptionInputs in, double vol) {
    in.volatility = vol;
    return in;
}

}  // namespace

TEST(ImpliedVol, RecoversHullVolatility) {
    const auto call = hull_example();
    EXPECT_NEAR(implied_vol(call, price(call)), 0.20, 1e-9);
    EXPECT_NEAR(implied_vol(call, 4.76), 0.200066, 1e-6);
}

TEST(ImpliedVol, MatchesRelianceExample) {
    const OptionInputs call{OptionType::Call, 2900.0, 3000.0, 30.0 / 365.0, 0.065, 0.0};
    EXPECT_NEAR(implied_vol(call, 45.0), 0.238096, 1e-6);
}

TEST(ImpliedVol, RoundTripsAcrossGrid) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        for (double s : {80.0, 100.0, 120.0}) {
            for (double t : {0.25, 1.0, 2.0}) {
                for (double vol : {0.05, 0.2, 0.5, 1.0, 1.5}) {
                    const OptionInputs in{type, s, 100.0, t, 0.03, vol};
                    const double market = price(in);
                    const double iv = implied_vol(in, market);
                    const double time_value = market - price(with_vol(in, 0.0));

                    EXPECT_NEAR(price(with_vol(in, iv)), market, 1e-8)
                        << (type == OptionType::Call ? "call" : "put") << " S=" << s << " T=" << t
                        << " vol=" << vol;

                    // Deep ITM with tiny time value: the price can't pin sigma down.
                    if (time_value > 1e-6) {
                        EXPECT_NEAR(iv, vol, 1e-8)
                            << (type == OptionType::Call ? "call" : "put") << " S=" << s
                            << " T=" << t << " vol=" << vol;
                    }
                }
            }
        }
    }
}

TEST(ImpliedVol, CallAndPutGiveSameVolUnderParity) {
    const auto call = hull_example();
    auto put = call;
    put.type = OptionType::Put;
    const double put_price = price(call) - call.spot + call.strike * exp(-call.rate * call.time_to_expiry);
    EXPECT_NEAR(implied_vol(put, put_price), implied_vol(call, price(call)), 1e-9);
}

TEST(ImpliedVol, IgnoresVolatilityFieldInInputs) {
    const double market = price(hull_example());
    EXPECT_NEAR(implied_vol(with_vol(hull_example(), numeric_limits<double>::quiet_NaN()), market),
                0.20, 1e-9);
}

TEST(ImpliedVol, PriceAtLowerBoundGivesZeroVol) {
    const auto call = hull_example();
    EXPECT_NEAR(implied_vol(call, price(with_vol(call, 0.0))), 0.0, 1e-9);
}

TEST(ImpliedVol, RejectsPricesNoVolCanProduce) {
    const auto call = hull_example();
    const double lower = price(with_vol(call, 0.0));
    EXPECT_THROW((void)implied_vol(call, lower - 0.01), invalid_argument);
    EXPECT_THROW((void)implied_vol(call, call.spot), invalid_argument);
    EXPECT_THROW((void)implied_vol(call, -1.0), invalid_argument);
    EXPECT_THROW((void)implied_vol(call, numeric_limits<double>::quiet_NaN()), invalid_argument);
}

TEST(ImpliedVol, RejectsZeroTimeToExpiry) {
    auto call = hull_example();
    call.time_to_expiry = 0.0;
    EXPECT_THROW((void)implied_vol(call, 2.0), invalid_argument);
}

TEST(ImpliedVol, RejectsInvalidInputs) {
    auto call = hull_example();
    call.spot = -42.0;
    EXPECT_THROW((void)implied_vol(call, 4.76), invalid_argument);
}
