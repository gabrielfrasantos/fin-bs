# What is read and what is written

## Summary

Three things cross the boundary between this project and the world outside it: a **price history**
comes in, a **scenario** goes both ways, and a **report** goes out. This document states what each of
them must be, what is guaranteed about reading and writing them, and which of their properties are
modelling inputs rather than presentation.

## Motivation

Every other layer of the model is handed values that are already correct: an amount is a whole number
of minor units, a date is a day of the calendar, a volatility is positive. Those guarantees exist
because something refuses the alternatives, and the only place that can refuse them is the boundary.
Inside the model a malformed value is a defect and is treated as one; at the boundary a malformed
value is *ordinary*, because a person typed it, and it has to be reported by name rather than
aborting the run.

Two things make that harder than it sounds.

1. **The world quotes prices more precisely than money is counted.** A published close carries four,
   six, sometimes eight decimals. Cash is counted in whole minor units. Something has to decide
   where the precision goes, and it has to decide the same way every time.
2. **A saved scenario must reload as the same scenario.** A person edits a run, saves it, reopens it
   a month later and expects the same numbers. If any field makes a round trip through a decimal
   representation, the reloaded scenario is *nearly* the one that was saved, and every result is
   then nearly reproducible - which is the same as not reproducible.

## Definitions & notation

| Term | Meaning |
|---|---|
| Price history | a sequence of closing prices, one per date, for one underlying |
| Scenario | the complete description of a run: the capital, the horizon, the levers, and the ordered list of underlyings |
| Cycle table | one row per underlying per cycle: what was written, what it earned, what it cost, and what was held afterwards |
| Ledger | every cashflow of a run, dated, typed and signed |
| Basis point | one ten-thousandth |

A **lever** is a setting that applies to the whole book: the target delta, the length of a cycle,
the delta at which a position is rolled away, and the fraction of cash held back. A **market** is
what is stated per underlying: a spot, the volatility its path moves with, the implied volatility
its options are priced at, a drift, a dividend yield, the rate withheld from a dividend, and the
spacing of the listed strikes.

## Derivation / Rules

### What a price history must be

| Requirement | Why it is a requirement |
|---|---|
| The columns are named, and named as a date and a close | a file whose columns are in another order is silently a different file |
| At least two closes | a return is a difference between two prices |
| Dates strictly increasing | a repeated date is two closes for one day; an earlier date after a later one is a sorting accident |
| Every close strictly positive | the logarithm of a return is taken |
| A positive span under the day count it is read with | a span of no time gives a volatility of no meaning |

The currency and the day count are **not** in the file. A file of numbers cannot say what currency
its numbers are in without a convention nobody has agreed on, and the day count is a modelling
choice - the same file read against calendar days and against trading days yields two different
annualised volatilities, both correct. Both are therefore supplied by whoever asks for the history,
and are part of the question rather than part of the data.

A close carrying more precision than a minor unit is **rounded to the nearest minor unit, ties to
even** ([decision D47](decisions.md)). It is not refused, because refusing it would reject almost
every published price file; and it is not truncated, because truncation biases every price downwards
and a systematic bias compounds exactly like a return does.

A row ended by a carriage return, and a blank row, are tolerated. Everything else is refused,
naming the row and quoting what was found there.

### What a scenario must be

A scenario states, in one place: the capital, the currency, the day the run starts, how many cycles
it runs and how long a cycle is, the four levers, the reference date of the wealth tax, how many
paths a simulation draws, the seed those paths are drawn from, which tax rules and which fee
schedule apply, and an **ordered list** of underlyings.

Two properties of that list are contracts rather than conveniences.

- **The order is significant.** It breaks ties in who is served collateral first
  ([decision D43](decisions.md)), so reordering the rows is a change to the scenario and not a change
  to its presentation. Reading preserves the order; writing preserves the order; a scenario written
  and read back lists its underlyings in the order it was given.
- **No underlying appears twice.** A book that names the same underlying twice would wheel it against
  itself out of one pot, and the two sleeves would disagree about what is held.

Every amount in a scenario is stated in whole minor units and every proportion in whole basis
points ([decision D48](decisions.md)). No decimal number appears anywhere in a scenario. This is what
makes a saved scenario reload as *the same* scenario rather than as one that is nearly it, and it is
also what removes any need for a numeric parser of this project's own: whole numbers are the one
thing a document format reads without ambiguity.

A scenario that cannot describe a run is refused **naming the field that cannot**: a missing field,
a field written as the wrong kind of value, a field outside the range it allows, a list of no
underlyings, a repeated ticker, a start that is not a day of the calendar, or an assessment date
that not every year holds. Nothing partial comes into existence.

