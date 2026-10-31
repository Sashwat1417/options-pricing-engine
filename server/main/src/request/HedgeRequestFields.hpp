#pragma once

#include "ope/option.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>

// Fields shared by the hedging requests. Each throws std::invalid_argument naming the field.
namespace ope::server::hedge_fields {

// spot, strike, expiry, rate, vol as a call.
OptionInputs callOption(const nlohmann::json& body);
double quantity(const nlohmann::json& body);
double drift(const nlohmann::json& body);
std::uint64_t seed(const nlohmann::json& body);

}  // namespace ope::server::hedge_fields
