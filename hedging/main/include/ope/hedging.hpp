#pragma once

#include "ope/option.hpp"

#include <cstdint>
#include <vector>

namespace ope {

// One rebalance: a row of Hull's Table 19.2.
struct HedgeStep {
    double time;             // years since the option was sold
    double spot;             // S at this rebalance
    double delta;            // Δ held after rebalancing
    double shares_bought;    // negative means sold
    double purchase_cost;    // shares_bought · spot
    double trading_cost;     // cost_rate · |shares_bought| · spot, paid on buys and sells
    double cumulative_cost;  // all purchases and interest so far
    double interest;         // interest on cumulative_cost until the next rebalance
};

struct HedgeResult {
    std::vector<HedgeStep> steps;  // one per path point, the last at expiry
    double total_trading_cost;     // sum of trading_cost, before interest
    double hedge_cost;             // final cumulative cost, net of the strike received if exercised
    double pv_hedge_cost;          // hedge_cost discounted to today; ≈ BS price · quantity
};

// Delta-hedges `quantity` sold European calls along `path`: path[0] is today's price and
// path.back() the price at expiry, with evenly spaced rebalances in between.
// option.spot must equal path[0]; option.volatility is the σ used for delta.
// Each trade pays cost_rate × its value (e.g. 0.001 = 0.1%) for spread and commission.
// Throws std::invalid_argument on invalid inputs.
[[nodiscard]] HedgeResult hedge_short_call(const OptionInputs& option, double quantity,
                                           const std::vector<double>& path,
                                           double cost_rate = 0.0);

// How the simulated stock really moves. Δ is always computed at option.volatility (the implied
// vol the option was sold at), so setting volatility here apart from it models hedging with
// the wrong vol: the P&L then depends on realised vs implied volatility.
struct RealWorld {
    double drift;       // μ
    double volatility;  // realised σ
};

// Simulates one real-world path (`rebalances` steps) from `seed` and delta-hedges along it.
// The same seed always gives the same path.
[[nodiscard]] HedgeResult hedge_simulated_path(const OptionInputs& option, double quantity,
                                               RealWorld world, int rebalances,
                                               std::uint64_t seed, double cost_rate = 0.0);

struct HedgeCostStats {
    double mean_pv_cost;  // ≈ bs_value
    double sd_pv_cost;
    double bs_value;      // Black-Scholes price · quantity
};

// Hedges `paths` simulated paths and summarises the PV hedge cost (Hull Table 19.1).
[[nodiscard]] HedgeCostStats hedge_cost_stats(const OptionInputs& option, double quantity,
                                              RealWorld world, int rebalances, int paths,
                                              std::uint64_t seed, double cost_rate = 0.0);

}  // namespace ope
