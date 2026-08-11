# Decision log

Numbered, dated modelling decisions. Append; do not rewrite history. If a decision is reversed, add
a new entry that supersedes the old one and mark the old one accordingly.

Format: **context** (the forces), **decision** (what we do), **consequences** (what this costs us).

---

## D1 - Cash is counted in whole minor units

*Date: 2026-08-11. Status: accepted.*

**Context.** The project compounds cashflows over decades and over up to $10^6$ simulated paths.
Decimal amounts are not exactly representable in binary floating point, and any systematic rounding
bias compounds exactly like a return does.

**Decision.** Every cash amount is a signed whole number of minor units - cents - carried together
with its currency. There is no implicit route from a decimal to a cash amount: converting one has to
be asked for, and it rounds half-to-even.

**Consequences.** Rounding becomes an explicit, reviewable step instead of an invisible one, and
every percentage-based fee has a visible point at which it becomes a whole number of cents. The
range of representable amounts is finite and is checked rather than silently wrapped. Rationale:
[money and rounding](../theory/money-and-rounding.md).

---

## D2 - Currency travels with the amount and is checked when amounts meet

*Date: 2026-08-11. Status: accepted.*

**Context.** The alternative is to make the currency part of the *kind* of quantity, so that adding
euros to dollars is not merely wrong but inexpressible.

**Decision.** A single kind of cash amount, carrying its currency as a value. Amounts of different
currencies may not be added, subtracted or compared; attempting it stops the run.

**Consequences.** The failure mode that matters - a silent euro-plus-dollar addition - is still
caught, just when it happens rather than before the run starts, and the checks are exercised on
every run. In exchange, a ledger can hold cashflows in several currencies in one sequence, which the
stricter alternative would have forced into a tagged union. Since the currency of a cashflow is read
from a configuration file rather than known in advance, and the FX model has to convert at run time
anyway, the stricter alternative would have bought less than it cost.

**Reconsider if** the ledger turns out to be single-currency in practice once the strategy engine
exists.

---

## D3 - Model arithmetic is double precision; single precision only inside external fits

*Date: 2026-08-11. Status: accepted.*

**Context.** The external numerical library this project builds on targets microcontrollers and
works in single precision. Option pricing and implied-volatility solving in single precision lose
roughly half the significant digits, and the implied-volatility search is asked to converge to
$10^{-5}$.

**Decision.** Rates, volatilities, deltas and price paths are double precision throughout the model.
Single precision appears only inside a call into the external library, never in a quantity the model
hands around.

**Consequences.** No precision parameter has to be threaded through the whole model. Estimators that
delegate to the external library accept a documented precision loss inside the fit while reporting
their results at full precision. This also enforces the rule that the external library's own
concepts never surface in this project's vocabulary.

**Reconsider if** the single-precision round-trip inside a fit turns out to dominate the estimation
error.

---

## D4 - Impossible states stop the run; recoverable conditions are reported

*Date: 2026-08-11. Status: accepted.*

**Context.** A currency mismatch, an out-of-range amount, a reversed date pair and a negative
volatility are all defects: no valid input file can produce them, and no caller can sensibly carry
on. A malformed configuration value or an option quoted below intrinsic value are different - they
come from outside and the user can fix them.

**Decision.** Defects stop the run immediately and loudly, in every build, at the point they are
detected. Conditions that originate outside the model are reported as recoverable errors or as an
absent result, and are validated at the boundary where external data enters.

**Consequences.** A projection never continues on a wrapped or nonsensical value, so a corrupted
number cannot quietly reach a report. The dividing line - *validate at the boundary, refuse to
continue inside* - is what keeps the internal checks from degenerating into input validation.
Every one of those refusals is specified by a test, so "this stops the run" is a promise rather than
a comment.

---

## D5 - Dates are whole days on the proleptic Gregorian calendar

*Date: 2026-08-11. Status: accepted.*

**Context.** The model needs civil dates, ordering, differences and weekdays. It never needs a time
of day: the strategy trades at most daily, and an option expires on a date.

**Decision.** A date is a signed whole count of days from 1 January 1970. The year, month and day
are derived when needed rather than stored. No time zones, no daylight saving, no intraday
resolution.

**Consequences.** Ordering and differencing are single subtractions, which matters because the
simulation compares dates in its innermost loop. Deriving the calendar triple costs a little
arithmetic, paid only in reports and loaders. Options that settle at the market close are modelled
as expiring at the end of their date. Rationale:
[day-count conventions](../theory/day-count-conventions.md).

---

## D6 - Each financial quantity is its own concept with its own invariant

*Date: 2026-08-11. Status: accepted.*

**Context.** In an option pricing formula the interest rate, the dividend yield, the volatility and
the year fraction are all small decimals. Swapping two of them produces a plausible price, not a
visible failure.

**Decision.** Each quantity is named, carries its unit and convention, states its admissible range
when it is created, and offers only the conversions that make sense for it. A rate is normalised to
one canonical compounding convention rather than carrying a label saying which it is.

**Consequences.** Swapping two arguments stops being possible. Stating a quantity is more verbose
than writing a bare decimal - "a volatility of 20% per annum" rather than "0.2" - which is the
point: the unit is recorded where the number enters the model, once, instead of being remembered at
every use.
