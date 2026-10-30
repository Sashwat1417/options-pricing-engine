#include "ope/sweep.hpp"

#include "ope/black_scholes.hpp"
#include "ope/greeks.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using std::invalid_argument;

namespace {

using ope::OptionInputs;
using ope::OptionType;
using ope::sweep;
using ope::SweepAxis;

// Hull Ch 15 worked example.
OptionInputs hull_example() { return {OptionType::Call, 42.0, 40.0, 0.5, 0.10, 0.20}; }

}  // namespace

TEST(Sweep, SpotAxisMatchesPriceAtEachPoint) {
    const auto points = sweep(hull_example(), SweepAxis::Spot, 30.0, 50.0, 3);
    ASSERT_EQ(points.size(), 3u);

    const double spots[] = {30.0, 40.0, 50.0};
    for (size_t i = 0; i < points.size(); ++i) {
        auto in = hull_example();
        in.spot = spots[i];
        EXPECT_DOUBLE_EQ(points[i].x, spots[i]);
        EXPECT_DOUBLE_EQ(points[i].call, ope::price(in));
        in.type = OptionType::Put;
        EXPECT_DOUBLE_EQ(points[i].put, ope::price(in));
    }
}

TEST(Sweep, PayoffsUseSpotAtEachPoint) {
    const auto points = sweep(hull_example(), SweepAxis::Spot, 30.0, 50.0, 3);
    ASSERT_EQ(points.size(), 3u);
    EXPECT_DOUBLE_EQ(points[0].call_payoff, 0.0);
    EXPECT_DOUBLE_EQ(points[0].put_payoff, 10.0);
    EXPECT_DOUBLE_EQ(points[2].call_payoff, 10.0);
    EXPECT_DOUBLE_EQ(points[2].put_payoff, 0.0);
}

TEST(Sweep, VolAndExpiryAxesVaryTheRightField) {
    const auto by_vol = sweep(hull_example(), SweepAxis::Vol, 0.1, 0.3, 3);
    ASSERT_EQ(by_vol.size(), 3u);
    EXPECT_DOUBLE_EQ(by_vol[1].x, 0.2);
    EXPECT_DOUBLE_EQ(by_vol[1].call, ope::price(hull_example()));

    const auto by_expiry = sweep(hull_example(), SweepAxis::Expiry, 0.25, 0.75, 3);
    ASSERT_EQ(by_expiry.size(), 3u);
    EXPECT_DOUBLE_EQ(by_expiry[1].x, 0.5);
    EXPECT_DOUBLE_EQ(by_expiry[1].call, ope::price(hull_example()));
}

TEST(Sweep, IncludesBothEndpoints) {
    const auto points = sweep(hull_example(), SweepAxis::Spot, 25.0, 60.0, 8);
    ASSERT_EQ(points.size(), 8u);
    EXPECT_DOUBLE_EQ(points.front().x, 25.0);
    EXPECT_DOUBLE_EQ(points.back().x, 60.0);
}

TEST(Sweep, RejectsFewerThanTwoSteps) {
    EXPECT_THROW((void)sweep(hull_example(), SweepAxis::Spot, 30.0, 50.0, 1), invalid_argument);
}

TEST(Sweep, RejectsInvalidPoints) {
    EXPECT_THROW((void)sweep(hull_example(), SweepAxis::Spot, -10.0, 50.0, 3), invalid_argument);
}

TEST(Sweep, GreeksMatchAtEachPoint) {
    const auto points = sweep(hull_example(), SweepAxis::Spot, 30.0, 50.0, 5);
    ASSERT_EQ(points.size(), 5u);
    for (const auto& p : points) {
        auto in = hull_example();
        in.spot = p.x;
        in.type = OptionType::Call;
        EXPECT_DOUBLE_EQ(p.call_delta, ope::greeks(in).delta) << "S=" << p.x;
        EXPECT_DOUBLE_EQ(p.call_theta, ope::greeks(in).theta) << "S=" << p.x;
        EXPECT_DOUBLE_EQ(p.call_gamma, ope::greeks(in).gamma) << "S=" << p.x;
        EXPECT_DOUBLE_EQ(p.call_vega, ope::greeks(in).vega) << "S=" << p.x;
        in.type = OptionType::Put;
        EXPECT_DOUBLE_EQ(p.put_delta, ope::greeks(in).delta) << "S=" << p.x;
        EXPECT_DOUBLE_EQ(p.put_theta, ope::greeks(in).theta) << "S=" << p.x;
        EXPECT_DOUBLE_EQ(p.put_gamma, ope::greeks(in).gamma) << "S=" << p.x;
        EXPECT_DOUBLE_EQ(p.put_vega, ope::greeks(in).vega) << "S=" << p.x;
    }
}
