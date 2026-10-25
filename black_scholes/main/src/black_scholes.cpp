#include "ope/black_scholes.hpp"

#include "ope/normal.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

using std::exp;
using std::invalid_argument;
using std::log;
using std::max;
using std::sqrt;

namespace ope {

D1D2 d1d2(const OptionInputs& in) {
    validate(in);

    const double vol_sqrt_t = in.volatility * sqrt(in.time_to_expiry);
    if (vol_sqrt_t == 0.0) {
        throw invalid_argument("d1d2 is undefined when volatility * sqrt(time_to_expiry) = 0");
    }

    const double drift = (in.rate + 0.5 * in.volatility * in.volatility) * in.time_to_expiry;
    const double d1 = (log(in.spot / in.strike) + drift) / vol_sqrt_t;
    return {d1, d1 - vol_sqrt_t};
}

double price(const OptionInputs& in) {
    validate(in);

    const double discounted_strike = in.strike * exp(-in.rate * in.time_to_expiry);
    const bool is_call = in.type == OptionType::Call;

    if (in.volatility * sqrt(in.time_to_expiry) == 0.0) {
        return is_call ? max(in.spot - discounted_strike, 0.0)
                       : max(discounted_strike - in.spot, 0.0);
    }

    const auto [d1, d2] = d1d2(in);
    return is_call ? in.spot * norm_cdf(d1) - discounted_strike * norm_cdf(d2)
                   : discounted_strike * norm_cdf(-d2) - in.spot * norm_cdf(-d1);
}

}  // namespace ope
