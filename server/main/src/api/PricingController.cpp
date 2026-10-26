#include "api/PricingController.hpp"

#include "response/ErrorResponse.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <functional>
#include <stdexcept>
#include <string>

using httplib::Request;
using httplib::Response;
using nlohmann::json;
using std::exception;
using std::function;
using std::invalid_argument;
using std::string;

namespace ope::server {

namespace {

void send(Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void postJson(httplib::Server& server, const char* path, function<json(const json&)> handle) {
    server.Post(path, [handle](const Request& req, Response& res) {
        try {
            send(res, 200, handle(json::parse(req.body)));
        } catch (const json::exception& e) {
            send(res, 400, ErrorResponse{string("invalid JSON: ") + e.what()}.toJson());
        } catch (const invalid_argument& e) {
            send(res, 400, ErrorResponse{e.what()}.toJson());
        } catch (const exception&) {
            send(res, 500, ErrorResponse{"internal server error"}.toJson());
        }
    });
}

}  // namespace

PricingController::PricingController(const PricingService& service) : service_(service) {}

void PricingController::registerRoutes(httplib::Server& server) const {
    server.Get("/health",
               [](const Request&, Response& res) { send(res, 200, {{"status", "ok"}}); });

    postJson(server, "/api/v1/price", [this](const json& body) {
        return service_.price(PriceRequest::fromJson(body)).toJson();
    });
    postJson(server, "/api/v1/iv", [this](const json& body) {
        return service_.impliedVol(IvRequest::fromJson(body)).toJson();
    });
    postJson(server, "/api/v1/sweep", [this](const json& body) {
        return service_.sweep(SweepRequest::fromJson(body)).toJson();
    });
}

}  // namespace ope::server
