#include "response/HedgePathResponse.hpp"

using nlohmann::json;

namespace ope::server {

json HedgePathResponse::toJson() const {
    json steps = json::array();
    for (const auto& s : result.steps) {
        steps.push_back({{"time", s.time},
                         {"spot", s.spot},
                         {"delta", s.delta},
                         {"shares_held", s.delta * quantity},
                         {"shares_bought", s.shares_bought},
                         {"purchase_cost", s.purchase_cost},
                         {"trading_cost", s.trading_cost},
                         {"cumulative_cost", s.cumulative_cost},
                         {"interest", s.interest}});
    }
    return {{"steps", steps},
            {"total_trading_cost", result.total_trading_cost},
            {"hedge_cost", result.hedge_cost},
            {"pv_hedge_cost", result.pv_hedge_cost},
            {"bs_value", bs_value},
            {"premium_received", premium_received},
            {"pnl", premium_received - result.pv_hedge_cost},
            {"exercised", exercised},
            {"rebalances", rebalances},
            {"seed", seed}};
}

}  // namespace ope::server
