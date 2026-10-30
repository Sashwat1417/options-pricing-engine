#include "service/PricingService.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using nlohmann::json;
using ope::OptionInputs;
using ope::OptionType;
using ope::SweepAxis;
using ope::server::GreeksRequest;
using ope::server::IvRequest;
using ope::server::PriceRequest;
using ope::server::PricingService;
using ope::server::SweepRequest;
using std::invalid_argument;

namespace {

// Hull Ch 15 worked example.
OptionInputs hullCall() { return {OptionType::Call, 42.0, 40.0, 0.5, 0.10, 0.20}; }

const PricingService service{};

}  // namespace

TEST(PricingService, PricesHullCallAndPut) {
    EXPECT_NEAR(service.price({hullCall()}).price, 4.759422392871535, 1e-12);

    auto put = hullCall();
    put.type = OptionType::Put;
    EXPECT_NEAR(service.price({put}).price, 0.808599372900096, 1e-12);
}

TEST(PricingService, RecoversHullImpliedVol) {
    EXPECT_NEAR(service.impliedVol({hullCall(), 4.76}).impliedVol, 0.200066, 1e-6);
}

TEST(PricingService, PassesOnLibraryValidationErrors) {
    auto bad = hullCall();
    bad.volatility = -0.2;
    EXPECT_THROW((void)service.price({bad}), invalid_argument);
    EXPECT_THROW((void)service.impliedVol({hullCall(), 3.0}), invalid_argument);
}

TEST(PricingService, SweepResponseHasEqualLengthArrays) {
    const SweepRequest request{hullCall(), SweepAxis::Spot, "spot", 30.0, 50.0, 3};
    const json out = service.sweep(request).toJson();
    EXPECT_EQ(out.at("vary"), "spot");
    EXPECT_EQ(out.at("x"), json({30.0, 40.0, 50.0}));
    for (const char* key :
         {"call", "put", "call_payoff", "put_payoff", "call_delta", "put_delta", "call_theta",
          "put_theta", "call_gamma", "put_gamma"}) {
        EXPECT_EQ(out.at(key).size(), 3u) << key;
    }
    EXPECT_EQ(out.at("put_payoff"), json({10.0, 0.0, 0.0}));
}

TEST(PricingService, ReturnsHullCh19Greeks) {
    const OptionInputs ch19{OptionType::Call, 49.0, 50.0, 0.3846, 0.05, 0.20};
    EXPECT_NEAR(service.greeks(GreeksRequest{ch19}).greeks.delta, 0.5216016339715761, 1e-12);
    EXPECT_NEAR(service.greeks(GreeksRequest{ch19}).greeks.theta, -4.305389964546104, 1e-11);
    EXPECT_NEAR(service.greeks(GreeksRequest{ch19}).greeks.gamma, 0.06554537725247868, 1e-12);
}

TEST(PricingService, ResponsesSerialiseWithApiFieldNames) {
    EXPECT_TRUE(service.price({hullCall()}).toJson().contains("price"));
    EXPECT_TRUE(service.impliedVol({hullCall(), 4.76}).toJson().contains("implied_vol"));
    EXPECT_TRUE(service.greeks(GreeksRequest{hullCall()}).toJson().contains("delta"));
    EXPECT_TRUE(service.greeks(GreeksRequest{hullCall()}).toJson().contains("theta"));
    EXPECT_TRUE(service.greeks(GreeksRequest{hullCall()}).toJson().contains("gamma"));
}
