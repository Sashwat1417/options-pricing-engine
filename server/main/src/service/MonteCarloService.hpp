#pragma once

#include "request/McConvergenceRequest.hpp"
#include "request/McPriceRequest.hpp"
#include "response/McResponses.hpp"

namespace ope::server {

// Runs Monte Carlo requests through ope::monte_carlo, with the Black-Scholes price alongside.
// Stateless. Throws std::invalid_argument when the library rejects the inputs.
class MonteCarloService {
public:
    McPriceResponse price(const McPriceRequest& request) const;
    McConvergenceResponse convergence(const McConvergenceRequest& request) const;
};

}  // namespace ope::server
