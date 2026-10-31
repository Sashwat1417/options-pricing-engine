#pragma once

#include "ope/hedging.hpp"
#include "ope/option.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace ope::server {

// POST /api/v1/hedge/path
// {spot, strike, expiry, rate, vol, quantity, premium, drift, real_vol?, rebalances, seed,
//  prices?}
// `prices`, if given, is the path to hedge (e.g. Hull's Table 19.2); otherwise one path is
// simulated from `seed` at drift `drift` and realised vol `real_vol` (default: vol), with
// `rebalances` steps. Δ always uses vol.
struct HedgePathRequest {
    static constexpr int kMaxRebalances = 1000;

    OptionInputs option;  // a call
    double quantity;
    double premium;       // per share, received when the option was sold
    RealWorld world;
    int rebalances;
    std::uint64_t seed;
    std::vector<double> prices;  // empty: simulate

    static HedgePathRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
