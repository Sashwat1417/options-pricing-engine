#include "response/HedgeFrequencyResponse.hpp"

using nlohmann::json;

namespace ope::server {

json HedgeFrequencyResponse::toJson() const {
    json out_rows = json::array();
    for (const auto& r : rows) {
        out_rows.push_back({{"rebalances", r.rebalances},
                            {"mean_pv_cost", r.stats.mean_pv_cost},
                            {"sd_pv_cost", r.stats.sd_pv_cost},
                            {"sd_over_bs", r.stats.sd_pv_cost / bs_value}});
    }
    return {{"bs_value", bs_value}, {"paths", paths}, {"seed", seed}, {"rows", out_rows}};
}

}  // namespace ope::server
