#include "ope/hedging.hpp"

#include "ope/black_scholes.hpp"
#include "ope/gbm.hpp"
#include "ope/greeks.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <stdexcept>
#include <vector>

using std::exp;
using std::invalid_argument;
using std::mt19937_64;
using std::sqrt;
using std::vector;

namespace {

using ope::GbmParams;
using ope::hedge_short_call;
using ope::HedgeResult;
using ope::OptionInputs;
using ope::OptionType;

// Hull 19.1: the bank sold calls on 100,000 shares. S=49, K=50, r=5%, σ=20%, T=20 weeks.
const OptionInputs kHullCall{OptionType::Call, 49.0, 50.0, 0.3846, 0.05, 0.20};
constexpr double kQuantity = 100000.0;
constexpr double kRealWorldDrift = 0.13;

// 21 weekly prices from 49 to `end`, moving in a straight line.
vector<double> straight_path(double end, int steps = 20) {
    vector<double> path;
    for (int k = 0; k <= steps; ++k) path.push_back(49.0 + (end - 49.0) * k / steps);
    return path;
}

struct Spread {
    double mean;
    double sd;
};

// PV hedge cost over `paths` real-world paths with `steps` rebalances each.
Spread hedge_cost_spread(int paths, int steps, unsigned seed) {
    mt19937_64 rng(seed);
    const GbmParams p{kHullCall.spot, kRealWorldDrift, kHullCall.volatility,
                      kHullCall.time_to_expiry, steps};
    vector<double> costs;
    for (int i = 0; i < paths; ++i) {
        costs.push_back(
            hedge_short_call(kHullCall, kQuantity, ope::simulate_gbm_path(p, rng)).pv_hedge_cost);
    }
    double sum = 0.0;
    for (double c : costs) sum += c;
    const double mean = sum / paths;
    double squares = 0.0;
    for (double c : costs) squares += (c - mean) * (c - mean);
    return {mean, sqrt(squares / (paths - 1))};
}

}  // namespace

TEST(Hedging, OneRowPerPathPoint) {
    const auto result = hedge_short_call(kHullCall, kQuantity, straight_path(55.0));
    ASSERT_EQ(result.steps.size(), 21u);
    EXPECT_DOUBLE_EQ(result.steps.front().time, 0.0);
    EXPECT_NEAR(result.steps.back().time, kHullCall.time_to_expiry, 1e-12);
}

TEST(Hedging, FirstWeekBuysDeltaShares) {
    const auto first = hedge_short_call(kHullCall, kQuantity, straight_path(55.0)).steps.front();
    const double delta = ope::greeks(kHullCall).delta;  // 0.5216 (Hull: 0.522)
    EXPECT_NEAR(first.delta, delta, 1e-12);
    EXPECT_NEAR(first.shares_bought, delta * kQuantity, 1e-6);   // ≈ 52,160 shares
    EXPECT_NEAR(first.purchase_cost, delta * kQuantity * 49.0, 1e-4);
    EXPECT_NEAR(first.cumulative_cost, first.purchase_cost, 1e-6);
}

TEST(Hedging, HoldsDeltaTimesQuantityAfterEveryRebalance) {
    const auto result = hedge_short_call(kHullCall, kQuantity, straight_path(52.0));
    double shares = 0.0;
    for (const auto& step : result.steps) {
        shares += step.shares_bought;
        EXPECT_NEAR(shares, step.delta * kQuantity, 1e-6) << "t=" << step.time;
        EXPECT_NEAR(step.purchase_cost, step.shares_bought * step.spot, 1e-6);
    }
}

TEST(Hedging, UsesDeltaAtTimeLeft) {
    const auto path = straight_path(53.0);
    const auto result = hedge_short_call(kHullCall, kQuantity, path);
    for (size_t k = 0; k + 1 < path.size(); ++k) {
        OptionInputs now = kHullCall;
        now.spot = path[k];
        now.time_to_expiry = kHullCall.time_to_expiry - result.steps[k].time;
        EXPECT_NEAR(result.steps[k].delta, ope::greeks(now).delta, 1e-12) << "week " << k;
    }
}

