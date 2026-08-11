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

- [ ] Active-year **Box 3** parameters confirmed against Belastingdienst.
- [ ] Current **DEGIRO fee schedule** confirmed for options, shares and currency conversion.
- [ ] **Black-Scholes** prices and Greeks match an independent reference within tolerance.
- [ ] **Estimators** recover a known $\sigma$ and $\mu$ from synthetic geometric Brownian motion.
- [ ] **Monte-Carlo** mean converges to the closed-form expectation.
- [x] Cash arithmetic is counted in whole minor units, not in floating point.

## Already settled

| Item | Decision | Where |
|---|---|---|
| Cash precision | whole minor units, half-to-even rounding | [theory](../theory/money-and-rounding.md), [D1](../design/decisions.md) |
| Day-count conventions | ACT/365F and ACT/252, no holiday calendar | [theory](../theory/day-count-conventions.md) |
| Rate convention | continuously compounded as the canonical form | [theory](../theory/rates-and-compounding.md) |
| Contract multiplier | 100 by default, part of a contract's identity | [theory](../theory/option-contract-conventions.md) |
