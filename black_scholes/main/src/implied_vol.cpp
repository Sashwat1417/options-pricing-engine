#include "ope/implied_vol.hpp"

#include "ope/black_scholes.hpp"

#include <cmath>
#include <sstream>
#include <stdexcept>

using std::invalid_argument;
using std::isfinite;
using std::ostringstream;

namespace ope {

namespace {

constexpr double kMaxVol = 5.0;
constexpr double kVolTolerance = 1e-10;
constexpr int kMaxIterations = 100;

}  // namespace

double implied_vol(const OptionInputs& inputs, double market_price) {
    OptionInputs trial = inputs;
    trial.volatility = 0.0;
    validate(trial);

    if (trial.time_to_expiry == 0.0) {
        throw invalid_argument("implied_vol needs time_to_expiry > 0");
    }

    const double lowest = price(trial);
    trial.volatility = kMaxVol;
    const double highest = price(trial);

    if (!isfinite(market_price) || market_price < lowest || market_price > highest) {
        ostringstream msg;
        msg << "implied_vol: market_price " << market_price << " is outside [" << lowest << ", "
            << highest << "], the prices reachable with volatility in [0, " << kMaxVol << "]";
        throw invalid_argument(msg.str());
    }

    double lo = 0.0;
    double hi = kMaxVol;
    for (int i = 0; i < kMaxIterations && hi - lo > kVolTolerance; ++i) {
        trial.volatility = 0.5 * (lo + hi);
        if (price(trial) < market_price) {
            lo = trial.volatility;
        } else {
            hi = trial.volatility;
        }
    }
    return 0.5 * (lo + hi);
}

}  // namespace ope
