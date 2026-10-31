#include "ope/mc_pricer.hpp"

#include "ope/black_scholes.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

using std::exp;
using std::invalid_argument;
using std::max;
using std::numeric_limits;
using std::sqrt;
using std::uint64_t;

namespace {

using ope::mc_price;
using ope::McResult;
using ope::OptionInputs;
using ope::OptionType;

// Hull Ch 15 example: S=42, K=40, T=0.5, r=10%, σ=20%.
OptionInputs hull(OptionType type, double strike = 40.0) {
    return {type, 42.0, strike, 0.5, 0.10, 0.20};
}

}  // namespace

// Statistical tests: each estimate must land within 4 standard errors of the exact value,
// because std::normal_distribution differs between standard libraries for the same seed.

TEST(McPricer, CallMatchesBlackScholes) {
    const auto call = hull(OptionType::Call);
    const McResult mc = mc_price(call, 200000, 1);
    EXPECT_GT(mc.std_error, 0.0);
    EXPECT_NEAR(mc.price, ope::price(call), 4.0 * mc.std_error);
}

TEST(McPricer, PutMatchesBlackScholes) {
    const auto put = hull(OptionType::Put);
    const McResult mc = mc_price(put, 200000, 2);
    EXPECT_GT(mc.std_error, 0.0);
    EXPECT_NEAR(mc.price, ope::price(put), 4.0 * mc.std_error);
}

TEST(McPricer, MatchesBlackScholesAcrossStrikes) {
    uint64_t seed = 100;
    for (double strike : {30.0, 36.0, 40.0, 44.0, 50.0, 60.0}) {
        for (auto type : {OptionType::Call, OptionType::Put}) {
            const auto option = hull(type, strike);
            const McResult mc = mc_price(option, 100000, seed++);
            EXPECT_NEAR(mc.price, ope::price(option), 4.0 * mc.std_error)
                << "K=" << strike << (type == OptionType::Call ? " call" : " put");
        }
    }
}

TEST(McPricer, StdErrorHalvesWithFourTimesThePaths) {
    const auto call = hull(OptionType::Call);
    const double ratio =
        mc_price(call, 10000, 3).std_error / mc_price(call, 40000, 4).std_error;
    EXPECT_NEAR(ratio, 2.0, 0.1);
}

TEST(McPricer, SameSeedGivesSameResult) {
    const auto call = hull(OptionType::Call);
    const McResult a = mc_price(call, 5000, 42);
    const McResult b = mc_price(call, 5000, 42);
    EXPECT_EQ(a.price, b.price);
    EXPECT_EQ(a.std_error, b.std_error);
    EXPECT_NE(mc_price(call, 5000, 43).price, a.price);
}

TEST(McPricer, SatisfiesPutCallParity) {
    const McResult call = mc_price(hull(OptionType::Call), 200000, 5);
    const McResult put = mc_price(hull(OptionType::Put), 200000, 6);
    const double parity = 42.0 - 40.0 * exp(-0.10 * 0.5);
    const double se = sqrt(call.std_error * call.std_error + put.std_error * put.std_error);
    EXPECT_NEAR(call.price - put.price, parity, 4.0 * se);
}

TEST(McPricer, ZeroVolatilityIsTheDiscountedForwardPayoff) {
    auto call = hull(OptionType::Call);
    call.volatility = 0.0;
    const McResult mc = mc_price(call, 100, 7);
    EXPECT_NEAR(mc.price, max(42.0 - 40.0 * exp(-0.10 * 0.5), 0.0), 1e-12);
    EXPECT_NEAR(mc.std_error, 0.0, 1e-12);
}

TEST(McPricer, RejectsInvalidInputs) {
    const auto call = hull(OptionType::Call);
    EXPECT_THROW((void)mc_price(call, 1, 1), invalid_argument);
    EXPECT_THROW((void)mc_price(call, 0, 1), invalid_argument);

    auto expired = call;
    expired.time_to_expiry = 0.0;
    EXPECT_THROW((void)mc_price(expired, 1000, 1), invalid_argument);

    auto bad_spot = call;
    bad_spot.spot = -1.0;
    EXPECT_THROW((void)mc_price(bad_spot, 1000, 1), invalid_argument);

    auto nan_vol = call;
    nan_vol.volatility = numeric_limits<double>::quiet_NaN();
    EXPECT_THROW((void)mc_price(nan_vol, 1000, 1), invalid_argument);
}

