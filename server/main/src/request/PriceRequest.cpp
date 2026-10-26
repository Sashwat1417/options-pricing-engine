#include "request/PriceRequest.hpp"

#include "request/JsonReader.hpp"

using nlohmann::json;

namespace ope::server {

PriceRequest PriceRequest::fromJson(const json& body) {
    PriceRequest request{json_reader::marketInputs(body, json_reader::optionType(body))};
    request.inputs.volatility = json_reader::number(body, "vol");
    return request;
}

}  // namespace ope::server
