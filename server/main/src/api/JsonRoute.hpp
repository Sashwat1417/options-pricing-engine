#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <functional>

namespace ope::server {

// Registers POST `path`: parses the body as JSON and returns handle(body) with 200.
// Malformed JSON or std::invalid_argument -> 400 {"error": ...}; anything else -> 500.
void postJson(httplib::Server& server, const char* path,
              std::function<nlohmann::json(const nlohmann::json&)> handle);

}  // namespace ope::server
