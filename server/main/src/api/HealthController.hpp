#pragma once

#include <httplib.h>

#include <string>

namespace ope::server {

// GET /health -> {"status": "ok"}
// GET /version -> {"version": <library version>, "commit": <deployed commit>}
class HealthController {
public:
    explicit HealthController(std::string commit);

    void registerRoutes(httplib::Server& server) const;

private:
    std::string commit_;
};

}  // namespace ope::server
