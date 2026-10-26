#include "ope/black_scholes.hpp"
#include "ope/implied_vol.hpp"
#include "ope/option.hpp"
#include "ope/sweep.hpp"

#include <CLI/CLI.hpp>

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
using std::setprecision;
using std::string;

namespace {

using ope::OptionInputs;
using ope::OptionType;
using ope::SweepAxis;

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

// CSV: <axis>,call,put,call_payoff,put_payoff
void print_sweep(const OptionInputs& in, SweepAxis axis, const string& axis_name, double from,
                 double to, int steps) {
    cout << axis_name << ",call,put,call_payoff,put_payoff\n" << setprecision(10);
    for (const auto& p : ope::sweep(in, axis, from, to, steps)) {
        cout << p.x << ',' << p.call << ',' << p.put << ',' << p.call_payoff << ','
             << p.put_payoff << '\n';
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

    const map<string, SweepAxis> axes{
        {"spot", SweepAxis::Spot}, {"vol", SweepAxis::Vol}, {"expiry", SweepAxis::Expiry}};
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
            const SweepAxis axis = axes.at(axis_name);
            if (axis != SweepAxis::Vol && vol_opt->count() == 0) {
                throw invalid_argument("--vol is required unless --vary vol");
            }
            print_sweep(in, axis, axis_name, from, to, steps);
        }
    } catch (const exception& e) {
        cerr << "error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
