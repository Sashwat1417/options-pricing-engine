#pragma once

namespace ope {

enum class OptionType { Call, Put };

// Everything that describes one vanilla European option in the Black-Scholes world.
// The stock's real expected return (mu) is deliberately absent: under risk-neutral
// pricing it drops out, so the price never depends on it.
struct OptionInputs {
    OptionType type;
    double spot;            // S: current price of the underlying
    double strike;          // K: same currency as spot
    double time_to_expiry;  // T: in years (6 months = 0.5)
    double rate;            // r: risk-free rate, annual, continuously compounded (10% = 0.10)
    double volatility;      // sigma: annualised volatility of returns (20% = 0.20)
};

// Throws std::invalid_argument naming the first offending field.
//   spot, strike:        finite and > 0
//   time_to_expiry:      finite and >= 0   (0 = at expiry, priced as the payoff)
//   volatility:          finite and >= 0   (0 = no randomness)
//   rate:                finite, any sign  (negative rates are real)
void validate(const OptionInputs& inputs);

}  // namespace ope
