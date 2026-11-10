#pragma once

#include "ope/mc_pricer.hpp"
#include "ope/option.hpp"

#include <cstdint>
#include <vector>

// Average price options (Hull 26.13): a call pays max(S_ave − K, 0), a put max(K − S_ave, 0).
// Every function throws std::invalid_argument on invalid inputs, T <= 0 or fixings < 1.

namespace ope {

struct AsianInputs {
    OptionInputs option;
    int fixings;         // m prices averaged, observed at T/m, 2T/m, …, T
    double yield = 0.0;  // q: dividend yield, or the foreign rate for a currency (Hull 17)
};

// Exact price when S_ave is the geometric average: ln S_ave is normal, so S_ave is lognormal.
[[nodiscard]] double geometric_asian_price(const AsianInputs& asian);

// Hull's approximation for the arithmetic average (26.3, 26.4): treat S_ave as lognormal with
// the same first two moments, then use Black's model.
[[nodiscard]] double asian_lognormal_price(const AsianInputs& asian);

// Monte Carlo price of the arithmetic average option. The control variate is the geometric
// average option, whose exact price is known.
[[nodiscard]] McResult mc_asian_price(const AsianInputs& asian, int paths, std::uint64_t seed,
                                      VarianceReduction method = VarianceReduction::None);

// Hull's treasurer: `notional` units of foreign currency arrive in equal parts at the fixings
// and are converted at each fixing's rate, so the domestic total is notional × S_ave. An average
// rate put, bought today at `premium` per unit, floors that total at notional × K.
struct AverageRateHedge {
    AsianInputs put;  // option.type must be Put
    double notional;
    double premium;
};

// One simulated year, in domestic currency at T. The premium is carried to T at r.
struct HedgeOutcome {
    double average_rate;  // S_ave
    double unhedged;      // notional × S_ave
    double put_payoff;    // notional × max(K − S_ave, 0)
    double put_pnl;       // put_payoff − premium cost
    double hedged;        // unhedged + put_pnl, never below notional × K − premium cost
};

// Outcomes of `paths` simulated years under the risk-neutral drift r − q, sorted by S_ave.
[[nodiscard]] std::vector<HedgeOutcome> simulate_average_rate_hedge(
    const AverageRateHedge& hedge, int paths, std::uint64_t seed);

}  // namespace ope
