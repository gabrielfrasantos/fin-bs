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

### 0.1a Several underlyings at once

The strategy is run on **N underlyings in parallel**, not one. A run is described by a list of
underlyings, each with its own market and its own listed strike grid, all wheeled side by side out
of **one shared pot of cash**. The settled shape:

| Question | Answer |
|---|---|
| What is per-underlying? | volatility, drift, dividend yield, spot, strike increment |
| What is global? | the target delta, the cycle length, the roll delta limit, the cash buffer |
| How is capital shared? | one pool; each cycle, the wheels that need fresh cash claim it **greedily, highest premium per unit of collateral first**, ties broken by the order the underlyings appear in the scenario |
| Do the wheels share a clock? | yes — the cycle length is global, so all N open and expire on the same dates |
| Are the paths correlated? | **not in v1** — each underlying is drawn independently. This overstates diversification and is called out as a known limitation (§6) with its own later task |
| Currency? | all underlyings are EUR-denominated in v1 |

The consequence that matters most: **Box 3 is struck once, on the whole book**, not once per
underlying. A wheel that is part of a book therefore never assesses tax itself.

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
  new third-party parser is introduced. Writing a scenario back out uses emil's
  `infra/syntax/JsonFormatter.hpp`, for the same reason.
- **GUI:** **Qt 6 Quick (QML)**, taken from the system, *not* fetched by CMake — it is far too
  large to build from source per configure. It is a **hard dependency**: the build requires
  `Qt6::Quick`, `Qt6::QuickControls2` and `Qt6::Test`. No Qt Charts: the fan chart is painted by a
  `QQuickPaintedItem` over geometry computed in the Qt-free presentation layer. GUI tests run under
  the `offscreen` platform plugin with the software renderer, so they need no display and no
  graphics driver (Epic 14; the Widgets window of Epic 11 is history).
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
- **A command-line interface.** The GUI is the interface; see §4.9 and Epic 11.
- **Correlation between underlyings**, and **underlyings in more than one currency** — both are
  deliberate v1 limitations of the multi-underlying model (§0.1a), each with its own later task.
- **Staggered expiries.** All wheels share one cycle clock.

---

## 1. Goals & Success Criteria

**Primary goal:** Given a starting capital, **a list of underlyings**, their markets and a time
horizon, estimate the **compounded after-cost, after-Box-3-tax growth** of a covered-call / wheel
strategy run across all of them out of one pot of cash, and **recommend contract sizing / strike
selection** under a chosen risk target — through a desktop application.

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
7. **Runs a book of N underlyings out of one pot of cash**, and reconciles: the pot is never
   overdrawn, no unit of collateral backs two positions, and the wealth tax is struck once on the
   whole book.
8. **Is driven from a desktop window**: a scenario is edited, saved and reloaded, a run is started,
   watched and cancelled, and its outcome is read as a table and a chart.

---

## 2. High-Level Architecture

```
+-------------------------------------------------------------+
|                    Desktop application (Qt)                 |
|  edit a scenario, run it on a worker thread, show progress, |
|  cancel, draw the outcome, export CSV                       |
+------------------------------+------------------------------+
                               |
+------------------------------v------------------------------+
|                    Presentation (Qt-free)                   |
|  scenario document, run controller, table + chart models    |
+------------------------------+------------------------------+
                               |
+------------------------------v------------------------------+
|                            Book                             |
|  N wheels, one pot of cash, one ledger, one tax assessment  |
|  claims collateral greedily, richest premium per unit first |
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

Qt appears **only in the top box**. Nothing below it knows a window exists: the simulator reports
its progress through an interface of this project's own, and the presentation layer is plain C++ so
that what the window shows can be asserted without one.

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
│   └── scenarios/example.json        # a runnable scenario (a list of underlyings)
├── data/
│   ├── prices/                       # historical price CSVs (user-provided)
│   └── chains/                       # options chain snapshots
├── source/
│   ├── core/            # Money (integer cents), Date/DayCount, Rate      → fin.core
│   ├── pricing/         # BlackScholes, Greeks, ImpliedVolatility         → fin.pricing
│   ├── estimation/      # VolatilityEstimator, DriftEstimator             → fin.estimation
│   ├── costs/           # BrokerFees, FxModel                             → fin.costs
│   ├── tax/             # Box3TaxModel, DividendTax                       → fin.tax
│   ├── strategy/        # WheelEngine, Position, CompoundingLedger, Book  → fin.strategy
│   ├── sim/             # Rng, GbmPathGenerator, MonteCarlo               → fin.sim
│   ├── optimize/        # StrikeSizingOptimizer                           → fin.optimize
│   ├── io/              # CSV/JSON loaders, ReportWriter                  → fin.io
│   ├── present/         # Qt-free scenario document, run controller,      → fin.present
│   │                    # table and chart models
│   ├── gui/             # Qt Quick models, painted chart, the QML window  → fin.gui
│   └── app/             # main.cpp — the engine, the window, the demo tour → fin.app
│       (each module also contains test/ and, where useful, test_doubles/)
└── documentation/       # written docs — see §3.1
    ├── TEMPLATE.md      # start every new document from this
    ├── design/          # why the code is shaped this way + the decision log
    ├── theory/          # the finance/maths being implemented, with worked examples
    └── validation/      # external sources for tax/fee parameters + the pre-use checklist
```

**Dependency direction (no cycles):**

```
app → gui → present → io → {strategy, optimize, sim} → {pricing, estimation, tax, costs} → core → numerical-toolbox
```

**Qt is confined to `gui/` and `app/`.** `present/` is deliberately Qt-free: it holds what the
window is showing, as plain data, so a presenter can be asserted with the same GoogleTest machinery
as the rest of the project. `gui/` translates that data into widgets and translates a click back
into a call, and holds no logic worth a test of its own beyond the ones Epic 11 lists.

