# Quantities and their conventions

## Summary

The model is built from a small vocabulary of quantities: a **cash amount**, a **date**, a **year
fraction**, an **interest rate**, a **volatility**, a **delta**, and the **identity of an option
contract**. This document fixes what each one means, what values it may take, and which form is
treated as canonical. Everything else in the project is expressed in these terms.

## Motivation

The failure mode this vocabulary exists to prevent is not a crash. It is a *plausible wrong number*:

- a strike carried as a decimal fraction, so a settlement amount ends up a hundredth of a cent off;
- a per-day volatility used where an annualised one is meant, understating risk by a factor of 16;
- a nominal rate used where an effective one is meant;
- a premium quoted per share entered in the ledger without the 100x contract multiplier;
- euros added to dollars.

Each of those produces output that still looks like a projection. Treating each quantity as a
distinct concept - with its own units, its own admissible range and its own conversions - turns
most of those mistakes into something that cannot be expressed at all, and the rest into an
immediate, loud failure at the point where the mistake was made rather than three steps downstream.

## Definitions & notation

| Quantity | Represents | Admissible values | Canonical form |
|---|---|---|---|
| Currency | the unit a cash amount is denominated in | a closed set: EUR, USD | - |
| Cash amount | an amount of money | limited by the counter's range | a signed whole number of minor units, plus its currency |
| Date | a civil date | any valid proleptic Gregorian date | a signed whole count of days from 1 January 1970 |
| Day-count convention | how a date pair becomes a year fraction | a closed set: ACT/365F, ACT/252 | - |
| Interest rate | an annualised interest rate | effective annual rate $> -100\%$ | continuously compounded, per annum |
| Volatility | annualised standard deviation of log-returns | $\sigma \ge 0$ | annualised, per $\sqrt{\text{year}}$ |
| Delta | sensitivity of an option's value to the underlying | $-1 \le \Delta \le 1$ | signed |
| Option type | call or put | a closed set | - |
| Option contract | the identity of a tradeable option | non-empty underlying, strike $> 0$, multiplier $> 0$ | - |

Units are part of the definition, not a footnote. Every rate and every volatility in this project
is **per annum**; there is no such thing as a monthly rate here, only a rate applied over a year
fraction.

## Derivation / Rules

### Properties shared by every quantity

1. **The unit is stated where the number enters the model.** A bare decimal is never accepted as a
   volatility, a rate or a cash amount; the caller must say which convention the number is in.
   This is the single highest-value rule in the whole vocabulary.
2. **Values are immutable.** Operations produce new values. A quantity that has been recorded in a
   ledger or a simulated path can never be changed behind the reader's back.
3. **Invariants hold from the moment a value exists.** A volatility is non-negative, a delta lies in
   $[-1,1]$, a strike is positive. Nothing downstream re-checks, because nothing downstream can
   receive a value that failed.
4. **Ordering is defined only where it is meaningful.** Cash amounts and dates are totally ordered;
   comparing amounts in different currencies is a defect, not a question with an answer.
5. **Impossible states stop the run.** See [decision D4](decisions.md).

### Cash amounts

Counted in whole minor units - cents - with the currency attached to the amount rather than
inferred from context. There is no "amount without a currency": a zero balance is zero *euro* or
zero *dollar*.

| Operation | Rule |
|---|---|
| Addition, subtraction, negation | both operands must be the same currency; the result is checked against the counter's range before it is produced |
| Multiplication by a whole number | exact; range-checked |
| Multiplication by a fraction | rounds half-to-even |
| Division by a whole number | rounds half-to-even, computed without leaving whole numbers |
| Comparison | both operands must be the same currency |

Multiplying by a whole number and multiplying by a fraction are deliberately different operations
rather than one operation that accepts both. Sharing them would make "two contracts" and "1.5% fee"
look alike, and would let a divisor of $3$ and a divisor of $3.4$ be confused. Rationale for the
representation and the rounding rule:
[money and rounding](../theory/money-and-rounding.md).

Converting a cash amount to a decimal is possible but is treated as a one-way boundary: decimals go
into pricing formulas and statistics, and any figure that comes back and becomes a real cashflow is
rounded once, explicitly, on the way in.

