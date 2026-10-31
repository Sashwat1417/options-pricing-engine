#include "api/JsonRoute.hpp"

#include "response/ErrorResponse.hpp"

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

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

}  // namespace

void postJson(httplib::Server& server, const char* path, function<json(const json&)> handle) {
    server.Post(path, [handle = std::move(handle)](const Request& req, Response& res) {
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

}  // namespace ope::server
