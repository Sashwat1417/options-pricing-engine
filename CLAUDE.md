# Options Pricing Engine — Project Context

## Who's building this
Sashwat — SDE I at Meesho (backend, Java/Spring Boot/Kafka/Redis/MySQL), IIT Roorkee ECE grad,
Codeforces Candidate Master / CodeChef 5★. Building a quant developer portfolio targeting
Jane Street, Optiver, IMC, Citadel/Citadel Securities, and Two Sigma.

## Portfolio strategy
Two flagship projects, then one clean resume revision (Q_Resume) incorporating both, before
actively applying.
1. **Order Book** — COMPLETE. C++17, REST API (cpp-httplib), MongoDB (replica set), Kafka,
   Redis dedup (SET NX PX + in-memory fallback), transactional outbox + change streams,
   price-time priority matching, integer price keys, post-seed match sweep for crash recovery,
   Gmail SMTP notification service. Documented in Confluence (HLD/LLD + Kafka Concepts doc).
2. **Options Pricing Engine** — ACTIVE, currently starting implementation. This repo.

## Architecture (fixed)
Three C++ layers. Python is used strictly for visualization — no business logic in Python.

- **Layer 1 — Black-Scholes analytical pricer.** Closed-form f(S,t) for vanilla European
  calls/puts. *Currently being implemented.*
- **Layer 2 — Monte Carlo simulation engine.** For exotic/path-dependent options with no
  closed form. Blocked on Hull Ch 21 (Monte Carlo sections).
- **Layer 3 — Delta hedging simulator.** Simulates a market maker's hedge and resulting P&L.
  Blocked on Hull Ch 19 (Greeks).

## Where things stand
- Hull's *Options, Futures, and Other Derivatives* (11th Global Ed.): Chapters 1, 10–15 done.
  Ch 19 and the Monte Carlo part of Ch 21 remain, being read in parallel with Layer 1 build —
  not a gate on Layer 1 anymore.
- Layer 1 scope: implement closed-form Black-Scholes pricing for vanilla calls/puts.
- Working principle: when implementation hits a conceptual wall (e.g. "why N(d1) here and
  not N(d2)"), that's a targeted signal to go re-read that specific bit of Hull — not a reason
  to pause and reread the whole book.
- Paul Wilmott's *Introduces Quantitative Finance* is queued after Hull, before full build-out.

## Key domain notes (don't re-derive these)
- From a quant firm's perspective, the real intellectual problem is inverting market prices to
  extract implied volatility and aggregating Greeks across a portfolio in real time — not just
  computing a price given σ. Keep this in mind as Layer 1 evolves.
- Black-Scholes prices in a risk-neutral world where μ (real expected return) vanishes
  entirely. f(S,t) solves the BS PDE with a terminal payoff condition — swap that condition to
  generalize to other derivatives.
- Closed-form solutions exist only for simple/vanilla payoffs; path-dependency requires Monte
  Carlo. This is the exact Layer 1 / Layer 2 boundary.

## How to work with Sashwat on this
- Explain the "why" before implementation — he dislikes black-box formulas in code.
- Plain-language explanation + concrete analogy before the math/formula.
- He drives the pace — don't tack on "ready to move to the next part?" prompts.
- Use implementation friction itself as the signal for what to re-explain, rather than
  requiring full theory mastery before writing code.
- Build in C++17 to mirror production quant system architecture; Python only for plotting/viz.

## Reference
- Primary text: Hull, *Options, Futures, and Other Derivatives*, 11th Global Edition.
- Confluence (Atlassian Rovo): Space ID 131074, Cloud ID c6ef0bc1-03cd-4dfa-8a10-dfc1b6f87adc.
  Order Book HLD/LLD page 491521, Kafka Concepts 393246, Order Book parent page 393218.