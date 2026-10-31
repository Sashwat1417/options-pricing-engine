#pragma once

#include "ope/hedging.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>

namespace ope::server {

// {"steps": [{time, spot, delta, shares_held, shares_bought, purchase_cost, trading_cost,
//             cumulative_cost, interest}], "total_trading_cost",
//  "hedge_cost", "pv_hedge_cost", "bs_value", "premium_received", "pnl", "exercised",
//  "rebalances", "seed"}. Money in dollars; pnl = premium_received − pv_hedge_cost.
struct HedgePathResponse {
    HedgeResult result;
    double quantity;
    double bs_value;
    double premium_received;
    bool exercised;
    int rebalances;
    std::uint64_t seed;

    nlohmann::json toJson() const;
};

}  // namespace ope::server
