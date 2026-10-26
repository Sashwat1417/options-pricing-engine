#pragma once

#include "ope/option.hpp"

#include <nlohmann/json.hpp>

namespace ope::server {

// POST /api/v1/price  {type, spot, strike, expiry, rate, vol}
struct PriceRequest {
    OptionInputs inputs;

    static PriceRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
