# Running several wheels at once

## Summary

A book is several wheels run side by side out of one pot of cash. This note says how the pot is
divided when more than one wheel wants it, why the division has to happen before any of them
resolves, and what the arrangement does to the wealth tax. It ends with a two-underlying, two-cycle
example that can be checked on paper.

## Motivation

Running one wheel commits everything to one company. The obvious remedy is to run several, but as
soon as they share a balance the arithmetic stops being a repetition of the single-name case:

- **They compete.** Two cash-secured puts written on the same day are collateralised out of the same
  balance. Whichever is written first may leave the other nothing.
- **They must compete simultaneously.** If each wheel were run from writing to expiry before the
  next began, the first would have already released its collateral by the time the second asked for
  it, and the competition would never happen. Every wheel would be sized as though it had the whole
  balance, and the book would commit several times the money it has.
- **The wealth tax is not per position.** It is levied on what a person holds. Assessing each wheel
  separately would grant the tax-free allowance once per underlying, which for a book of five names
  would exempt five times the wealth the rules exempt.

## Definitions & notation

| Term | Meaning |
|---|---|
| Book | an ordered list of underlyings, wheeled in parallel out of one balance |
| Cycle | one turn of the clock: every wheel writes an option on the same day and every option expires on the same day |
| Claim | one wheel's request, in one cycle, for the collateral it needs to write |
| Premium yield | for a cash-secured put, the premium per share divided by the strike: what one unit of committed cash earns |
| Service order | the order in which claims are granted |
| Reference date | the day in the year on which wealth is measured for tax |

A premium yield is a pure number per cycle, not per year. The contract multiplier cancels: a put on
100 shares at a strike of $K$ commits $100K$ and earns $100p$, so the ratio is $p/K$ either way.

## Derivation / Rules

### The cycle has two phases

A cycle is run in two passes over the book.

1. **Writing.** Every wheel, in service order, decides how many contracts it can collateralise from
   the balance *as it then stands*, opens them, and receives the premium less the charge for
   opening. Collateral committed here stays committed for the rest of the pass, so a wheel served
   later sees only what is left.
2. **Settling.** Every wheel, in the order the book lists them, resolves the option it wrote: it is
   rolled away, expires worthless, or is assigned. Only now is collateral released.

Separating the two is what makes the competition real. Run the other way round — each wheel taken
from writing to expiry in turn — the first wheel's collateral would be free again before the second
asked for it, and the book would appear to afford every position at once.

### The service order

Within one cycle:

1. Wheels that need no fresh cash go first. A covered call is collateralised in shares, so it takes
   nothing from the balance and cannot crowd anybody out; it is written whatever else happens.
2. Wheels that want to write a cash-secured put follow, in **decreasing premium yield** — the
   richest premium for each unit of cash committed is served first.
3. Two claims of exactly equal yield are served in the order the book lists them. The order of the
   list is therefore part of the description of the strategy, not a presentational detail.

A wheel that finds the balance exhausted writes nothing that cycle. That is a result, not an error:
it is reported as a cycle with no contracts.

### The tax is struck on the book

The charge is computed once, from the whole book valued at the reference date — cash, plus the
shares of every underlying at its own price. The allowance is granted once. No individual wheel ever
assesses.

The assessment is struck at a **cycle boundary**, which is the moment at which nothing is open. What
is held is then unambiguously cash and shares, and the balance is at its least committed, so the
charge is at its most likely to be payable.

## Worked example

A book listing ACME then BETA, opened with 20,000.00, no cash buffer, nothing rolled. Charges are
0.75 per option contract, and 3.00 for a settlement in shares.

**Cycle 1** — ACME: strike 40.00, premium 1.20, closing at 35.00. BETA: strike 50.00, premium 1.00,
closing at 55.00.

Yields are $1.20/40.00 = 0.0300$ for ACME and $1.00/50.00 = 0.0200$ for BETA, so ACME is served
first although both want cash.

