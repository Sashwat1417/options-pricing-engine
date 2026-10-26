#pragma once

#include "ope/option.hpp"

#include <nlohmann/json.hpp>

#include <string>

// Typed field access on a request body. Each throws std::invalid_argument naming the field.
namespace ope::server::json_reader {

const nlohmann::json& field(const nlohmann::json& body, const char* name);
double number(const nlohmann::json& body, const char* name);
std::string text(const nlohmann::json& body, const char* name);
OptionType optionType(const nlohmann::json& body);

// spot, strike, expiry, rate; volatility is left at 0.
OptionInputs marketInputs(const nlohmann::json& body, OptionType type);

}  // namespace ope::server::json_reader
