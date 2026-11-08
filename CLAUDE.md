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
2. **Options Pricing Engine** — ACTIVE. Layers 1 and 3 built, Layer 2 in progress (vanilla
   Monte Carlo + variance reduction done, exotics next); served over a REST API and web UI,
   deployed on Render. This repo.

## Architecture (fixed)
Three C++ layers. Python is used strictly for visualization — no business logic in Python.
The same rule covers the web UI (`server/web/`, JS): it reads inputs, calls the C++ REST API
(`ope-server`), and displays results; unit conversion is fine, pricing is not.

- **Layer 1 — Black-Scholes analytical pricer** (`black_scholes/`). *Done.* Closed-form price
  for vanilla European calls/puts, implied vol (bisection), all five Greeks (Δ Θ Γ ν ρ),
  sweeps. Tested against Hull Ch 15 and Ch 19 examples and the BS PDE identity.
- **Layer 2 — Monte Carlo simulation engine** (`monte_carlo/`). For exotic/path-dependent
  options with no closed form. *In progress.* Done: GBM path simulator; `mc_price` for
  European calls/puts with standard error and 95% interval (Hull 21.6), checked against
  Black-Scholes; variance reduction by antithetic variables and a control variate (discounted
  S_T, regression β) (Hull 21.7). Next: Asian, barrier, lookback (Hull Ch 26). Layer 2's goal
  (pricing what has no formula) is met only once these exist.
- **Layer 3 — Delta hedging simulator** (`hedging/`). *Done.* Hedges a sold call along a path;
  reproduces Hull Tables 19.1–19.3; realised vs implied vol (volatility trading); transaction
  costs (Hull 19.10).

## Where things stand
- Hull's *Options, Futures, and Other Derivatives* (11th Global Ed.): Chapters 1, 10–15 and
  19 done, plus 21.6 (Monte Carlo simulation) and 21.7 (variance reduction).
- Roadmap, in order:
  1. Layer 2 exotics: Asian (geometric Asian as control variate), then barrier, then lookback.
     Read the matching Ch 26 section before each.
  2. Release v0.6.0 (`develop` has the Monte Carlo pricer and variance reduction unreleased);
     add Layer 2 to the atlas and a Layer 2 HLD/LLD page in Confluence.
  3. Portfolio Greeks: many positions across strikes/expiries, Greeks aggregated and
     recomputed as the market moves (see the first domain note). Not in any layer yet.
  4. Wilmott, then the Q_Resume revision, then applying.
- Built around the layers: `cli/` (`ope` tool), `server/` (`ope-server` REST API + web UI with
  Pricing, Greeks, Implied vol, Hedging and Monte Carlo tabs), `viz/` (Python charts), Docker,
  Kubernetes manifest, GitHub Actions CI, Render deployment (usually kept suspended; test
  locally).
- Git flow: `feature/*` → `develop` (squash PR) → `main` (release PR, merge commit, version tag
  in `CMakeLists.txt`). Never delete `develop`.
- Working principle: when implementation hits a conceptual wall (e.g. "why N(d1) here and
  not N(d2)"), that's a targeted signal to go re-read that specific bit of Hull — not a reason
  to pause and reread the whole book.
- Paul Wilmott's *Introduces Quantitative Finance* is queued after Hull, before full build-out.

## Key domain notes (don't re-derive these)
- From a quant firm's perspective, the real intellectual problem is inverting market prices to
  extract implied volatility and aggregating Greeks across a portfolio in real time — not just
  computing a price given σ. Keep this in mind as Layer 1 evolves.
- Delta hedging: the average hedging cost is the BS value whatever μ is; its spread falls as
  1/√(rebalances). Hedging at implied vol while the stock moves at realised vol makes the P&L
  depend on realised vs implied vol, not direction. Trading costs scale with the shares traded
  (many times the option's value), so they create an optimal rebalancing frequency.
- Black-Scholes prices in a risk-neutral world where μ (real expected return) vanishes
  entirely. f(S,t) solves the BS PDE with a terminal payoff condition — swap that condition to
  generalize to other derivatives.
- Closed-form solutions exist only for simple/vanilla payoffs; path-dependency requires Monte
  Carlo. This is the exact Layer 1 / Layer 2 boundary.
- Monte Carlo prices simulate at the risk-neutral drift r, never μ. Standard error = ω/√M
  (ω: sd of discounted payoffs), so accuracy costs paths quadratically. Variance reduction
  shrinks ω instead: speedup = (plain SE / method SE)², the plain paths one path is worth.
  The control-variate β is a regression slope (≈ delta for a call: a static hedge); Hull's
  f_A − f_B + f_B(exact) is the β = 1 case. Both methods help most in the money, ≈ 1× deep
  out of the money (importance-sampling territory).

## How to work with Sashwat on this
- Explain the "why" before implementation — he dislikes black-box formulas in code.
- Plain-language explanation + concrete analogy before the math/formula.
- He drives the pace — don't tack on "ready to move to the next part?" prompts.
- Use implementation friction itself as the signal for what to re-explain, rather than
  requiring full theory mastery before writing code.
- Build in C++17 to mirror production quant system architecture; Python only for plotting/viz.

## Reference
- Primary text: Hull, *Options, Futures, and Other Derivatives*, 11th Global Edition.
- Confluence (Atlassian Rovo, site sashwatpratapsinghcoc.atlassian.net): Space ID 131074,
  Cloud ID 87de896b-114b-45e8-b55d-40f59f3c015e.
  Order Book parent page 327681, Order Book HLD/LLD page 393217.
  Options Pricing Engine parent page 425985, Layer 1 HLD/LLD page 327701,
  Layer 3 HLD/LLD page 1540097, Server/Web UI/Deployment HLD/LLD page 1507330.
- Notes: `docs/notes/` (e.g. trading costs, latency and why C++).