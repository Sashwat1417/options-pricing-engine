#pragma once

#include "ope/option.hpp"

#include <nlohmann/json.hpp>

namespace ope::server {

// POST /api/v1/iv  {type, spot, strike, expiry, rate, price}
struct IvRequest {
    OptionInputs inputs;
    double marketPrice;

    static IvRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
