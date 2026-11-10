#pragma once

#include "ope/asian.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace ope::server {

// Limits shared by the Asian requests. Work is path steps: paths × fixings, run once per
// method.
struct AsianLimits {
    static constexpr int kMaxFixings = 1000;
    static constexpr long long kMaxWork = 10000000;
};

// POST /api/v1/mc/asian/price
//   {type, spot, strike, expiry, rate, vol, fixings, yield?, paths, seed}
// yield: q, a dividend yield or the foreign rate for a currency; default 0.
struct AsianPriceRequest {
    AsianInputs asian;
    int paths;
    std::uint64_t seed;

    static AsianPriceRequest fromJson(const nlohmann::json& body);
};

// POST /api/v1/mc/asian/convergence
//   {type, spot, strike, expiry, rate, vol, fixings, yield?, seed, paths?: [..]}
// Without paths: the default counts, up to the largest set that fits the work limit.
struct AsianConvergenceRequest {
    AsianInputs asian;
    std::uint64_t seed;
    std::vector<int> paths;

    static AsianConvergenceRequest fromJson(const nlohmann::json& body);
};

// POST /api/v1/mc/asian/hedge
//   {spot, strike, expiry, rate, vol, fixings, yield?, notional, premium, paths, seed}
// Hull's treasurer: notional units of foreign currency over the fixings, floored by an
// average rate put bought at `premium` per unit. Always a put.
struct AsianHedgeRequest {
    static constexpr int kMaxPaths = 200000;

    AverageRateHedge hedge;
    int paths;
    std::uint64_t seed;

    static AsianHedgeRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
