#include "ope/option.hpp"

#include <cmath>
#include <sstream>
#include <stdexcept>

using std::invalid_argument;
using std::isfinite;
using std::ostringstream;

namespace ope {

namespace {

void require(bool ok, const char* field, const char* rule, double value) {
    if (ok) {
        return;
    }
    ostringstream msg;
    msg << "OptionInputs." << field << " must be " << rule << ", got " << value;
    throw invalid_argument(msg.str());
}

}  // namespace

void validate(const OptionInputs& in) {
    require(isfinite(in.spot) && in.spot > 0.0, "spot", "finite and > 0", in.spot);
    require(isfinite(in.strike) && in.strike > 0.0, "strike", "finite and > 0", in.strike);
    require(isfinite(in.time_to_expiry) && in.time_to_expiry >= 0.0, "time_to_expiry",
            "finite and >= 0", in.time_to_expiry);
    require(isfinite(in.rate), "rate", "finite", in.rate);
    require(isfinite(in.volatility) && in.volatility >= 0.0, "volatility", "finite and >= 0",
            in.volatility);
}

}  // namespace ope
