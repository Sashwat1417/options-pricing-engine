#include "service/MonteCarloService.hpp"

#include "ope/asian.hpp"
#include "ope/black_scholes.hpp"
#include "ope/mc_pricer.hpp"

#include <cstdint>

using std::uint64_t;

namespace ope::server {

namespace {

McEstimates allMethods(const OptionInputs& option, int paths, uint64_t seed) {
    return {mc_price(option, paths, seed, VarianceReduction::None),
            mc_price(option, paths, seed, VarianceReduction::Antithetic),
            mc_price(option, paths, seed, VarianceReduction::ControlVariate)};
}

McEstimates allAsianMethods(const OptionInputs& option, int fixings, int paths, uint64_t seed) {
    return {mc_asian_price(option, fixings, paths, seed, VarianceReduction::None),
            mc_asian_price(option, fixings, paths, seed, VarianceReduction::Antithetic),
            mc_asian_price(option, fixings, paths, seed, VarianceReduction::ControlVariate)};
}

AsianReferences asianReferences(const OptionInputs& option, int fixings) {
    return {asian_lognormal_price(option, fixings), geometric_asian_price(option, fixings)};
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
    return {allAsianMethods(request.option, request.fixings, request.paths, request.seed),
            asianReferences(request.option, request.fixings), request.fixings, request.paths,
            request.seed};
}

AsianConvergenceResponse MonteCarloService::asianConvergence(
    const AsianConvergenceRequest& request) const {
    AsianConvergenceResponse response{asianReferences(request.option, request.fixings),
                                      request.fixings, request.seed, {}};
    for (int n : request.paths) {
        response.rows.push_back(
            {n, allAsianMethods(request.option, request.fixings, n, request.seed)});
    }
    return response;
}

}  // namespace ope::server
