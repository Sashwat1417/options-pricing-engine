#pragma once

#include "ope/option.hpp"

#include <vector>

namespace ope {

enum class SweepAxis { Spot, Vol, Expiry };

struct SweepPoint {
    double x;
    double call;
    double put;
    double call_payoff;  // at expiry
    double put_payoff;
    double call_delta;
    double put_delta;
    double call_theta;  // per year
    double put_theta;
    double call_gamma;
    double put_gamma;
    double call_vega;  // per unit of σ
    double put_vega;
    double call_rho;  // per unit of r
    double put_rho;
};

// Prices (and Greeks of) a call and a put at `steps` evenly spaced values of `axis` in [from, to].
// base.type is ignored. Throws std::invalid_argument if steps < 2 or any point is invalid.
[[nodiscard]] std::vector<SweepPoint> sweep(const OptionInputs& base, SweepAxis axis, double from,
                                            double to, int steps);

}  // namespace ope
