# Money and rounding

## Summary

All cash amounts in this project are counted in **whole minor units** - cents - with the currency
attached to the amount. Every operation that could produce a fraction of a cent rounds
**half-to-even** ("banker's rounding"). This document explains why binary floating point is
disqualified for money and why half-to-even is the right tie-break for a compounding projection.

## Motivation

The whole point of the project is to compound a small edge over many periods. A wheel projection
generates on the order of $10^2$ cash events per year per path, times $10^4$ to $10^6$ simulated
paths. Any systematic error in the cash arithmetic compounds exactly like the returns do, so a bias
of a fraction of a cent per event is not negligible - it is a *fake alpha* (or a fake drag) that
grows geometrically.

Two independent error sources have to be eliminated:

1. **Representation error** - decimal amounts are not representable in binary floating point.
2. **Tie-break bias** - the way half-way values are resolved.

### Why not binary floating point?

$0.10$ has no finite binary expansion, so $0.1 + 0.2 \ne 0.3$ in IEEE-754 double precision. Double
precision carries 53 significant bits, so it represents every whole number up to
$2^{53} \approx 9.0 \times 10^{15}$ exactly - about 90 trillion euro expressed in cents, far beyond
anything modelled here. So double precision *could* hold the cent counts exactly. What it cannot do
is prevent an intermediate result that is not a whole number of cents from being kept and carried
forward. Counting in whole units makes the rounding step **explicit and auditable** instead of
implicit and invisible.

There is a second, sharper reason. Optimisers are routinely allowed to assume relaxed
floating-point semantics: to reassociate expressions, to assume no infinities or
not-a-number values, and to ignore the ambient rounding mode. Any money algorithm that relied on
strict IEEE semantics, or on a primitive that honours the current rounding mode, would then be
silently transformed into a different algorithm. Whole-number arithmetic is immune, and the rounding
rule below is defined by comparing exact remainders rather than by delegating to a
rounding-mode-dependent operation.

### Why not a general fixed-point number?

A fixed-point number with a scale of one hundred is exactly what a cash amount *is*. Naming it money
and attaching a currency buys type safety that a general-purpose fixed-point number cannot: adding
euros to dollars is a modelling error, whereas adding two fixed-point numbers is not.

## Definitions & notation

| Symbol | Meaning | Unit |
|---|---|---|
| $c$ | stored value | minor units (cents), $-2^{63} \le c < 2^{63}$ |
| $a$ | amount in major units | euro or dollar, $a = c/100$ |
| $\mathrm{rte}(x)$ | round half to even | $\mathbb{R} \to \mathbb{Z}$ |

Currencies form a closed set. An amount always carries exactly one; there is no currency-less amount
and no default currency.

## Derivation / Rules

### Round half to even

For $x \in \mathbb{R}$, let $n = \lfloor x \rfloor$ and $f = x - n \in [0,1)$:

$$
\mathrm{rte}(x) =
\begin{cases}
n & f < \tfrac{1}{2} \\
n+1 & f > \tfrac{1}{2} \\
n & f = \tfrac{1}{2},\ n \text{ even} \\
n+1 & f = \tfrac{1}{2},\ n \text{ odd}
\end{cases}
$$

Defining this through the **floor** rather than through truncation makes it behave symmetrically for
negative numbers with no special case: $\mathrm{rte}(-2.5) = -2$ and $\mathrm{rte}(-3.5) = -4$.

### Why half-to-even and not half-away-from-zero

Assume the fractional part of an amount has a symmetric continuous distribution plus a genuine atom
at exactly one half - which it does, constantly, because premiums, strikes and fee percentages are
quoted in halves and quarters.

- **Half-away-from-zero** maps $+\tfrac{1}{2} \mapsto +1$ and $-\tfrac{1}{2} \mapsto -1$: the
  expected error per tie is $\tfrac{1}{2}$ in magnitude, *always away from zero*. Over $n$ ties the
  accumulated bias is $\tfrac{n}{2}$ minor units, in the direction that inflates gross flows.
- **Half-to-even** resolves ties by the parity of the neighbour, which is independent of the sign
  and, empirically, equidistributed. The expected error per tie is $0$, so the accumulated error is
  a random walk of $O(\sqrt{n})$ rather than a drift of $O(n)$.

For a thirty-year monthly projection with several roundings per event, $O(n)$ versus $O(\sqrt{n})$ is
the difference between a euro of phantom return and a result free of rounding drift. Half-to-even is
also the IEEE-754 default and the convention used by most European settlement systems, so the
figures stay comparable with a broker statement.

