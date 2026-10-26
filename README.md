# options-pricing-engine
Multi-layer C++ options pricing engine: closed-form Black-Scholes, Monte Carlo simulation, and delta-hedging P&amp;L, with Python visualisation.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/price_vs_spot-dark.png">
  <img alt="Call and put price today vs payoff at expiry, Hull Ch 15 example" src="docs/images/price_vs_spot.png">
</picture>

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/time_decay-dark.png">
  <img alt="Call price vs spot for 1 year, 3 months, 1 month and 1 day to expiry" src="docs/images/time_decay.png">
</picture>

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/price_vs_vol-dark.png">
  <img alt="Call and put price rising with volatility" src="docs/images/price_vs_vol.png">
</picture>

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

## REST API

```sh
build/debug/server/ope-server            # listens on $PORT, default 8080

curl localhost:8080/health
# {"status":"ok"}

curl localhost:8080/api/v1/price -H 'Content-Type: application/json' \
     -d '{"type":"call","spot":42,"strike":40,"expiry":0.5,"rate":0.1,"vol":0.2}'
# {"price":4.759422392871535}
```

| Endpoint | Body | Response |
|---|---|---|
| `GET /health` | | `{"status":"ok"}` |
| `POST /api/v1/price` | `type, spot, strike, expiry, rate, vol` | `{"price": …}` |
| `POST /api/v1/iv` | `type, spot, strike, expiry, rate, price` | `{"implied_vol": …}` |
| `POST /api/v1/sweep` | `spot, strike, expiry, rate, vol, vary, from, to, steps` (`vol` not needed when `vary` is `vol`) | arrays `x, call, put, call_payoff, put_payoff` |

Bad input returns `400 {"error": "..."}`.

## Charts

Python only draws: `viz/plot.py` runs `ope sweep` and plots the CSV it returns.

```sh
python3 -m venv viz/.venv && viz/.venv/bin/pip install -r viz/requirements.txt
viz/.venv/bin/python viz/plot.py          # writes docs/images/*.png (light + dark)
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
server/                       `ope-server` REST API
├── main/src/api/             PricingController: routes, JSON ⇄ DTOs, HTTP status
├── main/src/service/         PricingService: calls ope::black_scholes
├── main/src/request/         PriceRequest, IvRequest, SweepRequest (+ JsonReader)
├── main/src/response/        PriceResponse, IvResponse, SweepResponse, ErrorResponse
└── test/                     request/ and service/ unit tests
cmake/                        shared build helpers (warnings, sanitizers, GoogleTest, CLI11, httplib, json)
viz/                          Python plotting (draws only, no pricing logic)
docs/images/                  generated charts
```
