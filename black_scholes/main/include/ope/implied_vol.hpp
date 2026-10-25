#pragma once

#include "ope/option.hpp"

namespace ope {

// σ in [0, 5] such that price(inputs with σ) = market_price, found by bisection.
// inputs.volatility is ignored.
// Throws std::invalid_argument on invalid inputs, T = 0, or a price no σ in [0, 5] can produce.
[[nodiscard]] double implied_vol(const OptionInputs& inputs, double market_price);

}  // namespace ope
