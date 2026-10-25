#include "ope/black_scholes.hpp"

#include <cmath>
#include <stdexcept>

using std::invalid_argument;
using std::log;
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

}  // namespace ope
