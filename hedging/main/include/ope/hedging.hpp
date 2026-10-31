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
    double cumulative_cost;  // all purchases and interest so far
    double interest;         // interest on cumulative_cost until the next rebalance
};

struct HedgeResult {
    std::vector<HedgeStep> steps;  // one per path point, the last at expiry
    double hedge_cost;             // final cumulative cost, net of the strike received if exercised
    double pv_hedge_cost;          // hedge_cost discounted to today; ≈ BS price · quantity
};

// Delta-hedges `quantity` sold European calls along `path`: path[0] is today's price and
// path.back() the price at expiry, with evenly spaced rebalances in between.
// option.spot must equal path[0]; option.volatility is the σ used for delta.
// Throws std::invalid_argument on invalid inputs.
[[nodiscard]] HedgeResult hedge_short_call(const OptionInputs& option, double quantity,
                                           const std::vector<double>& path);

// Simulates one real-world path (drift μ, σ = option.volatility, `rebalances` steps) from
// `seed` and delta-hedges along it. The same seed always gives the same path.
[[nodiscard]] HedgeResult hedge_simulated_path(const OptionInputs& option, double quantity,
                                               double drift, int rebalances, std::uint64_t seed);

struct HedgeCostStats {
    double mean_pv_cost;  // ≈ bs_value
    double sd_pv_cost;
    double bs_value;      // Black-Scholes price · quantity
};

// Hedges `paths` simulated paths and summarises the PV hedge cost (Hull Table 19.1).
[[nodiscard]] HedgeCostStats hedge_cost_stats(const OptionInputs& option, double quantity,
                                              double drift, int rebalances, int paths,
                                              std::uint64_t seed);

}  // namespace ope
