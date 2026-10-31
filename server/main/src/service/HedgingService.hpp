#pragma once

#include "request/HedgeFrequencyRequest.hpp"
#include "request/HedgePathRequest.hpp"
#include "response/HedgeFrequencyResponse.hpp"
#include "response/HedgePathResponse.hpp"

namespace ope::server {

// Runs hedging requests through ope::hedging. Stateless.
// Throws std::invalid_argument when the library rejects the inputs.
class HedgingService {
public:
    HedgePathResponse hedgePath(const HedgePathRequest& request) const;
    HedgeFrequencyResponse hedgeFrequency(const HedgeFrequencyRequest& request) const;
};

}  // namespace ope::server
