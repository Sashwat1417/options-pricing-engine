#pragma once

#include "ope/option.hpp"

#include <cstdint>

namespace ope {

struct McResult {
    double price;      // mean discounted payoff
    double std_error;  // sd of discounted payoffs / √paths
};

// Prices a European call or put by simulating S_T under the risk-neutral drift r:
// S_T = S·exp[(r − σ²/2)·T + σ·ε·√T], price ≈ e^(−rT)·mean(payoff).
// The same seed always gives the same result.
// Throws std::invalid_argument on invalid inputs, T = 0 or paths < 2.
[[nodiscard]] McResult mc_price(const OptionInputs& option, int paths, std::uint64_t seed);

}  // namespace ope