namespace {

using ope::VarianceReduction;

constexpr VarianceReduction kMethods[] = {VarianceReduction::None, VarianceReduction::Antithetic,
                                          VarianceReduction::ControlVariate};

const char* name(VarianceReduction m) {
    switch (m) {
        case VarianceReduction::None:
            return "plain";
        case VarianceReduction::Antithetic:
            return "antithetic";
        case VarianceReduction::ControlVariate:
            return "control variate";
    }
    return "?";
}

// How many plain paths one path of `method` is worth: (SE plain / SE method)².
double speedup(const OptionInputs& option, VarianceReduction method, uint64_t seed) {
    const double plain = mc_price(option, 100000, seed).std_error;
    const double reduced = mc_price(option, 100000, seed, method).std_error;
    return (plain / reduced) * (plain / reduced);
}

}  // namespace

TEST(McVarianceReduction, EveryMethodMatchesBlackScholesAcrossStrikes) {
    uint64_t seed = 200;
    for (auto method : kMethods) {
        for (double strike : {30.0, 36.0, 40.0, 44.0, 50.0, 60.0}) {
            for (auto type : {OptionType::Call, OptionType::Put}) {
                const auto option = hull(type, strike);
                const McResult mc = mc_price(option, 100000, seed++, method);
                EXPECT_NEAR(mc.price, ope::price(option), 4.0 * mc.std_error + 1e-12)
                    << name(method) << " K=" << strike
                    << (type == OptionType::Call ? " call" : " put");
            }
        }
    }
}

TEST(McVarianceReduction, AntitheticCutsTheErrorForInTheMoneyOptions) {
    EXPECT_GT(speedup(hull(OptionType::Call, 40.0), VarianceReduction::Antithetic, 11), 2.5);
    EXPECT_GT(speedup(hull(OptionType::Put, 50.0), VarianceReduction::Antithetic, 12), 8.0);
}

TEST(McVarianceReduction, ControlVariateCutsTheErrorForInTheMoneyOptions) {
    EXPECT_GT(speedup(hull(OptionType::Call, 40.0), VarianceReduction::ControlVariate, 13), 8.0);
    EXPECT_GT(speedup(hull(OptionType::Put, 50.0), VarianceReduction::ControlVariate, 14), 5.0);
}

// Calls and puts move one way with ε, so neither method should ever do clearly worse.
// Deep out-of-the-money strikes are left out: few paths pay, so the SE estimates are noisy.
TEST(McVarianceReduction, NeverClearlyWorseThanPlain) {
    uint64_t seed = 300;
    for (auto method : {VarianceReduction::Antithetic, VarianceReduction::ControlVariate}) {
        for (double strike : {36.0, 40.0, 44.0, 50.0}) {
            for (auto type : {OptionType::Call, OptionType::Put}) {
                EXPECT_GT(speedup(hull(type, strike), method, seed++), 0.85)
                    << name(method) << " K=" << strike;
            }
        }
    }
}

// A deep in-the-money call pays S_T − K on every path: a straight line in the control
// e^(−rT)·S_T, so the control variate removes all the noise.
TEST(McVarianceReduction, ControlVariateIsExactWhenThePayoffIsLinearInTheControl) {
    const auto call = hull(OptionType::Call, 10.0);
    const McResult mc = mc_price(call, 1000, 15, VarianceReduction::ControlVariate);
    EXPECT_NEAR(mc.price, ope::price(call), 1e-9);
    EXPECT_NEAR(mc.std_error, 0.0, 1e-6);
}

TEST(McVarianceReduction, SameSeedGivesSameResult) {
    for (auto method : kMethods) {
        const auto call = hull(OptionType::Call);
        const McResult a = mc_price(call, 5000, 42, method);
        const McResult b = mc_price(call, 5000, 42, method);
        EXPECT_EQ(a.price, b.price) << name(method);
        EXPECT_EQ(a.std_error, b.std_error) << name(method);
    }
}

TEST(McVarianceReduction, ZeroVolatilityIsExactForEveryMethod) {
    auto call = hull(OptionType::Call);
    call.volatility = 0.0;
    for (auto method : kMethods) {
        const McResult mc = mc_price(call, 100, 7, method);
        EXPECT_NEAR(mc.price, 42.0 - 40.0 * exp(-0.10 * 0.5), 1e-12) << name(method);
        EXPECT_NEAR(mc.std_error, 0.0, 1e-12) << name(method);
    }
}

TEST(McVarianceReduction, NeedsAtLeastFourPaths) {
    const auto call = hull(OptionType::Call);
    for (auto method : {VarianceReduction::Antithetic, VarianceReduction::ControlVariate}) {
        EXPECT_THROW((void)mc_price(call, 3, 1, method), invalid_argument) << name(method);
        EXPECT_NO_THROW((void)mc_price(call, 4, 1, method)) << name(method);
    }
}
