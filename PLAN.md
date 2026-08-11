# Options Income & Compounding Strategy Estimator (C++)

A C++ project for estimating and planning a **covered call / cash-secured put "wheel"** style
options-income strategy that **reinvests premiums over time (compounding)**, built on top of the
[`embedded-pro/numerical-toolbox-cpp`](https://github.com/embedded-pro/numerical-toolbox-cpp)
numerical library, with **Dutch (Netherlands) taxation** and **DEGIRO** cost modeling baked in.

> ⚠️ **Not financial or tax advice.** This is a modeling/estimation tool. All tax logic must be
> validated against current Belastingdienst rules for your tax year before use.

---

## 0. Clarifications & Assumptions (read this first)

Everything below is an **explicit assumption** so nothing is hidden. Adjust in `config/` before building.

### 0.1 What "cover sell" means here
I'm interpreting *"cover sell for options"* as **covered calls** (selling call options against 100
shares you own) and — since you want to *increase investment over time* — the full **Wheel strategy**:

1. Sell **cash-secured puts** → collect premium; if assigned, you buy 100 shares.
2. Once you own shares, sell **covered calls** → collect premium; if assigned, you sell the shares.
3. **Reinvest** all net premiums + capital into more contracts (compounding).

If you actually meant something else (e.g. naked options, spreads, LEAPS), flag it — the engine is
modular enough to extend, but the initial scope is **Wheel = CSP + Covered Call only**.

### 0.2 The library reality check (IMPORTANT)
`numerical-toolbox-cpp` is an **embedded DSP / control / estimation** library. It has **no built-in
options pricing, no Black–Scholes, no finance primitives**. What it *does* give us that's directly
useful:

| Need in this project | Library component we'll use |
|---|---|
| Fit implied volatility / drift from historical prices | `Estimators → Linear/Polynomial Regression`, `Total Least Squares` |
| Online volatility / return tracking from a price stream | `Estimators → Recursive Least Squares`, `Filters → Kalman / EMA` |
| Smooth noisy price series before estimation | `Filters → Exponential Moving Average`, `FIR/IIR` |
| Optimize contract count / strike selection to a target | `Optimization → Gradient Descent`, `Regularization → L1/L2` |
| Numerical CDF (Φ) for Black–Scholes via `Erfc` | `math::Erfc` (already in the library) |
| Monte-Carlo path integration of price SDE | `Solvers → Runge-Kutta integrators` |
| Matrix/linear algebra for the fits | `Solvers → Cholesky, Gaussian Elimination, SVD` |

Concrete toolbox entry points we will link against (verified against the pinned `v3.0.0` tree):
`numerical.math` (`numerical/math/Math.hpp` → `math::Erfc`, `Matrix.hpp`, `Statistics.hpp`),
`numerical.estimators` (`estimators/offline/LinearRegression.hpp`, `TotalLeastSquares.hpp`,
`estimators/online/RecursiveLeastSquares.hpp`), `numerical.filters`,
`numerical.solver` (`solvers/RungeKuttaIntegrators.hpp`, `DormandPrince45.hpp`, `OdeSystem.hpp`),
`numerical.optimization` (`optimization/GradientDescent.hpp`, `ObjectiveFunction.hpp`) and
`numerical.regularization`.

**Everything finance-specific (option pricing, Greeks, assignment logic, tax, DEGIRO fees) we build
ourselves** as a new library layer *on top* of the toolbox. This plan makes that split explicit.

