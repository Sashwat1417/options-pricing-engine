#include "api/HedgingController.hpp"

#include "api/JsonRoute.hpp"

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace ope::server {

HedgingController::HedgingController(const HedgingService& service) : service_(service) {}

void HedgingController::registerRoutes(httplib::Server& server) const {
    postJson(server, "/api/v1/hedge/path", [this](const json& body) {
        return service_.hedgePath(HedgePathRequest::fromJson(body)).toJson();
    });
    postJson(server, "/api/v1/hedge/frequency", [this](const json& body) {
        return service_.hedgeFrequency(HedgeFrequencyRequest::fromJson(body)).toJson();
    });
}

}  // namespace ope::server
