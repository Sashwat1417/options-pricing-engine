#include "request/HedgeFrequencyRequest.hpp"

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

vector<int> parseRebalances(const json& body) {
    const auto it = body.find("rebalances");
    if (it == body.end()) {
        return {4, 5, 10, 20, 40, 80};
    }
    if (!it->is_array() || it->empty() || it->size() > 10) {
        throw invalid_argument("rebalances must be an array of 1 to 10 integers");
    }
    vector<int> counts;
    for (const json& n : *it) {
        if (!n.is_number_integer() || n.get<long long>() < 1 ||
            n.get<long long>() > HedgePathRequest::kMaxRebalances) {
            throw invalid_argument("rebalances must contain integers in [1, " +
                                   to_string(HedgePathRequest::kMaxRebalances) + "]");
        }
        counts.push_back(n.get<int>());
    }
    return counts;
}

}  // namespace

HedgeFrequencyRequest HedgeFrequencyRequest::fromJson(const json& body) {
    HedgeFrequencyRequest request{};
    request.option = hedge_fields::callOption(body);
    request.quantity = hedge_fields::quantity(body);
    request.world = hedge_fields::realWorld(body, request.option.volatility);
    request.paths = static_cast<int>(json_reader::integer(body, "paths", 2, kMaxPaths));
    request.seed = hedge_fields::seed(body);
    request.rebalances = parseRebalances(body);

    long long work = 0;
    for (int n : request.rebalances) work += static_cast<long long>(request.paths) * n;
    if (work > kMaxWork) {
        throw invalid_argument("paths × total rebalances must be <= " + to_string(kMaxWork) +
                               "; use fewer paths or rebalances");
    }
    return request;
}

}  // namespace ope::server
