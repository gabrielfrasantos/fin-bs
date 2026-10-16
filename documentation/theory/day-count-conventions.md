# Day-count conventions

## Summary

Option pricing needs time expressed as a **year fraction** $\tau$, not as a number of days. The map
from a pair of calendar dates to $\tau$ is a *convention*, not a fact, and choosing a different one
moves option prices. This project implements **ACT/365F** - the default, used for discounting and
for time to expiry - and **ACT/252**, which counts business days and is the right choice when the
volatility in hand was estimated from trading-day returns.

## Motivation

The Black-Scholes formula takes $\tau = T - t$ in years. Two things go wrong if the year fraction is
treated casually:

1. **Leap years.** 2024 has 366 days. Dividing by 365 gives $\tau = 1.00274$; dividing by 366 gives
   $\tau = 1$. An at-the-money call is worth roughly $S\sigma\sqrt{\tau}/\sqrt{2\pi}$, so a $0.27\%$
   change in $\tau$ moves it by $0.14\%$ - about one basis point of spot at $\sigma = 20\%$. Small,
   but *systematic*: it applies to every option in a thirty-year projection.
2. **Mismatched estimation and pricing.** Historical volatility computed from daily closes is
   almost always annualised with $\sqrt{252}$, because there are about 252 trading days in a year.
   Feeding that volatility into a model whose $\tau$ counts *calendar* days mixes two clocks. Over a
   weekend, calendar time advances by $3/365$ while trading time advances by $1/252$. This is why
   weekend time decay is systematically over-estimated by naive tools.

The convention is therefore always stated explicitly. There is no correct default, so there is no
default.

## Definitions & notation

| Symbol | Meaning |
|---|---|
| $d_1, d_2$ | start and end civil dates, $d_1 \le d_2$ |
| $A(d_1,d_2)$ | actual calendar days between them, $d_2 - d_1$ |
| $B(d_1,d_2)$ | business days in $[d_1, d_2)$: Monday to Friday, weekends excluded |
| $\tau$ | year fraction |

All dates are **proleptic Gregorian** civil dates - no time zones, no local time, no daylight
saving. A date is a whole day; anything finer would be false precision for a strategy that trades at
most daily.

## Derivation / Rules

### ACT/365 Fixed

$$
\tau_{\mathrm{ACT/365F}}(d_1, d_2) = \frac{A(d_1,d_2)}{365}
$$

The denominator is *fixed* at 365 - that is what "F" means. It does **not** become 366 in a leap
year, and that is what makes it well defined for a period spanning several years: there is no
ambiguity about which year's length applies. It is the market convention for sterling, yen and
Australian dollar money markets, and the usual default for equity option pricing.

### ACT/252

$$
\tau_{\mathrm{ACT/252}}(d_1, d_2) = \frac{B(d_1,d_2)}{252}
$$

252 is the conventional number of exchange trading days in a year: $365.25$ days, minus about
$104.4$ weekend days, gives roughly 261, minus roughly nine public holidays. This project counts
**weekends only**; a holiday calendar is deliberately out of scope for the first version, which
biases $B$ upward by up to about nine days per year - a $3.6\%$ relative error on a one-year $\tau$,
shrinking in absolute terms for short-dated options. Use ACT/252 only when the volatility in hand
was itself estimated in trading time, and record the discrepancy when comparing against a broker's
quoted price.

### Counting business days without walking the whole range

$[d_1, d_2)$ is half-open. Split it into whole weeks plus a remainder of at most six days:

$$
A = 7w + \rho, \quad 0 \le \rho < 7 \implies B = 5w + \#\{\text{weekdays among the last } \rho \text{ days}\}
$$

Every whole week contributes exactly five business days regardless of alignment, so only the
remainder has to be examined - at most six checks, independent of the horizon.

### Civil dates without a time-zone database

A date is held as a single **whole count of days from 1 January 1970**, and the year, month and day
are derived when needed. The key idea is that the Gregorian calendar repeats exactly every 400
years, an *era* of

$$
400 \times 365 + 100 - 4 + 1 = 146097 \text{ days}
$$

(400 base years, plus one day every 4 years, minus one every 100, plus one every 400). Shifting the
year so that it begins on 1 March moves the leap day to the end of the year, which removes every
special case from the month-length arithmetic; the month-length sequence
31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 28 is then generated in closed form by
$\lfloor (153m + 2)/5 \rfloor$. The result is free of branching, exact across the whole supported
range, and needs no locale, no time-zone database and no calendar library.

Holding the day count rather than the calendar triple also makes ordering and differencing single
subtractions, which matters because the simulation compares dates in its innermost loop.

## Worked example

| From | To | $A$ | $B$ | ACT/365F | ACT/252 |
|---|---|---|---|---|---|
| 2024-01-01 | 2025-01-01 | 366 | 262 | 1.002740 | 1.039683 |
| 2023-01-01 | 2024-01-01 | 365 | 260 | 1.000000 | 1.031746 |
| 2025-01-17 | 2025-02-21 | 35 | 25 | 0.095890 | 0.099206 |
| 2025-06-20 | 2025-06-23 | 3 | 1 | 0.008219 | 0.003968 |

The last row is the weekend effect in isolation: Friday to Monday is three times as much time in
calendar terms as in trading terms. An option seller who marks time decay with ACT/365F books three
days of decay across a weekend during which the market cannot move.

The 2024 leap year shows up as $\tau > 1$ under ACT/365F for a period that is, by the calendar,
exactly one year. That is not an error: it is the definition of a *fixed* denominator.

## Edge cases & pitfalls

- **Ordering.** $\tau$ is defined only for $d_1 \le d_2$. A reversed pair is a defect and stops the
  run; a signed year fraction would let a negative time to expiry reach the pricing formula, where
  its square root is not a number.
- **Same day.** $\tau = 0$. Pricing on the expiry date must use intrinsic value rather than dividing
  by $\sqrt{\tau}$.
- **Expiry Fridays.** Standard monthly US equity options expire on the **third Friday** of the
  month, so the weekday of a date is part of the chain and roll-rule logic. 1 January 1970 was a
  Thursday, which fixes the phase of the weekday calculation.
- **No holidays.** ACT/252 over-counts business days around Christmas, Easter and national holidays.
  Documented here; to be revisited if a holiday calendar is ever added.
- **No intraday time.** Expiry is a date, not a timestamp. Options that settle at the market close
  are modelled as expiring at the end of that date.

## Alternatives considered

| Option | Verdict |
|---|---|
| A standard-library calendar type | Rejected: support is uneven across the toolchains this project builds on, and the time-zone data is a runtime dependency it does not need |
| Julian day number as a decimal | Rejected: reintroduces floating point where exact whole numbers work |
| 30/360 conventions | Not implemented: they exist for bond coupon accrual, not for equity options |
| ACT/ACT (ISDA) | Not implemented: it splits the period per calendar year and differs materially only for multi-year accruals, which this model does not have |

## Verification

Every row of the table above is checked, along with the leap-year boundaries (1900 is not a leap
year, 2000 is), a round trip from day count to calendar triple and back across two centuries, the
weekday phase against known dates, and the refusal of a reversed date pair.

## References

- H. Hinnant, *chrono-Compatible Low-Level Date Algorithms*,
  <https://howardhinnant.github.io/date_algorithms.html> (public domain).
- ISDA, *2006 ISDA Definitions*, section 4.16 - day-count fraction definitions.
- ACT/252 and the 252-trading-day year: standard practice in Brazilian fixed income (B3) and in
  equity volatility annualisation.
