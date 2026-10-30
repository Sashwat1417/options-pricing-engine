#include "ope/greeks.hpp"

#include "ope/black_scholes.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

using std::exp;
using std::invalid_argument;

namespace {

using ope::greeks;
using ope::OptionInputs;
using ope::OptionType;

// Hull Ch 19 example: S=49, K=50, T=20 weeks, r=5%, σ=20%.
OptionInputs hull_ch19(OptionType type = OptionType::Call) {
    return {type, 49.0, 50.0, 0.3846, 0.05, 0.20};
}

OptionInputs with_spot(OptionInputs in, double spot) {
    in.spot = spot;
    return in;
}

OptionInputs with_expiry(OptionInputs in, double t) {
    in.time_to_expiry = t;
    return in;
}

// θ = ∂V/∂t = −∂V/∂T: time passing shrinks T.
double finite_difference_theta(const OptionInputs& in) {
    const double h = 1e-5;
    const double t = in.time_to_expiry;
    return -(ope::price(with_expiry(in, t + h)) - ope::price(with_expiry(in, t - h))) / (2.0 * h);
}

// Γ as the slope of the delta curve.
double finite_difference_gamma(const OptionInputs& in) {
    const double h = 1e-4 * in.spot;
    return (greeks(with_spot(in, in.spot + h)).delta - greeks(with_spot(in, in.spot - h)).delta) /
           (2.0 * h);
}

OptionInputs with_vol(OptionInputs in, double vol) {
    in.volatility = vol;
    return in;
}

// ν as the slope of price against σ.
double finite_difference_vega(const OptionInputs& in) {
    const double h = 1e-5;
    return (ope::price(with_vol(in, in.volatility + h)) -
            ope::price(with_vol(in, in.volatility - h))) /
           (2.0 * h);
}

OptionInputs with_rate(OptionInputs in, double r) {
    in.rate = r;
    return in;
}

// ρ as the slope of price against r.
double finite_difference_rho(const OptionInputs& in) {
    const double h = 1e-6;
    return (ope::price(with_rate(in, in.rate + h)) - ope::price(with_rate(in, in.rate - h))) /
           (2.0 * h);
}

// Δ as the literal slope of the price curve.
double finite_difference_delta(const OptionInputs& in) {
    const double h = 1e-4 * in.spot;
    return (ope::price(with_spot(in, in.spot + h)) - ope::price(with_spot(in, in.spot - h))) /
           (2.0 * h);
}

}  // namespace

TEST(Delta, MatchesHullCh19Example) {
    EXPECT_NEAR(greeks(hull_ch19()).delta, 0.522, 5e-4);
    EXPECT_NEAR(greeks(hull_ch19()).delta, 0.5216016339715761, 1e-12);
    EXPECT_NEAR(greeks(hull_ch19(OptionType::Put)).delta, -0.4783983660284239, 1e-12);
}

TEST(Delta, IsSlopeOfPriceCurve) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        for (double s : {30.0, 45.0, 50.0, 55.0, 80.0}) {
            for (double t : {0.05, 0.5, 2.0}) {
                for (double vol : {0.1, 0.3, 0.8}) {
                    const OptionInputs in{type, s, 50.0, t, 0.05, vol};
                    EXPECT_NEAR(greeks(in).delta, finite_difference_delta(in), 1e-6)
                        << (type == OptionType::Call ? "call" : "put") << " S=" << s
                        << " T=" << t << " vol=" << vol;
                }
            }
        }
    }
}

TEST(Delta, CallMinusPutIsOne) {
    for (double s : {30.0, 49.0, 50.0, 70.0}) {
        const double call = greeks(with_spot(hull_ch19(OptionType::Call), s)).delta;
        const double put = greeks(with_spot(hull_ch19(OptionType::Put), s)).delta;
        EXPECT_NEAR(call - put, 1.0, 1e-15) << "S=" << s;
    }
}

TEST(Delta, StaysWithinBounds) {
    for (double s = 5.0; s <= 150.0; s += 5.0) {
        const double call = greeks(with_spot(hull_ch19(OptionType::Call), s)).delta;
        const double put = greeks(with_spot(hull_ch19(OptionType::Put), s)).delta;
        EXPECT_GE(call, 0.0) << "S=" << s;
        EXPECT_LE(call, 1.0) << "S=" << s;
        EXPECT_GE(put, -1.0) << "S=" << s;
        EXPECT_LE(put, 0.0) << "S=" << s;
    }
}

TEST(Delta, RisesWithSpot) {
    double previous = -1.0;
    for (double s = 30.0; s <= 70.0; s += 2.0) {
        const double d = greeks(with_spot(hull_ch19(), s)).delta;
        EXPECT_GT(d, previous) << "S=" << s;
        previous = d;
    }
}

