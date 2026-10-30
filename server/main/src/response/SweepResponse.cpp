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
    json callTheta = json::array();
    json putTheta = json::array();
    json callGamma = json::array();
    json putGamma = json::array();
    json callVega = json::array();
    json putVega = json::array();
    for (const auto& p : points) {
        x.push_back(p.x);
        call.push_back(p.call);
        put.push_back(p.put);
        callPayoff.push_back(p.call_payoff);
        putPayoff.push_back(p.put_payoff);
        callDelta.push_back(p.call_delta);
        putDelta.push_back(p.put_delta);
        callTheta.push_back(p.call_theta);
        putTheta.push_back(p.put_theta);
        callGamma.push_back(p.call_gamma);
        putGamma.push_back(p.put_gamma);
        callVega.push_back(p.call_vega);
        putVega.push_back(p.put_vega);
    }
    return {{"vary", vary},
            {"x", x},
            {"call", call},
            {"put", put},
            {"call_payoff", callPayoff},
            {"put_payoff", putPayoff},
            {"call_delta", callDelta},
            {"put_delta", putDelta},
            {"call_theta", callTheta},
            {"put_theta", putTheta},
            {"call_gamma", callGamma},
            {"put_gamma", putGamma},
            {"call_vega", callVega},
            {"put_vega", putVega}};
}

}  // namespace ope::server
