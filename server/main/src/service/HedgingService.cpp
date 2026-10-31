#include "service/HedgingService.hpp"

#include "ope/black_scholes.hpp"
#include "ope/hedging.hpp"
#include "ope/implied_vol.hpp"

#include <optional>
#include <stdexcept>

using std::invalid_argument;
using std::nullopt;
using std::optional;

namespace ope::server {

HedgePathResponse HedgingService::hedgePath(const HedgePathRequest& request) const {
    const HedgeResult result =
        request.prices.empty()
            ? hedge_simulated_path(request.option, request.quantity,
                                   request.world, request.rebalances, request.seed,
                                   request.costRate)
            : hedge_short_call(request.option, request.quantity, request.prices,
                               request.costRate);

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
        response.rows.push_back(
            {n, hedge_cost_stats(request.option, request.quantity,
                                 request.world, n, request.paths, request.seed,
                                 request.costRate)});
    }
    return response;
}

HedgeVolStudyResponse HedgingService::hedgeVolStudy(const HedgeVolStudyRequest& request) const {
    optional<double> premiumVol = nullopt;
    try {
        premiumVol = implied_vol(request.option, request.premium);
    } catch (const invalid_argument&) {
    }

    HedgeVolStudyResponse response{request.option.volatility,
                                   premiumVol,
                                   price(request.option) * request.quantity,
                                   request.premium * request.quantity,
                                   request.paths,
                                   request.rebalances,
                                   request.seed,
                                   {}};
    for (double vol : request.realVols) {
        response.rows.push_back(
            {vol, hedge_cost_stats(request.option, request.quantity, {request.drift, vol},
                                   request.rebalances, request.paths, request.seed,
                                   request.costRate)});
    }
    return response;
}

}  // namespace ope::server
