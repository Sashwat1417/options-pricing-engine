#pragma once

#include "ope/option.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace ope::server {

// POST /api/v1/hedge/vol-study
// {spot, strike, expiry, rate, vol, quantity, premium, drift, rebalances, paths, seed,
//  real_vols?: [..]}
// Sells at implied `vol`, hedges with Δ at `vol`, while the stock really moves at each
// realised vol in turn (Wilmott: hedging with implied vol).
struct HedgeVolStudyRequest {
    static constexpr long long kMaxWork = 2000000;  // paths × rebalances × real vols

    OptionInputs option;
    double quantity;
    double costRate;  // per trade, fraction of its value
    double premium;
    double drift;
    int rebalances;
    int paths;
    std::uint64_t seed;
    std::vector<double> realVols;  // default {0.10, 0.15, 0.20, 0.25, 0.30}

    static HedgeVolStudyRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
