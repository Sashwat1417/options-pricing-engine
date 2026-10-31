#include "ope/gbm.hpp"

#include <cmath>
#include <stdexcept>

using std::exp;
using std::invalid_argument;
using std::isfinite;
using std::mt19937_64;
using std::normal_distribution;
using std::sqrt;
using std::vector;

namespace ope {

namespace {

void validate(const GbmParams& p) {
    if (!isfinite(p.spot) || p.spot <= 0.0) {
        throw invalid_argument("GbmParams.spot must be finite and > 0");
    }
    if (!isfinite(p.drift)) {
        throw invalid_argument("GbmParams.drift must be finite");
    }
    if (!isfinite(p.volatility) || p.volatility < 0.0) {
        throw invalid_argument("GbmParams.volatility must be finite and >= 0");
    }
    if (!isfinite(p.horizon) || p.horizon <= 0.0) {
        throw invalid_argument("GbmParams.horizon must be finite and > 0");
    }
    if (p.steps < 1) {
        throw invalid_argument("GbmParams.steps must be >= 1");
    }
}

}  // namespace

vector<double> simulate_gbm_path(const GbmParams& p, mt19937_64& rng) {
    validate(p);

    const double dt = p.horizon / p.steps;
    const double drift_per_step = (p.drift - 0.5 * p.volatility * p.volatility) * dt;
    const double shock_scale = p.volatility * sqrt(dt);
    normal_distribution<double> epsilon(0.0, 1.0);

    vector<double> path;
    path.reserve(static_cast<size_t>(p.steps) + 1);
    path.push_back(p.spot);
    for (int i = 0; i < p.steps; ++i) {
        path.push_back(path.back() * exp(drift_per_step + shock_scale * epsilon(rng)));
    }
    return path;
}

}  // namespace ope
