#pragma once

#include "ope/greeks.hpp"

#include <nlohmann/json.hpp>

namespace ope::server {

// {"delta", "theta"}; theta per year
struct GreeksResponse {
    Greeks greeks;

    nlohmann::json toJson() const { return {{"delta", greeks.delta}, {"theta", greeks.theta}}; }
};

}  // namespace ope::server
