#pragma once

#include <nlohmann/json.hpp>

namespace ope::server {

struct IvResponse {
    double impliedVol;

    nlohmann::json toJson() const { return {{"implied_vol", impliedVol}}; }
};

}  // namespace ope::server
