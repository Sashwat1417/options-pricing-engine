#include "request/GreeksRequest.hpp"

#include "request/PriceRequest.hpp"

using nlohmann::json;

namespace ope::server {

// Same body as /price.
GreeksRequest GreeksRequest::fromJson(const json& body) {
    return {PriceRequest::fromJson(body).inputs};
}

}  // namespace ope::server
