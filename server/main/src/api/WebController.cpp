#include "api/WebController.hpp"

#include <stdexcept>
#include <utility>

using std::runtime_error;
using std::string;

namespace ope::server {

WebController::WebController(string webDir) : webDir_(std::move(webDir)) {}

void WebController::registerRoutes(httplib::Server& server) const {
    if (!server.set_mount_point("/", webDir_)) {
        throw runtime_error("web directory not found: " + webDir_);
    }
}

}  // namespace ope::server
