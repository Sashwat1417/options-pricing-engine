#pragma once

#include "ope/option.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>

namespace ope::server {

// POST /api/v1/mc/price  {type, spot, strike, expiry, rate, vol, paths, seed}
// Runs every variance-reduction method on `paths` paths.
struct McPriceRequest {
    static constexpr int kMaxPaths = 2000000;  // per method

    OptionInputs option;
    int paths;
    std::uint64_t seed;

    static McPriceRequest fromJson(const nlohmann::json& body);
};

// type, spot, strike, expiry, rate and vol, shared by the Monte Carlo requests.
OptionInputs mcOption(const nlohmann::json& body);
std::uint64_t mcSeed(const nlohmann::json& body);

}  // namespace ope::server