No Qt Designer `.ui` files: the naming lint requires PascalCase for source files and a hand-written
widget keeps the layout reviewable in a diff.

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

### 4.4 Tax (`source/tax/`) — implemented
- `Box3Parameters`: plain data — tax year, tax-free allowance, imputed returns on deposits and on
  investments, the Box 3 rate and the dividend withholding rate, all as whole minor units and whole
  basis points. `io/TaxConfig` reads them from `config/tax/nl_<year>.json`; nothing is in the code.
- `Box3TaxModel`: deemed return on wealth measured at the peildatum, reduced by the share of the
  taxable base in that wealth, taxed at the Box 3 rate → the annual **tax drag**. It accepts only a
  description of what is *held*, so it cannot be handed an income.
- `DividendTax`: withholding at the payer's rate, credited against the annual charge up to the
  creditable rate; the excess is reported as leakage.
- The projection strikes the charge on the reference date inside the cycle that contains it, pays it
  from free cash and posts it to the ledger on that date.
- **Tests:** hand-checked worked examples, the boundary exactly at the allowance, zero wealth, and a
  full-year projection that pays nothing below the allowance however much premium it collected.
- Design: [tax contracts](documentation/design/tax.md); theory:
  [wealth taxation of an investment portfolio](documentation/theory/box3-taxation.md).

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

### 4.6 Simulation (`source/sim/`) — implemented
- `Rng`: a seeded stream of uniform and standard-normal variates; the transform is the project's own,
  so a seed reproduces a run exactly on any platform.
- `GbmPathGenerator`: geometric Brownian motion in the logarithm of the price, the drift integrated
  with the toolbox **Runge-Kutta** integrator and the diffusion drawn from its exact law over each
  step, on whatever schedule of dates it is given. It produces a price history the estimators accept.
- `WheelScript`: the terms of the wheel — cycle length, target delta, strike grid, roll observation,
  annual reference date — turned into the dates a run needs a price for, and then, one cycle at a
  time, into the periods the strategy engine runs over. It selects the strike and prices the premium
  from the model, because a simulated future has no options chain.
- `MonteCarlo`: runs the same `Projection` over N simulated paths, collecting terminal equity, CAGR,
  assignments and the deepest drawdown. `OutcomeDistribution`: percentiles by rank, means, worst
  drawdown and assignment frequency.
- **Tests:** the stream is reproducible and its normals have the right mean and variance; a path with
  no volatility is the integrated drift exactly; the mean terminal price and the log-variance converge
  to their closed forms within three standard errors; one path reproduces the projection driven by the
  same periods; ten thousand paths never reach a charge they cannot settle.
- Design: [simulation contracts](documentation/design/simulation.md); theory:
  [Monte-Carlo projection of a wheel](documentation/theory/monte-carlo-simulation.md).

### 4.7 Optimization (`source/optimize/`) — implemented
- `StrategySetting`: the three choices a run makes — the delta options are written at, the fraction
  of free cash held back, and the delta at which a position is rolled away — together with the range
  of settings worth considering and the mapping between a setting and its place in that range.
- `StrategyObjective`: scores a setting by simulating the wheel under it, from a fixed seed, and
  reports the mean after-cost, after-tax growth rate less a charge for whatever risk limit it passed.
  Its slope is a difference taken over a step large enough that the run on either side of it differs.
- `RiskLimits`: the deepest drawdown and the highest assignment rate that are acceptable, the amount
  by which an outcome passed them, and what that costs.
- `StrikeSizingOptimizer`: walks downhill with the toolbox **gradient descent**, over the distance
  from the setting already in use, which the toolbox **L1/L2 regularization** may charge for.
- `Recommendation`: the setting arrived at, what it achieved, and which limits it is up against.
- **Tests:** the known floor of a convex bowl is found, and found on the nearest face when it lies
  outside the range; the shrunk and the soft-thresholded answers match their closed forms; a setting
  is scored identically every time it is asked for; a drawdown limit is met by holding back cash and
  an assignment limit by rolling sooner, each paid for in growth.
- Design: [optimisation contracts](documentation/design/optimization.md); theory:
  [searching for a setting](documentation/theory/searching-for-a-setting.md).

### 4.8 Book (`source/strategy/Book.hpp`) — implemented

What turns one wheel into N. It owns the things there is exactly one of — the cash, the ledger, the
tax assessment — and lends them to each wheel in turn.

- **A shared account.** `Account` holds the opening balance, the holdings, the ledger and the tax
  rules as one thing. `Projection` gained a second constructor over an account it does not own,
  keeping the one it does, so N projections can be run against the same cash without either of them
  learning that the others exist. Which of the two a wheel is survives being copied, which is what
  keeps a simulation's per-path copy independent.
- **A ticker on every holding.** Shares are no longer a bare count: a lot names what it is a lot of,
  cover is judged per underlying, and the value of the holdings is asked for against a price per
  underlying. Valuing a holding that was given no price, or giving one two prices, is refused.
- **Two passes over a cycle.** Every wheel writes before any wheel settles
  ([D42](documentation/design/decisions.md)). Run the other way round, the first wheel releases its
  collateral before the second asks for it and the book quietly writes several times the positions
  it can pay for.
- **An order of service.** `CollateralAllocation` — wheels that need no fresh cash first, then
  cash-secured puts by descending premium per unit of collateral, ties by the order the book lists
  its underlyings. Named and tested on its own, because it is the only place where one underlying
  can crowd out another.
- **One assessment.** The wealth tax is struck on the whole book at a cycle boundary and posted
  once. A wheel running inside a book is refused a reference date.
- **Tests:** a scripted book of one underlying is the single-name projection cent for cent; the pot
  is never overdrawn and the ledger still reconciles with N wheels; a richer premium is served first
  whatever order the scenario lists it in; the allowance is granted to the book and not to every
  underlying in it; a hand-checked two-name, two-cycle run.
