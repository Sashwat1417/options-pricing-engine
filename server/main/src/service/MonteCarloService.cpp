#include "service/MonteCarloService.hpp"

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

}  // namespace ope::server
