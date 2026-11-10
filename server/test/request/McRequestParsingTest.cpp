#include "request/AsianRequests.hpp"
#include "request/McConvergenceRequest.hpp"
#include "request/McPriceRequest.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using nlohmann::json;
using ope::OptionType;
using ope::server::AsianConvergenceRequest;
using ope::server::AsianLimits;
using ope::server::AsianPriceRequest;
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

// Hull Example 26.3 with 12 monthly fixings.
json hullAsian() {
    return {{"type", "call"}, {"spot", 50},     {"strike", 50},  {"expiry", 1},  {"rate", 0.1},
            {"vol", 0.4},     {"fixings", 12}, {"paths", 50000}, {"seed", 42}};
}

string asianPriceError(const json& body) {
    try {
        (void)AsianPriceRequest::fromJson(body);
    } catch (const invalid_argument& e) {
        return e.what();
    }
    return "";
}

string asianConvergenceError(const json& body) {
    try {
        (void)AsianConvergenceRequest::fromJson(body);
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

TEST(AsianPriceRequest, ParsesFixingsPathsAndSeed) {
    const auto r = AsianPriceRequest::fromJson(hullAsian());
    EXPECT_EQ(r.option.type, OptionType::Call);
    EXPECT_DOUBLE_EQ(r.option.volatility, 0.4);
    EXPECT_EQ(r.fixings, 12);
    EXPECT_EQ(r.paths, 50000);
    EXPECT_EQ(r.seed, 42u);
}

TEST(AsianPriceRequest, RejectsBadFixingsAndCapsPathSteps) {
    json body = hullAsian();
    body.erase("fixings");
    EXPECT_EQ(asianPriceError(body), "missing field: fixings");

    for (const json& bad : {json(0), json(AsianLimits::kMaxFixings + 1), json(2.5)}) {
        body = hullAsian();
        body["fixings"] = bad;
        EXPECT_NE(asianPriceError(body), "") << bad;
    }

    body = hullAsian();
    body["fixings"] = 1000;
    body["paths"] = 10000;
    EXPECT_EQ(asianPriceError(body), "");
    body["paths"] = 10001;
    EXPECT_NE(asianPriceError(body).find("paths × fixings"), string::npos);
}

TEST(AsianConvergenceRequest, DefaultPathCountsStopAtTheWorkLimit) {
    json body = hullAsian();
    body.erase("paths");
    EXPECT_EQ(AsianConvergenceRequest::fromJson(body).paths.back(), 200000);

    body["fixings"] = 252;
    EXPECT_EQ(AsianConvergenceRequest::fromJson(body).paths.back(), 20000);

    body["fixings"] = AsianLimits::kMaxFixings;
    EXPECT_EQ(AsianConvergenceRequest::fromJson(body).paths,
              (vector<int>{1000, 2000, 5000}));
}

TEST(AsianConvergenceRequest, ExplicitPathCountsMustFitTheWorkLimit) {
    json body = hullAsian();
    body["paths"] = {1000, 4000};
    EXPECT_EQ(AsianConvergenceRequest::fromJson(body).paths, (vector<int>{1000, 4000}));

    body["paths"] = {500000, 500000};
    EXPECT_NE(asianConvergenceError(body).find("paths × fixings"), string::npos);
    body["paths"] = {3};
    EXPECT_NE(asianConvergenceError(body), "");
}
