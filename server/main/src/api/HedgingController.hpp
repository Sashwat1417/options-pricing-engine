#pragma once

#include "service/HedgingService.hpp"

#include <httplib.h>

namespace ope::server {

// POST /api/v1/hedge/{path,frequency,vol-study}.
// Bad input -> 400 {"error": ...}, unexpected failure -> 500.
class HedgingController {
public:
    explicit HedgingController(const HedgingService& service);

    void registerRoutes(httplib::Server& server) const;

private:
    const HedgingService& service_;
};

}  // namespace ope::server