### 0.3 Environment & tooling assumptions
- **Language/standard:** C++20, CMake ≥ 3.24, GoogleTest (matches the toolbox's conventions).
- **Dependency wiring:** both dependencies are **fetched by CMake** at configure time, each pinned to
  an exact commit: `numerical-toolbox-cpp` (at `v3.0.0`) and `embedded-infra-lib` (emil). Nothing is
  vendored and there are no submodules, so a plain `git clone` is enough. Because the toolbox only
  `FetchContent`s emil when it is the *top-level* project, this repo declares and makes emil
  available **first**, at the same pin the toolbox uses; that pin has to be kept in step with the
  toolbox's own.
- **Numeric types:** the toolbox is `float`-only. Our layer uses `double` for rates, volatilities
  and price paths, and **integer cents** for every cash amount; `float` appears only at the call
  boundary into a toolbox algorithm. See §6.4 and
  [decisions D1/D3](documentation/design/decisions.md).
- **Platform:** desktop Linux/macOS/Windows (host build), *not* an MCU — so the toolbox's `no-heap`
  constraint is relaxed for our layer, but we'll still prefer bounded containers where cheap.
- **JSON/CSV:** parsing uses emil's `infra/syntax/Json.hpp` (already on the dependency graph) — no
  new third-party parser is introduced.
- **Data:** you provide historical price CSVs and an options chain snapshot (CSV/JSON). **No live
  broker API integration in v1** — DEGIRO has no official retail API. See §7.
- **Currency:** EUR base. If trading USD-denominated underlyings (e.g. US ETFs), FX is modeled.

### 0.4 Tax assumptions (Netherlands) — must be confirmed per tax year
- Private retail investor (natuurlijk persoon), **not** a business (`resultaat uit overige
  werkzaamheden` / self-employment is out of scope).
- Investments are taxed under **Box 3 (belastbaar inkomen uit sparen en beleggen)** — a *wealth/
  deemed-return* tax on the value of assets on the reference date (1 January, "peildatum"), **not** a
  capital-gains tax on realized profit. This is the single most important modeling difference vs. a
  US-style tool.
- Consequences we model:
  - **Premiums and capital gains are generally not individually taxed** as income in Box 3; instead
    the **portfolio value** drives a deemed return that is taxed.
  - The engine therefore computes **net-of-cost pre-tax returns** *and* a separate **Box 3 tax
    drag** on year-end asset value, with configurable `heffingvrij vermogen` (tax-free allowance),
    `forfaitair rendement` percentages, and the Box 3 tax rate.
- **Configurable, versioned tax parameters** (`config/tax/nl_<year>.json`) because the Netherlands
  is actively reforming Box 3 (transition to actual-return basis). **Do not hardcode rates.**
- Dividend withholding tax (dividendbelasting, 15%) on shares held is modeled as a credit/leakage.
- ⚠️ A validation checkpoint (§9) requires confirming the active-year rules with an authoritative
  source before trusting outputs.

### 0.5 DEGIRO cost assumptions
- Model **per-trade commissions/fees** for options and shares from a `config/broker/degiro.json`
  fee schedule (options handling fee per contract, exchange connectivity fee, share transaction
  costs, currency-handling/AutoFX cost for USD).
- Fees are user-editable because DEGIRO's schedule changes and varies by product/market.

### 0.6 Explicitly OUT of scope for v1
- Live/automated trading or order routing.
- Real-time market data feeds.
- Naked options, multi-leg spreads, futures.
- Box 1 / Box 2 tax situations, corporate structures.
- Portfolio-margin modeling.

---

## 1. Goals & Success Criteria

**Primary goal:** Given a starting capital, an underlying, an options chain, and a time horizon,
estimate the **compounded after-cost, after-Box-3-tax growth** of a covered-call / wheel strategy,
and **recommend contract sizing / strike selection** under a chosen risk target.

**Success criteria:**
1. Produces a year-by-year projection table (capital, premiums collected, assignments, fees, Box 3
   tax drag, ending equity, CAGR).
2. Prices options and Greeks with a validated Black–Scholes implementation (unit-tested vs. known
   references).
3. Estimates volatility/drift from a supplied price series using the toolbox estimators.
4. Runs a Monte-Carlo simulation to give a **distribution** of outcomes (P5/P50/P95), not just a
   point estimate.
5. Optimizer suggests contracts/strikes to hit a target income or max drawdown.
6. All tax & fee parameters are external config; no magic numbers in code.

---

## 2. High-Level Architecture

```
+-------------------------------------------------------------+
|                        CLI / App layer                      |
|  parse config, run scenarios, print/export reports (CSV)    |
+------------------------------+------------------------------+
                               |
+------------------------------v------------------------------+
|                      Strategy Engine                        |
|  Wheel state machine: sell CSP -> assign -> covered call    |
|  -> assign -> repeat; reinvest premiums (compounding)       |
+----+-------------------+-------------------+----------------+
     |                   |                   |
+----v-----+   +---------v--------+   +------v-----------+
| Pricing  |   |  Estimation      |   |  Tax & Costs     |
| (BS,     |   |  (vol/drift from |   |  Box 3 tax +     |
| Greeks,  |   |  price series)   |   |  DEGIRO fees +   |
| assign.) |   |                  |   |  FX + dividends  |
+----+-----+   +---------+--------+   +------------------+
     |                   |
+----v-------------------v------------------------------------+
|          numerical-toolbox-cpp (external dependency)        |
|  Estimators · Filters · Optimization · Solvers · math::Erfc |
+-------------------------------------------------------------+
```

---

## 3. Project Layout

Directory and file naming follow the repo's existing `.ls-lint.yml` rules (snake_case/kebab-case
directories, PascalCase `.hpp`/`.cpp`) and the emil convention of a per-module `test/`
subdirectory registered with `emil_add_test`.

```
fin-bs/
├── CMakeLists.txt
├── CMakePresets.json
├── PLAN.md                           # this file
├── README.md
├── config/
│   ├── tax/nl_2025.json              # Box 3 params (versioned per year)
│   ├── broker/degiro.json            # fee schedule
│   └── scenarios/example.json        # a runnable scenario
├── data/
│   ├── prices/                       # historical price CSVs (user-provided)
│   └── chains/                       # options chain snapshots
├── source/
│   ├── core/            # Money (integer cents), Date/DayCount, Rate      → fin.core
│   ├── pricing/         # BlackScholes, Greeks, ImpliedVolatility         → fin.pricing
│   ├── estimation/      # VolatilityEstimator, DriftEstimator             → fin.estimation
│   ├── costs/           # BrokerFees, FxModel                             → fin.costs
│   ├── tax/             # Box3TaxModel, DividendTax                       → fin.tax
│   ├── strategy/        # WheelEngine, Position, CompoundingLedger        → fin.strategy
│   ├── sim/             # Rng, GbmPathGenerator, MonteCarlo               → fin.sim
│   ├── optimize/        # StrikeSizingOptimizer                           → fin.optimize
│   ├── io/              # CSV/JSON loaders, ReportWriter                  → fin.io
│   └── app/             # main.cpp CLI                                    → fin.app
│       (each module also contains test/ and, where useful, test_doubles/)
└── documentation/       # written docs — see §3.1
    ├── TEMPLATE.md      # start every new document from this
    ├── design/          # why the code is shaped this way + the decision log
    ├── theory/          # the finance/maths being implemented, with worked examples
    └── validation/      # external sources for tax/fee parameters + the pre-use checklist
```

