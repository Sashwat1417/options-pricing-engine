#pragma once

#include "ope/option.hpp"

#include <nlohmann/json.hpp>

namespace ope::server {

// POST /api/v1/greeks  {type, spot, strike, expiry, rate, vol}
struct GreeksRequest {
    OptionInputs inputs;

    static GreeksRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
