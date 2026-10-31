# Trading costs, latency, and why the engine is C++

Notes on why hedging costs matter so much, and why market makers build pricing and risk
systems in C++. They tie the Hedging tab's transaction-cost study to how real trading
desks work.

## 1. What a "trading cost" is

At any moment a stock has two prices, not one:

| | Example (Reliance) | Meaning |
|---|---|---|
| **Bid** | ₹2,899.90 | The best price someone will pay you, so you **sell** at this |
| **Ask** | ₹2,900.10 | The best price someone will sell to you, so you **buy** at this |
| **Spread** | ₹0.20 | Ask − bid |

The price on a chart (₹2,900) is usually the middle, but trades don't happen there. A buy
pays half the spread above it, and a sell receives half the spread below it. Here that is
₹0.10 on ₹2,900, about **0.003% per trade**.

- **Tight spread:** bid and ask are close together. Large, liquid stocks have tight spreads;
  rarely traded stocks have wide ones (e.g. bid ₹99 / ask ₹101 costs about 1% per trade).
- **Fees:** brokerage, exchange fees and taxes (in India, STT) on top of the spread.

In the engine, `cost_rate` ≈ **half the spread + fees**, as a fraction of each trade's value.
Typical values are 0.01–0.05% for a liquid stock; 1% would be a very illiquid one.

## 2. Why small costs matter so much in hedging

Hedging trades far more in shares than the option is worth. On one simulated path of
Hull's 19.1 option (premium $300k), the hedge bought and sold about **$7.85M** of shares,
about 26× the premium:

- week 0: buy 52,160 shares × $49 ≈ $2.56M;
- the option expired worthless, so all of them were sold again, ≈ $2.3M;
- weekly adjustments in between made up the rest.

Every 0.1% of trading cost therefore took about **$7.85k** from a profit that was only
about $60k to begin with:

| Cost per trade | Fees on this path | Bank P&L |
|---|---|---|
| 0% | $0 | +$56.7k |
| 0.05% | ≈ $3.9k | ≈ +$52.8k |
| 0.1% | ≈ $7.9k | ≈ +$48.9k |
| 1% | $78.5k | −$21.3k |

With costs, rebalancing more often stops being free. The spread of outcomes falls, but
fees rise, so there is a sweet spot (Hull 19.10). At 0.1% per trade, over many paths:

| Rebalances | Mean hedge cost | Std dev | Mean bank P&L |
|---|---|---|---|
| 4 | $251k | $100k | +$49k |
| 20 (weekly) | $250k | $45k | +$50k |
| 80 | $257k | $24k | +$43k |
| 400 | $274k | $16k | +$26k |
| 2000 | $313k | $27k | −$13k |

## 3. Scenario: slippage, when the hedge arrives late

A client buys 1,000 Reliance call options from a market maker. Δ = 0.5, so the market maker
must buy 50,000 shares at once to hedge.

| Time | Market | What happens |
|---|---|---|
| t = 0 | Bid 2,899.90 / Ask **2,900.10** | The market maker decides to buy at about 2,900.10 |
| t = 1 ms | Ask 2,900.30 | A faster firm reacted to the same signal and bought first |
| t = 3 ms | Ask 2,900.60 | |
| **t = 5 ms** | Order arrives | It fills at **2,900.60** instead of 2,900.10 |

Loss: 0.50 × 50,000 = **₹25,000 on one hedge**, before any spread or fee. That gap between
the price seen and the price paid is **slippage**. A system that reacts in microseconds
trades before the price moves. Market makers hedge thousands of times a day, so these
small losses add up to millions.

## 4. Scenario: stale quotes, getting "picked off"

Market makers earn the spread by always showing a bid and an ask. Say one quotes a Reliance
call at **₹45.00 / ₹45.20** (Reliance at ₹2,900, implied vol 23.8%).

News hits and Reliance jumps to ₹2,920; the option is now worth about ₹50.

| System | Outcome |
|---|---|
| **Slow** (10 ms to re-price and update quotes) | For those 10 ms it still offers to sell at ₹45.20 an option worth ₹50. Fast traders buy immediately, and it loses about ₹4.80 per option. It has been **picked off** |
| **Fast** (microseconds) | Quotes move to about ₹49.90 / ₹50.10 before anyone can trade the old price |

A slow market maker has to quote **wider** spreads (e.g. ₹44.50 / ₹45.70) to cover the losses
from being picked off. Clients then trade with the faster firm that can safely quote
tighter. **Speed allows tighter quotes, and tighter quotes win the business.**

## 5. Why C++

On every tick of the underlying, a market maker's system must:

1. re-price every option on it: many strikes and expiries, calls and puts;
2. re-solve implied vols from market prices;
3. recompute the Greeks across the whole portfolio and aggregate them;
4. update its quotes and send hedge orders.

That is thousands of Black-Scholes evaluations per tick, all day long.

| Need | Why C++ |
|---|---|
| Raw speed | Compiled to native code with no interpreter; `price()` runs in well under a microsecond |
| Predictable timing | No garbage collector that can pause the program for milliseconds at the wrong moment |
| Memory control | Allocation is explicit (e.g. `reserve()` in the path simulator), so there are no surprise slowdowns |
| Close to the hardware | CPU caches, vector instructions and compact data layouts are all under the programmer's control |

C++ is one part of low latency. Firms also co-locate servers inside the exchange's data
centre and use specialised network cards, and sometimes FPGAs, for the fastest paths. But
the pricing and risk logic itself, which is what this engine implements (`price()`,
`greeks()`, `implied_vol()`), is almost always C++.
