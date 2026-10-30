#pragma once

#include "ope/greeks.hpp"

#include <nlohmann/json.hpp>

namespace ope::server {

// {"delta", "theta", "gamma", "vega"}; theta per year, vega per unit of σ
struct GreeksResponse {
    Greeks greeks;

    nlohmann::json toJson() const {
        return {{"delta", greeks.delta},
                {"theta", greeks.theta},
                {"gamma", greeks.gamma},
                {"vega", greeks.vega}};
    }
};

}  // namespace ope::server
