#include "ope/mc_pricer.hpp"

#include "estimators.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

using std::exp;
using std::invalid_argument;
using std::sqrt;
using std::uint64_t;
using std::vector;

namespace ope {

namespace {

// One draw ε of S_T. Control: e^(−rT)·S_T, whose exact mean is S.
class Vanilla {
public:
    explicit Vanilla(const OptionInputs& option)
        : option_(option),
          drift_((option.rate - 0.5 * option.volatility * option.volatility) *
                 option.time_to_expiry),
          shock_scale_(option.volatility * sqrt(option.time_to_expiry)),
          discount_(exp(-option.rate * option.time_to_expiry)) {}

    int shocks() const { return 1; }

    detail::Sample evaluate(const vector<double>& epsilon, double sign) const {
        const double spot_at_expiry =
            option_.spot * exp(drift_ + shock_scale_ * sign * epsilon[0]);
        return {discount_ * detail::payoff(option_.type, spot_at_expiry, option_.strike),
                discount_ * spot_at_expiry};
    }

private:
    const OptionInputs& option_;
    double drift_;
    double shock_scale_;
    double discount_;
};

}  // namespace

McResult mc_price(const OptionInputs& option, int paths, uint64_t seed,
                  VarianceReduction method) {
    validate(option);
    if (option.time_to_expiry <= 0.0) {
        throw invalid_argument("mc_price needs time_to_expiry > 0");
    }
    detail::require_paths(paths, method, "mc_price");
    return detail::estimate(Vanilla(option), option.spot, paths, seed, method);
}

}  // namespace ope
