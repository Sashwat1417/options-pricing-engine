#include "request/IvRequest.hpp"

#include "request/JsonReader.hpp"

using nlohmann::json;

namespace ope::server {

IvRequest IvRequest::fromJson(const json& body) {
    const OptionInputs inputs = json_reader::marketInputs(body, json_reader::optionType(body));
    return {inputs, json_reader::number(body, "price")};
}

}  // namespace ope::server
