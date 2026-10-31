#pragma once

#include "ope/option.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace ope::server {

// POST /api/v1/mc/convergence  {type, spot, strike, expiry, rate, vol, seed, paths?: [..]}
// Prices the option once per path count, all from the same seed.
struct McConvergenceRequest {
    static constexpr long long kMaxWork = 2000000;  // Σ paths, run once per method

    OptionInputs option;
    std::uint64_t seed;
    std::vector<int> paths;  // default 1,000 … 1,000,000

    static McConvergenceRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
