#pragma once

#include "service/MonteCarloService.hpp"

#include <httplib.h>

namespace ope::server {

// POST /api/v1/mc/{price,convergence}.
// Bad input -> 400 {"error": ...}, unexpected failure -> 500.
class MonteCarloController {
public:
    explicit MonteCarloController(const MonteCarloService& service);

    void registerRoutes(httplib::Server& server) const;

private:
    const MonteCarloService& service_;
};

}  // namespace ope::server
