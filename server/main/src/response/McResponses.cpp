#include "response/McResponses.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

using nlohmann::json;
using std::max;
using std::min;
using std::size_t;
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

constexpr int kHistogramBins = 30;

json histogram(const vector<HedgeOutcome>& outcomes) {
    double lo = outcomes.front().unhedged;
    double hi = outcomes.front().unhedged;
    for (const auto& o : outcomes) {
        lo = min({lo, o.unhedged, o.hedged});
        hi = max({hi, o.unhedged, o.hedged});
    }
    if (hi <= lo) hi = lo + 1.0;
    const double width = (hi - lo) / kHistogramBins;
    const auto bin = [&](double x) {
        return min(static_cast<int>((x - lo) / width), kHistogramBins - 1);
    };

    vector<double> edges;
    for (int i = 0; i <= kHistogramBins; ++i) edges.push_back(lo + i * width);
    vector<int> unhedged(kHistogramBins, 0);
    vector<int> hedged(kHistogramBins, 0);
    for (const auto& o : outcomes) {
        ++unhedged[static_cast<size_t>(bin(o.unhedged))];
        ++hedged[static_cast<size_t>(bin(o.hedged))];
    }
    return {{"edges", edges}, {"unhedged", unhedged}, {"hedged", hedged}};
}

json scenarios(const vector<HedgeOutcome>& outcomes) {
    json out = json::array();
    for (int pct : {5, 25, 50, 75, 95}) {
        const auto& o = outcomes[static_cast<size_t>(pct) * (outcomes.size() - 1) / 100];
        out.push_back({{"percentile", pct},
                       {"average_rate", o.average_rate},
                       {"unhedged", o.unhedged},
                       {"put_payoff", o.put_payoff},
                       {"put_pnl", o.put_pnl},
                       {"hedged", o.hedged}});
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

json AsianHedgeResponse::toJson() const {
    double unhedged = 0.0;
    double hedged = 0.0;
    double pnl = 0.0;
    double worstHedged = outcomes.front().hedged;
    int pays = 0;
    for (const auto& o : outcomes) {
        unhedged += o.unhedged;
        hedged += o.hedged;
        pnl += o.put_pnl;
        worstHedged = min(worstHedged, o.hedged);
        if (o.put_payoff > 0.0) ++pays;
    }
    const auto n = static_cast<double>(outcomes.size());
    return {{"fair_premium", fairPremium.price},
            {"fair_premium_std_error", fairPremium.std_error},
            {"premium", premium},
            {"notional", notional},
            {"premium_cost", premiumCost},
            {"floor", floor},
            {"paths", outcomes.size()},
            {"seed", seed},
            {"probability_put_pays", pays / n},
            {"mean", {{"unhedged", unhedged / n}, {"hedged", hedged / n}, {"put_pnl", pnl / n}}},
            {"worst", {{"unhedged", outcomes.front().unhedged}, {"hedged", worstHedged}}},
            {"scenarios", scenarios(outcomes)},
            {"histogram", histogram(outcomes)}};
}

}  // namespace ope::server