| Step | ACME | BETA | Balance |
|---|---|---|---|
| Opening | | | 20,000.00 |
| ACME writes | 20,000.00 / 4,000.00 = 5 puts, 20,000.00 committed | | 20,000.00 |
| Premium | +600.00 | | 20,600.00 |
| Charge | −3.75 | | 20,596.25 |
| BETA writes | | 596.25 free, 5,000.00 needed → **nothing** | 20,596.25 |
| ACME assigned | −20,000.00, 500 shares at 40.00 | | 596.25 |
| Charge | −3.00 | | 593.25 |

**Cycle 2** — ACME now holds shares, so it writes a covered call: strike 38.00, premium 0.90,
closing at 42.00. BETA: strike 50.00, premium 1.00, closing at 52.00.

| Step | ACME | BETA | Balance |
|---|---|---|---|
| ACME writes | 5 calls against 500 shares, no cash committed | | 593.25 |
| Premium | +450.00 | | 1,043.25 |
| Charge | −3.75 | | 1,039.50 |
| BETA writes | | 1,039.50 free, 5,000.00 needed → **nothing** | 1,039.50 |
| ACME assigned | 500 shares delivered at 38.00 | | 20,039.50 |
| Charge | −3.00 | | 20,036.50 |

The book closes at 20,036.50, holding no shares, having grown by 36.50 in a year in which the
underlying it committed everything to fell by an eighth and then recovered.

Two things in that table are the point of this note. BETA never wrote a single contract, because
ACME's richer premium took the whole balance in the first cycle and its shares occupied the second.
And the covered call in cycle 2 was written before BETA was even asked, because it needed no cash.

## Edge cases & pitfalls

- **A wheel that writes nothing.** Not an error, and not a gap in the record: the cycle is reported
  with no contracts and no cashflows.
- **Concentration.** The service rule ranks by premium yield, and the richest premium is the one on
  the most volatile underlying — which is also the one most likely to be assigned. Serving the
  richest premium first therefore concentrates the book in its riskiest name. This is a property of
  the rule, not an accident of it, and it is why the rule is stated separately from everything else.
  The cap per name is the lever against it: with a cap of $c$ (a fraction of the book's equity at
  the cycle's opening, $E_0$) a name writing puts at strike $K$ takes at most
  $\lfloor c \, E_0 / (100 K) \rfloor$ contracts, whatever the balance would afford, and the next
  name is served from what remains. A cap of one is the book as described above.
- **Independence.** Underlyings are drawn independently. Real equities fall together, so every
  drawdown and every assignment rate a book reports is optimistic, and a book of many names will
  look far safer than it is.
- **A charge that cannot be paid.** Striking the tax at a cycle boundary makes the balance as free
  as it ever gets, but a book that commits everything can still be unable to settle a charge. The
  cash buffer is the lever.
- **Assignment frequency.** For a book the denominator is the number of options written, not the
  number of cycles: a book of three names over six cycles has eighteen chances to be assigned.

## Alternatives considered

- **Fixed sleeves.** Divide the capital up front, one sleeve per underlying, never transferred. This
  removes the competition entirely and with it the need for a service rule, but it also removes the
  compounding: a sleeve that is assigned and then recovers cannot lend its idle cash to a sleeve
  with a position worth writing.
- **Serving in list order.** Simple and reproducible, but it makes the answer depend on an
  arbitrary decision — and one nobody would defend as a strategy — rather than on anything about
  the market.
- **Allocating by target weight.** Each underlying gets a share of the balance in proportion to a
  weight. Predictable, but with whole contracts and listed strikes the weights are unreachable
  anyway, and a weight that cannot collateralise one contract simply wastes cash.
- **Choosing the allocation optimally.** The division that maximises expected growth is a knapsack
  problem solved per cycle. It is the right answer to a question nobody can act on: it needs the
  premiums of every underlying before any is written, and it changes discontinuously with them.
- **One clock per underlying.** Staggering expiries is more realistic and would smooth the
  competition, but it turns a cycle into a merged timeline of events and makes the shared balance
  much harder to reason about.

## References

Nothing external. The arithmetic above is reproduced by an automated test.
