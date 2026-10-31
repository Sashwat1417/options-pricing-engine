#include "api/MonteCarloController.hpp"

#include "api/JsonRoute.hpp"

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace ope::server {

MonteCarloController::MonteCarloController(const MonteCarloService& service)
    : service_(service) {}

void MonteCarloController::registerRoutes(httplib::Server& server) const {
    postJson(server, "/api/v1/mc/price", [this](const json& body) {
        return service_.price(McPriceRequest::fromJson(body)).toJson();
    });
    postJson(server, "/api/v1/mc/convergence", [this](const json& body) {
        return service_.convergence(McConvergenceRequest::fromJson(body)).toJson();
    });
}

}  // namespace ope::server
