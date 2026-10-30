#pragma once

#include "ope/option.hpp"

namespace ope {

struct Greeks {
    double delta;  // ∂price/∂S
};

// delta: call = N(d1), put = N(d1) − 1
// When σ√T = 0 the price is a kinked payoff, so delta is a step: call 1 if S > K·e^(−rT), else 0.
// Throws std::invalid_argument on invalid inputs.
[[nodiscard]] Greeks greeks(const OptionInputs& inputs);

}  // namespace ope
