#include "request/JsonReader.hpp"

#include <stdexcept>

using nlohmann::json;
using std::invalid_argument;
using std::string;
using std::to_string;

namespace ope::server::json_reader {

const json& field(const json& body, const char* name) {
    if (!body.is_object()) {
        throw invalid_argument("request body must be a JSON object");
    }
    const auto it = body.find(name);
    if (it == body.end()) {
        throw invalid_argument(string("missing field: ") + name);
    }
    return *it;
}

double number(const json& body, const char* name) {
    const json& value = field(body, name);
    if (!value.is_number()) {
        throw invalid_argument(string(name) + " must be a number");
    }
    return value.get<double>();
}

string text(const json& body, const char* name) {
    const json& value = field(body, name);
    if (!value.is_string()) {
        throw invalid_argument(string(name) + " must be a string");
    }
    return value.get<string>();
}

OptionType optionType(const json& body) {
    const string type = text(body, "type");
    if (type == "call") {
        return OptionType::Call;
    }
    if (type == "put") {
        return OptionType::Put;
    }
    throw invalid_argument("type must be \"call\" or \"put\"");
}

// Braced initialisation evaluates left to right, so the first missing field is reported.
long long integer(const json& body, const char* name, long long lo, long long hi) {
    const json& value = field(body, name);
    const string rule = string(name) + " must be an integer in [" + to_string(lo) + ", " +
                        to_string(hi) + "]";
    if (!value.is_number_integer()) {
        throw invalid_argument(rule);
    }
    const auto n = value.get<long long>();
    if (n < lo || n > hi) {
        throw invalid_argument(rule);
    }
    return n;
}

OptionInputs marketInputs(const json& body, OptionType type) {
    return {type,
            number(body, "spot"),
            number(body, "strike"),
            number(body, "expiry"),
            number(body, "rate"),
            0.0};
}

}  // namespace ope::server::json_reader
