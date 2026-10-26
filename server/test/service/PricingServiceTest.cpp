#include "service/PricingService.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using nlohmann::json;
using ope::OptionInputs;
using ope::OptionType;
using ope::SweepAxis;
using ope::server::IvRequest;
using ope::server::PriceRequest;
using ope::server::PricingService;
using ope::server::SweepRequest;
using std::invalid_argument;

namespace {

// Hull Ch 15 worked example.
OptionInputs hullCall() { return {OptionType::Call, 42.0, 40.0, 0.5, 0.10, 0.20}; }

const PricingService service;

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
    for (const char* key : {"call", "put", "call_payoff", "put_payoff"}) {
        EXPECT_EQ(out.at(key).size(), 3u) << key;
    }
    EXPECT_EQ(out.at("put_payoff"), json({10.0, 0.0, 0.0}));
}

TEST(PricingService, ResponsesSerialiseWithApiFieldNames) {
    EXPECT_TRUE(service.price({hullCall()}).toJson().contains("price"));
    EXPECT_TRUE(service.impliedVol({hullCall(), 4.76}).toJson().contains("implied_vol"));
}
