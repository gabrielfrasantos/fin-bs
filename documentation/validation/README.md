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
model they refer to do not exist.

- [ ] Active-year **Box 3** parameters confirmed against Belastingdienst - the allowance, the two
      imputed returns and the tax rate for 2025 are confirmed; the **dividend withholding rate is
      not**, and none of them is confirmed for any other year.
- [ ] Current **DEGIRO fee schedule** confirmed for options, shares and currency conversion -
      shares, handling, connectivity and currency handling are confirmed; the **option contract
      charge is not**, and it is the cost this project is most sensitive to.
- [x] **Black-Scholes** prices and Greeks match an independent reference within tolerance.
- [x] **Estimators** recover a known $\sigma$ and $\mu$ from synthetic geometric Brownian motion.
- [ ] **Monte-Carlo** mean converges to the closed-form expectation.
- [x] Cash arithmetic is counted in whole minor units, not in floating point.

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
| Borrowing | not modelled; wealth is non-negative | [D29](../design/decisions.md) |
