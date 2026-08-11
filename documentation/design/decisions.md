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

---

## D16 - Cash in configuration is written as a decimal and read as an exact count of minor units

*Date: 2026-08-11. Status: superseded by [D20](#d20---configuration-states-cash-in-minor-units).*

**Context.** Fee schedules and tax parameters are edited by people, so they are written the way a
tariff is published - a quarter of a euro is `0.25`, not `25`. Every cash amount inside the model is
a whole count of minor units. Something has to bridge the two, and the obvious bridge is a binary
floating-point number, which cannot represent most decimal fractions exactly.

**Decision.** A configured amount is read from its decimal text as an integer part and a fractional
part in fixed point, and converted to minor units by integer arithmetic. No binary float appears
anywhere on the path from the file to the amount. An amount written more finely than one minor unit
is refused, naming the key, rather than rounded.

**Consequences.** A published tariff of a quarter of a euro becomes exactly twenty-five minor units,
on every platform, forever - the configured value and the modelled value are the same number, so a
schedule can be checked against the broker's published page by reading it. Refusing a third of a cent
rather than rounding it means a schedule that cannot be represented is reported as an error at the
boundary rather than silently becoming a schedule that is nearly right. The cost is that a genuinely
sub-cent price - some venues quote per-share commissions that way - cannot be configured directly and
would have to be re-expressed.

**Reconsider if** a fee schedule appears whose prices are genuinely finer than a minor unit.

---

## D17 - A fee schedule is plain data, checked at the boundary and asserted at the point of use

*Date: 2026-08-11. Status: accepted.*

**Context.** Every other quantity in this project is a value with an invariant checked in its own
constructor. A fee schedule is a bag of half a dozen unrelated numbers with no behaviour, arriving
either from a file or from a caller writing a scenario, and those two origins fail in entirely
different ways.

**Decision.** The schedule itself is plain data with no constructor of its own. Its well-formedness -
one currency, no negative fee, no proportional rate beyond the whole amount - is a question that can
be asked of it. Reading a schedule from a file asks that question and *reports* a refusal, naming the
key that caused it; charging a fee with a schedule *asserts* it and refuses to compute at all.

**Consequences.** A malformed file produces a message a user can act on, and a malformed schedule
built in code stops at the point where it entered rather than producing a plausible total that is
quietly wrong. Naming the offending key matters more than it looks: a tariff has half a dozen
similar-looking amounts, and "one of the fees is negative" is not a usable report. The cost is that
the same conditions are expressed twice, once as a check and once as an assertion, and that a
schedule can be passed around in a state that is not yet known to be valid.

**Reconsider if** the number of configured schedules grows to the point where the two paths drift
apart.

---

## D18 - Proportional costs are whole basis points

*Date: 2026-08-11. Status: accepted.*

**Context.** Brokers quote proportional costs as percentages - a quarter of a percent to convert
currency, a quarter of a percent of portfolio value for exchange access. A percentage held as a
decimal fraction is a floating-point number, and multiplying an exact count of minor units by an
inexact fraction gives a charge that depends on the platform's rounding.

**Decision.** Every proportional cost is held as a whole number of basis points, and a charge is the
amount times the basis points divided by ten thousand, in integer arithmetic, rounded to the nearest
minor unit with halves going to the even one.

**Consequences.** The rate is exact, so only one rounding happens - the division - and it happens
under the same rule as every other cash rounding in the project. Charges are reproducible and
order-independent. The cost is that a rate finer than a basis point cannot be expressed; no published
retail tariff this project models is quoted that finely, and a rate that fine would be swamped by the
minor-unit rounding of the charge itself.

---

## D19 - A conversion and its cost are reported separately, and the cost has no direction

*Date: 2026-08-11. Status: accepted.*

**Context.** An automatic currency conversion can be modelled as a single effective rate - the spot
rate reduced by the handling cost - which gives the correct net amount in one step. The alternative
is to report the spot amount and the cost as two figures and let the caller subtract.

**Decision.** The spot amount and the handling cost are separate results. The cost is computed on the
size of the amount rather than on its signed value, so it is a cost whether money is coming in or
going out, and it is charged in the currency the amount was converted into.

**Consequences.** A ledger can post a conversion as two entries, and the annual report can show what
currency handling cost over a projection - which is the whole point of modelling it. Charging on the
size rather than the signed value means money paid out costs more than the amount paid, which is what
actually happens and what a signed multiplication would get backwards. A round trip therefore loses
twice the handling cost less its square, and that fraction is independent of the exchange rate, which
makes it checkable without knowing any rate. The cost is one more figure for callers to handle and
the obligation to state, every time, which currency a cost is expressed in.

**Reconsider if** a broker charges conversion asymmetrically between directions.

---

## D20 - Configuration states cash in minor units

*Date: 2026-08-11. Status: accepted. Supersedes D16.*

**Context.** [D16](#d16---cash-in-configuration-is-written-as-a-decimal-and-read-as-an-exact-count-of-minor-units)
had configured amounts written the way a tariff is published - a quarter of a euro as a decimal - and
converted to minor units on the way in. That conversion was the only reason the project needed a
reading layer of its own on top of the library that already parses the document: the library exposes
whole numbers, strings and booleans by name, but nothing that turns a decimal into an exact count of
minor units, and nothing that says a decimal was too fine to be one.

**Decision.** A configured amount is a whole number of minor units, and the key says so. The
published quarter of a euro is configured as twenty-five, not as `0.25`. Documents are then read
entirely with the library's own file reader and navigator, which own the document, validate it and
refuse a missing or mistyped key by name.

**Consequences.** The project owns no reading layer at all - no decimal decoding, no fixed-point
arithmetic, no separate error vocabulary to keep in step with the library's. The unit a value is
configured in is the unit the model counts in, so there is no conversion to be wrong about and no
"finer than a minor unit" case to handle: a fraction in a whole-number field is simply not a whole
number, and is refused by name like any other mistyped value. The price is readability - a reviewer
comparing the configuration against a broker's published page reads twenty-five where the page says a
quarter of a euro - which is why the key carries the unit in its name. A second price is that the
library reports a missing key and a mistyped key with the same wording, so the two are no longer
distinguishable by kind, only by the message; for a configuration file, whose reader is a person
looking at the file, the message is what matters.

**Reconsider if** a configuration format arrives that this project does not control, and that quotes
cash as decimals.

---

## D21 - A short position reserves cash, and the reservation is what makes it covered

*Date: 2026-08-11. Status: accepted.*

**Context.** A broker does not require the full purchase price to be set aside against a short put;
it requires a margin that depends on its own risk model, on the price of the underlying, on the day,
and on the account. Modelling that would make every projection a projection of one broker's margin
policy, and would let the model write positions that the strategy this project describes would never
write.

**Decision.** A short put reserves the entire purchase price - strike times contract size times
contracts - out of cash for as long as it is open, and a short call is covered by shares that no
other call covers. Reserved cash counts towards the balance but is not available to secure another
position, to pay a charge, or to buy anything. A position that cannot be covered this way is refused.

**Consequences.** The capital committed by a position is a known, exact amount that does not move
with the market, so the sizing rule is deterministic and a projection is reproducible. The cost is
that the model can never show the leverage a real account is offered, and will always report a lower
return than a margined version of the same strategy - which is the honest comparison for the strategy
being described, and an understatement for any other. It also means an assignment is fully funded by
construction: the cash paid is exactly the cash released.

**Reconsider if** a margined variant of the strategy is brought into scope, in which case the
requirement becomes a policy supplied from outside rather than a rule in the model.

---

## D22 - A short option is carried at what it would cost to settle it

*Date: 2026-08-11. Status: accepted.*

**Context.** Equity has to include the liability of an open short option. Valuing it properly means
pricing it, which needs a volatility, a rate and a time to expiry at every point where equity is
asked for - and makes the reported equity of a strategy depend on the pricing model rather than on
what happened.

**Decision.** Equity is cash, plus shares at the stated price, less the intrinsic value of the short
positions at that same price. Extrinsic value is not carried.

**Consequences.** Equity is a function of the holdings and one price, with no model, no parameters
and nothing to estimate, so it can be checked by hand at any point. Because a projection reports its
rows at expiry, where extrinsic value is zero by definition, every published figure is exact rather
than approximate. It also makes assignment exactly value-neutral, which is the invariant that guards
the settlement arithmetic. The cost is that equity asked for mid-cycle understates the liability by
the extrinsic value, and therefore flatters a position that has moved against the writer; anything
that needs an accurate mid-cycle mark must price the position itself.

**Reconsider if** results are ever reported between expiries, for instance a mark-to-market drawdown
series rather than a per-cycle one.

---

## D23 - Cashflows are recorded and equity is derived

*Date: 2026-08-11. Status: accepted.*

**Context.** A compounding projection can carry its result as a running equity figure updated by each
event, or as an append-only record of what moved with the result computed from the holdings whenever
it is wanted. The first is cheaper to read and is what most spreadsheets do.

**Decision.** Every movement of cash is appended to a ledger as a dated, typed, signed entry, and
equity is derived from the holdings and a price each time it is asked for. Nothing stores an equity
figure. The types are fixed - premium, charge, assignment, dividend, tax, currency handling - and a
charge, a tax and a currency handling cost can only be negative while a dividend can only be
positive.

**Consequences.** The opening balance plus the ledger equals the closing balance at every point,
which is a check that runs on every projection rather than a claim. Because the entries are typed,
the report can answer what premiums earned and what charges cost over the whole horizon without a
second pass, and the sign rule means a charge booked backwards fails immediately rather than
inflating a return. The cost is that both a premium and an assignment must be allowed either sign -
buying a position back is a premium paid - so those two types carry no sign guarantee.

**Reconsider if** the number of entries becomes large enough that summing them dominates a
Monte-Carlo run, in which case a cached total that is checked against the sum, rather than replacing
it, is the compromise.

---

## D24 - A cycle is the unit of a projection, and a roll ends one

*Date: 2026-08-11. Status: accepted.*

**Context.** A projection has to advance in steps. The obvious step is a calendar period - a month, a
quarter, a year - but a wheel does not change state on the calendar; it changes when an option is
written, expires, is assigned or is closed. The obvious alternative is to make a roll extend the
current position into a later expiry, which is how the trade is usually described.

**Decision.** The step is one cycle: one option from the day it is written to the day it expires, is
assigned, or is closed early. A roll closes the position and ends the cycle; the replacement is the
next cycle's option, written from the balance the close left behind.

**Consequences.** Every row of a projection is one option's whole life, so premiums, charges and
settlements never straddle two rows, and a hand-checked table stays hand-checkable. Because a closed
position no longer exists, a rolled option cannot also be assigned - the single most damaging
double-count available in this model - and that follows from the structure rather than from a check.
The cost is that a row is not a calendar period, so anything that is annual by nature, taxation above
all, has to be struck on its own dates rather than on row boundaries.

**Reconsider if** a strategy variant holds several positions with different expiries at once, which
would make "the cycle" ambiguous.

---

## D25 - The strategy layer is driven by a script, and prices nothing

*Date: 2026-08-11. Status: accepted.*

**Context.** A projection needs a premium for every option it writes, a value for every position it
considers closing, and a price at every expiry. These could come from the valuation layer, given a
volatility and a rate.

**Decision.** They are supplied to the projection from outside, as a scripted sequence of periods.
The strategy layer decides how many contracts, which transitions are legal, when to roll and what
settles; it never computes what an option is worth. Assignment is delegated to the valuation layer's
moneyness rule and charges to the cost layer.

**Consequences.** The strategy can be tested against arithmetic that a reader can check on paper,
with no volatility anywhere in the test, and its results cannot be an artefact of the pricing model.
The same interface accepts a scripted path, a historical path or a simulated one without change,
which is what lets the Monte-Carlo layer reuse it unmodified. The cost is that the caller carries the
obligation to produce a consistent script - a premium that matches the volatility that generated the
path - and an inconsistent script produces a plausible-looking projection of nothing.

**Reconsider if** strike selection by target delta is brought inside the projection, which would
require it to price a chain rather than read a strike.

---

## D26 - The tax layer is given what is held, never what was earned

*Date: 2026-08-11. Status: accepted.*

**Context.** The tax this project models is a tax on wealth: it values the portfolio on one reference
date a year, imputes a return to it from published percentages, and charges a rate on that. Premiums,
capital gains and losses do not enter it. Almost every published tool for an options-income strategy
is written for a jurisdiction that taxes realised profit, and the mistake of carrying that assumption
across is invisible - it produces a plausible number that is wrong by a large factor in whichever
direction the strategy happens to run.

**Decision.** The only thing the tax layer accepts is a description of what is held on a date, split
by the kinds the rules distinguish. There is no argument by which a premium, a gain or a loss could
reach it, and no overload that takes one.

**Consequences.** The error cannot be made in the calling code, only by changing what the layer
accepts, which is a change a reviewer would see. A projection that collected a great deal of premium
and one that collected none are charged identically if they end the year holding the same thing, and
a losing year is still charged - both of which are correct here and would be nonsense under a profit
tax. The cost is that the layer cannot be reused, unmodified, for a jurisdiction that does tax profit;
that jurisdiction needs its own model, which is the honest answer.

---

## D27 - The share of the taxable base in the wealth is a derived proportion, not a configured rate

*Date: 2026-08-11. Status: accepted.*

**Context.** The allowance does not exempt a particular asset; it exempts a proportion of the whole
portfolio, and the deemed return is reduced in that same proportion. That proportion is a ratio of two
amounts, different for every portfolio, so unlike every other rate in this project it cannot be
[a whole number of basis points](#d18---proportional-costs-are-whole-basis-points). Holding it in
integers instead - multiplying the deemed return by the base before dividing by the wealth - is exact,
but the product overflows a signed sixty-four-bit count of minor units at a portfolio size that is
large and not absurd, and overflow here would stop a run that ought to succeed.

**Decision.** The proportion is a decimal. The taxable return is the deemed return scaled by it and
rounded to minor units once, at that point. This is the only place in the tax layer where an amount
passes through a decimal.

**Consequences.** There is no portfolio size at which the charge stops being computable, and the
result is still reproducible, because the scaling is one multiplication followed by the project's
usual rounding rule. The cost is one rounding that is not exact arithmetic, worth at most half a minor
unit on the taxable return, against a charge measured in hundreds. It also means the charge is not
exactly the tax rate times the imputed return times the taxable base, even when all wealth is of one
kind; the two differ by the intermediate rounding, which is why the worked example states the
intermediate figures rather than only the answer.

---

## D28 - A reference date is part of the script, and the charge is struck where it falls

*Date: 2026-08-11. Status: accepted.*

**Context.** [D24](#d24---a-cycle-is-the-unit-of-a-projection-and-a-roll-ends-one) makes a cycle the
unit of a projection, so no row boundary coincides with the turn of a year. An annual charge measured
on one date therefore falls *inside* a cycle, at a moment when a position is normally open. Valuing
the portfolio then needs a price, and [D25](#d25---the-strategy-layer-is-driven-by-a-script-and-prices-nothing)
says the strategy layer never produces one.

**Decision.** The period that contains a reference date carries it, together with the price of the
underlying on that date. The projection values the holdings as they stand at that instant - including
any open position, at its settlement value - charges the tax, pays it out of uncommitted cash, and
records it in the ledger on the reference date rather than on the row's date. A reference date the
projection reaches with no position open is struck all the same.

**Consequences.** The charge lands on the right holdings and on the right date without a cycle being
cut in two, and the ledger stays in date order without the projection having to reorder anything.
Because the charge is settled from uncommitted cash, a strategy that commits every last unit as
collateral has nothing left to pay with and the run stops; that is a real property of a fully invested
cash-secured strategy rather than an artefact, and the cash buffer is the lever against it. The cost
is that the caller must place each reference date in the period that contains it, and a script that
omits one silently projects a year with no tax.

---

## D29 - Only wealth is taxed; borrowing is out of scope

*Date: 2026-08-11. Status: accepted.*

**Context.** The published rules impute a negative return to borrowings above a threshold, and net
them against the imputed return on assets. Modelling that faithfully needs a further published
parameter, and raises a question the sources do not settle cleanly: what happens when the netted
return is negative.

**Decision.** Wealth is non-negative by construction and a debt cannot be expressed. The strategy this
project describes reserves the full purchase price against every written option
([D21](#d21---a-short-position-reserves-cash-and-the-reservation-is-what-makes-it-covered)) and never
borrows, so the term the model omits is one a projection cannot produce.

**Consequences.** The parameter set is smaller by one figure that could not have been confirmed
against a published source, and there is no unreachable branch computing a negative imputed return.
The limitation is stated rather than hidden: an investor who has borrowings elsewhere is charged more
in this model than they would be in reality, in the same way and for the same reason as an investor
with assets outside this portfolio is charged less.

**Reconsider if** a margined variant of the strategy is brought into scope, which is the same trigger
as D21.

---

## D30 - Randomness is seeded, and the transform from uniform to normal is ours

*Date: 2026-08-11. Status: accepted.*

**Context.** A distribution of outcomes is only useful if the same inputs produce it again: it has to
be comparable with another run, checkable by a test, and explainable to a reader. The obvious route -
asking the standard library for normally distributed numbers - does not give that. The library fixes
the *distribution* of the result but not the *algorithm* that produces it, so the same seed can yield
a different stream on another implementation, another platform, or another version of the same one.

**Decision.** Every source of randomness takes a seed, and the mapping from the underlying integer
stream to standard normal variates is written here: two uniforms in the open unit interval become two
normal variates by the Box-Muller transform, drawn and returned in a fixed order. Nothing in the layer
draws from a distribution the project does not define.

**Consequences.** A seed identifies a run exactly, in minor units, on any platform that has the same
arithmetic, which is what makes a simulated projection testable at all. Two variates are produced per
pair of uniforms and both are returned, so the number of draws consumed by a path is fixed and a path
does not depend on what was asked for before it. The cost is one small routine to own, verified
against the mean and variance it is supposed to have.

---

## D31 - A path is sampled exactly, on the dates the strategy reads

*Date: 2026-08-11. Status: accepted.*

**Context.** The price model has an exact solution: over any interval, the increment of the log price
is normal with a known mean and variance. A simulator can either use that, or discretise the process
and accept a bias that shrinks as the steps get smaller.

**Decision.** The log increment is drawn from its exact law, over whatever intervals it is asked for.
The strategy states the dates it needs a price for - each cycle boundary, each roll observation, each
reference date - and the path carries a price for exactly those dates and no others.

**Consequences.** There is no step-size error to trade against run time, and no minimum step below
which the simulation stops being trustworthy. A run of a wheel over several years costs a few dozen
draws instead of a few thousand, and the distribution it produces is the same one a daily grid would
have produced, because the strategy reads the price on a few dates either way. A daily grid is still
available and is what the estimators are fed, since they do read every observation. The cost is that
a quantity defined *between* the dates the strategy reads - the deepest intraperiod fall of the
underlying, say - cannot be reported without asking for more dates.

---

## D32 - The script is asked for one cycle at a time, after the previous one has settled

*Date: 2026-08-11. Status: accepted.*

**Context.** [D25](#d25---the-strategy-layer-is-driven-by-a-script-and-prices-nothing) has the
strategy layer driven by a script that says what is written, at what strike and for what premium.
Under a simulated path the script is generated rather than written out, and it immediately runs into
the fact that whether the next leg is a put or a call depends on whether the previous one was
assigned - which depends on the path.

**Decision.** The script is produced one cycle at a time, and each cycle is produced only after the
previous one has settled, from where the wheel actually stands. The generator of the script is told
the state; it does not predict it.

**Consequences.** The simulated run and the projection it drives cannot disagree about which leg was
written, because there is only one answer and it is read rather than guessed. A hand-written script
for a whole run remains possible and is what the deterministic projection uses, so nothing about the
strategy layer changes. The cost is that a script cannot be inspected in full before the run that
produces it.

---

## D33 - A strike is the nearest listed one whose delta does not exceed the target

*Date: 2026-08-11. Status: accepted.*

**Context.** A simulated future has no options chain to choose a strike from, so the strike has to
come from the model. Delta is the natural target - "sell the thirty-delta put" is how the strategy is
actually described - but no listed strike has exactly that delta, and delta is a continuous,
monotone function of the strike, so a target names an interval rather than a strike.

**Decision.** Strikes are whole multiples of a stated increment, and the one chosen is the nearest
one whose delta magnitude does not exceed the target: the highest such strike for a put, the lowest
for a call. The choice is found by bisection on the grid, which the monotonicity of delta in the
strike makes exact.

**Consequences.** The strike is one an exchange would list, and the risk written is never larger than
the risk configured - the error from the grid always falls on the conservative side, which is the
side a reader of a projection should be surprised by less often. Because the rule is one-sided, a
coarse strike grid systematically writes options slightly further out of the money, and therefore
collects slightly less premium, than the target implies. That is visible and bounded by the
increment, rather than being an average error of unknown sign.

**Reconsider if** a quoted options chain becomes available for the dates being projected, in which
case the strike and the premium should both come from it.

---

## D34 - Only the cash-secured leg is offered a roll

*Date: 2026-08-11. Status: accepted.*

**Context.** [D21](#d21---a-short-position-reserves-cash-and-the-reservation-is-what-makes-it-covered)
secures a short put with cash and a short call with shares. Rolling means buying the option back, and
buying it back needs cash. For a put the arithmetic always works: closing it releases collateral of
the full strike, and a put is never worth more than its strike. For a covered call it does not: the
position's collateral is shares, and repurchasing a call that has gone deep in the money needs cash
the position can only raise by selling those shares, which is not a move this model makes.

**Decision.** A simulated cycle attaches a roll observation only when the option written is a
cash-secured put. A covered call runs to expiry or assignment.

**Consequences.** A simulated run cannot stop partway through because it could not fund a repurchase -
the situation is not merely unlikely, it is unreachable. The risk-driven roll remains available on the
leg where it matters most, since it is a put going against the position that turns cash into shares.
The cost is that the model cannot express rolling a call up and out, which is a real technique; a
projection therefore takes assignment on calls slightly more often than a trader using that technique
would.

**Reconsider if** the strategy layer gains a way to fund a repurchase from the position it is
covering, which is a different strategy rather than a detail of this one.

---

## D35 - A percentile is an outcome that occurred

*Date: 2026-08-11. Status: accepted.*

**Context.** The 5th, 50th and 95th percentile of a set of simulated outcomes rarely fall on a path
exactly. The usual answer is to interpolate between the two neighbouring paths.

**Decision.** Outcomes are ordered by terminal equity and a percentile is the outcome at the rank
that percentile reaches, rounded up. No value is blended, and every figure reported at a percentile -
the growth rate, the deepest drawdown, how many cycles were assigned - is read from that same path.

**Consequences.** A reported percentile is a portfolio the simulation actually produced, and the
figures quoted beside it describe one coherent run rather than three unrelated ones. It also avoids
having to round an interpolated cash amount, which would put a rounding rule in the middle of a
reporting step. The cost is granularity: with few paths the percentiles are coarse and neighbouring
ones coincide, which is a true statement about how little a small run knows rather than a defect to
be smoothed over.
