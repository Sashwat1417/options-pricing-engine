#pragma once

#include "ope/asian.hpp"
#include "ope/mc_pricer.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace ope::server {

// 95% confidence interval: price ± 1.96 standard errors.
inline constexpr double kZ95 = 1.96;

// One estimate per method, all from the same paths and seed.
struct McEstimates {
    McResult plain;
    McResult antithetic;
    McResult controlVariate;
};

// {"bs_price", "paths", "seed",
//  "estimates": {"plain" | "antithetic" | "control_variate":
//                {price, std_error, ci_low, ci_high, speedup}}}
// speedup = (plain SE / method SE)²: how many plain paths one path of the method is worth.
struct McPriceResponse {
    McEstimates estimates;
    double bsPrice;
    int paths;
    std::uint64_t seed;

    nlohmann::json toJson() const;
};

struct McConvergenceRow {
    int paths;
    McEstimates estimates;
};

// {"bs_price", "seed", "rows": [{paths, estimates: {...}}]}
struct McConvergenceResponse {
    double bsPrice;
    std::uint64_t seed;
    std::vector<McConvergenceRow> rows;

    nlohmann::json toJson() const;
};

// The prices an Asian estimate is compared with: Hull's lognormal approximation (26.3, 26.4)
// and the exact geometric average price, the control variate's known mean.
struct AsianReferences {
    double lognormalPrice;
    double geometricPrice;
};

// {"lognormal_price", "geometric_price", "fixings", "paths", "seed", "estimates": {...}}
struct AsianPriceResponse {
    McEstimates estimates;
    AsianReferences references;
    int fixings;
    int paths;
    std::uint64_t seed;

    nlohmann::json toJson() const;
};

// {"lognormal_price", "geometric_price", "fixings", "seed", "rows": [{paths, estimates}]}
struct AsianConvergenceResponse {
    AsianReferences references;
    int fixings;
    std::uint64_t seed;
    std::vector<McConvergenceRow> rows;

    nlohmann::json toJson() const;
};

// The treasurer's year, hedged and unhedged, in domestic currency at T:
// {"fair_premium", "fair_premium_std_error", "premium", "notional", "premium_cost", "floor",
//  "paths", "seed", "probability_put_pays",
//  "mean": {unhedged, hedged, put_pnl}, "worst": {unhedged, hedged},
//  "scenarios": [{percentile, average_rate, unhedged, put_payoff, put_pnl, hedged}],
//  "histogram": {edges, unhedged, hedged}}
// Scenarios are the 5th … 95th percentiles of the average rate.
struct AsianHedgeResponse {
    McResult fairPremium;
    double premium;
    double notional;
    double premiumCost;
    double floor;
    std::vector<HedgeOutcome> outcomes;  // sorted by average rate
    std::uint64_t seed;

    nlohmann::json toJson() const;
};

}  // namespace ope::server