- Design: [the book](documentation/design/book.md); theory:
  [running several wheels at once](documentation/theory/running-several-wheels-at-once.md).

### 4.9 Presentation and GUI (`source/present/`, `source/gui/`, `source/app/`) — planned, Epic 11

- `present/ScenarioDocument`: everything a run needs, as edited — capital, horizon, the global
  levers, and a row per underlying — plus whether it is currently well formed and, if not, which
  field is wrong. Qt-free, so the rules about what may be entered are tested as arithmetic.
- `present/RunController`: starts a projection, a simulation or a search; reports progress as a
  fraction and a count; can be asked to stop and stops at the next path boundary. It talks to the
  simulator through the observer interface Epic 10 adds, so nothing below it knows about threads.
- `present/ResultTable` and `present/FanChartSeries`: the per-cycle rows and the ledger as columns
  of already-formatted text, and the percentile bands as plain sequences of points in model
  coordinates — the chart's geometry, decided and asserted before any painting happens.
- `gui/MainWindow`: the scenario on the left, the results on the right. The left panel is the
  underlying table (add, remove, reorder — order is a modelling input, see §4.8), the global levers,
  and the run buttons. The right is the per-cycle table, the ledger, the fan chart and the
  recommendation.
- `gui/FanChartItem`: a painted Qt Quick item drawing the percentile bands and the median path,
  with its own axes and ticks. Hand-drawn because Qt Charts is not a dependency (§0.3), and it is
  roughly a hundred lines because the geometry is computed elsewhere.
- `app/main.cpp`: constructs the application, loads the tax and broker configuration, shows the
  window. Nothing else.
- **Tests:** the presenters under GoogleTest as ordinary types; the widgets under **Qt Test** with
  the `offscreen` platform plugin, so a real window is built, filled in, clicked and read back with
  no display attached.

---

## 5. Data & Configuration Contracts

- **Price CSV:** `date,close` (adjusted close preferred).
- **Options chain:** `expiry,strike,type,bid,ask,iv,delta` (snapshot; user-exported). Not read in
  v1 — see Epic 9.
- **Scenario JSON:** starting capital, horizon, currency, the global levers (target delta, cycle
  length, roll delta limit, cash buffer), a **list of underlyings** — each with a ticker, a spot, a
  volatility, a drift, a dividend yield and a strike increment — the reference date, the seed, and
  links to the tax and broker configs. **The order of the list is significant**: it breaks ties in
  the allocation of collateral (§4.8). Cash is stated in whole cents and proportions in whole basis
  points, as everywhere else.
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
7. **Independent underlyings overstate diversification (new, Epic 10).** Drawing N paths
   independently makes a book of N names look far safer than it is; real equities fall together.
   Every drawdown and every assignment rate a book reports is therefore optimistic. Mitigation: the
   limitation is stated wherever a book result is shown, and a correlated generator is a named later
   task rather than an open-ended intention.
8. **Greedy allocation is not the best allocation (new, Epic 10).** Serving the richest premium
   first is a rule, not an optimum, and it will concentrate the book in whichever name is most
   volatile — which is also the one most likely to be assigned. Mitigation: the rule is isolated in
   one place so an alternative can be substituted and compared, and the concentration it produces is
   reported rather than hidden.
9. **Qt reaches further than intended (new, Epic 11).** The usual failure of a GUI on top of a
   library is that widgets start holding the model. Mitigation: Qt is a build dependency of two
   directories only (§3), what the window shows lives in a Qt-free layer that is tested without one,
   and the simulator reports progress through an interface of this project's own — there is no
   signal, slot or thread below `present/`.
10. **Qt is a system dependency, not a pinned one (new, Epic 11).** Unlike the toolbox and emil, Qt
    is whatever the machine has. Mitigation: a minimum version is required in CMake and the CI image
    installs it explicitly, so "works on my machine" fails at configure time rather than at runtime.

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

### Epic 6 — Tax (M4) — **complete**

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T6.1 | Box 3 config schema | `config/tax/nl_2025.json` + `tax/Box3Parameters.hpp` — `heffingvrij_vermogen`, deemed-return brackets/percentages, Box 3 rate, dividend WHT rate, `tax_year`. **Nothing hardcoded** | T1.1 | S | **done** |
| T6.2 | `Box3TaxModel` | `tax/Box3TaxModel.hpp` — deemed return on 1-Jan asset value minus allowance × rate. Tests: worked examples hand-checked; boundary exactly at the allowance; zero and negative net-asset cases | T6.1 | M | **done** |
| T6.3 | `DividendTax` | `tax/DividendTax.hpp` — 15% dividendbelasting withheld, modelled as a credit against Box 3. Tests: withholding + credit nets correctly; foreign-dividend leakage case | T6.1 | S | **done** |
| T6.4 | Tax integration | Annual tax drag posted to the ledger on the peildatum. Tests: a full-year projection shows tax charged **on assets**, not on premium income (guards the §0.4 modelling mistake) | T6.2, T5.6 | M | **done** |
| T6.5 | Validation log | `documentation/validation/sources.md` gains a row per Box 3 parameter: value, Belastingdienst URL, date checked; `documentation/theory/box3-taxation.md` explains why the tax is on assets rather than on premium income | T6.1 | S | **done** |

**Exit criteria (met):** a projection charges an annual wealth tax on the portfolio as it stands on
the reference date, from parameters read from a document that names its tax year; nothing about a tax
rate is in the code; the ledger still reconciles once the charge is struck; a portfolio below the
allowance is charged nothing however much premium it collected. 26 tests in the tax module, 11 for
the configuration loader and 7 more in the projection, green in both Debug and Release.

> **The tax cannot be handed an income.** The only input to the charge is a description of what is
> held, split into bank deposits and investments because the published rules impute a different return
> to each ([D26](documentation/design/decisions.md)). This is what makes the §0.4 mistake
> unwritable rather than merely documented: there is no argument for a premium to be passed as.

