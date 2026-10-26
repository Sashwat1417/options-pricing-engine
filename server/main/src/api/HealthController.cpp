#include "api/HealthController.hpp"

#include "ope/version.hpp"

#include <nlohmann/json.hpp>

#include <utility>

using httplib::Request;
using httplib::Response;
using nlohmann::json;
using std::string;

namespace ope::server {

HealthController::HealthController(string commit) : commit_(std::move(commit)) {}

void HealthController::registerRoutes(httplib::Server& server) const {
    server.Get("/health", [](const Request&, Response& res) {
        res.set_content(json{{"status", "ok"}}.dump(), "application/json");
    });
    server.Get("/version", [this](const Request&, Response& res) {
        const json body = {{"version", string(ope::version())}, {"commit", commit_}};
        res.set_content(body.dump(), "application/json");
    });
}

}  // namespace ope::server
