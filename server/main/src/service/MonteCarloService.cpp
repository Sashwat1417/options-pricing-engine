#include "service/MonteCarloService.hpp"

#include "ope/black_scholes.hpp"
#include "ope/mc_pricer.hpp"

namespace ope::server {

McPriceResponse MonteCarloService::price(const McPriceRequest& request) const {
    const McResult result = mc_price(request.option, request.paths, request.seed);
    return {result, ope::price(request.option), request.paths, request.seed};
}

McConvergenceResponse MonteCarloService::convergence(const McConvergenceRequest& request) const {
    McConvergenceResponse response{ope::price(request.option), request.seed, {}};
    for (int n : request.paths) {
        response.rows.push_back({n, mc_price(request.option, n, request.seed)});
    }
    return response;
}

}  // namespace ope::server
