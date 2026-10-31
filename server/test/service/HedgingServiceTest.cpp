#include "service/HedgingService.hpp"

#include <gtest/gtest.h>

using nlohmann::json;
using ope::OptionInputs;
using ope::OptionType;
using ope::server::HedgeFrequencyRequest;
using ope::server::HedgePathRequest;
using ope::server::HedgeVolStudyRequest;
using ope::server::HedgingService;

namespace {

const OptionInputs kHullCall{OptionType::Call, 49.0, 50.0, 0.3846, 0.05, 0.20};
const HedgingService service{};

}  // namespace

TEST(HedgingService, HedgesHullTable192Path) {
    HedgePathRequest request{kHullCall, 1e5, 3.0, {0.0, 0.2}, 20, 0, {}};
    request.prices = {49.00, 48.12, 47.37, 50.25, 51.75, 53.12, 53.00, 51.87, 51.38, 53.00, 49.88,
                      48.50, 49.88, 50.37, 52.13, 51.88, 52.87, 54.87, 54.62, 55.87, 57.25};
    const json out = service.hedgePath(request).toJson();
    EXPECT_NEAR(out.at("hedge_cost").get<double>(), 263300.0, 200.0);
    EXPECT_TRUE(out.at("exercised").get<bool>());
    EXPECT_NEAR(out.at("bs_value").get<double>(), 240046.0, 1.0);
    EXPECT_DOUBLE_EQ(out.at("premium_received").get<double>(), 300000.0);
    EXPECT_DOUBLE_EQ(out.at("pnl").get<double>(),
                     300000.0 - out.at("pv_hedge_cost").get<double>());
    EXPECT_EQ(out.at("steps").size(), 21u);
    EXPECT_TRUE(out.at("steps")[0].contains("cumulative_cost"));
    EXPECT_NEAR(out.at("steps")[0].at("shares_held").get<double>(), 52160.0, 1.0);
}

TEST(HedgingService, SimulatedPathIsReproducible) {
    const HedgePathRequest request{kHullCall, 1e5, 3.0, {0.13, 0.2}, 20, 42, {}};
    EXPECT_EQ(service.hedgePath(request).toJson(), service.hedgePath(request).toJson());
}

TEST(HedgingService, FrequencyStudyShowsSpreadShrinking) {
    const HedgeFrequencyRequest request{kHullCall, 1e5, {0.13, 0.2}, 1000, 7, {5, 20, 80}};
    const json rows = service.hedgeFrequency(request).toJson().at("rows");
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_GT(rows[0].at("sd_over_bs").get<double>(), rows[1].at("sd_over_bs").get<double>());
    EXPECT_GT(rows[1].at("sd_over_bs").get<double>(), rows[2].at("sd_over_bs").get<double>());
}

TEST(HedgingService, VolStudyProfitsWhenRealisedVolIsBelowPremiumVol) {
    const HedgeVolStudyRequest request{kHullCall, 1e5, 3.0, 0.13, 40, 1000, 3, {0.10, 0.20, 0.35}};
    const json out = service.hedgeVolStudy(request).toJson();
    EXPECT_NEAR(out.at("premium_implied_vol").get<double>(), 0.25, 0.01);  // $3 ≈ 25% vol
    const json rows = out.at("rows");
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_GT(rows[0].at("mean_pnl").get<double>(), rows[1].at("mean_pnl").get<double>());
    EXPECT_GT(rows[1].at("mean_pnl").get<double>(), 0.0);
    EXPECT_LT(rows[2].at("mean_pnl").get<double>(), 0.0);
}
