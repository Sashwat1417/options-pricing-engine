#pragma once

#include "service/PricingService.hpp"

#include <httplib.h>

namespace ope::server {

// POST /api/v1/{price,iv,sweep,greeks}.
// Bad input -> 400 {"error": ...}, unexpected failure -> 500.
class PricingController {
public:
    explicit PricingController(const PricingService& service);

    void registerRoutes(httplib::Server& server) const;

private:
    const PricingService& service_;
};

}  // namespace ope::server
