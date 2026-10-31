#pragma once

#include "ope/hedging.hpp"
#include "ope/option.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace ope::server {

// POST /api/v1/hedge/frequency
// {spot, strike, expiry, rate, vol, quantity, drift, real_vol?, paths, seed, rebalances?: [..]}
// Hull Table 19.1: hedge `paths` simulated paths at each rebalance count.
struct HedgeFrequencyRequest {
    static constexpr int kMaxPaths = 5000;
    static constexpr long long kMaxWork = 2000000;  // paths × Σ rebalances

    OptionInputs option;
    double quantity;
    RealWorld world;
    int paths;
    std::uint64_t seed;
    std::vector<int> rebalances;  // default {4, 5, 10, 20, 40, 80}

    static HedgeFrequencyRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
