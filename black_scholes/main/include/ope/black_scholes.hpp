#pragma once

#include "ope/option.hpp"

namespace ope {

struct D1D2 {
    double d1;
    double d2;
};

// d1 = [ln(S/K) + (r + σ²/2)·T] / (σ√T),  d2 = d1 − σ√T
// Throws std::invalid_argument on invalid inputs or when σ√T = 0.
[[nodiscard]] D1D2 d1d2(const OptionInputs& inputs);

}  // namespace ope
