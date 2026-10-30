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
    const double strike_rate_exposure = discounted_strike * in.time_to_expiry;  // K·T·e^(−rT)
    Greeks g{};

    if (vol_sqrt_t == 0.0) {
        const bool call_in_the_money = in.spot > discounted_strike;
        const bool put_in_the_money = discounted_strike > in.spot;
        g.delta = is_call ? (call_in_the_money ? 1.0 : 0.0) : (call_in_the_money ? 0.0 : -1.0);
        if (is_call && call_in_the_money) {
            g.theta = -in.rate * discounted_strike;
            g.rho = strike_rate_exposure;
        } else if (!is_call && put_in_the_money) {
            g.theta = in.rate * discounted_strike;
            g.rho = -strike_rate_exposure;
        }
        return g;
    }

    const auto [d1, d2] = d1d2(in);
    const double time_decay =
        -in.spot * norm_pdf(d1) * in.volatility / (2.0 * sqrt(in.time_to_expiry));

    g.delta = is_call ? norm_cdf(d1) : norm_cdf(d1) - 1.0;
    g.theta = is_call ? time_decay - in.rate * discounted_strike * norm_cdf(d2)
                      : time_decay + in.rate * discounted_strike * norm_cdf(-d2);
    g.gamma = norm_pdf(d1) / (in.spot * vol_sqrt_t);
    g.vega = in.spot * sqrt(in.time_to_expiry) * norm_pdf(d1);
    g.rho = is_call ? strike_rate_exposure * norm_cdf(d2) : -strike_rate_exposure * norm_cdf(-d2);
    return g;
}

}  // namespace ope
