#pragma once

namespace ope {

enum class OptionType { Call, Put };

struct OptionInputs {
    OptionType type;
    double spot;            // S
    double strike;          // K
    double time_to_expiry;  // T, in years
    double rate;            // r, continuously compounded
    double volatility;      // sigma, annualised
};

// Throws std::invalid_argument. T = 0 and sigma = 0 are allowed.
void validate(const OptionInputs& inputs);

}  // namespace ope
