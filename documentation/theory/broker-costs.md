# Broker costs

## Summary

A broker's tariff is a small set of rules - a charge per contract, a charge per transaction, and a
proportional charge with a ceiling - applied to a list of trades and to the value of a portfolio.
This note states those rules exactly, works one example by hand, and records where the rounding
happens.

## Motivation

Over a twenty-year projection a wheel strategy opens and closes thousands of option contracts. At a
per-contract charge of the order of a euro, total costs are of the order of the initial capital, so
costs are not a correction to the answer - they are a large part of it. That makes two mistakes
expensive.

The first is treating a per-contract charge as a per-transaction charge. Selling ten contracts costs
ten times what selling one costs, so a model that charges per transaction understates the cost of
exactly the growth it is trying to project: as capital compounds, position size grows, and the error
grows with it.

The second is charging a proportional cost without its ceiling. An exchange connectivity charge of a
quarter of a percent of portfolio value sounds negligible and is, until it is compounded for twenty
years on a growing portfolio: uncapped, a quarter of a percent per year removes about five per cent
of terminal wealth over two decades. The published charge is capped at a few euro, so the correct
model of it is essentially a constant, and the uncapped version is wrong by orders of magnitude.

## Definitions & notation

| Symbol | Meaning | Unit |
|---|---|---|
| $f_o$ | charge per option contract | minor units |
| $f_s$ | commission per share transaction | minor units |
| $f_h$ | handling charge per share transaction | minor units |
| $b_c$ | connectivity rate | basis points |
| $c$ | connectivity ceiling, per exchange per year | minor units |
| $b_x$ | currency handling cost | basis points |
| $n$ | number of contracts in an option trade | contracts |
| $V$ | portfolio value at the moment the connectivity charge is struck | minor units |
| $m$ | number of exchanges charged for | exchanges |

One basis point is $10^{-4}$. All charges are in one currency, the currency the schedule is quoted
in; converting a charge is a separate step with its own cost.

## Derivation / Rules

### A single trade

$$
\text{cost of an option trade of } n \text{ contracts} = n\,f_o
$$

$$
\text{cost of a share transaction} = f_s + f_h
$$

The share transaction cost does not depend on the number of shares: the published tariff for the home
market is a flat commission plus a flat handling charge. The option trade pays no handling charge -
the published tariff exempts options and futures from it - and this exemption is modelled rather than
averaged away.

### A list of trades

$$
\text{cost of a list} = \sum_{k} \text{cost of trade } k
$$

Because every term is a whole number of minor units, the sum is exact and independent of the order in
which the trades are added. There is no accumulation of rounding error over a projection, however
long it runs.

### The annual connectivity charge

$$
\text{connectivity} = m \cdot \min\!\left(\left[\frac{V \cdot b_c}{10^4}\right], c\right)
$$

where $[\cdot]$ rounds to the nearest minor unit, halves going to the even one. The charge is struck
once per calendar year for each exchange that was traded on or that holds a position; the home market
is not charged. Because the proportional part is capped, the charge is a constant for any portfolio
above $10^4 c / b_c$ - with the published figures, above one thousand units of the base currency.

## Worked example

With the published schedule - $f_o = 0.75$, $f_s = 2.00$, $f_h = 1.00$, $b_c = 25$, $c = 2.50$, all
in euro:

| Trade | Rule | Charge |
|---|---|---|
| 1 option contract | $1 \times 0.75$ | 0.75 |
| 3 option contracts | $3 \times 0.75$ | 2.25 |
| 100 shares | $2.00 + 1.00$ | 3.00 |
| 2 option contracts | $2 \times 0.75$ | 1.50 |
| 50 shares | $2.00 + 1.00$ | 3.00 |
| **Total** | | **10.50** |

Note that the two share transactions cost the same although one is twice the size of the other, and
that the six option contracts cost 4.50 in total however they are grouped into transactions.

The connectivity charge, for one exchange:

| Portfolio value | Proportional part | Charge |
|---|---|---|
| 400.00 | 1.00 | 1.00 |
| 900.00 | 2.25 | 2.25 |
| 1 000.00 | 2.50 | 2.50 (exactly at the ceiling) |
| 1 000 000.00 | 2 500.00 | 2.50 |

For three exchanges and a portfolio of 400.00 the charge is $3 \times 1.00 = 3.00$; for three
exchanges and a portfolio of 1 000.00 it is $3 \times 2.50 = 7.50$.

## Edge cases & pitfalls

| Case | Result |
|---|---|
| An empty list of trades | zero, in the schedule's currency |
| Zero exchanges, any portfolio | zero |
| Zero portfolio, any number of exchanges | zero |
| A proportional part landing on exactly half a minor unit | rounds to the nearest even minor unit |
| A portfolio quoted in another currency | refused; convert first, and pay the conversion cost |
| A trade of zero contracts or zero shares | not a trade; refused |

**The ceiling binds almost always.** For any portfolio worth more than one thousand, the
connectivity charge is exactly the ceiling. A projection that starts small and compounds crosses that
threshold early and never returns, so treating the charge as proportional throughout would overstate
costs enormously - which is precisely why the ceiling is part of the model rather than an afterthought.

**Charges are struck at different times.** Trade charges fall on the day of the trade; the
connectivity charge falls once a year. A projection that posts them all at year end is not wrong in
total, but it misstates the capital available to reinvest during the year.

**A schedule is one account's prices.** The published tariff differs between account types and
between markets. The figures used here are for one account type on one home market, and their
provenance and date are recorded rather than assumed.

## References

- [Cost contracts](../design/costs.md) - what the layer requires, guarantees and refuses.
- [Currency conversion](currency-conversion.md) - the cost of moving between currencies.
- [Money and rounding](money-and-rounding.md) - why every charge is a whole number of minor units.
- [External parameter sources](../validation/sources.md) - the published tariff, and when it was last
  confirmed.
