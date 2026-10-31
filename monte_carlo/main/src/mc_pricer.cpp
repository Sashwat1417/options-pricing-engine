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

// Discounted payoff and discounted S_T for one draw ε of the terminal price.
class Simulator {
public:
    explicit Simulator(const OptionInputs& option)
        : option_(option),
          drift_((option.rate - 0.5 * option.volatility * option.volatility) *
                 option.time_to_expiry),
          shock_scale_(option.volatility * sqrt(option.time_to_expiry)),
          discount_(exp(-option.rate * option.time_to_expiry)) {}

    double discounted_spot(double epsilon) const {
        return discount_ * option_.spot * exp(drift_ + shock_scale_ * epsilon);
    }

    double value(double epsilon) const {
        return discount_ *
               payoff(option_.type, option_.spot * exp(drift_ + shock_scale_ * epsilon),
                      option_.strike);
    }

private:
    const OptionInputs& option_;
    double drift_;
    double shock_scale_;
    double discount_;
};

// Welford's running mean and variance.
class RunningStats {
public:
    void add(double x) {
        ++n_;
        const double change = x - mean_;
        mean_ += change / n_;
        squares_ += change * (x - mean_);
    }

    McResult result() const { return {mean_, sqrt(squares_ / (n_ - 1) / n_)}; }

private:
    int n_ = 0;
    double mean_ = 0.0;
    double squares_ = 0.0;
};

McResult plain(const Simulator& sim, int paths, mt19937_64& rng) {
    normal_distribution<double> epsilon(0.0, 1.0);
    RunningStats stats;
    for (int i = 0; i < paths; ++i) stats.add(sim.value(epsilon(rng)));
    return stats.result();
}

McResult antithetic(const Simulator& sim, int paths, mt19937_64& rng) {
    normal_distribution<double> epsilon(0.0, 1.0);
    RunningStats pairs;
    for (int i = 0; i < paths / 2; ++i) {
        const double e = epsilon(rng);
        pairs.add(0.5 * (sim.value(e) + sim.value(-e)));
    }
    return pairs.result();
}

// Estimates price = mean(X) − β·(mean(Y) − S) with X the discounted payoff, Y = e^(−rT)·S_T
// and β = cov(X, Y) / var(Y), the regression of X on Y. The error is what Y cannot explain.
McResult control_variate(const Simulator& sim, double spot, int paths, mt19937_64& rng) {
    normal_distribution<double> epsilon(0.0, 1.0);
    double mean_x = 0.0;
    double mean_y = 0.0;
    double sxx = 0.0;
    double syy = 0.0;
    double sxy = 0.0;
    for (int i = 1; i <= paths; ++i) {
        const double e = epsilon(rng);
        const double x = sim.value(e);
        const double y = sim.discounted_spot(e);
        const double dx = x - mean_x;
        const double dy = y - mean_y;
        mean_x += dx / i;
        mean_y += dy / i;
        sxx += dx * (x - mean_x);
        syy += dy * (y - mean_y);
        sxy += dx * (y - mean_y);
    }
    const double beta = syy > 0.0 ? sxy / syy : 0.0;
    const double residual = max(sxx - beta * sxy, 0.0) / (paths - 2);
    return {mean_x - beta * (mean_y - spot), sqrt(residual / paths)};
}

}  // namespace

McResult mc_price(const OptionInputs& option, int paths, uint64_t seed,
                  VarianceReduction method) {
    validate(option);
    if (option.time_to_expiry <= 0.0) {
        throw invalid_argument("mc_price needs time_to_expiry > 0");
    }
    const int min_paths = method == VarianceReduction::None ? 2 : 4;
    if (paths < min_paths) {
        throw invalid_argument(method == VarianceReduction::None
                                   ? "mc_price needs paths >= 2"
                                   : "mc_price needs paths >= 4 with variance reduction");
    }

    const Simulator sim(option);
    mt19937_64 rng(seed);
    switch (method) {
        case VarianceReduction::Antithetic:
            return antithetic(sim, paths, rng);
        case VarianceReduction::ControlVariate:
            return control_variate(sim, option.spot, paths, rng);
        case VarianceReduction::None:
            break;
    }
    return plain(sim, paths, rng);
}

}  // namespace ope
