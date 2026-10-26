#include "request/SweepRequest.hpp"

#include "request/JsonReader.hpp"

#include <stdexcept>

using nlohmann::json;
using std::invalid_argument;
using std::string;
using std::to_string;

namespace ope::server {

namespace {

SweepAxis parseAxis(const string& vary) {
    if (vary == "spot") {
        return SweepAxis::Spot;
    }
    if (vary == "vol") {
        return SweepAxis::Vol;
    }
    if (vary == "expiry") {
        return SweepAxis::Expiry;
    }
    throw invalid_argument("vary must be \"spot\", \"vol\" or \"expiry\"");
}

int parseSteps(const json& body) {
    const json& value = json_reader::field(body, "steps");
    const string rule =
        "steps must be an integer in [2, " + to_string(SweepRequest::kMaxSteps) + "]";
    if (!value.is_number_integer()) {
        throw invalid_argument(rule);
    }
    const auto steps = value.get<long long>();
    if (steps < 2 || steps > SweepRequest::kMaxSteps) {
        throw invalid_argument(rule);
    }
    return static_cast<int>(steps);
}

}  // namespace

SweepRequest SweepRequest::fromJson(const json& body) {
    SweepRequest request{};
    request.base = json_reader::marketInputs(body, OptionType::Call);
    request.axisName = json_reader::text(body, "vary");
    request.axis = parseAxis(request.axisName);
    if (request.axis != SweepAxis::Vol) {
        request.base.volatility = json_reader::number(body, "vol");
    }
    request.from = json_reader::number(body, "from");
    request.to = json_reader::number(body, "to");
    request.steps = parseSteps(body);
    return request;
}

}  // namespace ope::server