TEST(Delta, DeepInAndOutOfTheMoney) {
    EXPECT_NEAR(greeks(with_spot(hull_ch19(), 200.0)).delta, 1.0, 1e-12);
    EXPECT_NEAR(greeks(with_spot(hull_ch19(), 5.0)).delta, 0.0, 1e-12);
    EXPECT_NEAR(greeks(with_spot(hull_ch19(OptionType::Put), 200.0)).delta, 0.0, 1e-12);
    EXPECT_NEAR(greeks(with_spot(hull_ch19(OptionType::Put), 5.0)).delta, -1.0, 1e-12);
}

TEST(Delta, IsStepAtExpiryAndZeroVol) {
    for (auto field : {0, 1}) {
        auto itm = with_spot(hull_ch19(), 55.0);
        auto otm = with_spot(hull_ch19(), 45.0);
        (field == 0 ? itm.time_to_expiry : itm.volatility) = 0.0;
        (field == 0 ? otm.time_to_expiry : otm.volatility) = 0.0;
        EXPECT_DOUBLE_EQ(greeks(itm).delta, 1.0);
        EXPECT_DOUBLE_EQ(greeks(otm).delta, 0.0);
        itm.type = OptionType::Put;
        otm.type = OptionType::Put;
        EXPECT_DOUBLE_EQ(greeks(itm).delta, 0.0);
        EXPECT_DOUBLE_EQ(greeks(otm).delta, -1.0);
    }
}

TEST(Delta, ThrowsOnInvalidInputs) {
    auto in = hull_ch19();
    in.spot = -1.0;
    EXPECT_THROW((void)greeks(in), invalid_argument);
}

TEST(Theta, MatchesHullCh19Example) {
    EXPECT_NEAR(greeks(hull_ch19()).theta, -4.31, 5e-3);
    EXPECT_NEAR(greeks(hull_ch19()).theta, -4.305389964546104, 1e-11);
    EXPECT_NEAR(greeks(hull_ch19(OptionType::Put)).theta, -1.853005672196868, 1e-11);
}

TEST(Theta, IsRateOfChangeAsTimePasses) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        for (double s : {30.0, 45.0, 50.0, 55.0, 80.0}) {
            for (double t : {0.05, 0.5, 2.0}) {
                for (double vol : {0.1, 0.3, 0.8}) {
                    const OptionInputs in{type, s, 50.0, t, 0.05, vol};
                    EXPECT_NEAR(greeks(in).theta, finite_difference_theta(in), 1e-4)
                        << (type == OptionType::Call ? "call" : "put") << " S=" << s
                        << " T=" << t << " vol=" << vol;
                }
            }
        }
    }
}

TEST(Theta, CallMinusPutIsInterestOnStrike) {
    for (double s : {30.0, 49.0, 50.0, 70.0}) {
        const auto call = with_spot(hull_ch19(OptionType::Call), s);
        const auto put = with_spot(hull_ch19(OptionType::Put), s);
        const double expected = -call.rate * call.strike * exp(-call.rate * call.time_to_expiry);
        EXPECT_NEAR(greeks(call).theta - greeks(put).theta, expected, 1e-12) << "S=" << s;
    }
}

TEST(Theta, LongCallDecaysNearTheMoney) {
    EXPECT_LT(greeks(hull_ch19()).theta, 0.0);
    const double at_the_money = greeks(with_spot(hull_ch19(), 50.0)).theta;
    const double far_out = greeks(with_spot(hull_ch19(), 30.0)).theta;
    EXPECT_LT(at_the_money, far_out);
}

TEST(Theta, DeepOutOfTheMoneyIsNearZero) {
    EXPECT_NEAR(greeks(with_spot(hull_ch19(), 5.0)).theta, 0.0, 1e-12);
    EXPECT_NEAR(greeks(with_spot(hull_ch19(OptionType::Put), 200.0)).theta, 0.0, 1e-12);
}

TEST(Theta, IsInterestOnStrikeAtZeroVol) {
    auto itm_call = with_spot(hull_ch19(), 55.0);
    itm_call.volatility = 0.0;
    const double kdf = itm_call.strike * exp(-itm_call.rate * itm_call.time_to_expiry);
    EXPECT_DOUBLE_EQ(greeks(itm_call).theta, -itm_call.rate * kdf);
    EXPECT_NEAR(greeks(itm_call).theta, finite_difference_theta(itm_call), 1e-6);

    auto otm_call = with_spot(itm_call, 45.0);
    EXPECT_DOUBLE_EQ(greeks(otm_call).theta, 0.0);

    auto itm_put = otm_call;
    itm_put.type = OptionType::Put;
    EXPECT_DOUBLE_EQ(greeks(itm_put).theta, itm_put.rate * kdf);
}