### Division

Division by a whole number rounds half-to-even too, computed exactly, without ever leaving whole
numbers:

$$
q = \left\lfloor \frac{n}{d} \right\rfloor_{\text{trunc}}, \quad r = n - qd, \quad
s = \mathrm{sgn}(n)\,\mathrm{sgn}(d)
$$

$$
n \mathbin{/} d =
\begin{cases}
q & 2|r| < |d| \\
q + s & 2|r| > |d| \\
q & 2|r| = |d|,\ q \text{ even} \\
q + s & 2|r| = |d|,\ q \text{ odd}
\end{cases}
$$

The comparison of $2|r|$ against $|d|$ is done without sign, so that the doubling cannot leave the
representable range: $|r| < |d| \le 2^{63}$ implies $2|r| < 2^{64}$.

### Range

Every operation is checked *before* it executes, because an out-of-range whole-number result cannot
be detected reliably after the fact:

- for a sum, the result is rejected if it would exceed the largest or smallest representable value;
- for a product, the result is rejected if the magnitude of one factor exceeds the largest
  representable value divided by the magnitude of the other.

A violation is a defect, not a runtime condition, so it stops the run rather than returning an error
code. The limit is around $9.2 \times 10^{16}$ minor units; reaching it means the model has
diverged, and continuing with a wrapped value would hide that.

## Worked example

A 1.5% fee on EUR 12.345, then splitting EUR 0.05 several ways:

| Step | Exact value | Stored | Note |
|---|---|---|---|
| EUR 12.345 as cents | 1234.5 | 1234 | tie, $1234$ is even |
| $\times\ 1.5\%$ | 18.51 | 19 | $f = 0.51 > \tfrac{1}{2}$ |
| 5 cents split three ways | $1.\overline{6}$ | 2 | $2|r| = 4 > |d| = 3$ |
| 5 cents split two ways | 2.5 | 2 | tie, $q = 2$ even |
| 7 cents split two ways | 3.5 | 4 | tie, $q = 3$ odd |
| -5 cents split two ways | -2.5 | -2 | tie, $q = -2$ even |

The last row matters: half-to-even is sign-symmetric, so a cashflow and its reversal cancel exactly.
That property is what keeps the ledger reconcilable.

## Edge cases & pitfalls

- **Mixed currencies.** Addition, subtraction and comparison require identical currencies; a
  mismatch is a defect, not a silent conversion. Cross-currency arithmetic goes through the FX
  model, which is the only place allowed to change a currency.
- **No amount without a currency.** A zero balance is zero euro or zero dollar. A default would pick
  a currency on the reader's behalf, which is exactly the class of error the currency exists to
  prevent.
- **Decimals are a one-way boundary.** A cash amount can be turned into a decimal so that pricing
  and statistics can consume it; the result must be converted back through an explicit rounding
  step, never accumulated as a decimal.
- **Rounding is not associative.** $\mathrm{rte}(\mathrm{rte}(x)\,y) \ne \mathrm{rte}(xy)$ in
  general. Round once, at the point where the amount becomes a real cashflow; keep intermediate
  ratios in full precision.

## Alternatives considered

| Option | Verdict |
|---|---|
| Amounts as double-precision decimals | Rejected: implicit fractional cents, and relaxed floating-point optimisation makes strict-IEEE reasoning unreliable |
| Extended-precision decimals | Rejected: platform-dependent width, same class of problem |
| Decimal floating point | Rejected: not portably available |
| An off-the-shelf money library | Rejected: the project takes on no third-party dependencies beyond the two it already has |
| Whole minor units plus a currency | **Chosen** |
| Currency fixed as part of the kind of amount | Rejected: see [decision D2](../design/decisions.md) |

## Verification

The claim that carries the most weight is exactness under accumulation: summing ten thousand
pseudo-random amounts reproduces the exact whole-number reference, and summing them in reverse order
gives the identical result. Whole-number addition is associative, so this is the guard against
anybody reintroducing a decimal intermediate. Each row of the worked example above is checked, as
are the range limits and the currency-mismatch refusal.

## References

- IEEE 754-2019, clause 4.3.1 - `roundTiesToEven` is the default rounding attribute.
- D. Goldberg, *What Every Computer Scientist Should Know About Floating-Point Arithmetic*,
  ACM Computing Surveys 23(1), 1991.
- M. Fowler, *Patterns of Enterprise Application Architecture*, the Money pattern.
