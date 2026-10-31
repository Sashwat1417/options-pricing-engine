#include "request/HedgePathRequest.hpp"

#include "request/HedgeRequestFields.hpp"
#include "request/JsonReader.hpp"

#include <stdexcept>
#include <string>

using nlohmann::json;
using std::invalid_argument;
using std::to_string;
using std::vector;

namespace ope::server {

namespace {

vector<double> parsePrices(const json& body) {
    const auto it = body.find("prices");
    if (it == body.end()) {
        return {};
    }
    const auto max_points = static_cast<size_t>(HedgePathRequest::kMaxRebalances) + 1;
    if (!it->is_array() || it->size() < 2 || it->size() > max_points) {
        throw invalid_argument("prices must be an array of 2 to " + to_string(max_points) +
                               " numbers");
    }
    vector<double> prices;
    for (const json& p : *it) {
        if (!p.is_number()) {
            throw invalid_argument("prices must contain only numbers");
        }
        prices.push_back(p.get<double>());
    }
    return prices;
}

}  // namespace

HedgePathRequest HedgePathRequest::fromJson(const json& body) {
    HedgePathRequest request{};
    request.option = hedge_fields::callOption(body);
    request.quantity = hedge_fields::quantity(body);
    request.premium = json_reader::number(body, "premium");
    if (request.premium < 0.0) {
        throw invalid_argument("premium must be >= 0");
    }
    request.prices = parsePrices(body);
    if (request.prices.empty()) {
        request.drift = hedge_fields::drift(body);
        request.rebalances =
            static_cast<int>(json_reader::integer(body, "rebalances", 1, kMaxRebalances));
        request.seed = hedge_fields::seed(body);
    } else {
        request.rebalances = static_cast<int>(request.prices.size()) - 1;
    }
    return request;
}

}  // namespace ope::server
