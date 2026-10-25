#pragma once

#include <cmath>

namespace ope {

namespace detail {

inline constexpr double kInvSqrt2 = 0.70710678118654752440;    // 1 / sqrt(2)
inline constexpr double kInvSqrt2Pi = 0.39894228040143267794;  // 1 / sqrt(2 pi)

}  // namespace detail

// Φ(x) = 1/√(2π) · ∫_{-∞}^{x} e^(-t²/2) dt  =  ½ · erfc(-x/√2)
// erfc keeps precision in the far left tail, where 1 + erf would round to 0.
[[nodiscard]] inline double norm_cdf(double x) noexcept {
    return 0.5 * std::erfc(-x * detail::kInvSqrt2);
}

// φ(x) = 1/√(2π) · e^(-x²/2)
[[nodiscard]] inline double norm_pdf(double x) noexcept {
    return detail::kInvSqrt2Pi * std::exp(-0.5 * x * x);
}

}  // namespace ope
