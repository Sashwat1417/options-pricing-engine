#pragma once

#include "ope/option.hpp"

namespace ope {

struct Greeks {
    double delta;  // ∂price/∂S
    double theta;  // ∂price/∂t, per year; t is calendar time, so θ = −∂price/∂T
    double gamma;  // ∂delta/∂S = ∂²price/∂S²
};

// delta: call = N(d1), put = N(d1) − 1
// theta: call = −S·φ(d1)·σ/(2√T) − r·K·e^(−rT)·N(d2)
//        put  = −S·φ(d1)·σ/(2√T) + r·K·e^(−rT)·N(−d2)
// gamma: φ(d1)/(S·σ·√T), the same for calls and puts
// When σ√T = 0 the price is the discounted payoff: delta is a step (call 1 if S > K·e^(−rT), else 0)
// theta is only the interest on the strike (call −r·K·e^(−rT) when in the money, else 0),
// and gamma is 0 (delta is flat either side of the step).
// Throws std::invalid_argument on invalid inputs.
[[nodiscard]] Greeks greeks(const OptionInputs& inputs);

}  // namespace ope
