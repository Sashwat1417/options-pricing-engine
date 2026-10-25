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

## Layout

One directory per layer. Each one has `main/` for code and `test/` for its tests.

```
black_scholes/                Layer 1 (target: ope::black_scholes)
├── main/include/ope/         public headers
├── main/src/                 implementation
└── test/                     GoogleTest unit tests
cmake/                        shared build helpers (warnings, sanitizers, GoogleTest)
docs/                         project atlas
```
