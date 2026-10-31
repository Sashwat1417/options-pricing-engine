#pragma once

#include "ope/hedging.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace ope::server {

// {"implied_vol", "premium_implied_vol" (null if no σ gives that premium), "bs_value",
//  "premium_received", "paths", "rebalances", "seed",
//  "rows": [{real_vol, mean_pv_cost, sd_pv_cost, mean_pnl}]}
struct HedgeVolStudyResponse {
    struct Row {
        double realVol;
        HedgeCostStats stats;
    };

    double impliedVol;
    std::optional<double> premiumImpliedVol;
    double bsValue;
    double premiumReceived;
    int paths;
    int rebalances;
    std::uint64_t seed;
    std::vector<Row> rows;

    nlohmann::json toJson() const;
};

}  // namespace ope::server
