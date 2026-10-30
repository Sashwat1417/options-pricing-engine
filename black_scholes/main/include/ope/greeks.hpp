#pragma once

#include "ope/option.hpp"

namespace ope {

struct Greeks {
    double delta;  // ∂price/∂S
    double theta;  // ∂price/∂t, per year; t is calendar time, so θ = −∂price/∂T
    double gamma;  // ∂delta/∂S = ∂²price/∂S²
    double vega;   // ∂price/∂σ, per unit of σ (divide by 100 for per 1% vol)
    double rho;    // ∂price/∂r, per unit of r (divide by 100 for per 1% rate)
};

// delta: call = N(d1), put = N(d1) − 1
// theta: call = −S·φ(d1)·σ/(2√T) − r·K·e^(−rT)·N(d2)
//        put  = −S·φ(d1)·σ/(2√T) + r·K·e^(−rT)·N(−d2)
// gamma: φ(d1)/(S·σ·√T), the same for calls and puts
// vega:  S·√T·φ(d1), the same for calls and puts
// rho:   call = K·T·e^(−rT)·N(d2), put = −K·T·e^(−rT)·N(−d2)
// When σ√T = 0 the price is the discounted payoff: delta is a step
// (call 1 if S > K·e^(−rT), else 0), theta is only the interest on the strike
// (call −r·K·e^(−rT) when in the money, else 0), rho is ±K·T·e^(−rT) when in the money,
// and gamma and vega are 0.
// Throws std::invalid_argument on invalid inputs.
[[nodiscard]] Greeks greeks(const OptionInputs& inputs);

}  // namespace ope
