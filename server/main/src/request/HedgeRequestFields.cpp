#include "request/HedgeRequestFields.hpp"

#include "request/JsonReader.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>

using nlohmann::json;
using std::fabs;
using std::invalid_argument;
using std::uint64_t;

namespace ope::server::hedge_fields {

OptionInputs callOption(const json& body) {
    OptionInputs option = json_reader::marketInputs(body, OptionType::Call);
    option.volatility = json_reader::number(body, "vol");
    return option;
}

double quantity(const json& body) {
    const double q = json_reader::number(body, "quantity");
    if (q <= 0.0 || q > 1e9) {
        throw invalid_argument("quantity must be in (0, 1e9]");
    }
    return q;
}

RealWorld realWorld(const json& body, double impliedVol) {
    const double mu = json_reader::number(body, "drift");
    if (fabs(mu) > 5.0) {
        throw invalid_argument("drift must be in [-5, 5]");
    }
    double vol = impliedVol;
    if (body.contains("real_vol")) {
        vol = json_reader::number(body, "real_vol");
        if (vol < 0.0 || vol > 5.0) {
            throw invalid_argument("real_vol must be in [0, 5]");
        }
    }
    return {mu, vol};
}

double costRate(const json& body) {
    if (!body.contains("cost_rate")) {
        return 0.0;
    }
    const double rate = json_reader::number(body, "cost_rate");
    if (rate < 0.0 || rate > 0.1) {
        throw invalid_argument("cost_rate must be in [0, 0.1]");
    }
    return rate;
}

uint64_t seed(const json& body) {
    return static_cast<uint64_t>(json_reader::integer(body, "seed", 0, 4294967295LL));
}

}  // namespace ope::server::hedge_fields
