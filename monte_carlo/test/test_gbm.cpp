#include "ope/gbm.hpp"

#include "ope/black_scholes.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

using std::exp;
using std::invalid_argument;
using std::log;
using std::max;
using std::mt19937_64;
using std::numeric_limits;
using std::sqrt;
using std::vector;

namespace {

using ope::GbmParams;
using ope::simulate_gbm_path;

// Hull Ch 19 example in the real world: S=49, μ=13%, σ=20%, T=20 weeks.
GbmParams hull_ch19(int steps = 20) { return {49.0, 0.13, 0.20, 0.3846, steps}; }

struct Sample {
    double mean;
    double sd;
};

Sample summarise(const vector<double>& xs) {
    double sum = 0.0;
    for (double x : xs) sum += x;
    const double mean = sum / static_cast<double>(xs.size());
    double squares = 0.0;
    for (double x : xs) squares += (x - mean) * (x - mean);
    return {mean, sqrt(squares / static_cast<double>(xs.size() - 1))};
}

// Final prices of `paths` simulated paths.
vector<double> final_prices(const GbmParams& p, int paths, mt19937_64& rng) {
    vector<double> out;
    out.reserve(static_cast<size_t>(paths));
    for (int i = 0; i < paths; ++i) out.push_back(simulate_gbm_path(p, rng).back());
    return out;
}

}  // namespace

TEST(Gbm, ReturnsStepsPlusOnePointsStartingAtSpot) {
    mt19937_64 rng(1);
    const auto path = simulate_gbm_path(hull_ch19(20), rng);
    ASSERT_EQ(path.size(), 21u);
    EXPECT_DOUBLE_EQ(path.front(), 49.0);
}

TEST(Gbm, SameSeedGivesSamePath) {
    mt19937_64 a(42);
    mt19937_64 b(42);
    EXPECT_EQ(simulate_gbm_path(hull_ch19(), a), simulate_gbm_path(hull_ch19(), b));

    mt19937_64 c(43);
    mt19937_64 d(42);
    EXPECT_NE(simulate_gbm_path(hull_ch19(), c), simulate_gbm_path(hull_ch19(), d));
}

TEST(Gbm, ZeroVolatilityGrowsAtDrift) {
    auto p = hull_ch19(10);
    p.volatility = 0.0;
    mt19937_64 rng(1);
    const auto path = simulate_gbm_path(p, rng);
    const double dt = p.horizon / p.steps;
    for (int k = 0; k <= p.steps; ++k) {
        EXPECT_NEAR(path[static_cast<size_t>(k)], p.spot * exp(p.drift * k * dt), 1e-12)
            << "k=" << k;
    }
}

TEST(Gbm, StaysPositiveEvenAtHighVolatility) {
    auto p = hull_ch19(250);
    p.volatility = 2.0;
    mt19937_64 rng(7);
    for (int i = 0; i < 200; ++i) {
        for (double s : simulate_gbm_path(p, rng)) ASSERT_GT(s, 0.0);
    }
}

// The tests below are statistical: each estimate must land within 4 standard errors of the
// exact value. They check distributions, not exact numbers, because std::normal_distribution
// differs between standard libraries (macOS vs Linux) for the same seed.

TEST(Gbm, MeanFinalPriceGrowsAtDrift) {
    mt19937_64 rng(11);
    const auto p = hull_ch19(1);
    const auto s = summarise(final_prices(p, 200000, rng));
    const double expected = p.spot * exp(p.drift * p.horizon);  // E[S_T] = S₀·e^(μT)
    EXPECT_NEAR(s.mean, expected, 4.0 * s.sd / sqrt(200000.0));
}

TEST(Gbm, LogReturnIsNormalWithVarianceSigmaSquaredT) {
    mt19937_64 rng(13);
    const auto p = hull_ch19(1);
    const int n = 200000;
    vector<double> log_returns;
    log_returns.reserve(n);
    for (double s : final_prices(p, n, rng)) log_returns.push_back(log(s / p.spot));
    const auto s = summarise(log_returns);

    const double variance = p.volatility * p.volatility * p.horizon;
    EXPECT_NEAR(s.mean, (p.drift - 0.5 * p.volatility * p.volatility) * p.horizon,
                4.0 * sqrt(variance / n));
    EXPECT_NEAR(s.sd * s.sd, variance, 4.0 * variance * sqrt(2.0 / (n - 1)));
}

TEST(Gbm, ManySmallStepsMatchOneBigStep) {
    mt19937_64 rng(17);
    const auto p = hull_ch19(50);
    const auto s = summarise(final_prices(p, 40000, rng));
    EXPECT_NEAR(s.mean, p.spot * exp(p.drift * p.horizon), 4.0 * s.sd / sqrt(40000.0));
}

// Risk-neutral paths (drift = r) price a European call at the Black-Scholes value.
TEST(Gbm, RiskNeutralPathsPriceCallAtBlackScholes) {
    const ope::OptionInputs call{ope::OptionType::Call, 42.0, 40.0, 0.5, 0.10, 0.20};
    const GbmParams p{call.spot, call.rate, call.volatility, call.time_to_expiry, 1};
    mt19937_64 rng(19);

    const int n = 200000;
    vector<double> discounted_payoffs;
    discounted_payoffs.reserve(n);
    const double discount = exp(-call.rate * call.time_to_expiry);
    for (double s_t : final_prices(p, n, rng)) {
        discounted_payoffs.push_back(discount * max(s_t - call.strike, 0.0));
    }
    const auto s = summarise(discounted_payoffs);
    EXPECT_NEAR(s.mean, ope::price(call), 4.0 * s.sd / sqrt(static_cast<double>(n)));
}

TEST(Gbm, RejectsInvalidParameters) {
    mt19937_64 rng(1);
    const double nan = numeric_limits<double>::quiet_NaN();
    for (auto bad : {GbmParams{0.0, 0.1, 0.2, 1.0, 10}, GbmParams{-5.0, 0.1, 0.2, 1.0, 10},
                     GbmParams{nan, 0.1, 0.2, 1.0, 10}, GbmParams{50.0, nan, 0.2, 1.0, 10},
                     GbmParams{50.0, 0.1, -0.2, 1.0, 10}, GbmParams{50.0, 0.1, 0.2, 0.0, 10},
                     GbmParams{50.0, 0.1, 0.2, 1.0, 0}}) {
        EXPECT_THROW((void)simulate_gbm_path(bad, rng), invalid_argument);
    }
}