TEST(Gamma, MatchesHullCh19Example) {
    EXPECT_NEAR(greeks(hull_ch19()).gamma, 0.066, 5e-4);
    EXPECT_NEAR(greeks(hull_ch19()).gamma, 0.06554537725247868, 1e-12);
}

TEST(Gamma, IsSlopeOfDeltaCurve) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        for (double s : {30.0, 45.0, 50.0, 55.0, 80.0}) {
            for (double t : {0.05, 0.5, 2.0}) {
                for (double vol : {0.1, 0.3, 0.8}) {
                    const OptionInputs in{type, s, 50.0, t, 0.05, vol};
                    // Relative tolerance: the finite difference's error grows with Γ itself.
                    const double gamma = greeks(in).gamma;
                    EXPECT_NEAR(gamma, finite_difference_gamma(in), 1e-6 + 1e-5 * gamma)
                        << (type == OptionType::Call ? "call" : "put") << " S=" << s
                        << " T=" << t << " vol=" << vol;
                }
            }
        }
    }
}

TEST(Gamma, IsSameForCallAndPut) {
    for (double s : {30.0, 49.0, 50.0, 70.0}) {
        EXPECT_DOUBLE_EQ(greeks(with_spot(hull_ch19(OptionType::Call), s)).gamma,
                         greeks(with_spot(hull_ch19(OptionType::Put), s)).gamma)
            << "S=" << s;
    }
}

TEST(Gamma, IsPositiveAndPeaksNearTheMoney) {
    double peak_spot = 0.0;
    double peak = -1.0;
    for (double s = 20.0; s <= 100.0; s += 0.5) {
        const double g = greeks(with_spot(hull_ch19(), s)).gamma;
        EXPECT_GT(g, 0.0) << "S=" << s;
        if (g > peak) {
            peak = g;
            peak_spot = s;
        }
    }
    EXPECT_NEAR(peak_spot, 50.0, 3.0);
}

TEST(Gamma, DeepInAndOutOfTheMoneyIsNearZero) {
    EXPECT_NEAR(greeks(with_spot(hull_ch19(), 5.0)).gamma, 0.0, 1e-12);
    EXPECT_NEAR(greeks(with_spot(hull_ch19(), 200.0)).gamma, 0.0, 1e-12);
}

TEST(Gamma, GrowsNearTheMoneyAsExpiryApproaches) {
    const auto at_the_money = with_spot(hull_ch19(), 50.0);
    EXPECT_GT(greeks(with_expiry(at_the_money, 0.02)).gamma,
              greeks(with_expiry(at_the_money, 1.0)).gamma);
}

TEST(Gamma, IsZeroAtZeroVol) {
    auto in = with_spot(hull_ch19(), 55.0);
    in.volatility = 0.0;
    EXPECT_DOUBLE_EQ(greeks(in).gamma, 0.0);
}

TEST(Vega, MatchesHullCh19Example) {
    EXPECT_NEAR(greeks(hull_ch19()).vega / 100.0, 0.121, 5e-4);
    EXPECT_NEAR(greeks(hull_ch19()).vega, 12.105242754243843, 1e-10);
}

TEST(Vega, IsSlopeOfPriceAgainstVolatility) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        for (double s : {30.0, 45.0, 50.0, 55.0, 80.0}) {
            for (double t : {0.05, 0.5, 2.0}) {
                for (double vol : {0.1, 0.3, 0.8}) {
                    const OptionInputs in{type, s, 50.0, t, 0.05, vol};
                    const double vega = greeks(in).vega;
                    EXPECT_NEAR(vega, finite_difference_vega(in), 1e-5 + 1e-6 * vega)
                        << (type == OptionType::Call ? "call" : "put") << " S=" << s
                        << " T=" << t << " vol=" << vol;
                }
            }
        }
    }
}

TEST(Vega, IsSameForCallAndPut) {
    for (double s : {30.0, 49.0, 50.0, 70.0}) {
        EXPECT_DOUBLE_EQ(greeks(with_spot(hull_ch19(OptionType::Call), s)).vega,
                         greeks(with_spot(hull_ch19(OptionType::Put), s)).vega)
            << "S=" << s;
    }
}

TEST(Vega, IsPositiveAndGrowsWithTimeAtTheMoney) {
    for (double s = 20.0; s <= 100.0; s += 5.0) {
        EXPECT_GT(greeks(with_spot(hull_ch19(), s)).vega, 0.0) << "S=" << s;
    }
    const auto at_the_money = with_spot(hull_ch19(), 50.0);
    EXPECT_LT(greeks(with_expiry(at_the_money, 0.05)).vega,
              greeks(with_expiry(at_the_money, 1.0)).vega);
}

