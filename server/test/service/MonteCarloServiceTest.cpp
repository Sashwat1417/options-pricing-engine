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

TEST(MonteCarloService, EveryMethodSitsNearBlackScholesWithA95PercentInterval) {
    const json out = service.price({kHullCall, 100000, 42}).toJson();
    EXPECT_NEAR(out.at("bs_price").get<double>(), 4.7594, 1e-4);
    EXPECT_EQ(out.at("paths").get<int>(), 100000);
    EXPECT_EQ(out.at("seed").get<int>(), 42);
    for (const char* method : {"plain", "antithetic", "control_variate"}) {
        const json& e = out.at("estimates").at(method);
        const double price = e.at("price").get<double>();
        const double se = e.at("std_error").get<double>();
        EXPECT_NEAR(price, out.at("bs_price").get<double>(), 4.0 * se) << method;
        EXPECT_NEAR(e.at("ci_low").get<double>(), price - 1.96 * se, 1e-12) << method;
        EXPECT_NEAR(e.at("ci_high").get<double>(), price + 1.96 * se, 1e-12) << method;
    }
}

TEST(MonteCarloService, SpeedupIsTheSquaredRatioOfStandardErrors) {
    const json e = service.price({kHullCall, 100000, 3}).toJson().at("estimates");
    EXPECT_DOUBLE_EQ(e.at("plain").at("speedup").get<double>(), 1.0);
    const double plain = e.at("plain").at("std_error").get<double>();
    const double cv = e.at("control_variate").at("std_error").get<double>();
    EXPECT_DOUBLE_EQ(e.at("control_variate").at("speedup").get<double>(),
                     (plain / cv) * (plain / cv));
    EXPECT_GT(e.at("antithetic").at("speedup").get<double>(), 2.5);
    EXPECT_GT(e.at("control_variate").at("speedup").get<double>(), 8.0);
}

TEST(MonteCarloService, SpeedupIsNullWhenThereIsNoError) {
    OptionInputs certain = kHullCall;
    certain.volatility = 0.0;
    const json e = service.price({certain, 100, 1}).toJson().at("estimates");
    EXPECT_TRUE(e.at("control_variate").at("speedup").is_null());
}

TEST(MonteCarloService, ConvergenceRowsNarrowAsPathsGrow) {
    const McConvergenceRequest request{kHullCall, 7, {1000, 10000, 100000}};
    const json out = service.convergence(request).toJson();
    const json& rows = out.at("rows");
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[2].at("paths").get<int>(), 100000);
    const auto se = [&](size_t i, const char* method) {
        return rows[i].at("estimates").at(method).at("std_error").get<double>();
    };
    for (const char* method : {"plain", "antithetic", "control_variate"}) {
        EXPECT_GT(se(0, method), se(1, method)) << method;
        EXPECT_GT(se(1, method), se(2, method)) << method;
        for (size_t i = 0; i < rows.size(); ++i) {
            EXPECT_NEAR(rows[i].at("estimates").at(method).at("price").get<double>(),
                        out.at("bs_price").get<double>(), 4.0 * se(i, method))
                << method << " row " << i;
        }
    }
}

TEST(MonteCarloService, LibraryRejectionsSurfaceAsInvalidArgument) {
    OptionInputs expired = kHullCall;
    expired.time_to_expiry = 0.0;
    EXPECT_THROW((void)service.price({expired, 1000, 1}), invalid_argument);
}
