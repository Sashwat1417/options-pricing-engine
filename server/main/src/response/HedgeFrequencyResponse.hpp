#pragma once

#include "ope/hedging.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace ope::server {

// {"bs_value", "paths", "seed",
//  "rows": [{rebalances, mean_pv_cost, sd_pv_cost, sd_over_bs}]}  (Hull Table 19.1)
struct HedgeFrequencyResponse {
    struct Row {
        int rebalances;
        HedgeCostStats stats;
    };

    double bs_value;
    int paths;
    std::uint64_t seed;
    std::vector<Row> rows;

    nlohmann::json toJson() const;
};

}  // namespace ope::server