TEST(Hedging, AccruesInterestOnCumulativeCost) {
    const auto steps = hedge_short_call(kHullCall, kQuantity, straight_path(53.0)).steps;
    const double dt = kHullCall.time_to_expiry / 20;
    for (size_t k = 0; k + 1 < steps.size(); ++k) {
        EXPECT_NEAR(steps[k].interest, steps[k].cumulative_cost * (exp(kHullCall.rate * dt) - 1),
                    1e-6);
        EXPECT_NEAR(steps[k + 1].cumulative_cost,
                    steps[k].cumulative_cost + steps[k].interest + steps[k + 1].purchase_cost,
                    1e-6);
    }
    EXPECT_DOUBLE_EQ(steps.back().interest, 0.0);
}

TEST(Hedging, ExercisedCallEndsHoldingAllSharesAndReceivesStrike) {
    const auto result = hedge_short_call(kHullCall, kQuantity, straight_path(57.0));
    EXPECT_DOUBLE_EQ(result.steps.back().delta, 1.0);
    EXPECT_NEAR(result.hedge_cost, result.steps.back().cumulative_cost - 50.0 * kQuantity, 1e-6);
    EXPECT_NEAR(result.pv_hedge_cost,
                result.hedge_cost * exp(-kHullCall.rate * kHullCall.time_to_expiry), 1e-6);
}

TEST(Hedging, UnexercisedCallEndsHoldingNoShares) {
    const auto result = hedge_short_call(kHullCall, kQuantity, straight_path(44.0));
    EXPECT_DOUBLE_EQ(result.steps.back().delta, 0.0);
    EXPECT_NEAR(result.hedge_cost, result.steps.back().cumulative_cost, 1e-6);
}

TEST(Hedging, NeverLooksAhead) {
    auto path = straight_path(53.0);
    const auto before = hedge_short_call(kHullCall, kQuantity, path).steps;
    for (size_t k = 11; k < path.size(); ++k) path[k] += 7.0;  // change the future after week 10
    const auto after = hedge_short_call(kHullCall, kQuantity, path).steps;
    for (size_t k = 0; k <= 10; ++k) {
        EXPECT_DOUBLE_EQ(after[k].delta, before[k].delta) << "week " << k;
        EXPECT_DOUBLE_EQ(after[k].cumulative_cost, before[k].cumulative_cost) << "week " << k;
    }
}

// Hull's central result: on average, hedging costs the Black-Scholes value (≈ $240,000),
// whatever the real-world drift.
TEST(Hedging, AverageCostIsBlackScholesValue) {
    const auto s = hedge_cost_spread(2000, 80, 101);
    const double bs_value = ope::price(kHullCall) * kQuantity;
    EXPECT_NEAR(s.mean, bs_value, 4.0 * s.sd / sqrt(2000.0));
    EXPECT_NEAR(s.mean, bs_value, 0.02 * bs_value);
}

// Hull Table 19.1: rebalancing more often makes the cost more predictable.
TEST(Hedging, CostSpreadShrinksWithMoreFrequentRebalancing) {
    const double every_4_weeks = hedge_cost_spread(2000, 5, 103).sd;
    const double weekly = hedge_cost_spread(2000, 20, 107).sd;
    const double every_quarter_week = hedge_cost_spread(2000, 80, 109).sd;
    EXPECT_LT(weekly, every_4_weeks);
    EXPECT_LT(every_quarter_week, weekly);
}

TEST(Hedging, RejectsInvalidInputs) {
    const auto path = straight_path(53.0);
    OptionInputs put = kHullCall;
    put.type = OptionType::Put;
    EXPECT_THROW((void)hedge_short_call(put, kQuantity, path), invalid_argument);
    EXPECT_THROW((void)hedge_short_call(kHullCall, 0.0, path), invalid_argument);
    EXPECT_THROW((void)hedge_short_call(kHullCall, kQuantity, {49.0}), invalid_argument);
    EXPECT_THROW((void)hedge_short_call(kHullCall, kQuantity, {50.0, 51.0}), invalid_argument);
    EXPECT_THROW((void)hedge_short_call(kHullCall, kQuantity, {49.0, -1.0}), invalid_argument);
}
