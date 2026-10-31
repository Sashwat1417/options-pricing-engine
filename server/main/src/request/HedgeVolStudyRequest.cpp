#include "request/HedgeVolStudyRequest.hpp"

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

vector<double> parseRealVols(const json& body) {
    const auto it = body.find("real_vols");
    if (it == body.end()) {
        return {0.10, 0.15, 0.20, 0.25, 0.30};
    }
    if (!it->is_array() || it->empty() || it->size() > 10) {
        throw invalid_argument("real_vols must be an array of 1 to 10 numbers");
    }
    vector<double> vols;
    for (const json& v : *it) {
        if (!v.is_number() || v.get<double>() < 0.0 || v.get<double>() > 5.0) {
            throw invalid_argument("real_vols must contain numbers in [0, 5]");
        }
        vols.push_back(v.get<double>());
    }
    return vols;
}

}  // namespace

HedgeVolStudyRequest HedgeVolStudyRequest::fromJson(const json& body) {
    HedgeVolStudyRequest request{};
    request.option = hedge_fields::callOption(body);
    request.quantity = hedge_fields::quantity(body);
    request.costRate = hedge_fields::costRate(body);
    request.premium = json_reader::number(body, "premium");
    if (request.premium < 0.0) {
        throw invalid_argument("premium must be >= 0");
    }
    request.drift = hedge_fields::realWorld(body, request.option.volatility).drift;
    request.rebalances = static_cast<int>(
        json_reader::integer(body, "rebalances", 1, HedgePathRequest::kMaxRebalances));
    request.paths = static_cast<int>(
        json_reader::integer(body, "paths", 2, HedgeFrequencyRequest::kMaxPaths));
    request.seed = hedge_fields::seed(body);
    request.realVols = parseRealVols(body);

    const long long work = static_cast<long long>(request.paths) * request.rebalances *
                           static_cast<long long>(request.realVols.size());
    if (work > kMaxWork) {
        throw invalid_argument("paths × rebalances × real_vols must be <= " +
                               to_string(kMaxWork) + "; use fewer paths or rebalances");
    }
    return request;
}

}  // namespace ope::server
