#pragma once

#include "ope/sweep.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace ope::server {

// {"vary", "x": [...], "call": [...], "put": [...], "call_payoff": [...], "put_payoff": [...]}
struct SweepResponse {
    std::string vary;
    std::vector<SweepPoint> points;

    nlohmann::json toJson() const;
};

}  // namespace ope::server
