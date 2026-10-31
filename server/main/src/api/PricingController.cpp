#include "api/PricingController.hpp"

#include "api/JsonRoute.hpp"

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace ope::server {

PricingController::PricingController(const PricingService& service) : service_(service) {}

void PricingController::registerRoutes(httplib::Server& server) const {
    postJson(server, "/api/v1/price", [this](const json& body) {
        return service_.price(PriceRequest::fromJson(body)).toJson();
    });
    postJson(server, "/api/v1/iv", [this](const json& body) {
        return service_.impliedVol(IvRequest::fromJson(body)).toJson();
    });
    postJson(server, "/api/v1/sweep", [this](const json& body) {
        return service_.sweep(SweepRequest::fromJson(body)).toJson();
    });
    postJson(server, "/api/v1/greeks", [this](const json& body) {
        return service_.greeks(GreeksRequest::fromJson(body)).toJson();
    });
}

}  // namespace ope::server
