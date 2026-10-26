#include "service/PricingService.hpp"

#include "ope/black_scholes.hpp"
#include "ope/implied_vol.hpp"
#include "ope/sweep.hpp"

namespace ope::server {

PriceResponse PricingService::price(const PriceRequest& request) const {
    return {ope::price(request.inputs)};
}

IvResponse PricingService::impliedVol(const IvRequest& request) const {
    return {ope::implied_vol(request.inputs, request.marketPrice)};
}

SweepResponse PricingService::sweep(const SweepRequest& request) const {
    return {request.axisName,
            ope::sweep(request.base, request.axis, request.from, request.to, request.steps)};
}

}  // namespace ope::server
