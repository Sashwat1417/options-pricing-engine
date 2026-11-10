#pragma once

#include "ope/mc_pricer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

// Estimators shared by every Monte Carlo product. A product turns the shocks ε of one path
// into a Sample; `sign` = −1 replays the same path with −ε (antithetic).
//
//   struct Product {
//       int shocks() const;
//       Sample evaluate(const std::vector<double>& epsilon, double sign) const;
//   };

namespace ope::detail {

struct Sample {
    double value;    // discounted payoff X
    double control;  // discounted control Y, whose exact mean is known
};

inline double payoff(OptionType type, double underlying, double strike) {
    return type == OptionType::Call ? std::max(underlying - strike, 0.0)
                                    : std::max(strike - underlying, 0.0);
}

inline void require_paths(int paths, VarianceReduction method, const std::string& caller) {
    if (method == VarianceReduction::None ? paths < 2 : paths < 4) {
        throw std::invalid_argument(method == VarianceReduction::None
                                        ? caller + " needs paths >= 2"
                                        : caller + " needs paths >= 4 with variance reduction");
    }
}

// Welford's running mean and variance.
class RunningStats {
public:
    void add(double x) {
        ++n_;
        const double change = x - mean_;
        mean_ += change / n_;
        squares_ += change * (x - mean_);
    }

    McResult result() const { return {mean_, std::sqrt(squares_ / (n_ - 1) / n_)}; }

private:
    int n_ = 0;
    double mean_ = 0.0;
    double squares_ = 0.0;
};

class Shocks {
public:
    Shocks(int count, std::uint64_t seed) : epsilon_(static_cast<std::size_t>(count)), rng_(seed) {}

    const std::vector<double>& next() {
        for (double& e : epsilon_) e = normal_(rng_);
        return epsilon_;
    }

private:
    std::vector<double> epsilon_;
    std::mt19937_64 rng_;
    std::normal_distribution<double> normal_{0.0, 1.0};
};

template <class Product>
McResult plain(const Product& product, int paths, Shocks& shocks) {
    RunningStats stats;
    for (int i = 0; i < paths; ++i) stats.add(product.evaluate(shocks.next(), 1.0).value);
    return stats.result();
}

template <class Product>
McResult antithetic(const Product& product, int paths, Shocks& shocks) {
    RunningStats pairs;
    for (int i = 0; i < paths / 2; ++i) {
        const auto& e = shocks.next();
        pairs.add(0.5 * (product.evaluate(e, 1.0).value + product.evaluate(e, -1.0).value));
    }
    return pairs.result();
}

// price = mean(X) − β·(mean(Y) − E[Y]) with β = cov(X, Y) / var(Y), the regression of X on
// Y. The error is what Y cannot explain.
template <class Product>
McResult control_variate(const Product& product, double control_mean, int paths,
                         Shocks& shocks) {
    double mean_x = 0.0;
    double mean_y = 0.0;
    double sxx = 0.0;
    double syy = 0.0;
    double sxy = 0.0;
    for (int i = 1; i <= paths; ++i) {
        const Sample s = product.evaluate(shocks.next(), 1.0);
        const double dx = s.value - mean_x;
        const double dy = s.control - mean_y;
        mean_x += dx / i;
        mean_y += dy / i;
        sxx += dx * (s.value - mean_x);
        syy += dy * (s.control - mean_y);
        sxy += dx * (s.control - mean_y);
    }
    const double beta = syy > 0.0 ? sxy / syy : 0.0;
    const double residual = std::max(sxx - beta * sxy, 0.0) / (paths - 2);
    return {mean_x - beta * (mean_y - control_mean), std::sqrt(residual / paths)};
}

template <class Product>
McResult estimate(const Product& product, double control_mean, int paths, std::uint64_t seed,
                  VarianceReduction method) {
    Shocks shocks(product.shocks(), seed);
    switch (method) {
        case VarianceReduction::Antithetic:
            return antithetic(product, paths, shocks);
        case VarianceReduction::ControlVariate:
            return control_variate(product, control_mean, paths, shocks);
        case VarianceReduction::None:
            break;
    }
    return plain(product, paths, shocks);
}

}  // namespace ope::detail
