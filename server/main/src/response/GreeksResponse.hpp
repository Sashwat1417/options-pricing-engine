#pragma once

#include "ope/greeks.hpp"

#include <nlohmann/json.hpp>

namespace ope::server {

// {"delta", "theta", "gamma", "vega", "rho"}; theta per year, vega and rho per unit of σ / r
struct GreeksResponse {
    Greeks greeks;

    nlohmann::json toJson() const {
        return {{"delta", greeks.delta},
                {"theta", greeks.theta},
                {"gamma", greeks.gamma},
                {"vega", greeks.vega},
                {"rho", greeks.rho}};
    }
};

}  // namespace ope::server
