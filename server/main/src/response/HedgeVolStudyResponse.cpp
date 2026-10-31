#include "response/HedgeVolStudyResponse.hpp"

using nlohmann::json;

namespace ope::server {

json HedgeVolStudyResponse::toJson() const {
    json out_rows = json::array();
    for (const auto& r : rows) {
        out_rows.push_back({{"real_vol", r.realVol},
                            {"mean_pv_cost", r.stats.mean_pv_cost},
                            {"sd_pv_cost", r.stats.sd_pv_cost},
                            {"mean_pnl", premiumReceived - r.stats.mean_pv_cost}});
    }
    return {{"implied_vol", impliedVol},
            {"premium_implied_vol", premiumImpliedVol ? json(*premiumImpliedVol) : json(nullptr)},
            {"bs_value", bsValue},
            {"premium_received", premiumReceived},
            {"paths", paths},
            {"rebalances", rebalances},
            {"seed", seed},
            {"rows", out_rows}};
}

}  // namespace ope::server
