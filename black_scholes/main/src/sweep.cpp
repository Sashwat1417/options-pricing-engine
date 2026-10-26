#include "ope/sweep.hpp"

#include "ope/black_scholes.hpp"

#include <algorithm>
#include <stdexcept>

using std::invalid_argument;
using std::max;
using std::vector;

namespace ope {

namespace {

double& field(OptionInputs& in, SweepAxis axis) {
    switch (axis) {
        case SweepAxis::Spot:
            return in.spot;
        case SweepAxis::Vol:
            return in.volatility;
        case SweepAxis::Expiry:
            return in.time_to_expiry;
    }
    throw invalid_argument("unknown sweep axis");
}

}  // namespace

vector<SweepPoint> sweep(const OptionInputs& base, SweepAxis axis, double from, double to,
                         int steps) {
    if (steps < 2) {
        throw invalid_argument("sweep needs steps >= 2");
    }

    vector<SweepPoint> points;
    points.reserve(static_cast<size_t>(steps));
    OptionInputs in = base;
    for (int i = 0; i < steps; ++i) {
        const double x = from + (to - from) * static_cast<double>(i) / (steps - 1);
        field(in, axis) = x;

        in.type = OptionType::Call;
        const double call = price(in);
        in.type = OptionType::Put;
        const double put = price(in);

        points.push_back(
            {x, call, put, max(in.spot - in.strike, 0.0), max(in.strike - in.spot, 0.0)});
    }
    return points;
}

}  // namespace ope
