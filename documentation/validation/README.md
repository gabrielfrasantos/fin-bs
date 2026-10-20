# Validation

Nothing in this folder is derived from the model. It records where **external, mutable facts** came
from, and when they were last confirmed. Tax rules and fee schedules change; a projection built on a
stale parameter is wrong in a way no automated check can detect.

## Rules

- Every parameter that originates outside this project gets a row in [sources.md](sources.md): what
  it is, its value, the authoritative URL, and the date checked.
- Re-confirm before trusting any projection, and always after a calendar-year rollover - Dutch Box 3
  parameters are versioned per tax year.
- Every such parameter is held as configuration data, never fixed inside the model.
- If a source cannot be confirmed, the parameter stays out of the default configuration.

## Pre-use checklist

Mirrors [PLAN.md](../../PLAN.md) section 9. Most of these cannot be ticked yet; the parts of the
model they refer to do not exist. Read
[how a projection is put together](../theory/methodology.md) first - it is the only document that
states every assumption of the chain at once, and it collects the known limits in one table.

- [ ] Active-year **Box 3** parameters confirmed against Belastingdienst - the allowance, the two
      imputed returns and the tax rate for 2025 are confirmed; the **dividend withholding rate is
      not**, and none of them is confirmed for any other year.
- [ ] Current **DEGIRO fee schedule** confirmed for options, shares and currency conversion -
      shares, handling, connectivity and currency handling are confirmed; the **option contract
      charge is not**, and it is the cost this project is most sensitive to.
- [x] **Black-Scholes** prices and Greeks match an independent reference within tolerance.
- [x] **Estimators** recover a known $\sigma$ and $\mu$ from synthetic geometric Brownian motion.
- [x] **Monte-Carlo** mean converges to the closed-form expectation - the simulated mean terminal
      price sits inside a three-standard-error band at a thousand, ten thousand and a hundred
      thousand paths, and the band narrows as the square root of that count.
- [x] Cash arithmetic is counted in whole minor units, not in floating point.
- [ ] **Understand that the underlyings of a book are drawn independently.** Real equities fall
      together, and nothing in the model says so. Every drawdown and every assignment rate a book
      reports is therefore optimistic, and the more names in the book the more optimistic it gets:
      adding an uncorrelated name of the same market narrows the distribution of outcomes in a way
      no real second holding would. This is the largest known error in a book's numbers and it
      cannot be confirmed away - it can only be modelled, which is a later task. See
      [D45](../design/decisions.md).
- [ ] **Understand that collateral goes to the richest premium first.** That is a rule, not an
      optimum, and it concentrates a book in whatever is most volatile - which is also whatever is
      most likely to be assigned. The cap per name ([D54](../design/decisions.md)) bounds how far
      that goes; set it, and check that the cycle table spreads contracts over the names you
      listed. See [D43](../design/decisions.md).

## Already settled

| Item | Decision | Where |
|---|---|---|
| Cash precision | whole minor units, half-to-even rounding | [theory](../theory/money-and-rounding.md), [D1](../design/decisions.md) |
| Day-count conventions | ACT/365F and ACT/252, no holiday calendar | [theory](../theory/day-count-conventions.md) |
| Rate convention | continuously compounded as the canonical form | [theory](../theory/rates-and-compounding.md) |
| Contract multiplier | 100 by default, part of a contract's identity | [theory](../theory/option-contract-conventions.md) |
| Option valuation | European with a continuous dividend yield | [theory](../theory/black-scholes.md), [D8](../design/decisions.md) |
| Assignment | deterministic; early exercise modelled for dividends only | [theory](../theory/assignment-and-early-exercise.md), [D10](../design/decisions.md) |
| Proportional costs | whole basis points, one rounding, half-to-even | [theory](../theory/broker-costs.md), [D18](../design/decisions.md) |
| Configured cash | stated directly in minor units, never as a decimal | [D20](../design/decisions.md) |
| Currency conversion | spot amount and handling cost reported separately | [theory](../theory/currency-conversion.md), [D19](../design/decisions.md) |
| Basis of taxation | wealth on one reference date, never income | [theory](../theory/box3-taxation.md), [D26](../design/decisions.md) |
| Price process | geometric Brownian motion, sampled exactly on the dates read | [theory](../theory/monte-carlo-simulation.md), [D31](../design/decisions.md) |
| Reported percentiles | an outcome at a rank, never an interpolation between two | [theory](../theory/monte-carlo-simulation.md), [D35](../design/decisions.md) |
| Borrowing | not modelled; wealth is non-negative | [D29](../design/decisions.md) |
| Sharing capital | one pot, richest premium per unit of collateral served first, ties by listed order, each name capped at a stated share of the opening equity | [theory](../theory/running-several-wheels-at-once.md), [D43](../design/decisions.md), [D54](../design/decisions.md) |
| Taxing a book | struck once on the whole book, at a cycle boundary, allowance granted once | [design](../design/book.md), [D44](../design/decisions.md) |
| Assignment rate | measured per option written, not per cycle | [D46](../design/decisions.md) |
| Reading a close | rounded to the nearest minor unit, ties to even, never refused | [design](../design/io.md), [D47](../design/decisions.md) |
| Writing a scenario | whole minor units and whole basis points only, so a save reloads identically | [design](../design/io.md), [D48](../design/decisions.md) |
| Reporting the tax | one row of the ledger, never a column of the cycle table | [design](../design/io.md), [D49](../design/decisions.md) |