TEST(Vega, DeepInAndOutOfTheMoneyIsNearZero) {
    EXPECT_NEAR(greeks(with_spot(hull_ch19(), 5.0)).vega, 0.0, 1e-10);
    EXPECT_NEAR(greeks(with_spot(hull_ch19(), 200.0)).vega, 0.0, 1e-10);
}

TEST(Vega, IsZeroAtZeroVol) {
    EXPECT_DOUBLE_EQ(greeks(with_vol(hull_ch19(), 0.0)).vega, 0.0);
}

TEST(Rho, MatchesHullCh19Example) {
    EXPECT_NEAR(greeks(hull_ch19()).rho / 100.0, 0.0891, 5e-5);
    EXPECT_NEAR(greeks(hull_ch19()).rho, 8.906574098800945, 1e-10);
    EXPECT_NEAR(greeks(hull_ch19(OptionType::Put)).rho, -9.95716587794938, 1e-10);
}

TEST(Rho, IsSlopeOfPriceAgainstRate) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        for (double s : {30.0, 45.0, 50.0, 55.0, 80.0}) {
            for (double t : {0.05, 0.5, 2.0}) {
                for (double vol : {0.1, 0.3, 0.8}) {
                    const OptionInputs in{type, s, 50.0, t, 0.05, vol};
                    EXPECT_NEAR(greeks(in).rho, finite_difference_rho(in), 1e-5)
                        << (type == OptionType::Call ? "call" : "put") << " S=" << s
                        << " T=" << t << " vol=" << vol;
                }
            }
        }
    }
}

TEST(Rho, CallMinusPutIsDiscountedStrikeTimesExpiry) {
    for (double s : {30.0, 49.0, 50.0, 70.0}) {
        const auto call = with_spot(hull_ch19(OptionType::Call), s);
        const auto put = with_spot(hull_ch19(OptionType::Put), s);
        const double expected =
            call.strike * call.time_to_expiry * exp(-call.rate * call.time_to_expiry);
        EXPECT_NEAR(greeks(call).rho - greeks(put).rho, expected, 1e-10) << "S=" << s;
    }
}

TEST(Rho, CallPositivePutNegative) {
    for (double s = 20.0; s <= 100.0; s += 5.0) {
        EXPECT_GT(greeks(with_spot(hull_ch19(), s)).rho, 0.0) << "S=" << s;
        EXPECT_LT(greeks(with_spot(hull_ch19(OptionType::Put), s)).rho, 0.0) << "S=" << s;
    }
}

TEST(Rho, GrowsWithTimeToExpiry) {
    const auto at_the_money = with_spot(hull_ch19(), 50.0);
    EXPECT_LT(greeks(with_expiry(at_the_money, 0.1)).rho,
              greeks(with_expiry(at_the_money, 2.0)).rho);
}

TEST(Rho, IsDiscountedStrikeTimesExpiryWhenInTheMoneyAtZeroVol) {
    auto itm_call = with_vol(with_spot(hull_ch19(), 55.0), 0.0);
    const double kt_df =
        itm_call.strike * itm_call.time_to_expiry * exp(-itm_call.rate * itm_call.time_to_expiry);
    EXPECT_DOUBLE_EQ(greeks(itm_call).rho, kt_df);
    EXPECT_NEAR(greeks(itm_call).rho, finite_difference_rho(itm_call), 1e-6);

    auto otm_call = with_spot(itm_call, 45.0);
    EXPECT_DOUBLE_EQ(greeks(otm_call).rho, 0.0);

    auto itm_put = otm_call;
    itm_put.type = OptionType::Put;
    EXPECT_DOUBLE_EQ(greeks(itm_put).rho, -kt_df);
}

// Hull 19.7: the Black-Scholes PDE written in Greeks, Θ + rSΔ + ½σ²S²Γ = rV.
TEST(GreeksRelationship, SatisfiesBlackScholesPde) {
    for (auto type : {OptionType::Call, OptionType::Put}) {
        for (double s : {30.0, 45.0, 50.0, 55.0, 80.0}) {
            for (double t : {0.05, 0.5, 2.0}) {
                for (double r : {0.0, 0.05, 0.10}) {
                    for (double vol : {0.1, 0.3, 0.8}) {
                        const OptionInputs in{type, s, 50.0, t, r, vol};
                        const auto g = greeks(in);
                        const double lhs =
                            g.theta + r * s * g.delta + 0.5 * vol * vol * s * s * g.gamma;
                        EXPECT_NEAR(lhs, r * ope::price(in), 1e-9)
                            << (type == OptionType::Call ? "call" : "put") << " S=" << s
                            << " T=" << t << " r=" << r << " vol=" << vol;
                    }
                }
            }
        }
    }
}
