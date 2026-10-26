#include "response/SweepResponse.hpp"

using nlohmann::json;

namespace ope::server {

json SweepResponse::toJson() const {
    json x = json::array();
    json call = json::array();
    json put = json::array();
    json callPayoff = json::array();
    json putPayoff = json::array();
    for (const auto& p : points) {
        x.push_back(p.x);
        call.push_back(p.call);
        put.push_back(p.put);
        callPayoff.push_back(p.call_payoff);
        putPayoff.push_back(p.put_payoff);
    }
    return {{"vary", vary},          {"x", x},
            {"call", call},          {"put", put},
            {"call_payoff", callPayoff}, {"put_payoff", putPayoff}};
}

}  // namespace ope::server
