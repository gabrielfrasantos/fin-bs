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
