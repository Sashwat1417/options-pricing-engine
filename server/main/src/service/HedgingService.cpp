#include "service/HedgingService.hpp"

#include "ope/black_scholes.hpp"
#include "ope/hedging.hpp"

namespace ope::server {

HedgePathResponse HedgingService::hedgePath(const HedgePathRequest& request) const {
    const HedgeResult result =
        request.prices.empty()
            ? hedge_simulated_path(request.option, request.quantity, request.drift,
                                   request.rebalances, request.seed)
            : hedge_short_call(request.option, request.quantity, request.prices);

    return {result,
            request.quantity,
            price(request.option) * request.quantity,
            request.premium * request.quantity,
            result.steps.back().spot > request.option.strike,
            request.rebalances,
            request.seed};
}

HedgeFrequencyResponse HedgingService::hedgeFrequency(const HedgeFrequencyRequest& request) const {
    HedgeFrequencyResponse response{price(request.option) * request.quantity, request.paths,
                                    request.seed, {}};
    for (int n : request.rebalances) {
        response.rows.push_back({n, hedge_cost_stats(request.option, request.quantity,
                                                     request.drift, n, request.paths,
                                                     request.seed)});
    }
    return response;
}

}  // namespace ope::server
