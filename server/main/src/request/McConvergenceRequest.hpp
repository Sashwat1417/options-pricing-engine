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

// 1,000, 2,000, 5,000 … 1,000,000.
std::vector<int> defaultPathCounts();

// The optional "paths" array: 1 to 20 integers in [4, McPriceRequest::kMaxPaths].
// Returns an empty vector when the field is absent.
std::vector<int> mcPathCounts(const nlohmann::json& body);

}  // namespace ope::server
