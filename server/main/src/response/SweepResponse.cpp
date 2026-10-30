#include "response/SweepResponse.hpp"

using nlohmann::json;

namespace ope::server {

json SweepResponse::toJson() const {
    json x = json::array();
    json call = json::array();
    json put = json::array();
    json callPayoff = json::array();
    json putPayoff = json::array();
    json callDelta = json::array();
    json putDelta = json::array();
    for (const auto& p : points) {
        x.push_back(p.x);
        call.push_back(p.call);
        put.push_back(p.put);
        callPayoff.push_back(p.call_payoff);
        putPayoff.push_back(p.put_payoff);
        callDelta.push_back(p.call_delta);
        putDelta.push_back(p.put_delta);
    }
    return {{"vary", vary},
            {"x", x},
            {"call", call},
            {"put", put},
            {"call_payoff", callPayoff},
            {"put_payoff", putPayoff},
            {"call_delta", callDelta},
            {"put_delta", putDelta}};
}

}  // namespace ope::server
