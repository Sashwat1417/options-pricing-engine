#include "request/AsianRequests.hpp"

#include "request/JsonReader.hpp"
#include "request/McConvergenceRequest.hpp"
#include "request/McPriceRequest.hpp"

#include <stdexcept>
#include <string>

using nlohmann::json;
using std::invalid_argument;
using std::to_string;

namespace ope::server {

namespace {

AsianInputs parseAsian(const json& body, const OptionInputs& option) {
    const int fixings = static_cast<int>(
        json_reader::integer(body, "fixings", 1, AsianLimits::kMaxFixings));
    const double yield = body.contains("yield") ? json_reader::number(body, "yield") : 0.0;
    return {option, fixings, yield};
}

void requireWithinWork(long long paths, int fixings) {
    if (paths * fixings > AsianLimits::kMaxWork) {
        throw invalid_argument("paths × fixings must be <= " + to_string(AsianLimits::kMaxWork) +
                               "; use fewer paths or fixings");
    }
}

}  // namespace

AsianPriceRequest AsianPriceRequest::fromJson(const json& body) {
    AsianPriceRequest request{};
    request.asian = parseAsian(body, mcOption(body));
    request.paths = static_cast<int>(
        json_reader::integer(body, "paths", 4, McPriceRequest::kMaxPaths));
    request.seed = mcSeed(body);
    requireWithinWork(request.paths, request.asian.fixings);
    return request;
}

AsianConvergenceRequest AsianConvergenceRequest::fromJson(const json& body) {
    AsianConvergenceRequest request{};
    request.asian = parseAsian(body, mcOption(body));
    request.seed = mcSeed(body);
    request.paths = mcPathCounts(body);

    if (request.paths.empty()) {
        long long total = 0;
        for (int n : defaultPathCounts()) {
            total += n;
            if (total * request.asian.fixings > AsianLimits::kMaxWork) break;
            request.paths.push_back(n);
        }
    }

    long long total = 0;
    for (int n : request.paths) total += n;
    requireWithinWork(total, request.asian.fixings);
    return request;
}

AsianHedgeRequest AsianHedgeRequest::fromJson(const json& body) {
    OptionInputs option = json_reader::marketInputs(body, OptionType::Put);
    option.volatility = json_reader::number(body, "vol");

    AsianHedgeRequest request{};
    request.hedge.put = parseAsian(body, option);
    request.hedge.notional = json_reader::number(body, "notional");
    request.hedge.premium = json_reader::number(body, "premium");
    request.paths = static_cast<int>(json_reader::integer(body, "paths", 100, kMaxPaths));
    request.seed = mcSeed(body);
    requireWithinWork(request.paths, request.hedge.put.fixings);
    return request;
}

}  // namespace ope::server
