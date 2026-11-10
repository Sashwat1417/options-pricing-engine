#include "service/MonteCarloService.hpp"

#include "ope/asian.hpp"
#include "ope/black_scholes.hpp"
#include "ope/mc_pricer.hpp"

#include <cmath>
#include <cstdint>

using std::exp;
using std::uint64_t;

namespace ope::server {

namespace {

McEstimates allMethods(const OptionInputs& option, int paths, uint64_t seed) {
    return {mc_price(option, paths, seed, VarianceReduction::None),
            mc_price(option, paths, seed, VarianceReduction::Antithetic),
            mc_price(option, paths, seed, VarianceReduction::ControlVariate)};
}

McEstimates allAsianMethods(const AsianInputs& asian, int paths, uint64_t seed) {
    return {mc_asian_price(asian, paths, seed, VarianceReduction::None),
            mc_asian_price(asian, paths, seed, VarianceReduction::Antithetic),
            mc_asian_price(asian, paths, seed, VarianceReduction::ControlVariate)};
}

AsianReferences asianReferences(const AsianInputs& asian) {
    return {asian_lognormal_price(asian), geometric_asian_price(asian)};
}

}  // namespace

McPriceResponse MonteCarloService::price(const McPriceRequest& request) const {
    return {allMethods(request.option, request.paths, request.seed), ope::price(request.option),
            request.paths, request.seed};
}

McConvergenceResponse MonteCarloService::convergence(const McConvergenceRequest& request) const {
    McConvergenceResponse response{ope::price(request.option), request.seed, {}};
    for (int n : request.paths) {
        response.rows.push_back({n, allMethods(request.option, n, request.seed)});
    }
    return response;
}

AsianPriceResponse MonteCarloService::asianPrice(const AsianPriceRequest& request) const {
    return {allAsianMethods(request.asian, request.paths, request.seed),
            asianReferences(request.asian), request.asian.fixings, request.paths, request.seed};
}

AsianConvergenceResponse MonteCarloService::asianConvergence(
    const AsianConvergenceRequest& request) const {
    AsianConvergenceResponse response{asianReferences(request.asian), request.asian.fixings,
                                      request.seed, {}};
    for (int n : request.paths) {
        response.rows.push_back({n, allAsianMethods(request.asian, n, request.seed)});
    }
    return response;
}

AsianHedgeResponse MonteCarloService::asianHedge(const AsianHedgeRequest& request) const {
    const AverageRateHedge& h = request.hedge;
    const OptionInputs& o = h.put.option;
    const double premiumCost = h.notional * h.premium * exp(o.rate * o.time_to_expiry);
    return {mc_asian_price(h.put, request.paths, request.seed, VarianceReduction::ControlVariate),
            h.premium,
            h.notional,
            premiumCost,
            h.notional * o.strike - premiumCost,
            simulate_average_rate_hedge(h, request.paths, request.seed),
            request.seed};
}

}  // namespace ope::server
