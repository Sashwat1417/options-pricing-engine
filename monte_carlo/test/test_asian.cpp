#include "ope/asian.hpp"

#include "ope/black_scholes.hpp"
#include "ope/gbm.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

using std::exp;
using std::invalid_argument;
using std::log;
using std::max;
using std::mt19937_64;
using std::sqrt;
using std::uint64_t;
using std::vector;

namespace {

using ope::asian_lognormal_price;
using ope::geometric_asian_price;
using ope::mc_asian_price;
using ope::McResult;
using ope::OptionInputs;
using ope::OptionType;
using ope::VarianceReduction;

// Hull Example 26.3: S=50, K=50, T=1, r=10%, σ=40%.
OptionInputs hull(OptionType type, double strike = 50.0) {
    return {type, 50.0, strike, 1.0, 0.10, 0.40};
}

// e^(−rT)·E[S_ave], the forward value of the arithmetic average.
double discounted_average_forward(const OptionInputs& o, int fixings) {
    double sum = 0.0;
    for (int i = 1; i <= fixings; ++i) {
        sum += o.spot * exp(o.rate * o.time_to_expiry * i / fixings);
    }
    return exp(-o.rate * o.time_to_expiry) * sum / fixings;
}

double speedup(const OptionInputs& option, int fixings, VarianceReduction method,
               uint64_t seed) {
    const double plain = mc_asian_price(option, fixings, 50000, seed).std_error;
    const double reduced = mc_asian_price(option, fixings, 50000, seed, method).std_error;
    return (plain / reduced) * (plain / reduced);
}

}  // namespace

TEST(AsianLognormal, ReproducesHullExample26_3) {
    const auto call = hull(OptionType::Call);
    EXPECT_NEAR(asian_lognormal_price(call, 12), 6.00, 0.005);
    EXPECT_NEAR(asian_lognormal_price(call, 52), 5.70, 0.005);
    EXPECT_NEAR(asian_lognormal_price(call, 250), 5.63, 0.005);
    EXPECT_NEAR(asian_lognormal_price(call, 100000), 5.62, 0.005);
}

TEST(AsianLognormal, SatisfiesPutCallParity) {
    for (int fixings : {1, 12, 250}) {
        const double parity = discounted_average_forward(hull(OptionType::Call), fixings) -
                              50.0 * exp(-0.10);
        EXPECT_NEAR(asian_lognormal_price(hull(OptionType::Call), fixings) -
                        asian_lognormal_price(hull(OptionType::Put), fixings),
                    parity, 1e-10)
            << fixings;
    }
}

// With one fixing at T the average is S_T itself: every Asian price is Black-Scholes.
TEST(Asian, OneFixingIsTheVanillaOption) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        const auto option = hull(type, 45.0);
        EXPECT_NEAR(geometric_asian_price(option, 1), ope::price(option), 1e-10);
        EXPECT_NEAR(asian_lognormal_price(option, 1), ope::price(option), 1e-10);
        const McResult mc =
            mc_asian_price(option, 1, 100000, 1, VarianceReduction::ControlVariate);
        EXPECT_NEAR(mc.price, ope::price(option), 1e-10);
    }
}

TEST(GeometricAsian, MatchesADirectSimulation) {
    mt19937_64 rng(7);
    for (auto type : {OptionType::Call, OptionType::Put}) {
        const auto option = hull(type);
        const ope::GbmParams params{50.0, 0.10, 0.40, 1.0, 12};
        double sum = 0.0;
        double sum_sq = 0.0;
        const int paths = 200000;
        for (int i = 0; i < paths; ++i) {
            const vector<double> path = ope::simulate_gbm_path(params, rng);
            double logs = 0.0;
            for (size_t k = 1; k < path.size(); ++k) logs += log(path[k]);
            const double g = exp(logs / 12);
            const double x = exp(-0.10) * (type == OptionType::Call ? max(g - 50.0, 0.0)
                                                                    : max(50.0 - g, 0.0));
            sum += x;
            sum_sq += x * x;
        }
        const double mean = sum / paths;
        const double se = sqrt((sum_sq / paths - mean * mean) / (paths - 1));
        EXPECT_NEAR(mean, geometric_asian_price(option, 12), 4.0 * se);
    }
}

// The geometric average never exceeds the arithmetic one, path by path.
TEST(GeometricAsian, CallIsBelowAndPutAboveTheArithmetic) {
    for (int fixings : {12, 52, 250}) {
        EXPECT_LT(geometric_asian_price(hull(OptionType::Call), fixings),
                  asian_lognormal_price(hull(OptionType::Call), fixings));
        EXPECT_GT(geometric_asian_price(hull(OptionType::Put), fixings),
                  asian_lognormal_price(hull(OptionType::Put), fixings));
    }
}

