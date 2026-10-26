#pragma once

#include "ope/option.hpp"
#include "ope/sweep.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace ope::server {

// POST /api/v1/sweep  {spot, strike, expiry, rate, vol, vary, from, to, steps}
// vol is not needed when vary is "vol".
struct SweepRequest {
    static constexpr int kMaxSteps = 2000;

    OptionInputs base;
    SweepAxis axis;
    std::string axisName;
    double from;
    double to;
    int steps;

    static SweepRequest fromJson(const nlohmann::json& body);
};

}  // namespace ope::server
