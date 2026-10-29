#include "ope/greeks.hpp"

#include "ope/black_scholes.hpp"
#include "ope/normal.hpp"

#include <cmath>

using std::exp;
using std::sqrt;

namespace ope {

Greeks greeks(const OptionInputs& in) {
    validate(in);

    double call_delta = 0.0;
    if (in.volatility * sqrt(in.time_to_expiry) == 0.0) {
        call_delta = in.spot > in.strike * exp(-in.rate * in.time_to_expiry) ? 1.0 : 0.0;
    } else {
        call_delta = norm_cdf(d1d2(in).d1);
    }

    const double delta = in.type == OptionType::Call ? call_delta : call_delta - 1.0;
    return {delta};
}

}  // namespace ope
