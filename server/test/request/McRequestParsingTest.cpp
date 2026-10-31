#include "request/McConvergenceRequest.hpp"
#include "request/McPriceRequest.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using nlohmann::json;
using ope::OptionType;
using ope::server::McConvergenceRequest;
using ope::server::McPriceRequest;
using std::invalid_argument;
using std::pair;
using std::string;
using std::vector;

namespace {

// Hull Ch 15: S=42, K=40, T=0.5, r=10%, σ=20%.
json hullPrice() {
    return {{"type", "call"}, {"spot", 42},  {"strike", 40},     {"expiry", 0.5},
            {"rate", 0.1},    {"vol", 0.2},  {"paths", 100000}, {"seed", 42}};
}

string priceError(const json& body) {
    try {
        (void)McPriceRequest::fromJson(body);
    } catch (const invalid_argument& e) {
        return e.what();
    }
    return "";
}

string convergenceError(const json& body) {
    try {
        (void)McConvergenceRequest::fromJson(body);
    } catch (const invalid_argument& e) {
        return e.what();
    }
    return "";
}

}  // namespace

TEST(McPriceRequest, ParsesOptionPathsAndSeed) {
    json body = hullPrice();
    body["type"] = "put";
    const auto r = McPriceRequest::fromJson(body);
    EXPECT_EQ(r.option.type, OptionType::Put);
    EXPECT_DOUBLE_EQ(r.option.spot, 42.0);
    EXPECT_DOUBLE_EQ(r.option.volatility, 0.2);
    EXPECT_EQ(r.paths, 100000);
    EXPECT_EQ(r.seed, 42u);
}

TEST(McPriceRequest, RejectsMissingAndOutOfRangeFields) {
    json body = hullPrice();
    body.erase("vol");
    EXPECT_EQ(priceError(body), "missing field: vol");

    for (const auto& [name, value] : vector<pair<string, json>>{
             {"paths", 3}, {"paths", McPriceRequest::kMaxPaths + 1}, {"paths", 1.5},
             {"seed", -1}, {"type", "straddle"}}) {
        body = hullPrice();
        body[name] = value;
        EXPECT_NE(priceError(body), "") << name << " = " << value;
    }
}

TEST(McConvergenceRequest, DefaultsToTenPathCountsUpToAMillion) {
    json body = hullPrice();
    body.erase("paths");
    const auto r = McConvergenceRequest::fromJson(body);
    ASSERT_EQ(r.paths.size(), 10u);
    EXPECT_EQ(r.paths.front(), 1000);
    EXPECT_EQ(r.paths.back(), 1000000);
}

TEST(McConvergenceRequest, ValidatesPathListAndCapsTotalWork) {
    json body = hullPrice();
    body["paths"] = {100, 1000};
    EXPECT_EQ(McConvergenceRequest::fromJson(body).paths, (vector<int>{100, 1000}));

    for (const json& bad : {json(1000), json::array(), json({3, 100}), json({100, "x"})}) {
        body["paths"] = bad;
        EXPECT_NE(convergenceError(body), "") << bad;
    }
    body["paths"] = {1000000, 1000000};
    EXPECT_EQ(convergenceError(body), "");
    body["paths"] = {1000000, 1000000, 4};
    EXPECT_NE(convergenceError(body).find("total paths"), string::npos);
}
