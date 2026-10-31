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

vector<int> parsePaths(const json& body) {
    const auto it = body.find("paths");
    if (it == body.end()) {
        return {1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000};
    }
    if (!it->is_array() || it->empty() || it->size() > 20) {
        throw invalid_argument("paths must be an array of 1 to 20 integers");
    }
    vector<int> counts;
    for (const json& n : *it) {
        if (!n.is_number_integer() || n.get<long long>() < 4 ||
            n.get<long long>() > McPriceRequest::kMaxPaths) {
            throw invalid_argument("paths must contain integers in [4, " +
                                   to_string(McPriceRequest::kMaxPaths) + "]");
        }
        counts.push_back(n.get<int>());
    }
    return counts;
}

}  // namespace

McConvergenceRequest McConvergenceRequest::fromJson(const json& body) {
    McConvergenceRequest request{};
    request.option = mcOption(body);
    request.seed = mcSeed(body);
    request.paths = parsePaths(body);

    long long work = 0;
    for (int n : request.paths) work += n;
    if (work > kMaxWork) {
        throw invalid_argument("total paths must be <= " + to_string(kMaxWork) +
                               "; use fewer or smaller path counts");
    }
    return request;
}

}  // namespace ope::server
