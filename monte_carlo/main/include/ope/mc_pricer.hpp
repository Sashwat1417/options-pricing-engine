#pragma once

#include "ope/option.hpp"

#include <cstdint>

namespace ope {

struct McResult {
    double price;      // estimated option value
    double std_error;  // sd of the per-sample values / √samples
};

// Variance reduction (Hull 21.7). Every method simulates the same number of prices S_T.
enum class VarianceReduction {
    None,
    Antithetic,      // pairs ε and −ε; paths / 2 pairs, so paths >= 4
    ControlVariate,  // control e^(−rT)·S_T, whose exact mean is S; paths >= 4
};

// Prices a European call or put by simulating S_T under the risk-neutral drift r:
// S_T = S·exp[(r − σ²/2)·T + σ·ε·√T], price ≈ e^(−rT)·mean(payoff).
[[nodiscard]] McResult mc_price(const OptionInputs& option, int paths, std::uint64_t seed,
                                VarianceReduction method = VarianceReduction::None);

}  // namespace ope