### Dates and year fractions

A date is a whole day. There is no time of day, no time zone and no daylight saving, because the
strategy trades at most once a day and an option expires on a date
([decision D5](decisions.md)).

A **year fraction** is deliberately not a property of a date. It is a property of a *pair* of dates
under a *named convention*, and the convention has to be chosen every time, because there is no
correct default: the right choice depends on how the volatility being used was estimated.
Rationale: [day-count conventions](../theory/day-count-conventions.md).

### Interest rate, volatility, delta

These are three separate concepts rather than three labels on one "small decimal number". They are
not interchangeable even dimensionally: an interest rate has units of inverse time, a volatility has
units of inverse square-root-of-time, and a delta is dimensionless. Adding a rate to a volatility is
meaningless even though both are typically around $0.2$.

An interest rate is normalised to the continuously compounded convention as soon as it is stated, so
that nothing downstream has to ask which kind of rate it is holding. Rationale:
[rates and compounding](../theory/rates-and-compounding.md) and
[volatility scaling](../theory/volatility-scaling.md).

### Option contract identity

A contract is *identity only*: underlying, expiry, strike, type and multiplier. Bid, ask, implied
volatility and Greeks are properties of a **quote** at a moment in time, and they are attached to a
contract rather than stored in it. This is what lets a contract be used as the key that answers
"which position am I short?" while prices change around it.

Contracts sort by underlying, then expiry, then strike, then type, then multiplier - the order
reports are grouped in and the order roll rules walk. The 100x multiplier is applied in exactly one
place, when notional is derived. Rationale:
[option contract conventions](../theory/option-contract-conventions.md).

## Worked example

Sizing a cash-secured put from EUR 50,000 of capital, on a EUR 180.00 strike expiring
21 March 2025, with the standard 100-share multiplier:

| Step | Computation | Value |
|---|---|---|
| Collateral per contract | $180.00 \times 100$ | EUR 18,000.00 |
| Contracts affordable | $\lfloor 50{,}000 / 18{,}000 \rfloor$ | 2 |
| Collateral reserved | $18{,}000 \times 2$ | EUR 36,000.00 |
| Capital left free | $50{,}000 - 36{,}000$ | EUR 14,000.00 |
| Time to expiry, ACT/365F, from 17 January 2025 | $63/365$ | 0.172603 years |
| Volatility over that life, $\sigma = 24\%$ | $0.24\sqrt{0.172603}$ | 0.099709 |
| Discount factor, effective annual 4% | $e^{-\ln(1.04)\times 0.172603}$ | 0.993253 |

Every quantity that becomes a cashflow is counted in whole minor units; every quantity that feeds a
formula is a decimal obtained by an explicit, named conversion. The boundary between the two is
visible at every step.

## Edge cases & pitfalls

- **There is no default currency.** A zero amount still has to say which currency it is zero in.
  That is intentional friction, and it is why comparing amounts across currencies is a defect.
- **Contract comparison inherits the currency rule** through the strike, so a chain containing two
  currencies cannot be sorted without converting first.
- **Delta is not a probability.** Its magnitude *overstates* the risk-neutral probability of
  finishing in the money; the correction needs quantities that only exist inside the pricing model.
- **A year fraction is undefined for a reversed date pair.** A negative time to expiry has no
  meaning in an option formula, so it is rejected rather than propagated.
- **ACT/252 ignores public holidays.** A documented limitation, quantified in the theory note.

## Alternatives considered

| Option | Verdict |
|---|---|
| One generic "tagged decimal" for every scalar | Rejected: it gives unit safety but no room for the conversions that are the whole reason to name the quantity |
| Currency inferred from context (a portfolio-wide base currency) | Rejected: cross-currency legs are exactly the case that needs to be explicit |
| Amounts as decimals, rounded only when reported | Rejected: see [money and rounding](../theory/money-and-rounding.md) |
| A single default day-count convention | Rejected: the correct choice depends on how the volatility was estimated, so it must be stated each time |
| Prices and Greeks carried on the contract | Rejected: identity must be stable while quotes change |

## References

- See the linked theory documents for the financial and mathematical background.
- [PLAN.md](../../PLAN.md) sections 3 and 6.
