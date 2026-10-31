#include "response/McResponses.hpp"

using nlohmann::json;

namespace ope::server {

namespace {

json estimate(const McResult& r) {
    return {{"price", r.price},
            {"std_error", r.std_error},
            {"ci_low", r.price - kZ95 * r.std_error},
            {"ci_high", r.price + kZ95 * r.std_error}};
}

}  // namespace

json McPriceResponse::toJson() const {
    json out = estimate(result);
    out["bs_price"] = bsPrice;
    out["paths"] = paths;
    out["seed"] = seed;
    return out;
}

json McConvergenceResponse::toJson() const {
    json out_rows = json::array();
    for (const auto& r : rows) {
        json row = estimate(r.result);
        row["paths"] = r.paths;
        out_rows.push_back(row);
    }
    return {{"bs_price", bsPrice}, {"seed", seed}, {"rows", out_rows}};
}

}  // namespace ope::server