A scenario may also carry a **`calibration` object** ([decision D55](decisions.md)): the folder its
price histories are in, relative to the configuration directory, with one `TICKER.csv` per
underlying, and the rule (`window_observations` 20 to 5000, `volatility_estimator` `historical` or
`exponentially_weighted`, `half_life_observations` 1 to the window, `drift_estimator`
`least_squares` or `stated`, `implied_premium_basis_points` -10000 to 10000). The object is all or
nothing: once present, every key is required and a missing one is refused by name, so a typo cannot
quietly turn calibration off. A scenario without the object states its terms outright. Written back,
the object comes out as it went in. The histories themselves are read by the window when a run is
asked for, each refused by ticker and line like any price history, and a name without a file is
refused by the path that was looked for.

### What a report is

Two tables, both written as rows of comma-separated values, both written from whole minor units and
never from a decimal number.

- The **cycle table** carries one row per underlying per cycle. It reports what state the wheel was
  in when the cycle closed, how many contracts it wrote, and the premium, costs, assignment
  settlement, dividend, cash and holdings that followed.
- The **ledger** carries one row per cashflow, in date order, each naming its kind and its sign.

The wealth tax appears in the ledger and **not** in a cycle row. A book is assessed as a whole, at a
cycle boundary, and the charge belongs to the book rather than to any one of its underlyings
([decisions D44](decisions.md) and [D49](decisions.md)). A cycle row whose tax column is zero is
therefore not a run that was untaxed; it is a row that is the wrong place to look.

## Worked example

A book of two underlyings, opened with 200,000.00 and a tenth of its cash held back, wheeled over two
half-year cycles. Both underlyings offer the same premium per unit of collateral in the first cycle -
0.80 against a strike of 40.00, and 1.00 against a strike of 50.00, both two hundredths - so the tie
is broken by the order they are listed in.

| Underlying | Cycle closed | State | Contracts | Premium | Costs | Assignment | Dividend | Cash | Shares | Value |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| first | 2025-07-18 | cash | 45 | 3,600.00 | 33.75 | 0.00 | 0.00 | 203,963.25 | 0 | 201,963.25 |
| second | 2025-07-18 | holding shares | 4 | 400.00 | 6.00 | -20,000.00 | 0.00 | 183,960.25 | 400 | 201,960.25 |
| first | 2026-01-16 | cash | 37 | 3,330.00 | 27.75 | 0.00 | 0.00 | 186,779.15 | 0 | 205,179.15 |
| second | 2026-01-16 | cash | 4 | 440.00 | 6.00 | 18,400.00 | 0.00 | 205,176.15 | 0 | 205,176.15 |

The first row is checkable by hand: nine tenths of 200,000.00 is 180,000.00, which at 4,000.00 of
collateral per contract writes forty-five contracts; forty-five hundred shares at 0.80 is 3,600.00 of
premium and forty-five contract charges of 0.75 is 33.75 of cost. The second underlying is served
from what is left, 23,566.25, of which nine tenths buys four contracts at 5,000.00 each.

Between the two cycles the book is assessed once, on 1 January, for a charge of 920.35 - a single row
in the ledger, and no column of the table above.

## Edge cases & pitfalls

- **A file with no rows and a file with no columns are different failures.** The first is a history
  too short to fit a return to; the second is very likely not a price file at all.
- **A close of exactly half a minor unit** settles on the even one, matching the rounding used
  everywhere else in the project, so a file of such prices carries no drift.
- **A scenario holding back all of its cash** is well formed and writes nothing. It is not an error:
  it is the honest answer to "what if I never write a contract".
- **A tax charge of nothing is not written to the ledger.** A book below the allowance has no row,
  rather than a row of zero.
- **The seed is a whole number of bounded width.** A scenario file carries whole numbers of a fixed
  size, so the space of reproducible runs is that size and not larger.
- **An empty report is a header and nothing else**, never an empty file: a reader that gets no rows
  should still be able to tell what it was reading.

## Alternatives considered

- **Refuse a close carrying more precision than a minor unit.** Exact, and unusable: published price
  files routinely carry six decimals, and the project would reject almost all of them.
- **State amounts in a scenario as decimals.** Familiar to whoever edits the file, and it breaks the
  round trip: a decimal read into binary floating point and written back is not always the same text,
  and the run it describes is then not always the same run.
- **Read the strikes actually listed for each underlying from a snapshot of the market.** Rejected
  for the same reason the snapshot is not read at all: strikes are generated on a listed grid
  ([decision D33](decisions.md)), and a snapshot taken today cannot price a position opened in a
  simulated future.
- **Write a report of one wide table rather than two.** A cycle row and a cashflow row have nothing
  in common but a date; joining them would give a table that is mostly empty and readable by nothing.

## References

- [Sharing one pot of cash between several wheels](../theory/running-several-wheels-at-once.md)
- [Money and rounding](../theory/money-and-rounding.md)
- [How a projection is put together](../theory/methodology.md)
- [Decision log](decisions.md): D20, D33, D43, D44, D47, D48, D49
