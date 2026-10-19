# How a projection is put together

## Summary

Every other document here describes one piece: a price, a Greek, a fee, a tax charge, a path, a
search. This one describes the whole, in the order a run actually happens - from a scenario and a
price history, through the estimates they imply, to a distribution of outcomes and the setting that
produced it. It is the document to read before trusting a number, because it is the only one that
states what all the assumptions are *at once*.

## Motivation

A projection of a wheel strategy is a chain of a dozen models. Each is defensible on its own and each
has a documented error; what nobody can hold in their head is what happens to the answer when all
twelve errors are present together. A reader who has checked the option formula and the tax formula
and the fee schedule may still not know that the two underlyings in their book are drawn
independently, or that a contract count is floored to a whole number twice a year, or that the tax is
struck at a cycle boundary rather than on the first of January.

So the chain is written down once, end to end, with every assumption stated at the point it enters,
and with the limits collected at the end rather than scattered.

## Definitions & notation

| Symbol | Unit | Meaning |
|---|---|---|
| $S_t$ | currency | the price of one share at time $t$ |
| $K$ | currency | the strike of an option |
| $T$ | years | the time from now to expiry |
| $\sigma$ | per year | annualised volatility of log returns |
| $\mu$ | per year | the drift of the price |
| $r$ | per year | the risk-free rate, continuously compounded |
| $q$ | per year | the dividend yield, continuously compounded |
| $\Delta$ | dimensionless | the sensitivity of an option's value to the price |
| $b$ | basis points | the fraction of free cash held back |
| $N$ | count | the number of underlyings in the book |
| $M$ | count | the number of simulated paths |

Amounts are whole minor units throughout; rates, volatilities and prices along a path are real
numbers. See [money and rounding](money-and-rounding.md) and
[decisions D1 and D3](../design/decisions.md).

## Derivation / Rules

### 1. What a run is given

A scenario states the capital, the day it starts, how many cycles of what length it runs for, the
four levers, the reference date of the wealth tax, the number of paths and the seed, and an ordered
list of underlyings each carrying its own spot, volatility, drift, dividend yield and strike spacing.
Alongside it sit the tax rules for the year and the broker's fee schedule, each dated and each
traceable to its source. Nothing in the chain below is written into the model itself.

Optionally a price history is supplied per underlying, from which the volatility and drift of that
underlying may be estimated rather than typed.

### 2. From prices to a market

Given closes $S_0, \dots, S_n$ on dates $t_0 < \dots < t_n$, the log returns are
$r_i = \ln(S_i / S_{i-1})$. Their sample standard deviation, scaled by the square root of the number
of periods in a year, is the annualised volatility; the slope of the log price against the year
fraction is the drift of the *logarithm*, and the drift of the price is that slope plus $\sigma^2/2$.

Two facts matter more than the formulas. The volatility of a series of $n$ returns is known to a
relative error of roughly $1/\sqrt{2n}$, so a year of daily data gives about a twentieth. The drift
is known to an absolute error of roughly $\sigma/\sqrt{T}$ over a span of $T$ years, so a year of
daily data pins a drift only to within the volatility itself - which is to say, not at all. A
projection is therefore far more sensitive to a typed drift than to a typed volatility, and the drift
is the number to vary rather than to trust. See
[log returns](log-returns-and-volatility.md) and [drift estimation](drift-estimation.md).

### 3. From a market to a price path

Each underlying is a geometric Brownian motion: over a span of $\tau$ years,

$$S_{t+\tau} = S_t \exp\left[\left(\mu - \tfrac{1}{2}\sigma^2\right)\tau + \sigma\sqrt{\tau}\,Z\right],
\qquad Z \sim \mathcal{N}(0,1).$$

This is sampled **exactly**, at only the dates the strategy reads, so there is no step-size error and
no cost to a long horizon ([decision D31](../design/decisions.md)). The normal draws come from the
project's own generator so that a seed reproduces a run everywhere
([decision D30](../design/decisions.md)).

Each underlying is drawn from its own model, **independently of the others**
([decision D45](../design/decisions.md)). This is the largest known error in the chain and it is
revisited in the limits below.

