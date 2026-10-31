#include "ope/hedging.hpp"

#include "ope/greeks.hpp"

#include <cmath>
#include <stdexcept>

using std::exp;
using std::invalid_argument;
using std::isfinite;
using std::vector;

namespace ope {

namespace {

void check_inputs(const OptionInputs& option, double quantity, const vector<double>& path) {
    validate(option);
    if (option.type != OptionType::Call) {
        throw invalid_argument("hedge_short_call needs a call option");
    }
    if (!isfinite(quantity) || quantity <= 0.0) {
        throw invalid_argument("quantity must be finite and > 0");
    }
    if (path.size() < 2) {
        throw invalid_argument("path needs at least 2 prices: today and expiry");
    }
    for (double s : path) {
        if (!isfinite(s) || s <= 0.0) {
            throw invalid_argument("path prices must be finite and > 0");
        }
    }
    if (path.front() != option.spot) {
        throw invalid_argument("path must start at option.spot");
    }
}

}  // namespace

HedgeResult hedge_short_call(const OptionInputs& option, double quantity,
                             const vector<double>& path) {
    check_inputs(option, quantity, path);

    const size_t last = path.size() - 1;
    const double dt = option.time_to_expiry / static_cast<double>(last);
    const double growth = exp(option.rate * dt) - 1.0;  // interest per rebalance period

    HedgeResult result{};
    result.steps.reserve(path.size());
    double previous_delta = 0.0;
    double carried = 0.0;  // cumulative cost plus interest from the last period

    // Only path[k] is read at week k: the hedge never sees future prices.
    for (size_t k = 0; k <= last; ++k) {
        OptionInputs now = option;
        now.spot = path[k];
        now.time_to_expiry = k == last ? 0.0 : option.time_to_expiry - static_cast<double>(k) * dt;

        HedgeStep step{};
        step.time = static_cast<double>(k) * dt;
        step.spot = path[k];
        step.delta = greeks(now).delta;
        step.shares_bought = (step.delta - previous_delta) * quantity;
        step.purchase_cost = step.shares_bought * step.spot;
        step.cumulative_cost = carried + step.purchase_cost;
        step.interest = k == last ? 0.0 : step.cumulative_cost * growth;

        carried = step.cumulative_cost + step.interest;
        previous_delta = step.delta;
        result.steps.push_back(step);
    }

    const bool exercised = path.back() > option.strike;
    result.hedge_cost =
        result.steps.back().cumulative_cost - (exercised ? option.strike * quantity : 0.0);
    result.pv_hedge_cost = result.hedge_cost * exp(-option.rate * option.time_to_expiry);
    return result;
}

}  // namespace ope