**Dependency direction (no cycles):**

```
app → io → {strategy, optimize, sim} → {pricing, estimation, tax, costs} → core → numerical-toolbox
```

### 3.1 Documentation layout

The code is the *what*; `documentation/` is the *why* and the *which formula*. It is split three
ways so each document has one job and one lifetime:

| Folder | Answers | Changes when |
|---|---|---|
| `design/` | Why is the code shaped like this? API contracts, invariants, trade-offs, the numbered decision log | the implementation changes |
| `theory/` | What is being computed, and why is that correct? Definitions, derivations, worked examples, edge cases | the finance/maths understanding changes — it should survive a rewrite |
| `validation/` | Do we trust the inputs? Authoritative sources for tax and fee parameters, dates checked, the §9 checklist | an external rule or fee schedule changes |

Rules:

- One document per topic, kebab-case filename, started from `documentation/TEMPLATE.md`.
- Maths in LaTeX (`$inline$`, `$$block$$`).
- **Documents are code-agnostic**: domain terms only — no identifiers, no file names, no snippets.
  A document must stay correct if the implementation is rewritten in another language.
- Every numeric claim in a `theory/` document is hand-checkable and is also verified automatically.
- Any parameter taken from the outside world (Belastingdienst, DEGIRO) is logged in
  `documentation/validation/sources.md` with a URL and the date it was checked — never hardcoded.
- Design decisions are appended to `documentation/design/decisions.md` as numbered, dated entries;
  reversing one means adding a superseding entry, not editing history.

Written so far: [quantities and their conventions](documentation/design/quantities.md), the
[decision log](documentation/design/decisions.md), and theory notes on
[money and rounding](documentation/theory/money-and-rounding.md),
[day-count conventions](documentation/theory/day-count-conventions.md),
[rates and compounding](documentation/theory/rates-and-compounding.md),
[volatility scaling](documentation/theory/volatility-scaling.md) and
[option contract conventions](documentation/theory/option-contract-conventions.md).

---

## 4. Core Components (detailed)

### 4.0 Core value types (`source/core/`) — implemented
- `Money` (integer cents + currency tag), `Date`/`DayCount` (ACT/365F, ACT/252), `Rate`
  (continuously compounded), `Volatility`, `Delta`, `OptionContract`.
- Header-only `fin.core`; named factories only, no implicit conversions, invariants checked on
  construction, preconditions abort via `really_assert`.
- **Tests:** invariants (growth × discount = 1, variance additivity, order-independent
  accumulation), hand-checked conventions, `static_assert` for the `constexpr` promise, and
  `EXPECT_DEATH` for every precondition.
- Design: [quantities and their conventions](documentation/design/quantities.md).

