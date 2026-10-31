#include "service/MonteCarloService.hpp"

#include "ope/black_scholes.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using nlohmann::json;
using ope::OptionInputs;
using ope::OptionType;
using ope::server::McConvergenceRequest;
using ope::server::McPriceRequest;
using ope::server::MonteCarloService;
using std::invalid_argument;

namespace {

const OptionInputs kHullCall{OptionType::Call, 42.0, 40.0, 0.5, 0.10, 0.20};
const MonteCarloService service{};

}  // namespace

TEST(MonteCarloService, PriceSitsNearBlackScholesWithA95PercentInterval) {
    const json out = service.price({kHullCall, 100000, 42}).toJson();
    const double price = out.at("price").get<double>();
    const double se = out.at("std_error").get<double>();
    EXPECT_NEAR(out.at("bs_price").get<double>(), 4.7594, 1e-4);
    EXPECT_NEAR(price, out.at("bs_price").get<double>(), 4.0 * se);
    EXPECT_NEAR(out.at("ci_low").get<double>(), price - 1.96 * se, 1e-12);
    EXPECT_NEAR(out.at("ci_high").get<double>(), price + 1.96 * se, 1e-12);
    EXPECT_EQ(out.at("paths").get<int>(), 100000);
    EXPECT_EQ(out.at("seed").get<int>(), 42);
}

TEST(MonteCarloService, ConvergenceRowsNarrowAsPathsGrow) {
    const McConvergenceRequest request{kHullCall, 7, {1000, 10000, 100000}};
    const json out = service.convergence(request).toJson();
    const json& rows = out.at("rows");
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[2].at("paths").get<int>(), 100000);
    EXPECT_GT(rows[0].at("std_error").get<double>(), rows[1].at("std_error").get<double>());
    EXPECT_GT(rows[1].at("std_error").get<double>(), rows[2].at("std_error").get<double>());
    for (const json& r : rows) {
        EXPECT_NEAR(r.at("price").get<double>(), out.at("bs_price").get<double>(),
                    4.0 * r.at("std_error").get<double>());
    }
}

TEST(MonteCarloService, LibraryRejectionsSurfaceAsInvalidArgument) {
    OptionInputs expired = kHullCall;
    expired.time_to_expiry = 0.0;
    EXPECT_THROW((void)service.price({expired, 1000, 1}), invalid_argument);
}