### 4. From a path to a position

At the open of each cycle, each wheel that holds no shares looks for a cash-secured put and each
wheel that holds shares looks for a covered call. The strike is the listed strike whose magnitude of
delta is closest to the target without exceeding it - the highest such strike for a put, the lowest
for a call ([decision D33](../design/decisions.md)). The premium is the Black-Scholes value of that
option under the stated volatility and rate, rounded to the minor unit *per share* before being
multiplied by the multiplier and the number of contracts.

The number of contracts is what the uncommitted cash can collateralise:

$$n = \left\lfloor \frac{C \cdot (1 - b/10^4)}{100 K} \right\rfloor,$$

where $C$ is the free cash. There is no margin: a short put reserves the full strike
([decision D21](../design/decisions.md)). The floor is a real drag at small capital and a negligible
one at large - see [position sizing](position-sizing-and-compounding.md).

### 5. From N positions to a book

The wheels share one pot. A cycle is therefore run in **two passes**
([decision D42](../design/decisions.md)): every wheel writes before any wheel settles. Run each wheel
from writing to expiry in turn and the first releases its collateral before the second asks for it,
so nothing ever competes and the book writes several times the positions it can pay for.

Collateral is granted in a fixed order: wheels needing no fresh cash first, then the cash-secured
puts by descending premium per unit of collateral, ties broken by the order the scenario lists them
([decision D43](../design/decisions.md)). A wheel that finds the pot empty writes nothing, which is
an outcome and not an error. See [running several wheels at once](running-several-wheels-at-once.md).

### 6. From a position to a settlement

Before expiry, an open position may be rolled - closed and rewritten - if its delta has breached the
limit, or if enough of its premium has already been captured, or if it is close enough to expiry, in
that order of precedence. A rolled position cannot then be assigned
([decision D24](../design/decisions.md)). A covered call is never offered a roll, because it is
collateralised in shares rather than in cash and buying one back can cost more cash than the book has
([decision D34](../design/decisions.md)). See [rolling a short option](rolling-a-short-option.md).

At expiry the option is assigned if it is in the money and expires worthless otherwise; a call may be
assigned early if the dividend it would forgo exceeds its remaining time value. See
[assignment and early exercise](assignment-and-early-exercise.md). An assignment is exactly
value-neutral - what is held is worth the same immediately before and immediately after - which
follows from reserving the whole strike.

### 7. What it costs

Every option written or bought back is charged per contract; every share transaction is charged a
commission and a handling fee; holding a position on an exchange is charged a proportional annual
fee up to a ceiling; a balance owed is charged debit interest at the broker's rate. Every charge is
a whole number of minor units computed from whole minor units and whole basis points, so a trade
list costs the sum of its trades exactly and no rounding drift accumulates over a long projection.
See [broker costs](broker-costs.md).

### 8. What it is taxed

The tax is a **wealth tax, not a tax on income**. What is taxed is the value of what is held on one
reference date a year: a deemed return is computed from the split between deposits and investments,
the allowance is subtracted to give a base, the deemed return is taken in the proportion the base
bears to the whole, and the tax rate is applied to that.

Premiums are not taxed as they are earned. A strategy whose income is large relative to its capital
is therefore taxed lightly relative to what it earns, and one whose capital is large relative to its
income is taxed heavily relative to what it earns - which is the single largest difference between
this projection and one built for a capital-gains jurisdiction. See
[wealth taxation](box3-taxation.md).

A book is assessed **once, on the whole book**, at the cycle boundary at or before the reference
date, when nothing is open ([decision D44](../design/decisions.md)). The allowance is granted once,
not once per underlying.

### 9. From one run to a distribution

Steps 3 to 8 are repeated over $M$ paths from one seed. Percentiles are reported as the outcome at a
rank, never as an interpolation between two ([decision D35](../design/decisions.md)). The error of a
reported mean falls as $1/\sqrt{M}$: at a thousand paths the mean terminal value of an underlying of
25% volatility over three years is known to roughly a percent, and buying another decimal costs a
hundred times the paths. See [Monte-Carlo projection](monte-carlo-simulation.md).

