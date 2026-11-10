#include "request/AsianRequests.hpp"

#include "request/JsonReader.hpp"
#include "request/McConvergenceRequest.hpp"
#include "request/McPriceRequest.hpp"

#include <stdexcept>
#include <string>

using nlohmann::json;
using std::invalid_argument;
using std::to_string;
using std::vector;

namespace ope::server {

namespace {

int parseFixings(const json& body) {
    return static_cast<int>(json_reader::integer(body, "fixings", 1, AsianLimits::kMaxFixings));
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
    request.option = mcOption(body);
    request.fixings = parseFixings(body);
    request.paths = static_cast<int>(
        json_reader::integer(body, "paths", 4, McPriceRequest::kMaxPaths));
    request.seed = mcSeed(body);
    requireWithinWork(request.paths, request.fixings);
    return request;
}

AsianConvergenceRequest AsianConvergenceRequest::fromJson(const json& body) {
    AsianConvergenceRequest request{};
    request.option = mcOption(body);
    request.fixings = parseFixings(body);
    request.seed = mcSeed(body);
    request.paths = mcPathCounts(body);

    if (request.paths.empty()) {
        long long total = 0;
        for (int n : defaultPathCounts()) {
            total += n;
            if (total * request.fixings > AsianLimits::kMaxWork) break;
            request.paths.push_back(n);
        }
    }

    long long total = 0;
    for (int n : request.paths) total += n;
    requireWithinWork(total, request.fixings);
    return request;
}

}  // namespace ope::server
