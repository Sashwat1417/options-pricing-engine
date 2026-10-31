#pragma once

#include <random>
#include <vector>

namespace ope {

struct GbmParams {
    double spot;        // S₀
    double drift;       // μ: real-world drift for hedging, r for risk-neutral pricing
    double volatility;  // σ, annualised
    double horizon;     // T, in years
    int steps;          // N
};

// One path S₀, S₁ … S_N of dS = μS·dt + σS·dz, using the exact lognormal step
// S(t+Δt) = S(t)·exp[(μ − σ²/2)·Δt + σ·ε·√Δt], ε ~ N(0, 1).
// Throws std::invalid_argument on invalid parameters.
[[nodiscard]] std::vector<double> simulate_gbm_path(const GbmParams& params, std::mt19937_64& rng);

}  // namespace ope
