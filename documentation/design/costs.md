# Cost contracts

## Summary

The cost layer answers two questions that stand between a gross strategy result and a net one: what
does the broker charge for a given list of trades, and what does it cost to hold or move money in a
currency that is not the reporting currency. It states what a fee schedule must be, what it
guarantees about the charges it computes, and where a schedule that is wrong is caught.

## Motivation

Costs are the one input to a compounding projection that is known exactly and is nevertheless the
easiest to get wrong by a large margin. A wheel strategy opens and closes short options continuously,
so a per-contract charge that is off by a few tens of cents changes a multi-decade projection by more
than a plausible error in the volatility estimate does. Two properties therefore matter more than any
particular fee.

1. **Nothing about a broker's prices is written into the model.** Every charge is read from a fee
   schedule that lives outside the model, is dated, and is traceable to the broker's own published
   tariff. A schedule changes; the model does not.
2. **A charge is never approximate.** Every fee is a whole number of minor units, computed from whole
   minor units and whole basis points. There is no path by which a fraction of a cent survives into a
   projection, and no path by which the same trade list produces two different totals.

## Definitions & notation

| Quantity | Unit | Meaning |
|---|---|---|
| Contract fee | minor units per contract | charged once for each option contract in a trade |
| Commission | minor units per trade | charged once per share transaction, whatever its size |
| Handling fee | minor units per trade | charged once per share transaction, in addition to commission |
| Connectivity rate | basis points of portfolio value | the proportional part of the annual charge for access to an exchange |
| Connectivity cap | minor units per exchange per year | the most the connectivity charge can be for one exchange in one year |
| Currency handling cost | basis points of the converted amount | charged on every automatic conversion between currencies |

A **basis point** is one ten-thousandth. Proportional costs are held as whole basis points rather than
as decimal fractions, so that the rate itself is exact and only the product with an amount has to be
rounded. See [decision D18](decisions.md).

A **trade** is a single transaction in one instrument: either a number of option contracts or a
number of shares. It carries what it traded and how many, and nothing else - a trade is not a
cashflow, and the cost layer never sees a price.

## Derivation / Rules

### What the caller must supply

| Requirement | Why it is a requirement |
|---|---|
| Every fee in the schedule quoted in one currency | a schedule is one broker's prices for one account |
| Every fee non-negative | a negative fee is a rebate, which this project does not model |
| Every proportional rate between zero and the whole amount | a cost larger than the amount it is charged on is not a cost |
| A trade of at least one contract or one share | a trade of nothing is a defect in the caller, not a free trade |
| A non-negative portfolio value for a connectivity charge | the charge is proportional to what is held |
| A non-negative number of days for debit interest | interest is charged on what is owed for the days it was owed |

These are checked in two different places, because they are two different failures.

- A schedule **read from a file** is external input. The file states every amount in whole minor
  units, the unit the model counts in, so there is nothing to convert. A missing key, a value of the
  wrong kind, a negative fee or a rate beyond the whole amount is *reported*, naming the key that was
  refused, and no schedule comes into existence. Nothing escapes the boundary and nothing aborts.
- A schedule **constructed inside the model** is a programming error if it is malformed, and the
  charge is refused outright at the point of use rather than producing a plausible wrong total.

See [decision D17](decisions.md).

### What the layer guarantees

- **A trade list costs the sum of its trades.** Charges are additive and order-independent, because
  they are exact integers; there is no accumulated rounding drift over a long projection.
- **The schedule's exemptions are modelled, not averaged.** Where the published tariff exempts a
  product from a charge - option trades from the per-transaction handling fee - the exemption is part
  of the model. A uniform "average fee per trade" would be simpler and would misprice exactly the
  instrument this project trades most.
- **A share transaction costs the same whatever its size.** The published tariff for the home market
  has no proportional component, so the cost of a share trade does not depend on the number of shares.
- **The connectivity charge is proportional up to a cap, and is charged once per exchange per year.**
  It is not a per-trade cost and is never charged on the home market.
- **A conversion and its cost are separate figures.** The layer reports what the amount became at the
  spot rate and what the conversion cost, rather than only the net. A ledger that must show fees
  cannot recover the fee from the net amount alone.
- **A conversion cost is charged on the size of a cashflow, not its direction.** Money coming in
  arrives short, money going out costs more; both are a cost. See
  [decision D19](decisions.md) and [currency conversion](../theory/currency-conversion.md).

- **Debit interest is a whole number of minor units.** It is the balance owed, times the rate, times
  the days, over ten thousand times three hundred and sixty-five, rounded once; a balance that owes
  nothing is charged nothing. See [decision D50](decisions.md).

### What the layer deliberately does not do

- **It does not know about markets or exchanges.** The number of exchanges a projection is charged
  for is supplied by the caller; the layer does not decide which market an instrument trades on.
- **It does not model tiered or negotiated pricing.** One schedule describes one account.
- **It does not hold an exchange rate over time.** A conversion model is a rate and a cost, both
  fixed; a projection that needs a path of exchange rates builds one and converts at each step.
- **It does not model the spread.** The difference between the bid and the ask is an implicit cost of
  the market, not a charge by the broker, and it belongs to valuation rather than to costs.

## Edge cases & pitfalls

| Case | What the model does |
|---|---|
| An empty trade list | costs nothing, in the schedule's currency |
| A connectivity charge on an empty portfolio | costs nothing, however many exchanges |
| A connectivity charge with no exchanges | costs nothing, however large the portfolio |
| A schedule with every fee at zero | is well formed, and every charge is zero |
| A proportional charge that lands exactly on a half minor unit | rounds to the nearest even minor unit, as every other cash amount does |
| A portfolio value in another currency | is refused rather than converted; conversion is an explicit step with a cost |
| A configured amount that is not a whole number of minor units | is refused when the file is read, naming the key |

## Alternatives considered

**A single "cost per trade" figure.** Tempting, and wrong in the direction that matters: a wheel
strategy's cost is dominated by per-contract option charges, which scale with position size, while a
flat per-trade figure does not. The error grows with the very compounding the project is trying to
measure.

**Fees as decimal fractions of a unit rather than whole minor units.** Rejected for the same reason
cash is counted in minor units everywhere else, and rejected a second time in the configuration file,
where writing amounts as decimals was the only thing that would have required a reading layer of our
own on top of the library that already parses the document. See
[money and rounding](../theory/money-and-rounding.md) and
[decisions D1 and D20](decisions.md).

**Folding the currency handling cost into the exchange rate.** An effective rate of the spot rate
less the handling cost gives the same net amount and makes the cost invisible: it could no longer be
reported as a fee, and a round trip would appear to lose money to the market rather than to the
broker. Rejected. See [decision D19](decisions.md).

## References

- [Broker costs](../theory/broker-costs.md) - how a schedule is applied, with a worked example.
- [Currency conversion](../theory/currency-conversion.md) - the base currency, the spot rate, and the
  cost of a round trip.
- [External parameter sources](../validation/sources.md) - where each published fee came from, and
  when it was last confirmed.
