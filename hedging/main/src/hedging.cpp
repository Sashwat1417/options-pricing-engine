#include "ope/hedging.hpp"

#include "ope/black_scholes.hpp"
#include "ope/gbm.hpp"
#include "ope/greeks.hpp"

#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>

using std::exp;
using std::fabs;
using std::invalid_argument;
using std::isfinite;
using std::mt19937_64;
using std::sqrt;
using std::uint64_t;
using std::vector;

namespace ope {

namespace {

void check_inputs(const OptionInputs& option, double quantity, const vector<double>& path,
                  double cost_rate) {
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
    if (!isfinite(cost_rate) || cost_rate < 0.0 || cost_rate > 0.1) {
        throw invalid_argument("cost_rate must be in [0, 0.1]");
    }
}

}  // namespace

HedgeResult hedge_short_call(const OptionInputs& option, double quantity,
                             const vector<double>& path, double cost_rate) {
    check_inputs(option, quantity, path, cost_rate);

    const size_t last = path.size() - 1;
    const double dt = option.time_to_expiry / static_cast<double>(last);
    const double growth = exp(option.rate * dt) - 1.0;

    HedgeResult result{};
    result.steps.reserve(path.size());
    double previous_delta = 0.0;
    double carried = 0.0;

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
        step.trading_cost = cost_rate * fabs(step.shares_bought) * step.spot;
        step.cumulative_cost = carried + step.purchase_cost + step.trading_cost;
        step.interest = k == last ? 0.0 : step.cumulative_cost * growth;

        carried = step.cumulative_cost + step.interest;
        previous_delta = step.delta;
        result.total_trading_cost += step.trading_cost;
        result.steps.push_back(step);
    }

    const bool exercised = path.back() > option.strike;
    result.hedge_cost =
        result.steps.back().cumulative_cost - (exercised ? option.strike * quantity : 0.0);
    result.pv_hedge_cost = result.hedge_cost * exp(-option.rate * option.time_to_expiry);
    return result;
}

HedgeResult hedge_simulated_path(const OptionInputs& option, double quantity, RealWorld world,
                                 int rebalances, uint64_t seed, double cost_rate) {
    validate(option);
    mt19937_64 rng(seed);
    const GbmParams params{option.spot, world.drift, world.volatility, option.time_to_expiry,
                           rebalances};
    return hedge_short_call(option, quantity, simulate_gbm_path(params, rng), cost_rate);
}

HedgeCostStats hedge_cost_stats(const OptionInputs& option, double quantity, RealWorld world,
                                int rebalances, int paths, uint64_t seed,
                                double cost_rate) {
    validate(option);
    if (paths < 2) {
        throw invalid_argument("hedge_cost_stats needs paths >= 2");
    }
    mt19937_64 rng(seed);
    const GbmParams params{option.spot, world.drift, world.volatility, option.time_to_expiry,
                           rebalances};

    double mean = 0.0;
    double squares = 0.0;
    for (int i = 1; i <= paths; ++i) {
        const double cost =
            hedge_short_call(option, quantity, simulate_gbm_path(params, rng), cost_rate)
                .pv_hedge_cost;
        const double change = cost - mean;
        mean += change / i;
        squares += change * (cost - mean);
    }
    return {mean, sqrt(squares / (paths - 1)), price(option) * quantity};
}

}  // namespace ope
