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

---

## D7 - A model value is a decimal per share; only a cashflow is counted in minor units

*Date: 2026-08-11. Status: accepted.*

**Context.** [D1](#d1---cash-is-counted-in-whole-minor-units) says cash is counted in whole minor
units. A theoretical option value looks like cash, so the obvious reading is that it should be too.
But it is not cash: it is the output of a continuous formula that is immediately fed back into
another continuous calculation - a sensitivity, a finite difference, or the search that inverts it.
Rounding a per-share value to the nearest minor unit puts an error of half a cent into it, and half
a cent divided by a typical vega is a volatility error thousands of times larger than the accuracy
the inversion is asked to reach.

**Decision.** Option values, sensitivities and intrinsic and extrinsic values are decimals, per
share, in the option's currency. The contract multiplier and the rounding to whole minor units are
applied together, once, at the single point where a model value becomes a cashflow that is recorded
in a ledger.

**Consequences.** The rule "money is never a decimal" narrows to the sharper and more defensible
"money that moves is never a decimal", and the boundary between the two worlds is one visible step
rather than a scattering of conversions. Spot and strike remain cash amounts - they are contractual
figures that really are exact in minor units - so the conversion into the formula happens on the way
in as well as on the way out. A reviewer can find every place a decimal becomes money by looking for
that one step.

**Reconsider if** a value ever needs to be compared for exact equality with a recorded cashflow.

---

## D8 - Options are valued with a European model and a continuous dividend yield

*Date: 2026-08-11. Status: accepted.*

**Context.** The options this project trades are American-style and are written on shares that pay
discrete dividends. The available closed-form valuation is European with a continuous yield.
Alternatives - lattices, or American closed-form approximations - price the exercise feature
directly, at one to three orders of magnitude more cost per valuation and without an analytic
inverse.

**Decision.** Value every option with the European formula extended by a continuous dividend yield.
Handle the American feature *outside* the valuation, as an explicit early-assignment rule that
compares a discrete dividend against the option's remaining extrinsic value.

**Consequences.** A projection can revalue a book as often as a Monte-Carlo run demands, and an
observed premium can be inverted into a volatility, which a lattice would not allow. The
approximation is visible: it lives in a named rule with its own tests, rather than being absorbed
into a price where nobody can see how large it is. The cost is that the European value is a lower
bound for an American call on a dividend-paying share, and the model does not quantify the gap.

**Reconsider if** the early-assignment rule turns out to fire often enough that the size of the gap
matters to the projection.

---

## D9 - A volatility that the market price does not determine is reported as absent

*Date: 2026-08-11. Status: accepted.*

**Context.** Inverting a price into a volatility has no answer when the price violates the
no-arbitrage bounds, when the option has expired, or when the search does not converge within its
budget. These are not defects: a chain snapshot contains stale and crossed quotes on every illiquid
strike, and the caller is the only one who can decide whether to skip the strike or substitute an
estimate. Separately, even when an answer exists it may not be *determined*: where vega is tiny, a
wide band of volatilities produces the identical price.

**Decision.** The inversion returns either a volatility or nothing. It does not throw, does not
return a sentinel value, and does not extrapolate outside the bounds. The bounds themselves are
answers rather than failures: a price exactly at the discounted intrinsic value implies zero
volatility. The search is safeguarded by bisection so that "no result" means the problem had no
answer, not that the method wandered off.

**Consequences.** Every caller is forced to have a policy for an uninvertible quote, at the point
where it loads the quote, which is the only place that has the context to choose one. This is the
same boundary rule as [D4](#d4---impossible-states-stop-the-run-recoverable-conditions-are-reported):
a defect stops the run, an external condition is reported. What the decision does *not* do is defend
against the ill-conditioned case, where an answer is returned that is technically correct and
practically meaningless; that has to be handled by weighting on quality, further up. Rationale:
[implied volatility](../theory/implied-volatility.md).

---

## D10 - Assignment is deterministic, and early assignment is modelled only for dividends

*Date: 2026-08-11. Status: accepted.*

**Context.** In reality, assignment is allocated randomly among short positions by the clearing
house, holders sometimes exercise when it is not rational, and an option pinned at its strike may go
either way. Modelling that faithfully would make every projection depend on counterparty behaviour
that cannot be estimated from any data this project has.

**Decision.** Assignment is a function of the inputs alone. At expiry a contract is assigned when it
is in the money by any amount, and not otherwise, with the boundary resolved in favour of not
assigning. Before expiry, only the dividend-driven early exercise of a call is modelled: it is the
one case with a clear economic trigger and a systematic, one-directional effect on a wheel. Early
exercise of a put is out of scope for now.

**Consequences.** A projection is reproducible from its seed, and the randomness in the model stays
where it can be justified - in the price path. Two known biases are accepted in exchange: outcomes
exactly at the strike are always resolved the same way, and a short put that would have been
assigned early is instead assigned at expiry, which shifts the timing of a share purchase the
strategy had already committed to without changing whether it happens.

**Reconsider if** the projection becomes sensitive to the timing of put assignment, or if a
distribution over assignment outcomes is needed to report a realistic worst case.

---

## D11 - An estimator is given a completed history, not a live stream

*Date: 2026-08-11. Status: accepted.*

**Context.** Two of the estimators in this project are recursive: they carry a running state that is
updated once per observation. The obvious shape for them is an object that is fed one observation at
a time. That shape has a cost here, because the state in question belongs to the external numerical
library, and an object that holds it either exposes the library's types in this project's vocabulary
or hides them behind an indirection that exists only to hide them.

**Decision.** Every estimator takes a completed history and returns either a single estimate or the
whole sequence of estimates that history would have produced, one per observation. Nothing recursive
is kept between calls.

**Consequences.** The external library's types stay entirely inside the calculation, so no consumer
of an estimate compiles against them, and an estimator is a value with no lifetime to manage and
nothing to reset. The sequence-returning form is what the recursive estimators are actually used for
- inspecting how an estimate settled, and comparing how fast two settings react - so nothing is lost.
The cost is that re-estimating after one new observation redoes the whole pass, which is linear work
that nothing in this project does in a loop.

**Reconsider if** a live data feed is added, at which point re-running the pass per tick stops being
free.

---

## D12 - The published drift is the drift of the price, corrected once

*Date: 2026-08-11. Status: accepted.*

**Context.** A price and its logarithm grow at different rates: the log price grows at half the
variance less than the price does. A regression on the log price therefore measures one of them while
a simulation that must reproduce the expected price needs the other. Both are called "the drift", and
both are small decimals in the same range, so a confusion between them is invisible.

**Decision.** The fitted quantity is named for what it is - the drift of the log price - and the
quantity published to the rest of the project is the drift of the price. The conversion between them
is applied at exactly one place, where the drift and the volatility are gathered into one set, which
is also the only place where both numbers are in hand at once.

**Consequences.** The two drifts are never both in circulation under the same name, and the
conversion cannot be applied twice or forgotten, because there is only one point at which it could
happen. It also forces the drift and the volatility to be published together rather than separately,
which is correct: the conversion makes them mutually dependent, so a drift quoted without the
volatility it was corrected with is not a complete answer.

---

## D13 - Ordinary least squares is the fit; orthogonal regression is an option, not the default

*Date: 2026-08-11. Status: accepted.*

**Context.** Fitting a line to the log price against time can be done by minimising vertical distance,
which assumes the time axis is exact, or perpendicular distance, which assumes both axes carry error.
The second is the standard remedy when the horizontal coordinate is a measurement.

**Decision.** Fit by minimising vertical distance. Offer the perpendicular fit as a substitutable
alternative - a named capability that the drift estimator is given, rather than a flag it inspects -
with both axes scaled to unit length first so that it does not depend on the units the price happens
to be quoted in.

**Consequences.** The default matches the data: an observation date is a recorded fact, not a
measurement, so there is no error in the horizontal coordinate for the perpendicular fit to correct.
On a real price path the perpendicular fit reports a materially larger drift - larger by the
reciprocal of the correlation between the two axes - because it attributes part of the path's own
randomness to the time axis. Keeping it available documents that the choice was made rather than
overlooked, and it is the right fit if a series whose timestamps are genuinely uncertain ever
arrives. Expressing the choice as a capability rather than as a closed set of names means the
centring, the scaling and the conversion of a slope into a rate are written once and shared, a third
fit can be added without touching the estimator that uses it, and neither fit can be selected without
an implementation existing. Rationale: [drift estimation](../theory/drift-estimation.md).

**Reconsider if** a data source appears whose timestamps are estimates rather than records.

---

## D14 - A price history is validated where it is created, and states its own frequency

*Date: 2026-08-11. Status: accepted.*

**Context.** Every estimator needs the same things to be true of its input - at least two
observations, dates that increase, positive prices in one currency - and every estimator needs to
know how many observations there are in a year in order to annualise. Each of those could be checked
by each estimator, and the frequency could be passed in alongside.

**Decision.** A dated price history is a value with an invariant, checked once when it is created. It
derives its own observation frequency from its dates and its day-count convention, and no caller is
ever asked for one.

**Consequences.** The checks happen once instead of five times, and the one mistake that would
otherwise be easy to make and impossible to see - annualising monthly closes as though they were
daily, which overstates volatility by a factor of about five - becomes inexpressible. The cost is
that the frequency of an irregular series is an average rather than a constant, which is the right
answer for a series with holidays in it and a lossy one for a series that changes sampling frequency
part way through.

---

## D15 - The fitted drift accepts a bounded number of observations

*Date: 2026-08-11. Status: accepted.*

**Context.** The external library solves least-squares problems whose size is fixed when the program
is built, not when it runs. A history read from a file has whatever length the file has.

**Decision.** Each fit publishes the longest window it accepts - several decades of daily
observations - and refuses a longer one outright. Shorter windows are padded to that size in a way
that provably leaves the fitted line unchanged, by adding observations at the centre of the data,
which contribute nothing to either the slope or the intercept.

**Consequences.** A history that is too long stops the run at the boundary where it entered rather
than being silently truncated to its first few decades, which would produce a plausible and wrong
answer. Every fit costs the same regardless of how much data it was given, which is wasteful for
short windows and irrelevant at the scale this project runs at. The padding is exact rather than
approximate, so the published limit is the only visible consequence of the underlying constraint.

**Reconsider if** intraday data brings histories longer than the published limit into normal use.
