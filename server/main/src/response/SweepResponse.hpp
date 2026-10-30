#pragma once

#include "ope/sweep.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace ope::server {

// {"vary", "x", "call", "put", "call_payoff", "put_payoff", "call_delta", "put_delta",
//  "call_theta", "put_theta", "call_gamma", "put_gamma"}, arrays; theta per year
struct SweepResponse {
    std::string vary;
    std::vector<SweepPoint> points;

    nlohmann::json toJson() const;
};

}  // namespace ope::server
