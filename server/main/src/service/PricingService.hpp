#pragma once

#include "request/GreeksRequest.hpp"
#include "request/IvRequest.hpp"
#include "request/PriceRequest.hpp"
#include "request/SweepRequest.hpp"
#include "response/GreeksResponse.hpp"
#include "response/IvResponse.hpp"
#include "response/PriceResponse.hpp"
#include "response/SweepResponse.hpp"

namespace ope::server {

// Runs requests through ope::black_scholes. Stateless.
// Throws std::invalid_argument when the library rejects the inputs.
class PricingService {
public:
    PriceResponse price(const PriceRequest& request) const;
    IvResponse impliedVol(const IvRequest& request) const;
    SweepResponse sweep(const SweepRequest& request) const;
    GreeksResponse greeks(const GreeksRequest& request) const;
};

}  // namespace ope::server
