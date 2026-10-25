#include "ope/option.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using ope::OptionInputs;
using ope::OptionType;

// Hull Ch 15 worked example: S=42, K=40, T=0.5, r=10%, sigma=20%.
OptionInputs hull_example() { return {OptionType::Call, 42.0, 40.0, 0.5, 0.10, 0.20}; }

// Runs validate() and returns the exception message, or "" if nothing was thrown.
std::string validation_error(const OptionInputs& in) {
    try {
        ope::validate(in);
    } catch (const std::invalid_argument& e) {
        return e.what();
    }
    return "";
}

// Passes when validate() throws and the message names `field`.
::testing::AssertionResult rejected_for(const OptionInputs& in, const std::string& field) {
    const std::string msg = validation_error(in);
    if (msg.empty()) {
        return ::testing::AssertionFailure() << "accepted, expected rejection of " << field;
    }
    if (msg.find(field) == std::string::npos) {
        return ::testing::AssertionFailure()
               << "rejected, but the message doesn't name " << field << ": \"" << msg << "\"";
    }
    return ::testing::AssertionSuccess();
}

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

}  // namespace

// ---- valid inputs ---------------------------------------------------------------

TEST(Validate, AcceptsHullExample) {
    EXPECT_NO_THROW(ope::validate(hull_example()));
}

TEST(Validate, AcceptsPut) {
    auto in = hull_example();
    in.type = OptionType::Put;
    EXPECT_NO_THROW(ope::validate(in));
}

TEST(Validate, AcceptsZeroTimeToExpiry) {
    auto in = hull_example();
    in.time_to_expiry = 0.0;
    EXPECT_NO_THROW(ope::validate(in));
}

TEST(Validate, AcceptsZeroVolatility) {
    auto in = hull_example();
    in.volatility = 0.0;
    EXPECT_NO_THROW(ope::validate(in));
}

TEST(Validate, AcceptsNegativeRate) {
    auto in = hull_example();
    in.rate = -0.005;
    EXPECT_NO_THROW(ope::validate(in));
}

// ---- out-of-range values --------------------------------------------------------

TEST(Validate, RejectsNonPositiveSpot) {
    for (double s : {0.0, -42.0}) {
        auto in = hull_example();
        in.spot = s;
        EXPECT_TRUE(rejected_for(in, "spot")) << "spot=" << s;
    }
}

TEST(Validate, RejectsNonPositiveStrike) {
    for (double k : {0.0, -40.0}) {
        auto in = hull_example();
        in.strike = k;
        EXPECT_TRUE(rejected_for(in, "strike")) << "strike=" << k;
    }
}

TEST(Validate, RejectsNegativeTimeToExpiry) {
    auto in = hull_example();
    in.time_to_expiry = -0.5;
    EXPECT_TRUE(rejected_for(in, "time_to_expiry"));
}

TEST(Validate, RejectsNegativeVolatility) {
    auto in = hull_example();
    in.volatility = -0.20;
    EXPECT_TRUE(rejected_for(in, "volatility"));
}

// ---- NaN and infinity, every numeric field --------------------------------------

TEST(Validate, RejectsNonFiniteInEveryField) {
    struct Field {
        std::string name;
        std::function<void(OptionInputs&, double)> set;
    };
    const std::vector<Field> fields = {
        {"spot", [](OptionInputs& in, double v) { in.spot = v; }},
        {"strike", [](OptionInputs& in, double v) { in.strike = v; }},
        {"time_to_expiry", [](OptionInputs& in, double v) { in.time_to_expiry = v; }},
        {"rate", [](OptionInputs& in, double v) { in.rate = v; }},
        {"volatility", [](OptionInputs& in, double v) { in.volatility = v; }},
    };

    for (const auto& field : fields) {
        for (double bad : {kNaN, kInf, -kInf}) {
            auto in = hull_example();
            field.set(in, bad);
            EXPECT_TRUE(rejected_for(in, field.name))
                << field.name << "=" << bad;
        }
    }
}