> **The reference date is part of the script.** A cycle is not a calendar period
> ([D24](documentation/design/decisions.md)), so the period containing the turn of the year carries
> the date and the price of the underlying on it. The projection values the holdings at that instant,
> including any open position, and records the charge on that date rather than on the row's
> ([D28](documentation/design/decisions.md)).

> **Deviations from the acceptance criteria.** T6.2 asks for a negative net-asset case; borrowing is
> not modelled, because the strategy never borrows and the published treatment of debts needs a
> parameter that could not be confirmed, so wealth is non-negative by construction
> ([D29](documentation/design/decisions.md)) and the boundary cases tested are zero wealth and wealth
> exactly at the allowance. T6.3's 15% withholding rate is configured rather than fixed, and is the
> one Box 3 figure in `validation/sources.md` still marked unconfirmed. The 2025 imputed return on
> deposits is the settled 1.37%, not the provisional 1.44% that was published during the year.

---

### Epic 7 — Monte-Carlo (M5) — **complete**

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T7.1 | Deterministic RNG | `sim/Rng.hpp` — seeded PRNG + normal variates. Tests: same seed → identical stream; sample mean/variance within tolerance at n=1e6 | — | S | **done** |
| T7.2 | SDE path generator | `sim/GbmPathGenerator.hpp` — GBM integrated with the toolbox Runge-Kutta/Dormand-Prince integrators for the drift term. Tests: E[S_t] converges to `S_0 e^{μt}`; log-variance converges to `σ²t` | T7.1, T3.6 | M | **done** |
| T7.3 | Simulation runner | `sim/MonteCarlo.hpp` — runs `WheelEngine` over N paths, collects terminal equity and CAGR. Tests: N=1 reproduces the deterministic projection of T5.6 | T7.2, T5.6 | M | **done** |
| T7.4 | Distribution aggregation | `sim/OutcomeDistribution.hpp` — P5/P50/P95, max drawdown, assignment frequency. Tests: percentiles of a known distribution; monotone ordering P5 ≤ P50 ≤ P95 | T7.3 | S | **done** |
| T7.5 | Convergence test | Statistical test that the MC mean approaches the closed-form expectation as N grows (with a fixed seed so CI is deterministic) | T7.4 | S | **done** |
| T7.6 | Epic 7 documentation | `design/simulation.md`, `theory/monte-carlo-simulation.md`, decision log entries D30-D35 | T7.1-T7.5 | M | **done** |

**Exit criteria (met):** a seeded run turns a price model and a description of how the wheel is run
into a distribution of after-cost, after-tax outcomes; the same seed reproduces it to the cent; one
path is exactly the deterministic projection of the script that path writes; the simulated mean sits
inside a three-standard-error band that narrows as the square root of the number of paths. 46 tests
in the simulation module, green in both Debug and Release.

> **A script cannot be written before the run it describes.** Whether a cycle writes a put or a call
> follows from whether the previous one was assigned, which follows from the path, so the script is
> produced one cycle at a time from where the wheel actually stands
> ([D32](documentation/design/decisions.md)). The strategy layer is untouched: it still receives
> periods and still prices nothing ([D25](documentation/design/decisions.md)).

> **The path is sampled exactly, on the dates the strategy reads.** The log increment of geometric
> Brownian motion is normal over any interval, so there is no step-size error to trade against run
> time, and a run costs a few dozen draws rather than a few thousand
> ([D31](documentation/design/decisions.md)). A daily grid is still generated when the purpose is to
> feed the estimators, and a path so generated returns the volatility it was made with.

> **Deviations from the acceptance criteria.** T7.2's Dormand-Prince alternative is not used: the
> drift of the log price is constant, the fixed-step Runge-Kutta rule integrates it exactly, and an
> adaptive integrator would only choose the same step. T7.3's "N=1 reproduces the deterministic
> projection" is expressed as the strongest form available — one path equals a projection driven
> directly by the periods that path writes — because the hand-checked table of T5.6 belongs to a
> price path chosen by hand and no seed produces it. Two structural limits are recorded rather than
> worked around: a covered call is never offered a roll, because repurchasing one needs cash the
> shares cannot raise ([D34](documentation/design/decisions.md)), and the strike comes from the model
> rather than from a chain, which arrives with T9.3 ([D33](documentation/design/decisions.md)).

---

### Epic 8 — Optimizer (M6) — **complete**

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T8.1 | Objective adapter | `optimize/StrategyObjective.hpp` implementing the toolbox `ObjectiveFunction` interface over `{target delta, contract count, cash buffer}` → after-tax CAGR. Tests: objective is finite and deterministic for a fixed seed | T7.4, T6.4 | M | **done** |
| T8.2 | Optimizer wrapper | `optimize/StrikeSizingOptimizer.hpp` on toolbox `GradientDescent` + L1/L2 regularization. Tests: on a **convex synthetic** objective the known optimum is found within tolerance | T8.1 | M | **done** |
| T8.3 | Constraints | Penalty terms for max drawdown and assignment probability. Tests: a constraint-violating optimum is rejected in favour of a feasible one | T8.2 | M | **done** |
| T8.4 | Recommendation output | `optimize/Recommendation.hpp` — chosen delta/contracts/buffer plus the achieved objective and binding constraints. Tests: output is reproducible for a fixed seed | T8.3 | S | **done** |
| T8.5 | Epic 8 documentation | `design/optimization.md`, `theory/searching-for-a-setting.md`, decision log entries D36-D40 | T8.1-T8.4 | M | **done** |

**Exit criteria (met):** a search over how the wheel is run returns a setting that the stated range
could have produced, the growth it achieved, and which risk limits it is up against; the same
experiment returns the same setting to the last digit; on a convex synthetic objective the known
optimum is found, and the shrunk and soft-thresholded answers match their closed forms exactly; a
drawdown limit is met by holding back cash and an assignment limit by rolling sooner, each at a
measured cost in growth. 44 tests in the optimisation module, green in both Debug and Release.

