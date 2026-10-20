# Documentation

Why this project models what it models, and which formulas it uses.

These documents are written in **domain terms**. They describe quantities, conventions, derivations
and decisions - not the software that implements them. Every number in them can be reproduced by
hand or in a spreadsheet, and none of them should need rewriting if the implementation is replaced.

> ⚠️ **Not financial or tax advice.** Every number this project produces is a model output.

## Folder layout

| Folder | Answers the question | Typical content |
|---|---|---|
| [`design/`](design/) | *Why is the model shaped like this?* | Quantities and their invariants, trade-offs, the decision log |
| [`theory/`](theory/) | *What is being computed, and why is that correct?* | Definitions, derivations, worked examples, edge cases |
| [`validation/`](validation/) | *Do we trust the inputs and outputs?* | Authoritative sources for tax and fee parameters, dates checked, the pre-use checklist |
| [`research/`](research/) | *Which markets and names is the model pointed at?* | Screens of candidate underlyings, with the criteria, the sources and the dates checked |

The split matters: a `theory/` document states facts about finance and mathematics that are true
regardless of this project, while a `design/` document records choices this project made and may
one day reverse.

## Conventions

- One document per topic, kebab-case filename.
- Start from [`TEMPLATE.md`](TEMPLATE.md) - drop the sections that do not apply rather than leaving
  them empty.
- Maths in LaTeX: `$inline$` and `$$block$$`.
- State units and sign conventions explicitly. Unit confusion is the most common source of
  finance errors.
- Every numeric claim is hand-checkable, and every hand-checkable claim is also verified
  automatically. If a figure appears in a table here, something checks it.
- Anything sourced from the outside world - tax rules, fee schedules, reference option prices - is
  logged in [`validation/`](validation/) with a URL and the date it was checked.
- No identifiers, no file names, no snippets. If a sentence only makes sense to someone reading the
  implementation, it belongs in the implementation, not here.

## Index

### Design

- [Quantities and their conventions](design/quantities.md) - money, dates, year fractions, interest
  rates, volatility, delta, option contracts.
- [Option valuation contracts](design/valuation.md) - what the valuation layer requires, guarantees
  and refuses to express.
- [Estimation contracts](design/estimation.md) - what a price history must be, what the estimators
  guarantee, and what they refuse to answer.
- [Cost contracts](design/costs.md) - what a fee schedule must be, what the cost layer guarantees,
  and where a schedule that is wrong is caught.
- [Strategy contracts](design/strategy.md) - what a portfolio may do, how cashflows are recorded,
  which transitions of the cycle exist, and what a projection reports.
- [Tax contracts](design/tax.md) - what a set of tax rules must be, what the tax layer guarantees,
  and why it cannot be handed an income.
- [Simulation contracts](design/simulation.md) - what a simulated run must be given, what it
  guarantees about reproducibility, and which options it refuses to write or close.
- [Optimisation contracts](design/optimization.md) - what a search over a strategy must be given,
  what it guarantees about an answer, and the size of position it refuses to let anyone state twice.
- [What is read and what is written](design/io.md) - what a price history, a scenario and a report
  must be, and which of their properties are modelling inputs rather than presentation.
- [Decision log](design/decisions.md) - numbered, dated modelling decisions.

### Theory

- [Money and rounding](theory/money-and-rounding.md) - minor units, half-to-even, error bounds.
- [Day-count conventions](theory/day-count-conventions.md) - ACT/365F, ACT/252, year fractions.
- [Rates and compounding](theory/rates-and-compounding.md) - continuous vs. effective annual.
- [Volatility scaling](theory/volatility-scaling.md) - the square-root-of-time rule and its limits.
- [Option contract conventions](theory/option-contract-conventions.md) - multiplier, notional,
  moneyness, delta.
- [Black-Scholes-Merton option valuation](theory/black-scholes.md) - the formula, its assumptions,
  parity and the no-arbitrage bounds.
- [Option Greeks](theory/option-greeks.md) - definitions, units, sign conventions, finite-difference
  verification.
- [Implied volatility](theory/implied-volatility.md) - inverting a price, when it has no answer, and
  how accurately it can be known.
- [Assignment and early exercise](theory/assignment-and-early-exercise.md) - the moneyness rule and
  the dividend-driven early assignment of a call.
- [Log returns and realised volatility](theory/log-returns-and-volatility.md) - log returns,
  annualisation, and the bias that quoting in minor units introduces.
- [Drift estimation](theory/drift-estimation.md) - the drift of the price against the drift of its
  logarithm, fitting the slope, and how little the answer is known.
- [Exponentially weighted volatility](theory/exponentially-weighted-volatility.md) - smoothing
  factors, half-lives, forgetting factors, and the trade between responsiveness and precision.
- [Calibration from a price history](theory/calibration.md) - the window, the estimators, the
  implied premium, what a backtest may look at, and why the drift is the loud figure.
- [Broker costs](theory/broker-costs.md) - charges per contract, per transaction and in proportion to
  a portfolio, with a ceiling, and why the ceiling matters.
- [Currency conversion](theory/currency-conversion.md) - the base currency, the spot rate, the
  handling cost, and the cost of a round trip.
- [The wheel](theory/wheel-strategy.md) - the cycle of cash-secured puts and covered calls, what each
  leg settles, and what a cycle is worth.
- [Position sizing and compounding](theory/position-sizing-and-compounding.md) - free capital, the
  cash buffer, whole contracts, and why the granularity is a drag.
- [Rolling a short option](theory/rolling-a-short-option.md) - the time, profit and risk triggers,
  what a roll costs, and why a rolled position cannot be assigned.
- [Wealth taxation of an investment portfolio](theory/box3-taxation.md) - why the tax falls on what
  is held on one date a year rather than on what was earned, and what that does to a strategy whose
  income is large relative to its capital.
- [Monte-Carlo projection of a wheel](theory/monte-carlo-simulation.md) - the price process, why the
  path is sampled exactly, how many paths an average needs, and how outcomes become percentiles.
- [Searching for a setting](theory/searching-for-a-setting.md) - the three levers a wheel is run
  with, why every candidate is scored against the same futures, and how a risk limit becomes part of
  the quantity being maximised.
- [How a projection is put together](theory/methodology.md) - the whole chain in the order a run
  happens, every assumption stated where it enters, and the limits collected in one place.

### Validation

- [Validation index & checklist](validation/README.md).
- [External parameter sources](validation/sources.md).

### Research

- [BENELUX option candidates](research/benelux-option-candidates.md) - ten Euronext Amsterdam and
  Brussels names screened for option liquidity, share price against the capital, and sector spread,
  with the scenario figures derived from them.
- [BENELUX backtest](research/benelux-backtest.md) - the ten names replayed over the year to
  2026-10-19 with the terms re-estimated at every opening, against the simulation's expectation.
