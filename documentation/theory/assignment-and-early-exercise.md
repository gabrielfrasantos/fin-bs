# Assignment and early exercise

## Summary

A wheel is short options, so it does not choose whether to exercise - it is *assigned* when the
holder on the other side chooses to exercise. This document states when the model treats a short
option as assigned: a moneyness rule at expiry, and a dividend-driven rule for calls before expiry.

## Motivation

Assignment is the event that drives the whole strategy. A short put that is assigned turns cash into
shares; a short call that is assigned turns shares back into cash; the sequence of those two events
*is* the wheel. Everything the projection reports - the capital base that compounds, the transaction
costs incurred, the value of the portfolio on the tax reference date - depends on getting the timing
and the trigger right.

Two errors are worth spending effort to avoid:

- **Missing early assignment on a call.** A short covered call assigned the day before an
  ex-dividend date hands over the shares *and* the dividend. A model that only ever assigns at
  expiry overstates dividend income and understates turnover, and it does so systematically, in the
  same direction, every year.
- **Treating at-the-money as assigned.** Assignment at exactly the strike is worth nothing to the
  holder. Modelling it as automatic quietly biases the share/cash mix of the portfolio.

## Definitions & notation

Notation follows [Black-Scholes-Merton option valuation](black-scholes.md). In addition:

| Symbol | Meaning | Unit |
|---|---|---|
| $S_T$ | spot price at expiry | currency |
| $D$ | dividend per share paid at the next ex-dividend date | currency |
| $I$ | intrinsic value, $\max(S-K,0)$ for a call and $\max(K-S,0)$ for a put | currency |
| $E = V - I$ | extrinsic value, also called time value | currency |

The outcome of the model is one of two states: **assigned** or **not assigned**. There is no partial
assignment: a contract is exercised in whole. A position of several contracts is assigned contract
by contract; the model does not represent a partially assigned single contract.

## Derivation / Rules

### At expiry

A rational holder exercises when, and only when, exercising is worth something:

| Contract | Assigned when | Not assigned when |
|---|---|---|
| Call | $S_T > K$ | $S_T \le K$ |
| Put | $S_T < K$ | $S_T \ge K$ |

The boundary is strict on both sides: exactly at the strike, exercise yields nothing and the option
is left to expire. In-the-money by one minor unit is enough to trigger assignment; the model does
not apply a materiality threshold.

### Before expiry

An American option can be exercised at any time, but doing so throws away its extrinsic value $E$,
which is by definition the amount by which the option is worth more alive than dead. Early exercise
is therefore only rational when something else - a payment that the option does not entitle its
holder to - is worth more than $E$. For an equity call, that something is the **dividend**.

Immediately before an ex-dividend date the holder of a call may either exercise, receive the share
cum-dividend and collect $D$, or hold and see the share drop by $D$ on the ex-date. Exercising
captures $D$ and forfeits $E$, so:

$$
\text{a short call is assigned early when} \quad I > 0 \quad \text{and} \quad D > E
$$

with $E$ evaluated over the time remaining from the ex-dividend date to expiry, on the cum-dividend
spot. The condition has two consequences that are worth reading off directly:

- **Only deep in-the-money, short-dated calls are at risk.** $E$ falls with moneyness and with
  remaining time, so the inequality only binds where extrinsic value has already been squeezed out.
- **A call on a share that pays no dividend is never exercised early.** With $D = 0$ the condition
  is unsatisfiable, which recovers the standard result and means the European value is exact in that
  case.

The extrinsic value used in the test already contains the benefit of deferring payment of the
strike - the interest earned on $K$ over the remaining life - so no separate interest term is
needed.

### What is deliberately not modelled

**Early exercise of a put.** It becomes rational when the interest earned on the strike exceeds the
remaining extrinsic value, which requires a deep in-the-money put and a meaningful interest rate. It
is left out for now for two reasons: it is rare at the rates and moneyness a wheel trades at, and
when it does happen it merely accelerates a share purchase the strategy had already committed to, so
the effect on a long-horizon projection is a timing shift rather than a change in outcome. See
[decision D10](../design/decisions.md).

**Irrational and probabilistic assignment.** Some holders exercise when they should not, and
assignment is allocated randomly among short positions by the clearing house, so an individual
account can be assigned when the aggregate is not. The model is deterministic: given the same
inputs it always returns the same outcome. Randomness in this project belongs to the price path, not
to counterparty behaviour.

**Pin risk.** When the underlying settles within a tick of the strike, whether a short option is
assigned is genuinely unknown until after the fact, and the resulting share position is unhedged
over the weekend. The model resolves the tie by not assigning.

## Worked example

A covered call with a strike of 100.00, held against a share trading at 120.00, with a volatility of
20% per annum and a risk-free rate of 3% per annum. The question is asked immediately before an
ex-dividend date.

| Time remaining after the ex-date | Option value | Intrinsic $I$ | Extrinsic $E$ | Dividend $D$ | Outcome |
|---|---|---|---|---|---|
| One week ($\tau = 0.02$) | 20.05998 | 20.00000 | 0.05998 | 0.50 | **assigned** |
| One week ($\tau = 0.02$) | 20.05998 | 20.00000 | 0.05998 | 0.01 | not assigned |
| One year ($\tau = 1.0$) | 24.54721 | 20.00000 | 4.54721 | 0.50 | not assigned |

The first two rows differ only in the size of the dividend, and the first and third only in the time
remaining. Both comparisons show the same thing: the decision is a race between the dividend and the
time value, and time value is what a longer-dated option has more of.

At expiry, the same contract:

| Spot at expiry | Outcome |
|---|---|
| 100.01 | assigned |
| 100.00 | not assigned |
| 99.99 | not assigned |

## Edge cases & pitfalls

**The extrinsic value is a difference of large, nearly equal numbers.** Deep in the money, the
option value and the intrinsic value agree to many digits, so their difference keeps far fewer. That
is acceptable here because the comparison is against a dividend that is orders of magnitude larger
than the lost precision - but it is not a quantity to build anything more delicate on.

**Which spot goes into the test.** The comparison is made on the **cum-dividend** spot, before the
share drops on the ex-date. Using the ex-dividend spot understates the intrinsic value and misses
assignments.

**Dividend yield versus discrete dividend.** The early-exercise question is inherently about a
*discrete* payment on a *specific* date; a continuous yield cannot express "there is a payment
tomorrow". The valuation used inside the test should therefore not also carry a dividend yield for
the same payment, or the dividend is counted twice.

**Assignment is not a valuation.** Once assigned, what matters is the cashflow: shares exchanged at
the strike, plus the fees that go with the transaction, plus - for a call assigned before an
ex-dividend date - the dividend that is no longer received. The model here answers only whether it
happened.

## References

- Hull, J. C., *Options, Futures, and Other Derivatives*, on the optimality of early exercise for
  American calls and puts.
- Merton, R. C. (1973), *Theory of Rational Option Pricing* - the result that an American call on a
  non-dividend-paying share is never exercised early.
