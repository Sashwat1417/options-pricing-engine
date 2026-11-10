#include "ope/asian.hpp"

#include "estimators.hpp"
#include "ope/normal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using std::exp;
using std::invalid_argument;
using std::isfinite;
using std::log;
using std::max;
using std::sort;
using std::sqrt;
using std::string;
using std::uint64_t;
using std::vector;

namespace ope {

namespace {

void require_asian(const AsianInputs& asian, const string& caller) {
    validate(asian.option);
    if (asian.option.time_to_expiry <= 0.0) {
        throw invalid_argument(caller + " needs time_to_expiry > 0");
    }
    if (asian.fixings < 1) {
        throw invalid_argument(caller + " needs fixings >= 1");
    }
    if (!isfinite(asian.yield)) {
        throw invalid_argument(caller + " needs a finite yield");
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

struct Averages {
    double arithmetic;
    double geometric;
};

// Walks one path through the fixings with lognormal steps at the drift r − q.
class AsianPath {
public:
    explicit AsianPath(const AsianInputs& asian)
        : fixings_(asian.fixings),
          log_spot_(log(asian.option.spot)),
          drift_((asian.option.rate - asian.yield -
                  0.5 * asian.option.volatility * asian.option.volatility) *
                 asian.option.time_to_expiry / asian.fixings),
          shock_scale_(asian.option.volatility *
                       sqrt(asian.option.time_to_expiry / asian.fixings)) {}

    int shocks() const { return fixings_; }

    Averages averages(const vector<double>& epsilon, double sign) const {
        double log_price = log_spot_;
        double sum = 0.0;
        double sum_of_logs = 0.0;
        for (double e : epsilon) {
            log_price += drift_ + shock_scale_ * sign * e;
            sum += exp(log_price);
            sum_of_logs += log_price;
        }
        return {sum / fixings_, exp(sum_of_logs / fixings_)};
    }

private:
    int fixings_;
    double log_spot_;
    double drift_;
    double shock_scale_;
};

// Value: the arithmetic average payoff. Control: the geometric average payoff.
class Asian {
public:
    explicit Asian(const AsianInputs& asian)
        : path_(asian),
          type_(asian.option.type),
          strike_(asian.option.strike),
          discount_(exp(-asian.option.rate * asian.option.time_to_expiry)) {}

    int shocks() const { return path_.shocks(); }

    detail::Sample evaluate(const vector<double>& epsilon, double sign) const {
        const Averages a = path_.averages(epsilon, sign);
        return {discount_ * detail::payoff(type_, a.arithmetic, strike_),
                discount_ * detail::payoff(type_, a.geometric, strike_)};
    }

private:
    AsianPath path_;
    OptionType type_;
    double strike_;
    double discount_;
};

}  // namespace

// ln G = (1/m)·Σ ln S(tᵢ), with tᵢ = i·Δt:
//   mean     ln S + (r − q − σ²/2)·Δt·(m + 1)/2
//   variance σ²·(1/m²)·Σᵢ Σⱼ min(tᵢ, tⱼ) = σ²·Δt·(m + 1)(2m + 1)/(6m)
double geometric_asian_price(const AsianInputs& asian) {
    require_asian(asian, "geometric_asian_price");
    const OptionInputs& o = asian.option;
    const double m = asian.fixings;
    const double dt = o.time_to_expiry / m;
    const double sigma2 = o.volatility * o.volatility;
    const double mean =
        log(o.spot) + (o.rate - asian.yield - 0.5 * sigma2) * dt * (m + 1) / 2;
    const double variance = sigma2 * dt * (m + 1) * (2 * m + 1) / (6 * m);
    return black(o.type, exp(mean + 0.5 * variance), variance, o.strike,
                 exp(-o.rate * o.time_to_expiry));
}

// Fᵢ = S·e^((r − q)·tᵢ), M₁ = (1/m)·ΣFᵢ, M₂ = (1/m²)·Σᵢ Σⱼ FᵢFⱼ·e^(σ²·min(tᵢ, tⱼ)),
// then F₀ = M₁ and σ²T = ln(M₂ / M₁²).
double asian_lognormal_price(const AsianInputs& asian) {
    require_asian(asian, "asian_lognormal_price");
    const OptionInputs& o = asian.option;
    const double dt = o.time_to_expiry / asian.fixings;
    const double sigma2 = o.volatility * o.volatility;
    double m1 = 0.0;
    double m2 = 0.0;
    double earlier = 0.0;
    for (int i = 1; i <= asian.fixings; ++i) {
        const double t = i * dt;
        const double forward = o.spot * exp((o.rate - asian.yield) * t);
        const double weighted = forward * exp(sigma2 * t);
        m1 += forward;
        m2 += forward * weighted + 2.0 * forward * earlier;
        earlier += weighted;
    }
    m1 /= asian.fixings;
    m2 /= static_cast<double>(asian.fixings) * asian.fixings;
    return black(o.type, m1, log(m2 / (m1 * m1)), o.strike, exp(-o.rate * o.time_to_expiry));
}

McResult mc_asian_price(const AsianInputs& asian, int paths, uint64_t seed,
                        VarianceReduction method) {
    require_asian(asian, "mc_asian_price");
    detail::require_paths(paths, method, "mc_asian_price");
    return detail::estimate(Asian(asian), geometric_asian_price(asian), paths, seed, method);
}

vector<HedgeOutcome> simulate_average_rate_hedge(const AverageRateHedge& hedge, int paths,
                                                 uint64_t seed) {
    require_asian(hedge.put, "simulate_average_rate_hedge");
    if (hedge.put.option.type != OptionType::Put) {
        throw invalid_argument("simulate_average_rate_hedge needs a put");
    }
    if (!isfinite(hedge.notional) || hedge.notional <= 0.0) {
        throw invalid_argument("simulate_average_rate_hedge needs notional > 0");
    }
    if (!isfinite(hedge.premium) || hedge.premium < 0.0) {
        throw invalid_argument("simulate_average_rate_hedge needs premium >= 0");
    }
    if (paths < 1) {
        throw invalid_argument("simulate_average_rate_hedge needs paths >= 1");
    }

    const OptionInputs& o = hedge.put.option;
    const AsianPath path(hedge.put);
    const double premium_cost =
        hedge.notional * hedge.premium * exp(o.rate * o.time_to_expiry);
    detail::Shocks shocks(path.shocks(), seed);
    vector<HedgeOutcome> outcomes;
    outcomes.reserve(static_cast<size_t>(paths));
    for (int i = 0; i < paths; ++i) {
        const double average = path.averages(shocks.next(), 1.0).arithmetic;
        const double unhedged = hedge.notional * average;
        const double payoff = hedge.notional * max(o.strike - average, 0.0);
        outcomes.push_back(
            {average, unhedged, payoff, payoff - premium_cost, unhedged + payoff - premium_cost});
    }
    sort(outcomes.begin(), outcomes.end(), [](const HedgeOutcome& a, const HedgeOutcome& b) {
        return a.average_rate < b.average_rate;
    });
    return outcomes;
}

}  // namespace ope
