#include "ope/black_scholes.hpp"
#include "ope/implied_vol.hpp"
#include "ope/option.hpp"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

using std::cerr;
using std::cout;
using std::exception;
using std::fixed;
using std::invalid_argument;
using std::map;
using std::max;
using std::setprecision;
using std::string;

namespace {

using ope::OptionInputs;
using ope::OptionType;

enum class Axis { Spot, Vol, Expiry };

void add_type_option(CLI::App& cmd, OptionType& type) {
    const map<string, OptionType> types{{"call", OptionType::Call}, {"put", OptionType::Put}};
    cmd.add_option("--type", type, "call or put")
        ->required()
        ->transform(CLI::CheckedTransformer(types, CLI::ignore_case));
}

void add_market_options(CLI::App& cmd, OptionInputs& in) {
    cmd.add_option("--spot", in.spot, "Spot price S")->required();
    cmd.add_option("--strike", in.strike, "Strike K")->required();
    cmd.add_option("--expiry", in.time_to_expiry, "Time to expiry T, in years")->required();
    cmd.add_option("--rate", in.rate, "Risk-free rate r, continuously compounded")->required();
}

double& axis_field(OptionInputs& in, Axis axis) {
    switch (axis) {
        case Axis::Spot:
            return in.spot;
        case Axis::Vol:
            return in.volatility;
        case Axis::Expiry:
            return in.time_to_expiry;
    }
    throw invalid_argument("unknown sweep axis");
}

// CSV: <axis>,call,put,call_payoff,put_payoff
void run_sweep(OptionInputs in, Axis axis, const string& axis_name, double from, double to,
               int steps) {
    cout << axis_name << ",call,put,call_payoff,put_payoff\n" << setprecision(10);
    for (int i = 0; i < steps; ++i) {
        axis_field(in, axis) = from + (to - from) * static_cast<double>(i) / (steps - 1);

        in.type = OptionType::Call;
        const double call = ope::price(in);
        in.type = OptionType::Put;
        const double put = ope::price(in);

        cout << axis_field(in, axis) << ',' << call << ',' << put << ','
             << max(in.spot - in.strike, 0.0) << ',' << max(in.strike - in.spot, 0.0) << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    CLI::App app{"Black-Scholes pricer for European options"};
    app.require_subcommand(1);

    OptionInputs in{OptionType::Call, 0.0, 0.0, 0.0, 0.0, 0.0};
    double market_price = 0.0;

    auto* price_cmd = app.add_subcommand("price", "Price a call or put");
    add_type_option(*price_cmd, in.type);
    add_market_options(*price_cmd, in);
    price_cmd->add_option("--vol", in.volatility, "Volatility sigma, annualised")->required();

    auto* iv_cmd = app.add_subcommand("iv", "Implied volatility from a market price");
    add_type_option(*iv_cmd, in.type);
    add_market_options(*iv_cmd, in);
    iv_cmd->add_option("--price", market_price, "Market price of the option")->required();

    const map<string, Axis> axes{{"spot", Axis::Spot}, {"vol", Axis::Vol}, {"expiry", Axis::Expiry}};
    Axis axis = Axis::Spot;
    string axis_name;
    double from = 0.0;
    double to = 0.0;
    int steps = 100;

    auto* sweep_cmd = app.add_subcommand("sweep", "CSV of call and put prices across a range");
    add_market_options(*sweep_cmd, in);
    auto* vol_opt = sweep_cmd->add_option("--vol", in.volatility, "Volatility sigma, annualised");
    sweep_cmd->add_option("--vary", axis_name, "Input to vary: spot, vol or expiry")
        ->required()
        ->check(CLI::IsMember({"spot", "vol", "expiry"}));
    sweep_cmd->add_option("--from", from, "Start of the range")->required();
    sweep_cmd->add_option("--to", to, "End of the range")->required();
    sweep_cmd->add_option("--steps", steps, "Number of points")->check(CLI::Range(2, 100000));

    CLI11_PARSE(app, argc, argv);

    try {
        if (*price_cmd) {
            cout << fixed << setprecision(6) << ope::price(in) << '\n';
        } else if (*iv_cmd) {
            cout << fixed << setprecision(6) << ope::implied_vol(in, market_price) << '\n';
        } else {
            axis = axes.at(axis_name);
            if (axis != Axis::Vol && vol_opt->count() == 0) {
                throw invalid_argument("--vol is required unless --vary vol");
            }
            run_sweep(in, axis, axis_name, from, to, steps);
        }
    } catch (const exception& e) {
        cerr << "error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
