# Currency conversion

## Summary

The project reports in one base currency, but a wheel strategy on a foreign-listed underlying
receives premiums and pays assignments in another. This note defines how an amount is converted, what
the automatic conversion costs, and why a round trip out of the base currency and back loses slightly
less than twice that cost.

## Motivation

A conversion cost is small and relentless. A quarter of a percent on each leg sounds like nothing
next to a market that moves by a percent a day, but a wheel strategy converts on every premium
received, every assignment settled and every dividend paid - dozens of times a year, for decades. Two
mistakes are easy to make and hard to see afterwards.

The first is folding the cost into the exchange rate. Using an effective rate rather than the spot
rate gives exactly the same net amount and makes the cost disappear from the accounts: it can no
longer be shown as a cost, and a round trip appears to lose money to the market rather than to the
broker. The cost is then invisible in exactly the report that is supposed to expose it.

The second is charging the cost only on money coming in. A conversion is charged in both directions;
money received arrives short, and money paid costs more than the amount paid. A model that charges
only one direction halves a cost that is already easy to overlook.

## Definitions & notation

| Symbol | Meaning | Unit |
|---|---|---|
| $B$ | base currency - the currency the project reports in | - |
| $F$ | foreign currency | - |
| $r$ | spot rate: units of $F$ per unit of $B$ | dimensionless |
| $h$ | handling cost of one automatic conversion | fraction of the converted amount |
| $b_x$ | the same cost expressed as whole basis points, $h = b_x/10^4$ | basis points |
| $A$ | an amount, signed: positive is received, negative is paid | minor units |

Signs follow the account: a positive amount increases what is held, a negative amount decreases it.

## Derivation / Rules

### The spot amount

$$
S(A) = \begin{cases} [\,A \cdot r\,] & A \text{ is in } B \\[4pt] [\,A / r\,] & A \text{ is in } F \end{cases}
$$

where $[\cdot]$ rounds to the nearest minor unit of the target currency, halves going to the even
one. The spot amount is what the conversion would produce if it were free, and it is reported as its
own figure.

### The handling cost

$$
H(A) = \left[\frac{|A| \cdot b_x}{10^4}\right] \ \ge 0
$$

The cost depends on the size of the amount, not on its direction, and it is always a cost. It is
charged in the currency of the amount it is computed on.

### The net conversion

$$
C(A) = S(A) - H\big(S(A)\big)
$$

The cost is struck on the converted amount, in the target currency, and subtracted. For a positive
amount this yields less than the spot amount; for a negative amount it yields a more negative one.
Both are the same statement: the account is worse off by $H$ than a free conversion would leave it.

Because $C$ is defined as a subtraction of two exact figures, the identity
$C(A) = S(A) - H(S(A))$ holds in whole minor units with no residue. A ledger can therefore post the
spot amount and the cost as two entries and arrive at the net amount exactly.

### A round trip

Ignoring rounding, converting out of the base currency and back gives

$$
C\big(C(A)\big) = \frac{A r (1-h)}{r}(1-h) = A(1-h)^2
$$

so the fraction lost is

$$
1 - (1-h)^2 = 2h - h^2 .
$$

A round trip costs **twice the handling cost less its square** - very slightly less than twice,
because the second leg is charged on an amount the first leg has already reduced. The spot rate
cancels completely: a round trip loses the same fraction whatever the exchange rate is, which is the
property that makes this checkable without knowing any rate.

## Worked example

Base currency euro, foreign currency dollar, $r = 1.10$ dollars per euro, $b_x = 25$ basis points
so $h = 0.0025$.

| Step | Spot amount | Handling cost | Net |
|---|---|---|---|
| 10 000.00 EUR out | 11 000.00 USD | 27.50 USD | 10 972.50 USD |
| 10 972.50 USD back | 9 975.00 EUR | 24.94 EUR | 9 950.06 EUR |

The round trip started with 10 000.00 and returned 9 950.06, a loss of 49.94. The predicted loss is

$$
10\,000 \times (2h - h^2) = 10\,000 \times (0.005 - 0.00000625) = 49.9375
$$

which agrees to within the rounding of two minor-unit conversions. Twice the handling cost alone
would have predicted 50.00; the six-cent difference is the square term, and it is the reason the
identity is stated with it.

Note also that the second leg's spot amount is 9 975.00 rather than 10 000.00: the first leg's cost
has already been taken out, so the return trip is converting a smaller amount.

## Edge cases & pitfalls

| Case | Result |
|---|---|
| A zero amount | converts to zero and costs nothing |
| A negative amount | converts to a negative amount and costs more than the spot amount |
| A handling cost of zero | a round trip returns what it started with, up to one minor unit of rounding |
| A rate of zero or a negative rate | not a rate; refused |
| A handling cost of the whole amount or more | not a cost; refused |
| A round trip of a very small amount | may lose nothing at all, because the cost rounds to zero |

**Rounding is not symmetric.** Converting out and back is not the identity even with no handling
cost, because each leg rounds to a whole minor unit. The residue is at most one minor unit per leg
and does not accumulate systematically, but a test that demands exact equality of a round trip will
fail for the wrong reason.

**The cost is charged in the target currency.** Comparing the two legs' costs directly means
comparing amounts in different currencies; they must be converted before they are added, which itself
rounds. The statement that survives is the one about the *fraction* lost, not the one about the sum
of the two costs.

**One rate, held fixed.** A conversion model here is a single rate, not a path. A projection that
needs the exchange rate to move builds that movement itself and converts at each step; nothing in
this note assumes the rate is constant over the projection, only that a single conversion happens at
a single rate.

## Alternatives considered

**An effective rate that already includes the cost.** Arithmetically identical for the net amount,
and it destroys the ability to report the cost. Rejected; see
[decision D19](../design/decisions.md).

**Charging the cost on the source amount rather than the converted amount.** Also arithmetically
close, and it makes the cost appear in the wrong currency for the ledger entry it belongs to.
Rejected.

## References

- [Cost contracts](../design/costs.md) - what the cost layer requires and guarantees.
- [Money and rounding](money-and-rounding.md) - minor units and half-to-even rounding.
- [External parameter sources](../validation/sources.md) - the published conversion cost, and when it
  was last confirmed.
