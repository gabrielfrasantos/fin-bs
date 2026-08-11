# Option valuation contracts

## Summary

The valuation layer answers four questions about a single option at a single moment: *what is it
worth*, *how does that worth move*, *what volatility does an observed price imply*, and *will a
short position in it be assigned*. This document states what the layer requires from its caller,
what it guarantees in return, and what it deliberately refuses to express.

## Motivation

Everything above this layer - the wheel state machine, the ledger, the optimiser, the Monte-Carlo
projection - is a loop around it. Two properties therefore matter more than any feature:

1. **It has no memory and no state.** The same question always gets the same answer. A projection
   that revalues a book a hundred million times must be reproducible from a seed, and a valuation
   that depended on anything not in its arguments would silently break that.
2. **It knows nothing about positions.** It does not know how many contracts are held, whether they
   are long or short, or what they cost. Direction and size are properties of a *position*; value
   and sensitivity are properties of a *contract*. Keeping them apart is what stops sign errors from
   hiding inside a valuation.

## Definitions & notation

The layer is built from four capabilities, layered in one direction:

| Capability | Question answered | Depends on |
|---|---|---|
| Normal distribution | what fraction of a standard normal lies below a point | - |
| Valuation | what is this option worth per share | normal distribution |
| Sensitivities | how does that value move | valuation |
| Inversion | what volatility explains an observed price | valuation, sensitivities |
| Assignment | will a short position be assigned | valuation |

A single aggregate carries the description of the option being valued: the spot, the strike, the
risk-free rate, the dividend yield, the year fraction to expiry and whether it is a call or a put.
**Volatility is deliberately not part of it.** It is the one input that is sometimes given and
sometimes solved for, and separating it means each capability's signature states exactly what it
needs: valuation and sensitivities are asked for a volatility, inversion is not - it returns one.

## Derivation / Rules

### What the caller must supply

| Requirement | Why it is a requirement |
|---|---|
| Spot and strike in the same currency | a valuation is a comparison of the two; mixing currencies has no meaning |
| Spot and strike strictly positive | the model is written in log-moneyness |
| Year fraction to expiry non-negative | a past expiry is not a shorter option, it is a defect |
| Dividend yield non-negative | a negative yield would allow a call delta above 1, which is not a delta |

Each of these is a defect rather than a recoverable error: no valid input file can produce one, and
no caller can sensibly carry on afterwards. They stop the run at the point of detection, in every
build. Volatility, rates and dates enforce their own admissible ranges as a condition of existing at
all, so this layer re-checks none of them. See [decision D4](decisions.md).

### What the layer guarantees

- **A value is per share, as a decimal, in the option's currency.** It is a model quantity, not a
  cashflow. The multiplier and the rounding to whole minor units are applied once, further up, at
  the point where a value becomes money that actually moves. See [decision D7](decisions.md).
- **Sensitivities are raw partial derivatives in the model's own units** - per unit of volatility,
  per year, per unit of rate. Per-point and per-day conventions are presentation. See
  [option Greeks](../theory/option-greeks.md).
- **Delta is a delta**, not a decimal: it carries its own admissible range, so a sign error in the
  put branch cannot survive.
- **Degenerate inputs give the limit, not a special case.** Zero volatility and zero time to expiry
  both give the discounted intrinsic value, including exactly at the money. No caller has to guard
  the day of expiry.
- **Inversion may have no answer, and says so.** An observed price outside the no-arbitrage bounds,
  an expired option, or an exhausted iteration budget all produce an absent result rather than an
  exception or a plausible-looking number. See [decision D9](decisions.md).

### What is deliberately impossible to express

- **A value without a volatility.** There is no default volatility anywhere in the layer.
- **A position.** No quantity, no direction, no cost basis, no profit and loss.
- **A partially assigned contract.** Assignment is a two-state answer.
- **An American valuation.** The model is European; the American question is asked separately and
  answered by an explicit rule, so the approximation is visible rather than buried in a price. See
  [assignment and early exercise](../theory/assignment-and-early-exercise.md).
- **A volatility surface.** One volatility, one valuation. A smile is a property of a chain of
  quotes and belongs to whatever loads them.

## Worked example

Valuing a covered call and deciding whether to keep it, for a share trading at 42.00 against a
40.00-strike call with six months to run, at a 10% rate and 20% volatility.

| Question asked | Answer | What the caller does with it |
|---|---|---|
| What is it worth? | 4.75942 per share | 475.94 of premium per contract, once the multiplier is applied and the figure is rounded to minor units |
| What is its delta? | 0.77913 | the short position carries -77.9 shares of equivalent exposure per contract |
| What is its time value? | 2.75942 | this is what is forfeited if it is assigned early |
| The market quotes it at 5.00 - what volatility is that? | 22.65% | higher than the 20% assumed, so the market is charging more than the estimate |
| Will it be assigned at expiry at a spot of 39.99? | no | the position survives to be rolled |

Notice where the multiplier and the rounding appear: once, in the first row, on the way out of the
layer. No figure inside the layer is a cash amount.

## Alternatives considered

**Bundling volatility into the option description.** Simpler to pass around, but it forces the
inversion to take a volatility it will ignore, or to reinterpret it as a starting guess. Either is a
signature that lies about what the operation needs.

**Returning values as cash amounts in minor units.** Consistent with the project's rule that money
is never a decimal, but it destroys the inversion: rounding a per-share value to the nearest minor
unit puts an error into the price that is far larger than the accuracy the volatility solve is asked
for. Recorded and explained as [decision D7](decisions.md).

**A stateful object holding the market and being asked about many strikes.** Faster, by reusing the
discount factors across a chain. Rejected while there is no measurement saying it matters; the
valuation is a handful of transcendental functions and the projection's cost is dominated by path
generation.

## References

- [Black-Scholes-Merton option valuation](../theory/black-scholes.md)
- [Option Greeks](../theory/option-greeks.md)
- [Implied volatility](../theory/implied-volatility.md)
- [Assignment and early exercise](../theory/assignment-and-early-exercise.md)
