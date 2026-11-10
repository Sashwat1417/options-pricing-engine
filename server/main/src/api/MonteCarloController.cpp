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
    postJson(server, "/api/v1/mc/asian/price", [this](const json& body) {
        return service_.asianPrice(AsianPriceRequest::fromJson(body)).toJson();
    });
    postJson(server, "/api/v1/mc/asian/convergence", [this](const json& body) {
        return service_.asianConvergence(AsianConvergenceRequest::fromJson(body)).toJson();
    });
    postJson(server, "/api/v1/mc/asian/hedge", [this](const json& body) {
        return service_.asianHedge(AsianHedgeRequest::fromJson(body)).toJson();
    });
}

}  // namespace ope::server
