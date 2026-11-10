#include "response/McResponses.hpp"

#include <vector>

using nlohmann::json;
using std::vector;

namespace ope::server {

namespace {

json estimate(const McResult& r, double plainStdError) {
    const double ratio = plainStdError / r.std_error;
    return {{"price", r.price},
            {"std_error", r.std_error},
            {"ci_low", r.price - kZ95 * r.std_error},
            {"ci_high", r.price + kZ95 * r.std_error},
            {"speedup", r.std_error > 0.0 && plainStdError > 0.0 ? json(ratio * ratio)
                                                                  : json(nullptr)}};
}

json byMethod(const McEstimates& e) {
    const double plain = e.plain.std_error;
    return {{"plain", estimate(e.plain, plain)},
            {"antithetic", estimate(e.antithetic, plain)},
            {"control_variate", estimate(e.controlVariate, plain)}};
}

json rowsJson(const vector<McConvergenceRow>& rows) {
    json out = json::array();
    for (const auto& r : rows) {
        out.push_back({{"paths", r.paths}, {"estimates", byMethod(r.estimates)}});
    }
    return out;
}

}  // namespace

json McPriceResponse::toJson() const {
    return {{"bs_price", bsPrice},
            {"paths", paths},
            {"seed", seed},
            {"estimates", byMethod(estimates)}};
}

json McConvergenceResponse::toJson() const {
    return {{"bs_price", bsPrice}, {"seed", seed}, {"rows", rowsJson(rows)}};
}

json AsianPriceResponse::toJson() const {
    return {{"lognormal_price", references.lognormalPrice},
            {"geometric_price", references.geometricPrice},
            {"fixings", fixings},
            {"paths", paths},
            {"seed", seed},
            {"estimates", byMethod(estimates)}};
}

json AsianConvergenceResponse::toJson() const {
    return {{"lognormal_price", references.lognormalPrice},
            {"geometric_price", references.geometricPrice},
            {"fixings", fixings},
            {"seed", seed},
            {"rows", rowsJson(rows)}};
}

}  // namespace ope::server
