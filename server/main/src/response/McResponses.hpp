#pragma once

#include "ope/mc_pricer.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace ope::server {

// 95% confidence interval: price ± 1.96 standard errors.
inline constexpr double kZ95 = 1.96;

// {"price", "std_error", "ci_low", "ci_high", "bs_price", "paths", "seed"}
struct McPriceResponse {
    McResult result;
    double bsPrice;
    int paths;
    std::uint64_t seed;

    nlohmann::json toJson() const;
};

// {"bs_price", "seed", "rows": [{paths, price, std_error, ci_low, ci_high}]}
struct McConvergenceResponse {
    struct Row {
        int paths;
        McResult result;
    };

    double bsPrice;
    std::uint64_t seed;
    std::vector<Row> rows;

    nlohmann::json toJson() const;
};

}  // namespace ope::server