// Every method must agree with the control variate, the tightest estimate.
TEST(McAsian, EveryMethodAgreesAcrossStrikes) {
    uint64_t seed = 40;
    for (auto method : {VarianceReduction::None, VarianceReduction::Antithetic}) {
        for (double strike : {35.0, 45.0, 50.0, 55.0, 70.0}) {
            for (auto type : {OptionType::Call, OptionType::Put}) {
                const auto option = hull(type, strike);
                const McResult mc = mc_asian_price(option, 12, 40000, seed++, method);
                const McResult cv = mc_asian_price(option, 12, 40000, seed++,
                                                   VarianceReduction::ControlVariate);
                EXPECT_NEAR(mc.price, cv.price, 4.0 * mc.std_error + 4.0 * cv.std_error)
                    << "K=" << strike;
            }
        }
    }
}

// The arithmetic average is only roughly lognormal; the approximation overprices,
// more so as σ grows (Hull's 6.00 against ≈ 5.94 by simulation).
TEST(McAsian, HullsApproximationOverpricesMoreAtHigherVolatility) {
    auto calm = hull(OptionType::Call);
    calm.volatility = 0.20;
    const McResult calm_mc =
        mc_asian_price(calm, 12, 200000, 30, VarianceReduction::ControlVariate);
    const double calm_gap = asian_lognormal_price(calm, 12) - calm_mc.price;

    const McResult mc = mc_asian_price(hull(OptionType::Call), 12, 200000, 31,
                                       VarianceReduction::ControlVariate);
    const double gap = asian_lognormal_price(hull(OptionType::Call), 12) - mc.price;

    EXPECT_GT(calm_gap, 0.005);
    EXPECT_LT(calm_gap, 0.02);
    EXPECT_GT(gap, 0.04);
    EXPECT_LT(gap, 0.07);
}

TEST(McAsian, SatisfiesPutCallParity) {
    const McResult call = mc_asian_price(hull(OptionType::Call), 12, 200000, 50,
                                         VarianceReduction::ControlVariate);
    const McResult put = mc_asian_price(hull(OptionType::Put), 12, 200000, 51,
                                        VarianceReduction::ControlVariate);
    const double se = sqrt(call.std_error * call.std_error + put.std_error * put.std_error);
    EXPECT_NEAR(call.price - put.price,
                discounted_average_forward(hull(OptionType::Call), 12) - 50.0 * exp(-0.10),
                4.0 * se);
}

TEST(McAsian, AveragingMakesItCheaperThanTheVanillaOption) {
    const auto call = hull(OptionType::Call);
    EXPECT_LT(mc_asian_price(call, 12, 100000, 60).price, 0.7 * ope::price(call));
}

TEST(McAsian, GeometricControlVariateIsWorthHundredsOfPaths) {
    EXPECT_GT(speedup(hull(OptionType::Call), 12, VarianceReduction::ControlVariate, 70), 100.0);
    EXPECT_GT(speedup(hull(OptionType::Put), 12, VarianceReduction::ControlVariate, 71), 100.0);
    EXPECT_GT(speedup(hull(OptionType::Call), 12, VarianceReduction::Antithetic, 72), 1.4);
}

// No randomness, yet the geometric average of the rising path S·e^(r·tᵢ) is S·e^(r·t̄),
// below the arithmetic average.
TEST(McAsian, ZeroVolatilityIsExactForEveryMethod) {
    auto call = hull(OptionType::Call);
    call.volatility = 0.0;
    const double exact = discounted_average_forward(call, 12) - 50.0 * exp(-0.10);
    const double geometric = exp(-0.10) * (50.0 * exp(0.10 * 13.0 / 24.0) - 50.0);
    EXPECT_NEAR(geometric_asian_price(call, 12), geometric, 1e-10);
    EXPECT_NEAR(asian_lognormal_price(call, 12), exact, 1e-10);
    for (auto method : {VarianceReduction::None, VarianceReduction::Antithetic,
                        VarianceReduction::ControlVariate}) {
        const McResult mc = mc_asian_price(call, 12, 100, 1, method);
        EXPECT_NEAR(mc.price, exact, 1e-10);
        EXPECT_NEAR(mc.std_error, 0.0, 1e-10);
    }
}

TEST(McAsian, SameSeedGivesSameResult) {
    const auto call = hull(OptionType::Call);
    const McResult a = mc_asian_price(call, 12, 5000, 42, VarianceReduction::Antithetic);
    const McResult b = mc_asian_price(call, 12, 5000, 42, VarianceReduction::Antithetic);
    EXPECT_EQ(a.price, b.price);
    EXPECT_EQ(a.std_error, b.std_error);
}

TEST(Asian, RejectsInvalidInputs) {
    const auto call = hull(OptionType::Call);
    EXPECT_THROW((void)geometric_asian_price(call, 0), invalid_argument);
    EXPECT_THROW((void)asian_lognormal_price(call, 0), invalid_argument);
    EXPECT_THROW((void)mc_asian_price(call, 0, 1000, 1), invalid_argument);
    EXPECT_THROW((void)mc_asian_price(call, 12, 1, 1), invalid_argument);
    EXPECT_THROW((void)mc_asian_price(call, 12, 3, 1, VarianceReduction::ControlVariate),
                 invalid_argument);

    auto expired = call;
    expired.time_to_expiry = 0.0;
    EXPECT_THROW((void)geometric_asian_price(expired, 12), invalid_argument);
    EXPECT_THROW((void)mc_asian_price(expired, 12, 1000, 1), invalid_argument);
}
