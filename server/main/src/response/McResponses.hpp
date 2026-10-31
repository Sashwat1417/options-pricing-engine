#pragma once

#include "ope/mc_pricer.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace ope::server {

// 95% confidence interval: price ± 1.96 standard errors.
inline constexpr double kZ95 = 1.96;

// One estimate per method, all from the same paths and seed.
struct McEstimates {
    McResult plain;
    McResult antithetic;
    McResult controlVariate;
};

// {"bs_price", "paths", "seed",
//  "estimates": {"plain" | "antithetic" | "control_variate":
//                {price, std_error, ci_low, ci_high, speedup}}}
// speedup = (plain SE / method SE)²: how many plain paths one path of the method is worth.
struct McPriceResponse {
    McEstimates estimates;
    double bsPrice;
    int paths;
    std::uint64_t seed;

    nlohmann::json toJson() const;
};

// {"bs_price", "seed", "rows": [{paths, estimates: {...}}]}
struct McConvergenceResponse {
    struct Row {
        int paths;
        McEstimates estimates;
    };

    double bsPrice;
    std::uint64_t seed;
    std::vector<Row> rows;

    nlohmann::json toJson() const;
};

}  // namespace ope::server
