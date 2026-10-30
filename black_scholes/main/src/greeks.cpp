#include "ope/greeks.hpp"

#include "ope/black_scholes.hpp"
#include "ope/normal.hpp"

#include <cmath>

using std::exp;
using std::sqrt;

namespace ope {

Greeks greeks(const OptionInputs& in) {
    validate(in);

    const bool is_call = in.type == OptionType::Call;
    const double discounted_strike = in.strike * exp(-in.rate * in.time_to_expiry);
    const double vol_sqrt_t = in.volatility * sqrt(in.time_to_expiry);

    if (vol_sqrt_t == 0.0) {
        const bool call_in_the_money = in.spot > discounted_strike;
        const bool put_in_the_money = discounted_strike > in.spot;
        const double call_delta = call_in_the_money ? 1.0 : 0.0;
        if (is_call) {
            return {call_delta, call_in_the_money ? -in.rate * discounted_strike : 0.0, 0.0, 0.0};
        }
        return {call_delta - 1.0, put_in_the_money ? in.rate * discounted_strike : 0.0, 0.0, 0.0};
    }

    const auto [d1, d2] = d1d2(in);
    const double call_delta = norm_cdf(d1);
    const double gamma = norm_pdf(d1) / (in.spot * vol_sqrt_t);
    const double vega = in.spot * sqrt(in.time_to_expiry) * norm_pdf(d1);
    const double time_decay =
        -in.spot * norm_pdf(d1) * in.volatility / (2.0 * sqrt(in.time_to_expiry));

    if (is_call) {
        return {call_delta, time_decay - in.rate * discounted_strike * norm_cdf(d2), gamma, vega};
    }
    return {call_delta - 1.0, time_decay + in.rate * discounted_strike * norm_cdf(-d2), gamma, vega};
}

}  // namespace ope
