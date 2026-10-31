#include "response/McResponses.hpp"

using nlohmann::json;

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

}  // namespace

json McPriceResponse::toJson() const {
    return {{"bs_price", bsPrice},
            {"paths", paths},
            {"seed", seed},
            {"estimates", byMethod(estimates)}};
}

json McConvergenceResponse::toJson() const {
    json out_rows = json::array();
    for (const auto& r : rows) {
        out_rows.push_back({{"paths", r.paths}, {"estimates", byMethod(r.estimates)}});
    }
    return {{"bs_price", bsPrice}, {"seed", seed}, {"rows", out_rows}};
}

}  // namespace ope::server
