#include "request/HedgeFrequencyRequest.hpp"
#include "request/HedgePathRequest.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

using nlohmann::json;
using ope::OptionType;
using ope::server::HedgeFrequencyRequest;
using ope::server::HedgePathRequest;
using std::invalid_argument;
using std::string;

namespace {

// Hull 19.1: S=49, K=50, T=20 weeks, r=5%, σ=20%, 100,000 options sold at $3.
json hullPath() {
    return {{"spot", 49},       {"strike", 50},   {"expiry", 0.3846}, {"rate", 0.05},
            {"vol", 0.2},       {"quantity", 1e5}, {"premium", 3.0},  {"drift", 0.13},
            {"rebalances", 20}, {"seed", 42}};
}

json hullFrequency() {
    return {{"spot", 49},   {"strike", 50},    {"expiry", 0.3846}, {"rate", 0.05}, {"vol", 0.2},
            {"quantity", 1e5}, {"drift", 0.13}, {"paths", 500},     {"seed", 7}};
}

string errorFor(const json& body, bool frequency = false) {
    try {
        if (frequency) {
            (void)HedgeFrequencyRequest::fromJson(body);
        } else {
            (void)HedgePathRequest::fromJson(body);
        }
    } catch (const invalid_argument& e) {
        return e.what();
    }
    return "";
}

}  // namespace

TEST(HedgePathRequest, ParsesSimulatedPath) {
    const auto r = HedgePathRequest::fromJson(hullPath());
    EXPECT_EQ(r.option.type, OptionType::Call);
    EXPECT_DOUBLE_EQ(r.option.volatility, 0.2);
    EXPECT_DOUBLE_EQ(r.quantity, 1e5);
    EXPECT_DOUBLE_EQ(r.premium, 3.0);
    EXPECT_DOUBLE_EQ(r.drift, 0.13);
    EXPECT_EQ(r.rebalances, 20);
    EXPECT_EQ(r.seed, 42u);
    EXPECT_TRUE(r.prices.empty());
}

TEST(HedgePathRequest, GivenPricesReplaceSimulation) {
    json body = hullPath();
    body.erase("drift");
    body.erase("rebalances");
    body.erase("seed");
    body["prices"] = {49.0, 50.0, 51.0};
    const auto r = HedgePathRequest::fromJson(body);
    EXPECT_EQ(r.prices.size(), 3u);
    EXPECT_EQ(r.rebalances, 2);
}

TEST(HedgePathRequest, RejectsOutOfRangeFields) {
    auto bad = [](const char* field, json value) {
        json body = hullPath();
        body[field] = value;
        return errorFor(body);
    };
    EXPECT_NE(bad("rebalances", 0).find("rebalances"), string::npos);
    EXPECT_NE(bad("rebalances", 1001).find("rebalances"), string::npos);
    EXPECT_NE(bad("rebalances", 2.5).find("rebalances"), string::npos);
    EXPECT_NE(bad("seed", -1).find("seed"), string::npos);
    EXPECT_NE(bad("quantity", 0).find("quantity"), string::npos);
    EXPECT_NE(bad("drift", 9).find("drift"), string::npos);
    EXPECT_NE(bad("premium", -1).find("premium"), string::npos);
    EXPECT_NE(bad("prices", json::array({49.0})).find("prices"), string::npos);
    EXPECT_NE(bad("prices", json::array({49.0, "x"})).find("prices"), string::npos);
}

TEST(HedgeFrequencyRequest, DefaultsToHullRebalanceCounts) {
    const auto r = HedgeFrequencyRequest::fromJson(hullFrequency());
    EXPECT_EQ(r.paths, 500);
    EXPECT_EQ(r.rebalances, (std::vector<int>{4, 5, 10, 20, 40, 80}));
}

TEST(HedgeFrequencyRequest, CapsTotalWork) {
    json body = hullFrequency();
    body["paths"] = 5000;
    body["rebalances"] = {500, 500};  // 5000 × 1000 = 5,000,000 > 2,000,000
    EXPECT_NE(errorFor(body, true).find("<= 2000000"), string::npos);

    body["paths"] = 20000;
    EXPECT_NE(errorFor(body, true).find("paths"), string::npos);
}