> **A contract count is not a lever.** A cash-secured position writes as many contracts as its
> uncommitted cash can collateralise ([D21](documentation/design/decisions.md)), so choosing a count
> and choosing a cash buffer are the same choice written twice
> ([D36](documentation/design/decisions.md)). The three chosen are the target delta, the buffer, and
> the delta at which a position is rolled away - the last being the lever that trades premium against
> assignment, and therefore the one an assignment limit acts on.

> **Every candidate is scored against the same futures.** The seed belongs to the experiment, not to
> the scoring ([D38](documentation/design/decisions.md)). What a comparison then measures is the
> difference between two strategies rather than between two samples, which is what makes a search
> usable at 64 paths at all. The same decision fixes the other half of the problem: the score is a
> staircase, because contracts are whole and strikes are listed, so its slope is a difference taken
> over a sixteenth of the range rather than over an infinitesimal, which would be exactly zero.

> **Deviations from the acceptance criteria.** T8.1's parameter vector is `{target delta, cash
> buffer, roll delta limit}` rather than `{target delta, contract count, cash buffer}`, for the
> reason above. The toolbox objective interface is implemented, but behind an interface of this
> project's own, so that no toolbox type reaches a public API (§0.2). T8.2's regularization is
> applied to the *change* to the setting rather than to the setting itself
> ([D40](documentation/design/decisions.md)): charging for the size of a target delta would pull
> every recommendation towards writing nothing, which is not a preference anybody expressed. T8.3's
> penalties are linear in the excess and never refuse a setting outright
> ([D39](documentation/design/decisions.md)), so a search under limits no setting can meet still
> reports how close it is possible to get.

---

### Epic 9 — I/O and reports (M7) — **complete; rescoped: the GUI is the interface, so there is no CLI**

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T9.1 | ~~JSON reading utility~~ | **struck** - emil already provides it: `infra/syntax/JsonFileReader.hpp` owns and validates a document, `JsonObjectNavigator.hpp` navigates it by key and refuses a missing or mistyped key by name. Loaders use those directly | — | — | **not needed** |
| T9.2 | Price CSV loader | `io/PriceCsvLoader.hpp` for `date,close`. Tests: golden file; unsorted dates rejected; duplicate dates rejected; CRLF tolerated | T1.2 | M | **done** |
| T9.3 | ~~Options chain loader~~ | **struck** — strikes come from the model on a listed grid ([D33](documentation/design/decisions.md)), and a snapshot chain cannot price a simulated future anyway. The `io/ChainSource` seam is left for whenever a live provider arrives | — | — | **deferred** |
| T9.4 | Scenario loader | `io/ScenarioLoader.hpp` — capital, horizon, currency, global levers, **a list of underlyings** (ticker, spot, volatility, drift, dividend yield, strike increment), reference date, seed, links to tax + broker configs. Reads with emil's navigator and **writes back with emil's `JsonFormatter`**, because the GUI saves. Tests: `config/scenarios/example.json` round-trips; a scenario with three underlyings round-trips **in order**; an empty list, a duplicate ticker and a mistyped field are each refused naming the offending key | T6.1, T4.4, T10.2 | M | **done** |
| T9.5 | Report writer | `io/ReportWriter.hpp` — the per-cycle table and the ledger as CSV, one row per cycle per underlying. This is what the GUI's *Export* does; there is no console table. Tests: golden output for a fixed book projection | T10.3 | M | **done** |
| T9.6 | ~~CLI~~ | **struck** — replaced by the desktop application (Epic 11). The end-to-end golden-run test it was carrying moves to T11.8 | — | — | **replaced** |
| T9.7 | Methodology docs | `documentation/theory/methodology.md` tying the theory notes into one end-to-end description (formulas, assumptions, limits) + the §9 checklist in `documentation/validation/README.md` filled in | T11.8 | M | **methodology done; the checklist waits on T11.8** |

**Exit criteria (met, except the checklist that waits on Epic 11):** a price history is read from a
file a data provider would actually produce, and every way it can be wrong is reported naming the row
rather than aborting; a scenario naming three underlyings is read, written and read back as the same
scenario, order included; a book projection exports a cycle table and a ledger whose every figure is
a whole number of cents; and the whole chain from a price to a recommendation is written down once,
end to end, with its limits collected in one table. 61 tests in the I/O module, green in both Debug
and Release.

> **Nothing in a scenario is a decimal.** Amounts are whole cents and proportions are whole basis
> points ([D48](documentation/design/decisions.md)), which is what makes a saved scenario reload as
> *the same* scenario rather than as one that is nearly it. It is also what keeps the promise that
> this project writes no parser: whole numbers are the one thing a document format reads without
> ambiguity. The cost is stated - a volatility is settable to a hundredth of a percent and no finer,
> and the seed is bounded by what such a file can hold.

> **A price file is met halfway.** Published closes carry more decimals than money has; they are
> rounded to the nearest cent, ties to even, on the digits as written
> ([D47](documentation/design/decisions.md)). Refusing them would reject almost every real file and
> truncating them would bias every price downwards, which compounds exactly like a return does.

> **Deviations from the acceptance criteria.** T9.2's currency and day count are supplied by the
> caller rather than read from the file, because a column of numbers cannot say what currency it is
> in and the day count is a modelling choice, not data. T9.4's scenario carries the terms of a book
> directly rather than restating them, so the loader adds no shape of its own; it also carries the
> path count, which the acceptance criteria did not name but a run needs. T9.5's tax appears in the
> ledger and never in a cycle row ([D49](documentation/design/decisions.md)) - a charge levied on a
> whole book cannot honestly be attributed to one of its names. T9.7's checklist half is not filled
> in: it depends on T11.8, which does not exist yet.

---

### Epic 10 — A book of N underlyings (M8) — **complete**

The strategy stops being about one ticker. Everything below the book is left alone except the one
thing that cannot be: a holding has to say what it is a holding *of*.

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T10.1 | Holdings gain a ticker | `strategy/Portfolio.hpp`: a share lot names its underlying; cover is judged per underlying; the value of the holdings is asked for against a price **per underlying**. Tests: the existing single-name invariants survive verbatim under one ticker; shares of one name never cover a call on another; assignment stays exactly value-neutral with two names held | T5.1 | M | **done** |
| T10.2 | A shared account | `strategy/Account.hpp` — the cash, the ledger and the tax model as one thing that several wheels can be run against; `Projection` gains a constructor over an account it does not own, keeping the one it does. Tests: a projection over its own account and one over a shared account of the same opening balance produce identical rows — the Epic 5 hand-checked table, run both ways | T10.1, T5.6 | M | **done** |
| T10.3 | `Book` | `strategy/Book.hpp` — N wheels on one clock over one account, advancing a cycle at a time in two passes. Tests: N=1 is the single-name projection cent for cent; the pot is never overdrawn; the ledger reconciles with N wheels; two names together never commit more than one wheel would on the same path | T10.2 | L → split allocation out if it grows | **done** |
| T10.4 | Allocation rule | `strategy/CollateralAllocation.hpp` — wheels needing no fresh cash first, then cash-secured puts by descending premium per unit of collateral, ties in scenario order. A named, substitutable rule, not a loop inside the book. Tests: a richer premium is served first whatever order the scenario lists; an exact tie goes to the earlier row; the wheel that finds the pot empty writes nothing and is not an error; the whole ordering of a cycle asserted against a literal expected sequence | T10.3 | M | **done** |
| T10.5 | One assessment for the book | The wealth tax struck once, on the whole book, from a price per underlying at the reference date; a wheel inside a book never assesses. Tests: the charge on a book equals the charge on the summed holdings and **not** the sum of N separate charges (the allowance is not multiplied by N — this is the mistake this test exists to prevent); a book below the allowance is charged nothing | T10.3, T6.4 | M | **done** |
| T10.6 | Simulating a book | `sim/BookScript.hpp` (per-underlying terms over the global levers) and a generator per underlying, drawn **independently**; `sim/BookMonteCarlo.hpp` runs a book. Tests: a one-underlying book runs; the same seed reproduces an N-underlying run to the cent; adding a second uncorrelated name of the same market narrows the distribution — asserted as the known limitation it is | T10.3, T7.3 | L | **done** |
| T10.7 | Progress and cancellation | `sim/RunObserver.hpp` — an interface the simulator reports each finished path to, and which may ask it to stop; the run returns the distribution of the paths it did finish. **No Qt anywhere near it.** Tests: a run reports exactly N completions in order; a run stopped at path k returns k outcomes and stops drawing; watching a run does not change it | T7.3 | M | **done** |
| T10.8 | Searching over a book | `optimize/StrategyObjective.hpp` scores a **book** under one global setting. The point stays three-dimensional ([D36](documentation/design/decisions.md)). Tests: a book is scored deterministically and its slope is finite across the box; a book of several names is not scored as the one listed first; a drawdown limit still binds | T10.6, T8.4 | M | **done** |
| T10.9 | Epic 10 documentation | `design/book.md`, `theory/running-several-wheels-at-once.md` (the shared pot, the allocation rule and a hand-checked two-name run), decision log entries D41–D46, and the independence limitation recorded in `validation/README.md` | T10.1–T10.8 | M | **done** |

**Exit criteria (met):** a scenario naming several underlyings is run out of one pot of cash; the
pot is never overdrawn and no unit of collateral backs two positions; the wealth tax is struck once
on the whole book; one underlying reproduces the single-name projection cent for cent; a run can be
watched and stopped without the engine knowing what is watching it. 116 tests in the strategy
module, 61 in the simulation module and 46 in the optimiser, green in both Debug and Release.

> **The wheel is not generalised; it is shared.** A projection keeps its single-name contract and
> its hand-checked table — what changes is that several of them can be pointed at the same cash.
> The alternative, one projection holding a book of wheel states, would have rewritten the tests
> that make the arithmetic checkable on paper.

> **The order of the underlyings is a modelling input.** It is not presentation: it breaks ties in
> who gets the collateral, so reordering the rows of a scenario is a change to the scenario. The
> GUI's list is therefore explicitly reorderable and the loader preserves order.

> **Two things had to change below the book that the plan did not anticipate.** The first is that a
> cycle is written by every wheel before it is settled by any
> ([D42](documentation/design/decisions.md)): run each wheel from writing to expiry in turn and the
> first releases its collateral before the second asks for it, so nothing ever competes and the book
> silently writes several times the positions it can pay for. The second is that the record of
> cashflows now keeps itself in date order rather than demanding it of the caller
> ([D41](documentation/design/decisions.md)), because whichever wheel is served second posts an
> opening cashflow after the first has already posted one at expiry.

> **Deviations from the acceptance criteria.** T10.5's charge is struck at a cycle boundary rather
> than at the reference date itself ([D44](documentation/design/decisions.md)), because that is the
> one moment at which nothing is open, so what is held is unambiguous and the balance is at its
> least committed. T10.6's "a one-underlying book reproduces the Epic 7 distribution for the same
> seed" is not asserted: a book assesses at a boundary and a wheel assesses mid-cycle, so the two
> differ by one charge's timing — the cent-for-cent equality is asserted where it is exact, on a
> scripted book against a scripted wheel. T10.8's "reproduces the Epic 8 recommendation" became
> "scores a book deterministically", since the objective now takes a book and the single-wheel
> experiment it would have been compared against no longer exists. Assignment frequency is measured
> per option written rather than per cycle ([D46](documentation/design/decisions.md)), because a
> book of three names has three chances a cycle.

---

### Epic 11 — Desktop application (M9)

Qt 6 Widgets, from the system, a hard build dependency. Qt Base only — no Charts, no QML. Every
chart is drawn with `QPainter` over geometry computed in a Qt-free layer.

| ID | Task | Deliverable / Acceptance criteria | Deps | Size | Status |
|---|---|---|---|---|---|
| T11.1 | Qt in the build | `find_package(Qt6 REQUIRED COMPONENTS Widgets Test)` with a stated minimum, `AUTOMOC` on the GUI targets only, `fin.gui` and `fin.app` added, GUI tests registered with `emil_add_test` and given `QT_QPA_PLATFORM=offscreen`. CI installs Qt explicitly. Acceptance: a window with nothing in it opens on a desktop and its test passes with no display; warnings-as-errors still holds, moc output included | — | M | **done** |
| T11.2 | Scenario document | `present/ScenarioDocument.hpp` — the edited scenario, Qt-free: capital, horizon, global levers, an ordered list of underlying rows, add/remove/move, and a validity report naming the first offending field. Tests: a row with a non-positive spot, a duplicate ticker, an empty list and a delta outside its range are each refused by name; moving a row changes the order and nothing else | T9.4 | M | **done** |
| T11.3 | Run controller | `present/RunController.hpp` — starts a projection, a simulation or a search, exposes progress as completed/total, cancels. Implements the T10.7 observer. Tests: progress reaches exactly total; cancelling mid-run leaves a partial, readable result and reports itself as cancelled; a failed run reports the failure rather than throwing at the caller | T10.7 | M | **done** |
| T11.4 | Result models | `present/ResultTable.hpp` (per-cycle rows and the ledger, as formatted columns) and `present/FanChartSeries.hpp` (percentile bands and the median as point sequences in model coordinates, plus the axis ranges and tick positions). Tests: money is formatted to the cent and never as a float; the bands are ordered P5 ≤ P50 ≤ P95 at every date; the axis of a flat series is still non-degenerate | T10.6 | M | **done** |
| T11.5 | Main window shell | `gui/MainWindow.hpp` — scenario panel left, results right, the underlying table with add/remove/reorder, the global levers, the run buttons, a progress bar and *Cancel*. Qt Test, offscreen: the window builds; typing into a field reaches the document; an invalid field disables *Run* and shows why; *Cancel* is enabled only while running | T11.2, T11.3 | L → one panel per commit | **done** |
| T11.6 | Fan chart widget | `gui/FanChartWidget.hpp` — `QPainter` bands, median, axes, ticks; resizes without redistributing the geometry. Qt Test, offscreen: rendering into an image produces the expected pixel at the median at a known date, and an empty series paints an empty frame rather than crashing | T11.4 | M | **done** |
| T11.7 | Prices, save, export | Load a price CSV per underlying and show the estimated volatility and drift it implies, with a button to adopt them into the row; save and load the scenario as JSON; export the results as CSV. Qt Test, offscreen: adopting an estimate changes exactly the two fields; save-then-load is the identical document, order included; export writes the T9.5 golden file | T11.5, T9.2, T9.4, T9.5 | M | **done** |
| T11.8 | End-to-end golden run | The example scenario, three underlyings, a fixed seed: opened, run, and its table and recommendation compared against a committed golden file — the test T9.6 used to carry. Acceptance: green under `offscreen` with no display, and reproducible between Debug and Release | T11.7, T10.8 | M | **done**, table only |
| T11.9 | Epic 11 documentation | `design/presentation.md` (what the window shows and why the model of it is Qt-free), decision log entries D53–D56, and the disclaimer shown beside every result recorded in `validation/README.md` | T11.1–T11.8 | M | |

**Exit criteria:** a scenario of several underlyings is edited, saved, reloaded, run, watched,
cancelled and exported from a window; every result carries the independence and model-risk
disclaimers; the whole suite, GUI included, is green with no display attached; and no type below
`present/` mentions Qt.

> **The window is tested, not screenshotted.** Widget tests drive real widgets under the `offscreen`
> platform plugin: they type, click, and read back. What they deliberately do not assert is
> appearance — the geometry a chart is drawn from is asserted in the Qt-free layer, where it is
> arithmetic, and only one pixel-level check exists to prove the painting maps geometry to the
> frame at all.

> **Deviation from the project's test convention.** GUI tests use Qt Test rather than GoogleTest,
> because a Qt Test binary supplies its own `main`. They are still registered with `emil_add_test`,
> which only names the executable, so the suite is one `ctest` run as before.

---

### Suggested implementation order

Epic 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → **10 → 9 → 11**.

Epic 10 now comes before Epic 9: the scenario loader (T9.4) and the report writer (T9.5) both have
to know about a list of underlyings, and writing them against a single-name model first would mean
writing them twice. T9.2 (the price CSV loader) is independent of all of it and can be pulled
forward at any time. Within Epic 10, T10.1 and T10.2 are the only ones that touch existing code;
everything after them is additive.

**Next task to pick up:** T11.9 (Epic 11 documentation), the last task of the plan.

> **Deviation from T11.8.** The golden run compares the cycle table and the ledger, not the
> recommendation. A recommendation is the endpoint of a gradient descent over doubles, and the
> toolbox's descent header compiles under fast maths, so it is not the kind of thing that can be
> promised to land on the same value in Debug and in Release. The table is whole cents throughout
> and is asserted in both.

---

### Epic 12 — Audit follow-ups

The High findings of the audit in `documentation/audit/`, taken in the order the owner chose.

| ID | Task | Detail | Depends | Size | Status |
|---|---|---|---|---|---|
| T12.1 | A charge free cash cannot meet is borrowed | `strategy/Portfolio` lets a payment with nothing reserved take the balance below zero; no put while below; `BrokerFees::DebitInterest` at the configured `debit_interest_basis_points`, posted as an `Interest` cashflow at the cycle's end; a simulation reports the share of paths that borrowed (D50). Tests: the book that used to abort borrows, charges 0.28 of interest and repays; the example scenario completes 10,000 paths | SIM-1, STRAT-1 | M | **done** |
| T12.2 | Dividends accrue from the yield | `ProjectionPeriod` carries the dividend per share and the withholding rate; shares held at a cycle's opening receive it net at its end; `dividend_withholding_basis_points` per underlying; a `dividend` column in the cycle table (D51) | STRAT-2 | M | **done** |
| T12.3 | Implied volatility separate from the path's | `implied_volatility_basis_points` per underlying prices the premium, the roll value, the delta and the strike; the path moves with `volatility_basis_points` (D52) | SIM-3 | S | **done** |

### Epic 13 — Candidate underlyings and calibration

The owner's three asks after the audit, taken one step at a time: names to point the model at, a
better interface, and a calibration that follows the market.

| ID | Task | Detail | Depends | Size | Status |
|---|---|---|---|---|---|
| T13.1 | BENELUX option candidates | A screen of Euronext Amsterdam and Brussels single-stock option classes for the wheel: option liquidity first, then a share price that fits the capital (one contract at or under about 5% of EUR 200,000), then at most two names per sector. Public-web sweep of 96 names, 53 fact sheets each challenged by three independent reviewers, three rankings and one synthesis, a completeness check; the ten picks and five alternates in `documentation/research/benelux-option-candidates.md`, the figures in `config/scenarios/benelux.json` | Epic 12 | M | **done** |
| T13.2 | Rule-based rolling re-estimation | Drift and volatility re-estimated from the price history on a rolling window as a run advances (the EMA and RLS estimators exist and are unused), with the rules stated as decisions and the backtest on the ten names; price histories supplied by the owner as `date,close` files | T13.1, T9.2 | L | pending |

### Epic 14 — Qt Quick interface

The owner's second ask after the audit: a better interface to see and work the parameters. Qt Quick
(QML) with the Material style replaces the Widgets window; the presentation layer stays Qt-free and
unchanged. The UX rules live in `.claude/skills/qt-ux/SKILL.md`.

| ID | Task | Detail | Depends | Size | Status |
|---|---|---|---|---|---|
| T14.1 | Models over the presentation layer | `gui/ScenarioModel`, `gui/UnderlyingsModel`, `gui/RunModel`, `gui/ResultTableModel`, `gui/ChartSeries`, `gui/Workspace`: properties and invokables QML binds to; every write goes through the document, every refusal is the document's sentence naming the field. Tests keep the golden projection check and every behaviour the Widgets tests pinned | Epic 11 | M | **done** |
| T14.2 | The window in QML | Scenario pane grouped as capital and horizon, levers, tax and simulation, underlyings; each field in its stored unit with the figure in words beneath; the results pane with highlight cards, summary, fan chart, cycles and ledger; progress and cancel; light and dark themes; keyboard shortcuts | T14.1 | L | **done** |
| T14.3 | Fan chart as a painted item | `gui/FanChartItem` paints the same geometry the Widgets chart did; one pixel of the median asserted in a test | T14.1 | S | **done** |
| T14.4 | Screenshots and a scripted tour | `FIN_BS_DEMO=<folder>` drives the window through a tour offscreen and writes a picture per step; pictures are produced on demand, not committed | T14.2 | S | **done** |
| T14.5 | Per-name collateral cap in the book | The BENELUX scenario showed the richest-premium-first rule handing the pot to one name; a cap per name, stated in the scenario, is the next modelling task before the calibration step | T13.1 | M | pending |

## 9. Validation Checklist (do before trusting numbers)

Tracked, with sources and dates, in
[`documentation/validation/README.md`](documentation/validation/README.md). Read
[`documentation/theory/methodology.md`](documentation/theory/methodology.md) first: it states every
assumption of the chain at once and collects the known limits in one table.

- [ ] Confirm the active-year **Box 3** parameters against Belastingdienst for your tax year.
- [ ] Confirm the **DEGIRO option contract charge** — the rest of the fee schedule (shares, handling,
      connectivity, AutoFX) was confirmed on 2026-08-11.
- [ ] Black–Scholes prices/Greeks match an independent reference within tolerance.
- [ ] Understand that a book's underlyings are drawn **independently**, so every drawdown and
      assignment rate it reports is optimistic (§6.7).
- [x] Estimators recover known σ/μ from synthetic GBM — done in T3.2/T3.4.
- [x] Monte-Carlo mean converges to closed-form expectation — done in T7.5.
- [x] Money math uses integer cents, not `float` — done in T1.1.

---

## 10. Open Questions — settled

1. **Strategy:** covered call + cash-secured put wheel. Confirmed.
2. **Underlyings:** several at once, all EUR-denominated in v1, each with its own market and strike
   grid, wheeled out of one pot of cash (§0.1a). Mixed currencies are a later task.
3. **Tax:** private Box 3 investor; the year is selected by the scenario.
4. **Price history:** `date,close` CSV, loaded per underlying (T9.2), with the estimated volatility
   and drift shown before they are adopted (T11.7).
5. **`float` vs `double`:** settled — integer cents for money, `double` for rates and volatilities
   (T1.1, decisions D1/D3).
6. **Interface:** a Qt 6 Widgets desktop application, no CLI (§4.9, Epic 11).

Still open, and deliberately deferred rather than forgotten:

- Correlation between underlyings — needs a correlation matrix in the scenario and a factorisation
  of the covariance; the toolbox has Cholesky.
- A better allocation rule than "richest premium first", once there is something to compare it to.
- Staggered expiries, so the wheels do not all turn on the same day.
