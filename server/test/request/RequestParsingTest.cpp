#include "request/IvRequest.hpp"
#include "request/PriceRequest.hpp"
#include "request/SweepRequest.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

using nlohmann::json;
using ope::OptionType;
using ope::SweepAxis;
using ope::server::IvRequest;
using ope::server::PriceRequest;
using ope::server::SweepRequest;
using std::invalid_argument;
using std::string;

namespace {

// Hull Ch 15 worked example.
json hullCall() {
    return {{"type", "call"}, {"spot", 42}, {"strike", 40}, {"expiry", 0.5}, {"rate", 0.1},
            {"vol", 0.2}};
}

json hullSweep() {
    json body = hullCall();
    body.erase("type");
    body.update({{"vary", "spot"}, {"from", 30}, {"to", 50}, {"steps", 3}});
    return body;
}

template <typename Parse>
::testing::AssertionResult rejected(Parse parse, const json& body, const string& mention) {
    try {
        (void)parse(body);
    } catch (const invalid_argument& e) {
        if (string(e.what()).find(mention) != string::npos) {
            return ::testing::AssertionSuccess();
        }
        return ::testing::AssertionFailure()
               << "message doesn't mention \"" << mention << "\": " << e.what();
    }
    return ::testing::AssertionFailure() << "accepted, expected rejection mentioning " << mention;
}

}  // namespace

TEST(PriceRequest, ParsesAllFields) {
    const auto request = PriceRequest::fromJson(hullCall());
    EXPECT_EQ(request.inputs.type, OptionType::Call);
    EXPECT_DOUBLE_EQ(request.inputs.spot, 42.0);
    EXPECT_DOUBLE_EQ(request.inputs.strike, 40.0);
    EXPECT_DOUBLE_EQ(request.inputs.time_to_expiry, 0.5);
    EXPECT_DOUBLE_EQ(request.inputs.rate, 0.1);
    EXPECT_DOUBLE_EQ(request.inputs.volatility, 0.2);
}

TEST(PriceRequest, ReportsFirstMissingFieldInOrder) {
    EXPECT_TRUE(rejected(PriceRequest::fromJson, json{{"type", "call"}, {"spot", 42}},
                         "missing field: strike"));
}

TEST(PriceRequest, RejectsNonNumericField) {
    json body = hullCall();
    body["spot"] = "42";
    EXPECT_TRUE(rejected(PriceRequest::fromJson, body, "spot must be a number"));
}

TEST(PriceRequest, RejectsUnknownOptionType) {
    json body = hullCall();
    body["type"] = "straddle";
    EXPECT_TRUE(rejected(PriceRequest::fromJson, body, "type"));
}

TEST(PriceRequest, RejectsNonObjectBody) {
    EXPECT_TRUE(rejected(PriceRequest::fromJson, json::array({1, 2}), "JSON object"));
}

TEST(IvRequest, ParsesMarketPrice) {
    json body = hullCall();
    body.erase("vol");
    body["price"] = 4.76;
    const auto request = IvRequest::fromJson(body);
    EXPECT_DOUBLE_EQ(request.marketPrice, 4.76);
    EXPECT_DOUBLE_EQ(request.inputs.spot, 42.0);
}

TEST(SweepRequest, ParsesAxisRangeAndSteps) {
    const auto request = SweepRequest::fromJson(hullSweep());
    EXPECT_EQ(request.axis, SweepAxis::Spot);
    EXPECT_EQ(request.axisName, "spot");
    EXPECT_DOUBLE_EQ(request.from, 30.0);
    EXPECT_DOUBLE_EQ(request.to, 50.0);
    EXPECT_EQ(request.steps, 3);
}

TEST(SweepRequest, VolSweepDoesNotNeedVol) {
    json body = hullSweep();
    body.erase("vol");
    body["vary"] = "vol";
    EXPECT_EQ(SweepRequest::fromJson(body).axis, SweepAxis::Vol);
}

TEST(SweepRequest, RejectsUnknownAxis) {
    json body = hullSweep();
    body["vary"] = "rate";
    EXPECT_TRUE(rejected(SweepRequest::fromJson, body, "vary"));
}

TEST(SweepRequest, RejectsBadStepCounts) {
    for (const json& steps : {json(1), json(SweepRequest::kMaxSteps + 1), json(2.5), json("10")}) {
        json body = hullSweep();
        body["steps"] = steps;
        EXPECT_TRUE(rejected(SweepRequest::fromJson, body, "steps")) << "steps=" << steps;
    }
}
