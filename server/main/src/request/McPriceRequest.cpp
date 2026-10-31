#include "request/McPriceRequest.hpp"

#include "request/JsonReader.hpp"

#include <cstdint>

using nlohmann::json;
using std::uint64_t;

namespace ope::server {

OptionInputs mcOption(const json& body) {
    OptionInputs option = json_reader::marketInputs(body, json_reader::optionType(body));
    option.volatility = json_reader::number(body, "vol");
    return option;
}

uint64_t mcSeed(const json& body) {
    return static_cast<uint64_t>(json_reader::integer(body, "seed", 0, 4294967295LL));
}

McPriceRequest McPriceRequest::fromJson(const json& body) {
    McPriceRequest request{};
    request.option = mcOption(body);
    request.paths = static_cast<int>(json_reader::integer(body, "paths", 4, kMaxPaths));
    request.seed = mcSeed(body);
    return request;
}

}  // namespace ope::server
