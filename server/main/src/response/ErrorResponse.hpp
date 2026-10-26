#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace ope::server {

struct ErrorResponse {
    std::string error;

    nlohmann::json toJson() const { return {{"error", error}}; }
};

}  // namespace ope::server
