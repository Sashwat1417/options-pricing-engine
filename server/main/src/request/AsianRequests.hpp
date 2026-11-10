#pragma once

#include "ope/option.hpp"

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

// POST /api/v1/mc/asian/price  {type, spot, strike, expiry, rate, vol, fixings, paths, seed}
struct AsianPriceRequest {
    OptionInputs option;
    int fixings;
    int paths;
    std::uint64_t seed;

    static AsianPriceRequest fromJson(const nlohmann::json& body);
};

// POST /api/v1/mc/asian/convergence  {type, spot, strike, expiry, rate, vol, fixings, seed,
//                                     paths?: [..]}
// Without paths: the default counts, up to the largest set that fits the work limit.
struct AsianConvergenceRequest {
    OptionInputs option;
    int fixings;
    std::uint64_t seed;
    std::vector<int> paths;

    static AsianConvergenceRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
