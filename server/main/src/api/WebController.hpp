#pragma once

#include <httplib.h>

#include <string>

namespace ope::server {

// Serves the static web UI (index.html, app.css, app.js) from webDir at "/".
class WebController {
public:
    explicit WebController(std::string webDir);

    // Throws std::runtime_error if webDir doesn't exist.
    void registerRoutes(httplib::Server& server) const;

private:
    std::string webDir_;
};

}  // namespace ope::server
