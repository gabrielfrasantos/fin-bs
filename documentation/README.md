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

### Validation

- [Validation index & checklist](validation/README.md).
- [External parameter sources](validation/sources.md).
