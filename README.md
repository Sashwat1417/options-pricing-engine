# options-pricing-engine
Multi-layer C++ options pricing engine: closed-form Black-Scholes, Monte Carlo simulation, and delta-hedging P&amp;L, with Python visualisation.

## Build and test

Requires CMake ≥ 3.25 and a C++17 compiler. GoogleTest is downloaded automatically.

```sh
# configure + build + test in one go (debug, warnings as errors, sanitizers on)
cmake --workflow --preset debug

# or step by step
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

# optimised build
cmake --preset release && cmake --build --preset release && ctest --preset release
```

## Command-line tool

After building, the `ope` binary is at `build/debug/cli/ope` (or `build/release/cli/ope`).

```sh
# price a call (Hull Ch 15 example)
ope price --type call --spot 42 --strike 40 --expiry 0.5 --rate 0.1 --vol 0.2
# 4.759422

# implied volatility from a market price
ope iv --type call --spot 42 --strike 40 --expiry 0.5 --rate 0.1 --price 4.76
# 0.200066

# CSV of call and put prices across a range of spot, vol or expiry
ope sweep --spot 42 --strike 40 --expiry 0.5 --rate 0.1 --vol 0.2 --vary spot --from 20 --to 60 --steps 200
```

## Layout

One directory per layer. Each one has `main/` for code and `test/` for its tests.

```
black_scholes/                Layer 1 (target: ope::black_scholes)
├── main/include/ope/         public headers
├── main/src/                 implementation
└── test/                     GoogleTest unit tests
cli/                          `ope` command-line tool
├── main/src/
└── test/                     smoke tests that run the binary
cmake/                        shared build helpers (warnings, sanitizers, GoogleTest, CLI11)
docs/                         project atlas
```
