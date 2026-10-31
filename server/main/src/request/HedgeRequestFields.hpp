#pragma once

#include "ope/hedging.hpp"
#include "ope/option.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>

// Fields shared by the hedging requests. Each throws std::invalid_argument naming the field.
namespace ope::server::hedge_fields {

// spot, strike, expiry, rate, vol as a call.
OptionInputs callOption(const nlohmann::json& body);
double quantity(const nlohmann::json& body);
// drift (required) and real_vol (optional, defaults to the implied vol the option was sold at).
RealWorld realWorld(const nlohmann::json& body, double impliedVol);
std::uint64_t seed(const nlohmann::json& body);

}  // namespace ope::server::hedge_fields
