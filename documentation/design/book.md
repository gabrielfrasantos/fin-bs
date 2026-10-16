# The book

## Summary

The book is the layer that turns one wheel into several. It owns the things there is exactly one of
— the balance, the record of cashflows, the tax rules — and lends them to each wheel in turn. This
note states what it guarantees, what it asks of a caller, and what it deliberately cannot express.

## Motivation

The strategy layer was built around one underlying: one wheel, its own balance, its own record.
Running several out of one pot could have been done by generalising that engine to hold a list of
wheels. It was not, for two reasons.

The first is that the single-name engine is the one whose arithmetic is checked by hand. A table of
six cycles that a reader can verify on paper is the strongest evidence the project has that the
settlement arithmetic is right, and rewriting the engine underneath it would have spent that
evidence.

The second is that "several wheels sharing a balance" is not a generalisation of "one wheel with a
balance" — it is a different thing sitting *above* it. What is shared is the account; what is not
shared is the state of each wheel. Naming that split is most of the design.

## Definitions & notation

| Term | Meaning |
|---|---|
| Account | the balance, the record of cashflows, and the tax rules in force, as one thing |
| Book | an ordered list of underlyings run over one account on one clock |
| Cycle | one turn of the clock, carrying one period for every underlying in the book |
| Claim | what one wheel asks of the balance in one cycle |
| Marks | a price for every underlying held, at one instant |

## Derivation / Rules

### What the caller must supply

| Requirement | Why it is a requirement |
|---|---|
| A book listing at least one underlying, each named and each named once | a book with a name in it twice has two wheels fighting over one holding |
| A cycle carrying exactly one period per underlying in the book | a book values itself at every settlement, and it cannot value a holding it was given no price for |
| Periods that carry no assessment of their own | the tax is struck on the book; a wheel that assessed would grant the allowance once per underlying |
| A price for every underlying held, whenever the book is valued or assessed | a holding valued at nothing is a wrong answer, not a missing one |

### What the layer guarantees

- **The balance is never committed twice.** Every wheel writes before any wheel settles, so a wheel
  served later sees only the cash its predecessors left. Collateral is released at settlement, never
  during the writing pass.
- **The order of service is a stated rule, not an accident of the loop.** It is named, isolated, and
  tested on its own, because it is the only place where one underlying can deprive another. It can
  be replaced without touching the book.
- **The order of the book is meaningful.** It breaks ties between equally attractive claims.
  Reordering the list is a change to the strategy.
- **One assessment, one allowance.** The wealth tax is computed from the whole book and posted once.
- **The record still explains the balance.** The opening balance plus every posted cashflow equals
  the balance at any moment, with any number of wheels running.
- **A wheel does not know it is in a book.** It is handed an account and told which underlying is
  its own. It refuses only a position in *that* underlying; another wheel's position is none of its
  business.

### What is deliberately impossible to express

- Valuing a book without a price for something it holds. There is no argument that means "and value
  the rest at zero".
- Assessing a wheel that is part of a book.
- Two wheels on the same underlying.
- A cycle that covers some of the book but not all of it.

### How a wheel is lent an account

A wheel either owns its account or was lent one, and which of the two survives being copied. A wheel
that owns its account carries its own balance wherever it is copied, which is what lets one
description of a strategy be run over thousands of simulated futures without one leaking into the
next. A wheel that was lent an account keeps writing to that same account however many times it is
copied, which is what lets several of them share a pot.

A wheel is built fresh for each cycle, and works out for itself whether it is holding shares or
holding cash by looking at what the account holds. That is possible because a cycle boundary is the
one moment at which nothing is open: every option written has expired, been assigned, or been rolled
away. Carrying the state forward separately would be a second copy of a fact the account already
records, and two copies of a fact eventually disagree.

## Worked example

A book listing ACME then BETA, opened with 20,000.00. ACME's put yields 3% of the cash it commits
and BETA's yields 2%, so ACME is served first and takes the entire balance; BETA writes nothing.
ACME is assigned, and in the next cycle its shares back a covered call that needs no cash at all —
so it is served before BETA is even asked, and BETA writes nothing again.

The full arithmetic is in
[running several wheels at once](../theory/running-several-wheels-at-once.md), and is reproduced by
an automated test.

## Edge cases & pitfalls

- **A cycle in which nothing is written.** Every wheel still reports a row.
- **Settling in list order.** Settlement is done in the order the book lists its underlyings, not in
  the order they were served, so that a run is reproducible. Nothing depends on it: each wheel
  settles only its own position, and the record sorts itself by date.
- **A charge the book cannot settle.** Assessing at a cycle boundary makes this as unlikely as it
  can be made, but a fully committed book can still fail to raise the cash. The buffer is the lever.
- **The equity on every row is the book's, not the underlying's.** Cash is shared, so there is no
  meaningful per-underlying equity to report. The contracts, premium, charges, assignment and share
  count on a row belong to that underlying; the balance and the equity belong to the book.

## Alternatives considered

- **Generalising the single-name engine to hold a book of wheels.** One engine instead of two
  layers, at the cost of rewriting the tests that make the arithmetic checkable by hand.
- **Keeping a separate record per wheel and merging them.** Removes the need for the record to sort
  itself, but makes "the balance" a derived quantity that no single object owns.
- **Requiring cashflows to be posted in date order, as before.** Impossible once several wheels
  share one clock and one record: whichever wheel is served second posts an opening cashflow after
  the first has already posted one at expiry.

## References

Nothing external.
