# Option contract conventions

## Summary

The *identity* of a tradeable option is its underlying, expiry, strike, type and multiplier - and
nothing else. This document records the conventions that identity encodes: in particular the
100-share multiplier that turns every quoted premium into a cashflow a hundred times larger, and the
delta conventions used for strike selection.

## Motivation

Every capital calculation in the wheel strategy is a multiplier calculation:

- a cash-secured put on a EUR 50 strike locks up **EUR 5,000**, not EUR 50;
- a premium quoted at EUR 1.20 pays **EUR 120** per contract, before costs;
- assignment delivers **100 shares**, not one.

Getting the factor of one hundred wrong is a hundredfold error in the compounding rate, and it is
the kind of error that produces plausible-looking output. Applying the multiplier in exactly one
place - where notional is derived - is what keeps it from being retyped, and mistyped, at each use.

Separating *identity* from *state* matters for the same reason. A contract answers "which position
am I short?" and must therefore be stable and comparable; prices, Greeks and mark-to-market values
change continuously and belong to a quote at a moment in time.

## Definitions & notation

| Symbol | Meaning |
|---|---|
| $K$ | strike, a cash amount |
| $S$ | spot price of the underlying |
| $T$ | expiry date |
| $m$ | contract multiplier, shares per contract (100 for standard US and European equity options) |
| $\Delta$ | option delta, $\partial V/\partial S$ |

## Derivation / Rules

### Notional and collateral

$$
\text{Notional} = K \times m
$$

For the wheel this is the number that actually constrains the strategy:

| Leg | Capital effect |
|---|---|
| Short cash-secured put | reserve $K \times m$ in cash until expiry or assignment |
| Assignment of that put | pay $K \times m$, receive $m$ shares |
| Short covered call | reserve $m$ shares, already held |
| Assignment of that call | deliver $m$ shares, receive $K \times m$ |

Reserved collateral is *not* available for further contracts. This is the mechanism that makes the
compounding non-trivial: the strategy's growth rate is bounded by how much collateral each cycle
ties up, not by the premium yield alone.

The strike is a cash amount rather than a decimal, for the reason given in
[money and rounding](money-and-rounding.md): $K \times m$ is a real settlement cashflow and must be
exact.

### Type, moneyness and assignment

$$
\text{Call intrinsic} = \max(S - K, 0), \qquad \text{Put intrinsic} = \max(K - S, 0)
$$

At expiry an option is exercised if and only if it is in the money - rational exercise, ignoring
pennies and exercise costs:

| | $S > K$ | $S = K$ | $S < K$ |
|---|---|---|---|
| Call | in the money, assigned | at the money, not assigned | out of the money, expires |
| Put | out of the money, expires | at the money, not assigned | in the money, assigned |

The at-the-money boundary is a convention: with a continuous price distribution
$P(S_T = K) = 0$, so the choice only shows up in scripted deterministic scenarios - which is exactly
why it must be written down rather than left to whichever inequality happened to be used. This
project treats **exactly at the money as not assigned**. Dividend-driven early assignment is a
separate model.

### Delta as a strike-selection coordinate

Under Black-Scholes,

$$
\Delta_{\text{call}} = e^{-q\tau}\,\Phi(d_1) \in (0, 1), \qquad
\Delta_{\text{put}} = -e^{-q\tau}\,\Phi(-d_1) \in (-1, 0)
$$

so delta has a fixed sign per option type and a magnitude bounded by one. Premium sellers quote
strikes *in delta* rather than in price - "sell the 30-delta call" - because delta is comparable
across underlyings, expiries and volatility regimes in a way that a strike distance is not.

Two properties justify that habit, and both are approximations worth stating explicitly:

1. $|\Delta|$ is close to the risk-neutral probability of finishing in the money. Exactly,
   $P^{\mathbb{Q}}(S_T > K) = \Phi(d_2)$, while $\Delta_{\text{call}} = e^{-q\tau}\Phi(d_1)$ with
   $d_1 = d_2 + \sigma\sqrt{\tau}$. Since $d_1 > d_2$, **delta overstates the probability of
   finishing in the money**, and the gap widens with $\sigma\sqrt{\tau}$. A 30-delta call is a *less
   than* 30% chance of assignment.
2. The risk-neutral probability is not the real-world one. It embeds the risk premium, so even
   $\Phi(d_2)$ is not the frequency of assignment one should expect to observe.

Delta is therefore a bounded quantity on $[-1, 1]$ with its magnitude available directly, and the
conversion to an assignment probability is deliberately kept out of it: that approximation needs
$d_1$ and $d_2$, which only exist inside the pricing model.

### Ordering

Contracts sort lexicographically by underlying, then expiry, then strike, then type, then
multiplier. Expiry before strike is intentional: chains are grouped by expiry in every user-facing
report, and roll rules key off "same underlying, next expiry".

## Worked example

Short one cash-secured put on AAPL, expiring 21 March 2025, strike EUR 180.00, multiplier 100,
premium quoted at EUR 3.45.

| Quantity | Computation | Value |
|---|---|---|
| Notional, that is collateral reserved | $180.00 \times 100$ | EUR 18,000.00 |
| Premium received | $3.45 \times 100$ | EUR 345.00 |
| Static yield on collateral | $345 / 18{,}000$ | 1.917% |
| Break-even if assigned | $K$ less premium per share | EUR 176.55 |
| Effective cost if assigned | $(180.00 - 3.45) \times 100$ | EUR 17,655.00 |

Transaction costs are excluded here; they are modelled separately, and they are large enough
relative to EUR 345 to matter.

## Edge cases & pitfalls

- **Non-standard multipliers exist.** Corporate actions - splits, special dividends - create
  adjusted contracts with multipliers other than 100 and odd-looking strikes. The multiplier is
  therefore part of a contract's identity with 100 as its default, rather than a fixed constant, so
  those contracts remain representable even though the first version never generates them.
- **The multiplier is part of identity.** Two contracts differing only in multiplier are different
  instruments and compare unequal - that is the adjusted-contract case above.
- **Currency lives in the strike.** A contract has no separate currency; it inherits the one on its
  strike. A dollar-strike contract therefore cannot be compared against a euro-strike one without
  going through the FX model.
- **Expiry is a date, not a timestamp.** See [day-count conventions](day-count-conventions.md).
- **Identity carries no market data.** Bid, ask, implied volatility and Greeks are attached to a
  quote that refers to a contract; they are never stored on the contract itself.

## Alternatives considered

| Option | Verdict |
|---|---|
| An exchange symbol string as the identity | Rejected: a packed 21-character symbol has to be parsed to be used, and it can express neither an exact cash strike nor a non-standard multiplier |
| The multiplier as a single global constant | Rejected: adjusted contracts become unrepresentable, and a constant tends to get retyped at each use |
| Separate concepts for calls and puts | Rejected: they share every field and would double every collection; the difference is one flag |
| A decimal strike | Rejected: strike times multiplier is a settlement cashflow |

## Verification

Checked: the notional figures above; equality and ordering including the multiplier; the currency of
the notional following the currency of the strike; the bounds on delta; and the refusal of a
contract that cannot exist - empty underlying, non-positive strike, zero multiplier.

## References

- J. Hull, *Options, Futures, and Other Derivatives*, chapters 10 and 19 - contract specifications
  and the Greeks.
- The Options Clearing Corporation, *Options Symbology Initiative* - contract adjustment rules.
