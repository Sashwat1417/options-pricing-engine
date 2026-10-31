#include "ope/mc_pricer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>

using std::exp;
using std::invalid_argument;
using std::max;
using std::mt19937_64;
using std::normal_distribution;
using std::sqrt;
using std::uint64_t;

namespace ope {

namespace {

double payoff(OptionType type, double spot_at_expiry, double strike) {
    return type == OptionType::Call ? max(spot_at_expiry - strike, 0.0)
                                    : max(strike - spot_at_expiry, 0.0);
}

}  // namespace

McResult mc_price(const OptionInputs& option, int paths, uint64_t seed) {
    validate(option);
    if (option.time_to_expiry <= 0.0) {
        throw invalid_argument("mc_price needs time_to_expiry > 0");
    }
    if (paths < 2) {
        throw invalid_argument("mc_price needs paths >= 2");
    }

    const double t = option.time_to_expiry;
    const double sigma = option.volatility;
    const double drift = (option.rate - 0.5 * sigma * sigma) * t;
    const double shock_scale = sigma * sqrt(t);
    const double discount = exp(-option.rate * t);
    mt19937_64 rng(seed);
    normal_distribution<double> epsilon(0.0, 1.0);

    double mean = 0.0;
    double squares = 0.0;
    for (int i = 1; i <= paths; ++i) {
        const double spot_at_expiry = option.spot * exp(drift + shock_scale * epsilon(rng));
        const double value = discount * payoff(option.type, spot_at_expiry, option.strike);
        const double change = value - mean;
        mean += change / i;
        squares += change * (value - mean);
    }
    return {mean, sqrt(squares / (paths - 1) / paths)};
}

}  // namespace ope