### 10. From a distribution to a recommendation

A search varies three levers - the target delta, the cash buffer and the delta at which a position is
rolled away - and maximises the growth rate less a penalty proportional to the amount by which a risk
limit is exceeded. The contract count is not a lever: it is the buffer written twice
([decision D36](../design/decisions.md)). Every candidate is scored against the *same* futures, so
what a comparison measures is the difference between two strategies rather than between two samples
([decision D38](../design/decisions.md)). A limit no setting can meet is not refused; the search
reports how close it is possible to get ([decision D39](../design/decisions.md)). See
[searching for a setting](searching-for-a-setting.md).

## Worked example

The chain end to end, on the smallest case that exercises all of it: one underlying, one cycle.

| Step | Input | Result |
|---|---|---|
| Market | 22 daily closes rising from 95.00 to 100.86 | $\sigma \approx 0.09$, and a drift too poorly determined to report |
| Path | spot 40.00, $\sigma$ 25%, $\mu$ 6%, half a year | one draw of $S_T$ |
| Position | target delta 0.30, strikes spaced 1.00, capital 10,000.00, no buffer | a put struck at 40.00 for 0.80 per share, 2 contracts |
| Premium | 0.80 per share, 100 per contract, 2 contracts | 160.00 |
| Costs | 0.75 per contract | 1.50 |
| Settlement | $S_T = 45.00$, above the strike | expires worthless; the collateral is released |
| Tax | 10,158.50 held, allowance 57,684.00 | nothing; the book is below the allowance |
| Value | | 10,158.50 |

The larger hand-checked examples live where they belong: six cycles of a single wheel in
[the wheel](wheel-strategy.md), and two underlyings over two cycles in
[running several wheels at once](running-several-wheels-at-once.md).

## Edge cases & pitfalls

- **The drift is an input, not a measurement.** No length of history available to a retail investor
  determines it. Treat every projection as conditional on a drift that was chosen.
- **Volatility is constant in this model.** Real volatility clusters and jumps; a constant volatility
  understates how often a position ends deep in the money, which is exactly the case a wheel is paid
  to take.
- **The tax can outrun the free cash.** A fully committed book assessed at a boundary must still pay,
  and the cash buffer is the documented lever for making sure it can.
- **A book looks safer the larger it is.** See the limits below. This is an artefact, and it grows
  with the size of the book.
- **Reproducibility is exact, correctness is not.** The same seed gives the same answer to the last
  minor unit. That says nothing at all about whether the answer is right.

## Limits

Collected here rather than scattered, because they are what a reader needs before deciding what a
number is worth.

| Limit | Effect on the answer |
|---|---|
| Underlyings drawn independently | every drawdown and assignment rate is optimistic, more so the larger the book |
| Constant volatility, no jumps | assignment and deep losses are understated |
| Collateral to the richest premium first | the book concentrates in its most volatile name, which is also the one most likely to be assigned |
| Wealth tax struck at a cycle boundary | ignores whatever would have been open on the reference date itself |
| Deterministic assignment at expiry | the real decision is a counterparty's and is not always rational |
| One currency | conversion costs are modelled but no run mixes currencies |
| No borrowing, no margin | a position always reserves its full obligation, which is conservative |
| Simultaneous expiries | all wheels turn on the same day, so nothing is staggered |
| Tax rules of one year | the Dutch wealth tax is being reformed, and a stale year is wrong in a way nothing here can detect |
| Debit interest on the opening balance | a balance borrowed to pay a charge is charged for the whole cycle at the rate configured, which is unconfirmed |
| Dividends accrued, not paid on ex-dates | a share held for part of a cycle earns nothing; the credit of domestic withholding against the charge is not modelled |
| One implied volatility per underlying, no bid-ask | the premium is the model's value at that figure; a fill at the bid would be lower |

## References

- [Validation index and checklist](../validation/README.md) - what must be confirmed before trusting
  any of this.
- [Decision log](../design/decisions.md) - every decision cited above, dated.
- Every other document in this folder; this one is their table of contents in the order a run
  happens.
