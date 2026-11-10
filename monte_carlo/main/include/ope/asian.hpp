#pragma once

#include "ope/mc_pricer.hpp"
#include "ope/option.hpp"

#include <cstdint>

// Average price options (Hull 26.13): a call pays max(S_ave − K, 0), a put max(K − S_ave, 0).
// S_ave averages `fixings` prices observed at T/m, 2T/m, …, T (m = fixings).
// Every function throws std::invalid_argument on invalid inputs, T <= 0 or fixings < 1.

namespace ope {

// Exact price when S_ave is the geometric average: ln S_ave is normal, so S_ave is lognormal.
[[nodiscard]] double geometric_asian_price(const OptionInputs& option, int fixings);

// Hull's approximation for the arithmetic average (26.3, 26.4): treat S_ave as lognormal with
// the same first two moments, then use Black's model.
[[nodiscard]] double asian_lognormal_price(const OptionInputs& option, int fixings);

// Monte Carlo price of the arithmetic average option. The control variate is the geometric
// average option, whose exact price is known.
[[nodiscard]] McResult mc_asian_price(const OptionInputs& option, int fixings, int paths,
                                      std::uint64_t seed,
                                      VarianceReduction method = VarianceReduction::None);

}  // namespace ope
