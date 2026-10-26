#pragma once

#include <nlohmann/json.hpp>

namespace ope::server {

struct PriceResponse {
    double price;

    nlohmann::json toJson() const { return {{"price", price}}; }
};

}  // namespace ope::server
