#pragma once

#include "ope/option.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace ope::server {

// POST /api/v1/hedge/path
// {spot, strike, expiry, rate, vol, quantity, premium, drift, rebalances, seed, prices?}
// `prices`, if given, is the path to hedge (e.g. Hull's Table 19.2); otherwise one path is
// simulated from `seed` with real-world drift `drift` and `rebalances` steps.
struct HedgePathRequest {
    static constexpr int kMaxRebalances = 1000;

    OptionInputs option;  // a call
    double quantity;
    double premium;       // per share, received when the option was sold
    double drift;
    int rebalances;
    std::uint64_t seed;
    std::vector<double> prices;  // empty: simulate

    static HedgePathRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