### 4.1 Pricing (`source/pricing/`)
- `BlackScholes<T>`: European call/put price using `N(x) = 0.5 * math::Erfc(-x/√2)` (reuse toolbox
  `math::Erfc` so we don't re-implement the CDF).
- `Greeks<T>`: delta, gamma, theta, vega, rho — used for strike selection (e.g. "sell 0.30-delta
  calls") and risk limits.
- `AssignmentModel`: given spot vs. strike at expiry, decide assignment; supports early-assignment
  heuristic on dividend dates.
- **Tests:** price/Greeks vs. published reference values (`EXPECT_NEAR`), put–call parity invariant.

### 4.2 Estimation (`source/estimation/`) — *this is where the toolbox earns its keep*
- `VolatilityEstimator`: historical σ from log-returns; optional **EMA smoothing** (toolbox
  `ExponentialMovingAverage`) and **online RLS** (toolbox `RecursiveLeastSquares`) for a rolling σ.
- `DriftEstimator`: log-price trend via toolbox **Linear/Polynomial Regression** or
  **Total Least Squares** (errors-in-variables, since prices are noisy).
- Output feeds both the pricing model (IV proxy) and the Monte-Carlo SDE.
- **Tests:** feed synthetic GBM data with known σ/μ, assert recovered params within tolerance.

### 4.3 Strategy Engine (`source/strategy/`) — implemented
- `Portfolio`: cash, share lots and short positions, with collateral reserved against short puts;
  cash is never negative and reserved cash is never spent twice. Equity is *derived* from the
  holdings and a price, never stored.
- `CompoundingLedger`: append-only, dated, typed cashflows; the opening balance plus the ledger is
  the closing balance at every point.
- `ReinvestmentPolicy`: free capital, less a cash buffer in basis points, divided by one contract's
  collateral — the compounding mechanism, and the only place capital becomes contracts.
- `WheelEngine`: a total function of (holding, event) over
  `CASH → SHORT_PUT → HOLDING_SHARES → SHORT_CALL → CASH`; anything else is refused.
- `RollRules`: roll on days to expiry, on the fraction of premium already captured, or on a delta
  cap, reported risk-first. `Projection`: runs the engine over a scripted path, one row per cycle.
- **Tests:** the full transition table, the value-neutrality of assignment, ledger reconciliation,
  and a hand-checked three-year table of premiums, charges, assignments and equity.
- Design: [strategy contracts](documentation/design/strategy.md); theory:
  [the wheel](documentation/theory/wheel-strategy.md),
  [position sizing and compounding](documentation/theory/position-sizing-and-compounding.md),
  [rolling a short option](documentation/theory/rolling-a-short-option.md).

### 4.4 Tax (`source/tax/`)
- `Box3TaxModel`: loads `nl_<year>.json`; computes deemed return on year-end asset value minus
  `heffingvrij vermogen`, applies Box 3 rate → annual **tax drag** subtracted from equity.
- `DividendTax`: 15% dividendbelasting on dividends of held shares (as leakage/credit).
- **Tests:** worked examples from config produce hand-checked tax amounts; boundary at allowance.

### 4.5 Costs (`source/costs/`) — implemented
- `FeeSchedule`: plain data — per-contract option fee, share commission and handling fee per trade,
  proportional connectivity cost with an annual cap per exchange, currency handling cost. Every
  proportional cost is whole basis points; every fee is whole minor units.
- `BrokerFees`: applies a schedule to a trade or a trade list, and strikes the annual connectivity
  charge. `FxModel`: EUR base, spot conversion with the AutoFX handling cost reported separately.
- `io/BrokerConfig` reads the schedule from `config/broker/degiro.json`; nothing about a broker's
  prices is in the code.
- **Tests:** hand-computed total for a known trade list, the cap boundary, contract-count scaling, and
  a round trip that loses twice the handling cost less its square whatever the rate.
- Design: [cost contracts](documentation/design/costs.md); theory:
  [broker costs](documentation/theory/broker-costs.md),
  [currency conversion](documentation/theory/currency-conversion.md).

### 4.6 Simulation (`source/sim/`)
- `MonteCarlo`: simulates many GBM (or estimated-SDE) price paths using toolbox **Runge-Kutta**
  integrators for the SDE drift term; runs the `WheelEngine` on each path; aggregates to
  P5/P50/P95 equity and CAGR distributions.
- **Tests:** convergence of the mean to closed-form expectation as paths → large.

### 4.7 Optimization (`source/optimize/`)
- `StrikeSizingOptimizer`: uses toolbox **Gradient Descent** with **L1/L2 regularization** to pick
  {strike delta, number of contracts, cash buffer} maximizing after-tax CAGR subject to a max
  drawdown / assignment-probability constraint.
- **Tests:** on a convex synthetic objective, optimizer finds the known optimum.

---

## 5. Data & Configuration Contracts

- **Price CSV:** `date,close` (adjusted close preferred).
- **Options chain:** `expiry,strike,type,bid,ask,iv,delta` (snapshot; user-exported).
- **Scenario JSON:** starting capital, underlying, horizon (years), target delta, roll rules,
  reinvestment policy, currency, links to tax+broker configs.
- **Tax config:** year, `heffingvrij_vermogen`, deemed-return brackets/percentages, Box 3 rate,
  dividend WHT rate. Versioned; the active year is selected in the scenario.

---

## 6. Key Risks & Design Decisions

1. **Library mismatch (biggest risk):** the toolbox is embedded/DSP, not finance. Mitigation:
   treat it strictly as the *numerics* layer (regression, filtering, optimization, RK, `Erfc`) and
   build all finance logic ourselves. This is already reflected in the architecture.
2. **Tax model volatility:** Box 3 rules are being reformed. Mitigation: fully externalized,
   year-versioned config + a mandatory validation step (§9).
3. **Box 3 ≠ capital-gains:** projections must show **tax on assets**, not on premium income — a
   common modeling mistake for NL. Called out explicitly in §0.4.
4. **`float` money precision:** premiums/fees accumulate rounding error over many cycles.
   **Decision (settled, T1.1):** cash is `fin::core::Money`, an `int64_t` count of cents with a
   currency tag and half-to-even rounding; `double` is used for rates/volatilities and converted to
   the toolbox's `float` only at the call boundary. See
   [money and rounding](documentation/theory/money-and-rounding.md).
5. **No live data / no DEGIRO API:** v1 is snapshot-driven; documented clearly.
6. **Model risk:** Black–Scholes/GBM understates tail/assignment risk. Mitigation: Monte-Carlo
   distributions + explicit disclaimer.

---

## 7. Data Sourcing (DEGIRO reality)
DEGIRO offers **no official retail API**. v1 workflow:
- Export/download price history (e.g. from your own CSVs or a data provider) into `data/prices/`.
- Manually snapshot the options chain into `data/chains/`.
- Fees are modeled from the published DEGIRO schedule in `config/broker/degiro.json`.
A future adapter interface (`io/ChainSource`) leaves room for a data provider later.
---

## 8. Work Breakdown

Milestones are decomposed into **independently implementable, independently testable tasks**.
Each task is meant to be one commit/PR: it adds one type or one behaviour, plus its tests, and
leaves `ctest --preset host-Debug` green.

**Conventions that apply to every task**
- One public type per header, PascalCase filenames, `namespace fin::<module>`; headers use
  `#pragma once`.
- Code carries no comments — names, types and tests are the documentation, and the *why* lives in
  `documentation/`.
- Every task ships GoogleTest coverage in the module's `test/` directory; no task is "done"
  without tests.
- No magic numbers: anything a user could plausibly want to change goes into `config/`.
- Money is `Money` (integer cents), never `float`/`double` — see §6.4.
- Toolbox types are only used *inside* a module; they never leak through a public `fin::` API.
- Every epic ships documentation alongside the code (§3.1): a `documentation/design/` note for the
  quantities and contracts it introduces, a `documentation/theory/` note for each non-obvious
  formula or convention it implements, and a `documentation/design/decisions.md` entry for every
  choice a reviewer could reasonably have made differently. Those notes describe the *model*, not
  the code.

Legend for **Size**: `S` ≈ a single sitting, `M` ≈ a focused session, `L` → should be split further
if it grows.

---

### Foundation (was Epic 0) — **complete**

Delivered and removed from the backlog: both dependencies fetched by CMake at an exact pin
(`numerical-toolbox-cpp` at `v3.0.0`, `embedded-infra-lib` at the pin the toolbox uses), host-only
presets (`host`, `host-single-Debug`, `coverage`), the `fin.core` module with a toolbox-linkage test,
a green CI workflow, and project metadata describing *this* project rather than the `e-foc` template
it was
scaffolded from.

**Exit criteria (met):** clean clone → `cmake --preset host && cmake --build --preset host-Debug &&
ctest --preset host-Debug` passes.

The e-foc leftovers this removed are recoverable from git history: the embedded build/release jobs,
the Requirements Validation workflow, and the `cycle-analysis` / `validate-requirements` /
`generate-requirements-pdf` composite actions. Release packaging is reduced to release-please only
— artifact jobs come back with the CLI (T9.6).

---

### Epic 1 — Core value types — **complete**

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T1.1 | `Money` | `source/core/Money.hpp` — `int64_t` cents, currency tag, `+ - * /`, banker's rounding, no implicit float conversion. Tests: order-independent exactness over 10k accumulations vs. an integer reference; overflow, division-by-zero and currency mixing guarded (`EXPECT_DEATH`) | — | M | **done** |
| T1.2 | `Date` + day count | `Date.hpp` (civil date as days since epoch, no `<chrono>` zoneinfo dependency), `DayCount.hpp` with ACT/365F and ACT/252 → year fraction. Tests: leap years, 200-year round-trip, expiry Fridays, known year-fraction pairs | — | S | **done** |
| T1.3 | Strong scalar types | `Rate.hpp` (stored continuously compounded), `Volatility.hpp`, `Delta.hpp` — prevent unit mix-ups. Tests: conversion round-trips, growth/discount inverses, √t scaling, variance additivity | — | S | **done** |
| T1.4 | `Contract` identity | `OptionContract.hpp`: underlying, expiry, strike (`Money`), `OptionType{Call,Put}`, multiplier (100). Tests: equality/ordering, multiplier applied to notional | T1.1, T1.2 | S | **done** |
| T1.5 | Epic 1 documentation | `documentation/` scaffolding (`design/`, `theory/`, `validation/`, `TEMPLATE.md`) plus the quantities design note, five theory notes and decision log entries D1–D6 | T1.1–T1.4 | M | **done** |

**Exit criteria (met):** `fin.core` exposes the project's vocabulary types; 62 tests green in both
Debug and Release; every precondition is specified by an `EXPECT_DEATH` rather than a comment.

---

### Epic 2 — Pricing (M1) — **complete**

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T2.1 | Normal CDF | `pricing/NormalDistribution.hpp`: `Φ(x) = 0.5 * math::Erfc(-x/√2)`, plus pdf. Tests: Φ(0)=0.5, symmetry Φ(-x)=1-Φ(x), tail values vs. published table within 1e-7 | — | S | **done** |
| T2.2 | Black–Scholes price | `pricing/BlackScholes.hpp` European call/put with continuous dividend yield `q`. Tests: ≥ 3 published reference prices (`EXPECT_NEAR`), **put–call parity** invariant, monotonicity in σ and in S | T2.1, T1.3 | M | **done** |
| T2.3 | Greeks | `pricing/Greeks.hpp`: delta, gamma, theta, vega, rho. Tests: each Greek cross-checked against a central finite difference of T2.2 within 1e-4; gamma ≥ 0; call delta ∈ (0,1) | T2.2 | M | **done** |
| T2.4 | Implied volatility | `pricing/ImpliedVolatility.hpp`: Newton on vega with bisection fallback and iteration cap. Tests: round-trip — price at σ then recover σ within 1e-5; converges for deep ITM/OTM; fails *gracefully* (no throw, returns `optional`) below intrinsic | T2.3 | M | **done** |
| T2.5 | Assignment | `pricing/AssignmentModel.hpp`: at-expiry moneyness rule + early-assignment heuristic when a dividend exceeds remaining extrinsic value. Tests: ITM/OTM/ATM boundaries, dividend-driven early call assignment | T2.2, T1.4 | M | **done** |
| T2.6 | Epic 2 documentation | `design/valuation.md`, four `theory/` notes (Black–Scholes, Greeks, implied volatility, assignment), decision log entries D7–D10, reference-valuation rows in `validation/sources.md` | T2.1–T2.5 | M | **done** |

**Exit criteria (met):** `fin.pricing` values a European option, its Greeks and its implied
volatility, and decides assignment; 41 tests green in both Debug and Release; every model input
carries its unit and every refusal is specified by an `EXPECT_DEATH`.

---

### Epic 3 — Estimation (M2) — **complete**

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T3.1 | Synthetic GBM generator | `estimation/test_doubles/GbmSeries.hpp` — deterministic seeded GBM with known μ/σ, on a business-day grid so the observation frequency is exactly 252. This is a **test double built first**, so every later estimator has ground truth | T1.2 | S | **done** |
| T3.2 | Log returns + historical σ | `estimation/PriceSeries.hpp` (dated closes, validated once, deriving their own periods-per-year), `estimation/LogReturns.hpp`, `HistoricalVolatility.hpp` (sample stddev × √annualization factor). Tests: recover σ from T3.1 within 5% at n=5000; zero-variance series → 0 | T3.1 | S | **done** |
| T3.3 | EMA volatility | `estimation/EmaVolatilityEstimator.hpp` wrapping the toolbox `ExponentialMovingAverage` filter. Tests: step change in σ decays at exactly the configured half-life; steady state matches T3.2 | T3.2 | M | **done** |
| T3.4 | Drift estimator | `estimation/DriftEstimator.hpp` — log-price vs. time, delegating to a `LineFit` interface with `OrdinaryLeastSquaresFit` and `TotalLeastSquaresFit` adapters over the toolbox `LinearRegression` / `TotalLeastSquares`. Tests: recover μ from T3.1; the two agree on a near-deterministic path and the orthogonal fit is the steeper one on a noisy path | T3.1 | M | **done** |
| T3.5 | Online/rolling σ | `estimation/RlsVolatilityEstimator.hpp` on toolbox `RecursiveLeastSquares`. Tests: without forgetting it equals the batch estimate of T3.2; the forgetting factor tracks a regime switch | T3.2 | M | **done** |
| T3.6 | Estimator facade | `estimation/MarketParameters.hpp` — one call takes a price series and returns `{drift, volatility, dividendYield}` in `fin::` units (no toolbox types leaked). Tests: facade output equals the underlying estimators, with the variance correction applied once | T3.2–T3.5 | S | **done** |
| T3.7 | Epic 3 documentation | `design/estimation.md`, three `theory/` notes (log returns and realised volatility, drift estimation, exponentially weighted volatility), decision log entries D11–D15 | T3.1–T3.6 | M | **done** |

**Exit criteria (met):** `fin.estimation` turns a dated price series into a drift, a volatility and a
carried dividend yield; 85 tests green in both Debug and Release; each published worked example is
reproduced by a test.

> **Deviation from the original acceptance criteria for T3.4.** The plan asked for a test showing
> that total least squares beats ordinary least squares "when the time axis is perturbed". That case
> cannot be constructed for a dated series: recorded dates must be distinct and increasing, so any
> error in them is bounded by the observation spacing, and the resulting attenuation of the ordinary
> fit is of the order of the reciprocal of the square of the number of observations — immeasurable.
> On a real path the orthogonal fit is instead the *worse* estimator, reporting a drift larger by the
> reciprocal of the correlation. It is kept as a selectable variant and the reasoning is recorded in
> [decision D13](documentation/design/decisions.md).

---

### Epic 4 — Costs & FX (part of M4, pulled early — the ledger needs it) — **complete**

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T4.1 | Fee schedule model | `costs/FeeSchedule.hpp` - plain data: per-contract option fee, per-trade share commission and handling fee, proportional connectivity cost with an annual cap per exchange, currency handling cost | T1.1 | S | **done** |
| T4.2 | `BrokerFees` | `costs/Trade.hpp` + `costs/BrokerFees.hpp` applying a `FeeSchedule` to a trade list. Tests: hand-computed total for a known 5-trade list; the connectivity cap exactly at, below and above the boundary; contract count scaling; options exempt from the handling fee | T4.1 | M | **done** |
| T4.3 | `FxModel` | `costs/FxModel.hpp` - EUR base, spot rate + AutoFX handling cost in bps, reported separately from the converted amount. Tests: a round trip loses twice the handling cost less its square, whatever the rate | T1.1 | S | **done** |
| T4.4 | Broker config loader | `io/BrokerConfig.hpp` reads `config/broker/degiro.json` into a `FeeSchedule`, using emil's own JSON file reader and object navigator - no reading layer of our own. Tests: the golden config parses; missing, mistyped, negative and out-of-range fields are each refused naming the offending key | T4.1 | M | **done** |
| T4.5 | Epic 4 documentation | `design/costs.md`, two `theory/` notes (broker costs, currency conversion), decision log entries D16-D20, a `validation/sources.md` row per published DEGIRO parameter | T4.1-T4.4 | M | **done** |

**Exit criteria (met):** `fin.costs` turns a trade list and a fee schedule into an exact cash charge
and converts between currencies at a stated cost; `fin.io` reads a schedule from a dated configuration
file and refuses a bad one by name; 45 tests green in both Debug and Release; no fee appears anywhere
in the code.

> **T9.1 turned out not to be a task.** It asked for a JSON reading utility over emil's parser with
> typed errors. emil already provides one: `infra/syntax/JsonFileReader.hpp` owns and validates the
> document and `JsonObjectNavigator.hpp` navigates it by key, refusing a missing or mistyped key by
> name. The loader uses those directly and catches at the boundary. The one thing they do not do -
> turn a decimal into an exact count of minor units - stopped being needed once configuration was
> stated in minor units ([D20](documentation/design/decisions.md)). T9.1 is struck from Epic 9.

> **Deviation from the original acceptance criteria for T4.1.** The plan placed the "percentage
> component with cap" on the share transaction cost. The published DEGIRO tariff has no proportional
> component on home-market share trades; the proportional-with-cap charge it does have is the annual
> exchange connectivity fee. The model follows the tariff rather than the plan's guess, so the cap
> boundary is still modelled and tested, on the charge that actually has one.

> ⚠️ **The option contract charge is not yet confirmed against a published source.** It is the cost a
> wheel strategy is most sensitive to. See [validation/sources.md](documentation/validation/sources.md).

---

### Epic 5 — Strategy engine (M3) — **complete**

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T5.1 | Portfolio state | `strategy/Portfolio.hpp` — cash, share lots, short option positions, collateral reserved. Tests: invariant "cash ≥ 0" and "collateral ≤ cash" enforced; opening a CSP reserves strike × 100 | T1.4, T1.1 | M | **done** |
| T5.2 | Cashflow ledger | `strategy/CompoundingLedger.hpp` — append-only, typed cashflows (`Premium`, `Fee`, `Assignment`, `Dividend`, `Tax`, `Fx`); equity is derived, never stored. Tests: ledger sums reconcile to portfolio equity after a scripted sequence | T5.1, T4.2 | M | **done** |
| T5.3 | Sizing / reinvestment policy | `strategy/ReinvestmentPolicy.hpp` — given free capital and a strike, how many contracts to open (this **is** the compounding mechanism); includes a configurable cash buffer. Tests: capital just below/above one contract's collateral; buffer respected | T5.2 | M | **done** |
| T5.4 | Wheel state machine | `strategy/WheelEngine.hpp` — `CASH → SHORT_PUT → HOLDING_SHARES → SHORT_CALL → CASH`. Pure function of (state, market event). Tests: every legal transition + every illegal transition rejected | T5.3, T2.5 | L → split per state if needed | **done** |
| T5.5 | Roll rules | `strategy/RollRules.hpp` — roll at N DTE, at X% of max profit, or when Δ breaches a threshold. Tests: each trigger fires exactly once on a scripted path; rolling never increases risk beyond the configured cap | T5.4 | M | **done** |
| T5.6 | Deterministic projection | `strategy/Projection.hpp` — run the engine over a scripted price path, emit per-period rows. Tests: hand-checked 3-year path produces the expected premium/assignment/equity table | T5.5 | M | **done** |
| T5.7 | Epic 5 documentation | `design/strategy.md`, three `theory/` notes (the wheel, position sizing and compounding, rolling a short option), decision log entries D21-D25 | T5.1-T5.6 | M | **done** |

**Exit criteria (met):** `fin.strategy` turns a scripted price path into a per-cycle table of
premiums, charges, assignments, cash, shares and equity; cash is never negative, collateral is never
spent twice, and the opening balance plus the ledger equals the closing balance at every point; 67
tests green in both Debug and Release.

> **A projection advances by cycles, not by calendar periods.** A row is one option from the day it
> is written to the day it expires, is assigned or is closed, because that is when a wheel changes
> state ([D24](documentation/design/decisions.md)). The hand-checked table is six half-year cycles
> over three years. Anything annual by nature — Box 3, above all — has to be struck on its own dates
> rather than on row boundaries, which Epic 6 does.

> **The strategy layer prices nothing.** Premiums, the value of a position at an observation and the
> price at expiry are supplied by the script; assignment is delegated to T2.5 and charges to T4.2
> ([D25](documentation/design/decisions.md)). This is what lets the whole engine be tested against
> arithmetic a reader can check on paper, with no volatility anywhere in the test, and what will let
> Epic 7 drive the same engine from simulated paths without changing it.

> **Deviation from the acceptance criteria for T5.5.** "Rolling never increases risk beyond the
> configured cap" is expressed as a test that the rules refuse a replacement whose delta exceeds the
> cap, rather than as a property of a roll the engine performs: the replacement is the next cycle's
> option and comes from the script, so the rules can only judge it, not choose it. Choosing it needs
> an options chain, which arrives with T9.3.

---

### Epic 6 — Tax (M4)

| ID | Task | Deliverable / Acceptance criteria | Deps | Size |
|---|---|---|---|---|
| T6.1 | Box 3 config schema | `config/tax/nl_2025.json` + `tax/Box3Parameters.hpp` — `heffingvrij_vermogen`, deemed-return brackets/percentages, Box 3 rate, dividend WHT rate, `tax_year`. **Nothing hardcoded** | T1.1 | S |
| T6.2 | `Box3TaxModel` | `tax/Box3TaxModel.hpp` — deemed return on 1-Jan asset value minus allowance × rate. Tests: worked examples hand-checked; boundary exactly at the allowance; zero and negative net-asset cases | T6.1 | M |
| T6.3 | `DividendTax` | `tax/DividendTax.hpp` — 15% dividendbelasting withheld, modelled as a credit against Box 3. Tests: withholding + credit nets correctly; foreign-dividend leakage case | T6.1 | S |
| T6.4 | Tax integration | Annual tax drag posted to the ledger on the peildatum. Tests: a full-year projection shows tax charged **on assets**, not on premium income (guards the §0.4 modelling mistake) | T6.2, T5.6 | M |
| T6.5 | Validation log | `documentation/validation/sources.md` gains a row per Box 3 parameter: value, Belastingdienst URL, date checked; `documentation/theory/box3-taxation.md` explains why the tax is on assets rather than on premium income | T6.1 | S |

---

### Epic 7 — Monte-Carlo (M5)

| ID | Task | Deliverable / Acceptance criteria | Deps | Size |
|---|---|---|---|---|
| T7.1 | Deterministic RNG | `sim/Rng.hpp` — seeded PRNG + normal variates. Tests: same seed → identical stream; sample mean/variance within tolerance at n=1e6 | — | S |
| T7.2 | SDE path generator | `sim/GbmPathGenerator.hpp` — GBM integrated with the toolbox Runge-Kutta/Dormand-Prince integrators for the drift term. Tests: E[S_t] converges to `S_0 e^{μt}`; log-variance converges to `σ²t` | T7.1, T3.6 | M |
| T7.3 | Simulation runner | `sim/MonteCarlo.hpp` — runs `WheelEngine` over N paths, collects terminal equity and CAGR. Tests: N=1 reproduces the deterministic projection of T5.6 | T7.2, T5.6 | M |
| T7.4 | Distribution aggregation | `sim/OutcomeDistribution.hpp` — P5/P50/P95, max drawdown, assignment frequency. Tests: percentiles of a known distribution; monotone ordering P5 ≤ P50 ≤ P95 | T7.3 | S |
| T7.5 | Convergence test | Statistical test that the MC mean approaches the closed-form expectation as N grows (with a fixed seed so CI is deterministic) | T7.4 | S |

---

### Epic 8 — Optimizer (M6)

| ID | Task | Deliverable / Acceptance criteria | Deps | Size |
|---|---|---|---|---|
| T8.1 | Objective adapter | `optimize/StrategyObjective.hpp` implementing the toolbox `ObjectiveFunction` interface over `{target delta, contract count, cash buffer}` → after-tax CAGR. Tests: objective is finite and deterministic for a fixed seed | T7.4, T6.4 | M |
| T8.2 | Optimizer wrapper | `optimize/StrikeSizingOptimizer.hpp` on toolbox `GradientDescent` + L1/L2 regularization. Tests: on a **convex synthetic** objective the known optimum is found within tolerance | T8.1 | M |
| T8.3 | Constraints | Penalty terms for max drawdown and assignment probability. Tests: a constraint-violating optimum is rejected in favour of a feasible one | T8.2 | M |
| T8.4 | Recommendation output | `optimize/Recommendation.hpp` — chosen delta/contracts/buffer plus the achieved objective and binding constraints. Tests: output is reproducible for a fixed seed | T8.3 | S |

---

### Epic 9 — I/O, CLI, reports (M7)

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T9.1 | ~~JSON reading utility~~ | **struck** - emil already provides it: `infra/syntax/JsonFileReader.hpp` owns and validates a document, `JsonObjectNavigator.hpp` navigates it by key and refuses a missing or mistyped key by name. Loaders use those directly | — | — | **not needed** |
| T9.2 | Price CSV loader | `io/PriceCsvLoader.hpp` for `date,close`. Tests: golden file; unsorted dates rejected; duplicate dates rejected; CRLF tolerated | T1.2 | M |
| T9.3 | Options chain loader | `io/ChainCsvLoader.hpp` for `expiry,strike,type,bid,ask,iv,delta`, behind an `io/ChainSource` interface so a live provider can be added later. Tests: golden file; crossed bid/ask rejected | T1.4 | M |
| T9.4 | Scenario loader | `io/ScenarioLoader.hpp` — starting capital, underlying, horizon, target delta, roll rules, reinvestment policy, currency, links to tax + broker configs. Tests: `config/scenarios/example.json` round-trips | T6.1, T4.4 | M |
| T9.5 | Report writer | `io/ReportWriter.hpp` — year-by-year CSV + aligned console table (capital, premiums, assignments, fees, tax drag, ending equity, CAGR). Tests: golden output for a fixed projection | T5.6 | M |
| T9.6 | CLI | `app/main.cpp` with subcommands `price`, `estimate`, `project`, `simulate`, `optimize`; `--scenario <file>`; non-zero exit on bad input. Tests: end-to-end run of the example scenario produces the golden report | T9.5, T8.4 | L → one subcommand per commit |
| T9.7 | Methodology docs | `documentation/theory/methodology.md` tying the theory notes into one end-to-end description (formulas, assumptions, limits) + the §9 checklist in `documentation/validation/README.md` filled in | T9.6 | M |

---

### Suggested implementation order

Epic 1 → Epic 2 → Epic 3 → Epic 4 → Epic 5 → Epic 6 → Epic 7 → Epic 8 → Epic 9.

The only useful reorderings: T9.1/T9.2 (loaders) can be pulled forward to feed real data into Epic 3,
and Epic 4 can slip after Epic 5 if the ledger initially runs fee-free.

**Next task to pick up:** T5.1 (`Portfolio`), followed by T5.2 (`CompoundingLedger`).

---

## 9. Validation Checklist (do before trusting numbers)

Tracked, with sources and dates, in
[`documentation/validation/README.md`](documentation/validation/README.md).

- [ ] Confirm the active-year **Box 3** parameters against Belastingdienst for your tax year.
- [ ] Confirm the **DEGIRO option contract charge** — the rest of the fee schedule (shares, handling,
      connectivity, AutoFX) was confirmed on 2026-08-11.
- [ ] Black–Scholes prices/Greeks match an independent reference within tolerance.
- [x] Estimators recover known σ/μ from synthetic GBM — done in T3.2/T3.4.
- [ ] Monte-Carlo mean converges to closed-form expectation.
- [x] Money math uses integer cents, not `float` — done in T1.1.

---

## 10. Open Questions for You
1. Confirm **covered call + cash-secured put wheel** is the intended strategy (vs. naked/spreads).
2. Which **underlyings** (US ETFs in USD? EUR-denominated)? This drives the FX model.
3. Your **tax year** and whether you're strictly a **private Box 3 investor**.
4. Preferred **price-history source** for the estimators (which CSV format).
5. `float` vs. `double` requirement — settled: integer cents for money, `double` for rates and
   volatilities (T1.1, decision D1/D3).
