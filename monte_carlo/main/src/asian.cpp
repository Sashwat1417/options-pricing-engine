#include "ope/asian.hpp"

#include "estimators.hpp"
#include "ope/normal.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using std::exp;
using std::invalid_argument;
using std::log;
using std::sqrt;
using std::string;
using std::uint64_t;
using std::vector;

namespace ope {

namespace {

void require_asian(const OptionInputs& option, int fixings, const string& caller) {
    validate(option);
    if (option.time_to_expiry <= 0.0) {
        throw invalid_argument(caller + " needs time_to_expiry > 0");
    }
    if (fixings < 1) {
        throw invalid_argument(caller + " needs fixings >= 1");
    }
}

// e^(−rT)·E[max(A − K, 0)] (or the put) for a lognormal A with mean `forward` and
// var(ln A) = `variance`: Black's model (Hull 18.7, 18.8).
double black(OptionType type, double forward, double variance, double strike,
             double discount) {
    if (variance <= 0.0) return discount * detail::payoff(type, forward, strike);
    const double sd = sqrt(variance);
    const double d1 = (log(forward / strike) + 0.5 * variance) / sd;
    const double d2 = d1 - sd;
    return type == OptionType::Call
               ? discount * (forward * norm_cdf(d1) - strike * norm_cdf(d2))
               : discount * (strike * norm_cdf(-d2) - forward * norm_cdf(-d1));
}

// One path of `fixings` lognormal steps. Value: the arithmetic average payoff.
// Control: the geometric average payoff.
class Asian {
public:
    Asian(const OptionInputs& option, int fixings)
        : option_(option),
          fixings_(fixings),
          log_spot_(log(option.spot)),
          drift_((option.rate - 0.5 * option.volatility * option.volatility) *
                 option.time_to_expiry / fixings),
          shock_scale_(option.volatility * sqrt(option.time_to_expiry / fixings)),
          discount_(exp(-option.rate * option.time_to_expiry)) {}

    int shocks() const { return fixings_; }

    detail::Sample evaluate(const vector<double>& epsilon, double sign) const {
        double log_price = log_spot_;
        double sum = 0.0;
        double sum_of_logs = 0.0;
        for (double e : epsilon) {
            log_price += drift_ + shock_scale_ * sign * e;
            sum += exp(log_price);
            sum_of_logs += log_price;
        }
        return {discount_ * detail::payoff(option_.type, sum / fixings_, option_.strike),
                discount_ * detail::payoff(option_.type, exp(sum_of_logs / fixings_),
                                           option_.strike)};
    }

private:
    const OptionInputs& option_;
    int fixings_;
    double log_spot_;
    double drift_;
    double shock_scale_;
    double discount_;
};

}  // namespace

// ln G = (1/m)·Σ ln S(tᵢ), with tᵢ = i·Δt:
//   mean     ln S + (r − σ²/2)·Δt·(m + 1)/2
//   variance σ²·(1/m²)·Σᵢ Σⱼ min(tᵢ, tⱼ) = σ²·Δt·(m + 1)(2m + 1)/(6m)
double geometric_asian_price(const OptionInputs& option, int fixings) {
    require_asian(option, fixings, "geometric_asian_price");
    const double m = fixings;
    const double dt = option.time_to_expiry / m;
    const double sigma2 = option.volatility * option.volatility;
    const double mean = log(option.spot) + (option.rate - 0.5 * sigma2) * dt * (m + 1) / 2;
    const double variance = sigma2 * dt * (m + 1) * (2 * m + 1) / (6 * m);
    return black(option.type, exp(mean + 0.5 * variance), variance, option.strike,
                 exp(-option.rate * option.time_to_expiry));
}

// Fᵢ = S·e^(r·tᵢ), M₁ = (1/m)·ΣFᵢ, M₂ = (1/m²)·Σᵢ Σⱼ FᵢFⱼ·e^(σ²·min(tᵢ, tⱼ)),
// then F₀ = M₁ and σ²T = ln(M₂ / M₁²).
double asian_lognormal_price(const OptionInputs& option, int fixings) {
    require_asian(option, fixings, "asian_lognormal_price");
    const double dt = option.time_to_expiry / fixings;
    const double sigma2 = option.volatility * option.volatility;
    double m1 = 0.0;
    double m2 = 0.0;
    double earlier = 0.0;
    for (int i = 1; i <= fixings; ++i) {
        const double t = i * dt;
        const double forward = option.spot * exp(option.rate * t);
        const double weighted = forward * exp(sigma2 * t);
        m1 += forward;
        m2 += forward * weighted + 2.0 * forward * earlier;
        earlier += weighted;
    }
    m1 /= fixings;
    m2 /= static_cast<double>(fixings) * fixings;
    return black(option.type, m1, log(m2 / (m1 * m1)), option.strike,
                 exp(-option.rate * option.time_to_expiry));
}

McResult mc_asian_price(const OptionInputs& option, int fixings, int paths, uint64_t seed,
                        VarianceReduction method) {
    require_asian(option, fixings, "mc_asian_price");
    detail::require_paths(paths, method, "mc_asian_price");
    return detail::estimate(Asian(option, fixings), geometric_asian_price(option, fixings),
                            paths, seed, method);
}

}  // namespace ope
