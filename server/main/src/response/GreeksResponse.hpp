#pragma once

#include "ope/greeks.hpp"

#include <nlohmann/json.hpp>

namespace ope::server {

// {"delta"}
struct GreeksResponse {
    Greeks greeks;

    nlohmann::json toJson() const { return {{"delta", greeks.delta}}; }
};

}  // namespace ope::server
