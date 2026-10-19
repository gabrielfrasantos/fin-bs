# Monte-Carlo projection of a wheel

## Summary

A projection over one price path answers "what would have happened if the underlying had done
exactly this". A Monte-Carlo projection answers the question that can actually be acted on: given a
model of how the underlying moves, what is the *range* of outcomes, and how often does each part of
that range occur. This note states the price model used, why the path is simulated exactly rather
than approximated, how many paths are needed before an average means anything, and how a set of
outcomes is reduced to a few reportable figures.

## Motivation

The wheel is path-dependent in three ways that make its distribution impossible to read off the
average path. It is assigned or not depending on where the price sits on one day, and that decision
changes what is written next. It compounds, so the size of every later position depends on every
earlier outcome. And it is taxed on the value of the holdings on one date a year, so the same total
return taxed at a different moment is a different result.

Because of this, the outcome of the average path is not the average outcome. Running the strategy
on a price that grows smoothly at the drift produces a projection in which no put is ever assigned,
no share is ever held, and the wheel collects premium forever - a result that is not merely
optimistic but structurally different from the one the strategy actually produces. Nothing short of
running the strategy on many paths recovers the assignment frequency, the drawdown, or the spread
between a good decade and a bad one.

The second reason is honesty about precision. An average over simulated paths is itself an estimate,
and quoting it without saying how well it is known invites a reader to believe digits that are
noise. The uncertainty of the average shrinks only as the square root of the number of paths, which
is slow: a hundred times as many paths buys one extra decimal place.

## Definitions & notation

| Symbol | Meaning | Units |
|---|---|---|
| $S_t$ | price of the underlying at time $t$ | currency per share |
| $\mu$ | arithmetic drift: the continuously compounded rate at which the *expected price* grows | per year |
| $\nu = \mu - \tfrac{1}{2}\sigma^2$ | log drift: the rate at which the *expected log price* grows | per year |
| $\sigma$ | volatility of the log price | per year$^{1/2}$ |
| $Z$ | a standard normal variate, mean zero and variance one | - |
| $N$ | number of simulated paths | - |
| $H$ | horizon of the projection, from its opening date to its last expiry | years |
| $g$ | compound annual growth rate over the horizon | per year |

Time is measured in year fractions on a stated day-count convention; $t$ always means elapsed years,
never elapsed steps.

## Derivation / Rules

### The price model

The underlying is modelled as geometric Brownian motion: the *logarithm* of the price is a random
walk with constant drift and constant variance per unit time,

$$\mathrm{d}\log S_t = \nu \,\mathrm{d}t + \sigma \,\mathrm{d}W_t .$$

Working in the logarithm rather than the price is what keeps the price positive by construction: no
sequence of draws, however extreme, can produce a negative or zero price, so no downstream
calculation has to guard against one.

The $\sigma$ here is the volatility the path moves with. The options the script writes on that path
are priced at a separately stated implied volatility ([decision D52](../design/decisions.md)); the
two are equal only if the scenario says so.

The distinction between the two drifts is the classic trap. The expected price grows at $\mu$,

$$\mathbb{E}[S_t] = S_0 e^{\mu t},$$

while the expected log price grows at the smaller $\nu = \mu - \tfrac{1}{2}\sigma^2$,

$$\mathbb{E}[\log S_t] = \log S_0 + \nu t, \qquad \operatorname{Var}[\log S_t] = \sigma^2 t .$$

The gap $\tfrac{1}{2}\sigma^2$ is not a correction factor; it is the difference between the average
of an exponential and the exponential of an average. At a volatility of 20% it is 2 percentage
points a year, which over a decade is the difference between a strategy that looks worth running and
one that does not. The simulation is told the arithmetic drift, because that is what an estimator
of returns reports and what a reader means by "expected return", and it converts to the log drift
once, in one place.

### Why the path is exact rather than approximated

Over any interval of length $h$ the increment of the log price is exactly normal:

$$\log S_{t+h} = \log S_t + \nu h + \sigma \sqrt{h}\, Z ,$$

with $Z$ independent of everything before $t$. This is not a discretisation of the process, it *is*
the process, sampled. Two consequences follow, and both matter.

First, there is no step-size error to trade against run time. A scheme applied to the price itself
rather than its logarithm - taking $S_{t+h} = S_t(1 + \mu h + \sigma\sqrt{h}Z)$ - is only correct in
the limit of small $h$, can produce a negative price, and leaves a bias that has to be balanced
against the cost of taking more steps. Sampling the log increment has none of these problems.

Second, the path may be sampled on *exactly* the dates the strategy looks at and nowhere else. A
wheel observes the price when it writes an option, when it considers rolling it, on the tax
reference date, and at expiry. Simulating every intervening day would be arithmetic nobody reads,
and it would not change the distribution of the prices on the dates that are read. So the strategy
states the dates it needs, and the path is generated on those dates.

The deterministic part of the increment - the $\nu h$ term - is obtained by integrating the drift
over the interval with a fixed-step Runge-Kutta rule. For a constant drift that integration is exact,
which is exactly the point: the drift enters through a general integrator, so a later model in which
the drift varies with time or with the price needs no new machinery, and the constant-drift case is
not silently special-cased.

### How many paths

The average of $N$ simulated terminal prices is an estimate of $\mathbb{E}[S_H]$ whose own standard
deviation - the standard error - is

$$\operatorname{se}(N) = \frac{\operatorname{sd}[S_H]}{\sqrt{N}}, \qquad
\operatorname{sd}[S_H] = S_0 e^{\mu H} \sqrt{e^{\sigma^2 H} - 1} .$$

An estimate is consistent with the model when it lies within about three standard errors of the
closed-form expectation; that is the check that says the simulator is drawing from the process it
claims to. The $\sqrt{N}$ is the whole story about cost: reducing the uncertainty tenfold needs a
hundred times the work, so there is a point past which more paths are not the answer.

### Reducing a set of outcomes

Each path yields a terminal equity, a growth rate, how many cycles ended in assignment, and the
deepest fall from a previous high. Four things are reported from the set.

**Percentiles.** The outcomes are ordered by terminal equity and the $p$-th percentile is the
outcome at rank $\lceil pN/100 \rceil$. This is a real outcome that really occurred in the
simulation, not a weighted blend of two neighbours: an interpolated percentile is a portfolio value
no path ever reached, and reporting one as a plausible result of a strategy is a small lie that is
hard to unsay. It also means every quantity reported at a percentile - the growth rate, the
drawdown, the assignment count - belongs to the *same* path, so the figures are mutually consistent.

Ordering by terminal equity also orders by growth rate, because over a fixed horizon from a fixed
opening balance the growth rate is a strictly increasing function of the terminal balance. The
median growth rate is therefore the growth rate of the median outcome, and no second ordering is
needed.

**Compound annual growth rate.** From an opening equity $E_0$ and a terminal equity $E_H$ over $H$
years,

$$g = \left(\frac{E_H}{E_0}\right)^{1/H} - 1 .$$

**Maximum drawdown.** The largest proportional fall from a previous peak of equity, measured at the
points the projection reports, $\max_k (P_k - E_k)/P_k$ where $P_k$ is the highest equity up to and
including point $k$. It is bounded below by zero and, because equity cannot become negative, is
always below one.

**Assignment frequency.** Assignments divided by cycles run, over all paths together. It is the
figure that says whether the strategy being modelled is really a wheel or is in practice a
premium-collecting programme that rarely takes delivery.

## Worked example

An underlying at 100.00 with an arithmetic drift of 8% and a volatility of 20%, over one year.

| Quantity | Formula | Value |
|---|---|---|
| Log drift | $\mu - \tfrac{1}{2}\sigma^2 = 0.08 - 0.02$ | 0.06 per year |
| Expected price | $100 \cdot e^{0.08}$ | 108.33 |
| Median price | $100 \cdot e^{0.06}$ | 106.18 |
| Standard deviation of the price | $108.33\sqrt{e^{0.04} - 1}$ | 21.88 |
| Variance of the log price | $\sigma^2 H$ | 0.0400 |

The expected price exceeds the median by 2.15, which is the whole of the $\tfrac{1}{2}\sigma^2$
effect: more than half of all paths end *below* the expected price. A projection that quotes only
an average is describing an outcome the majority of investors would not get.

The band the average has to fall inside, at three standard errors:

| Paths | Standard error | Three-sigma band |
|---|---|---|
| 1 000 | 0.692 | 2.08 |
| 10 000 | 0.219 | 0.657 |
| 100 000 | 0.069 | 0.208 |

A hundred times the work narrows the band by a factor of ten.

Percentiles by rank, on three outcomes ending at 10.00, 20.00 and 30.00:

| Percentile | Rank $\lceil pN/100 \rceil$ | Outcome |
|---|---|---|
| 1 | 1 | 10.00 |
| 33 | 1 | 10.00 |
| 34 | 2 | 20.00 |
| 67 | 3 | 30.00 |

## Edge cases & pitfalls

- **Prices are recorded in minor units.** Rounding a simulated price to the cent adds a quantisation
  error of at most half a minor unit, which on a price of 100.00 is a relative error below $10^{-4}$
  and contributes a variance five orders of magnitude below the volatility being modelled. It is
  invisible at any realistic price, but it would not be for an underlying quoted in single minor
  units, which is not a case this project models.
- **A percentile of a handful of paths is a single path.** With fewer than twenty paths the 5th and
  the 50th percentile are frequently the same outcome. Percentiles are worth quoting from hundreds
  of paths, not dozens.
- **Constant volatility is the model's weakest assumption.** Real volatility clusters and rises when
  prices fall, so a constant-volatility simulation understates the frequency of the sequence that
  hurts a wheel most: a sharp fall that assigns the put, followed by a period in which the calls
  written above the purchase price are worth very little.
- **The drift is barely knowable.** The uncertainty of an estimated drift falls with the *length* of
  the history, not the number of observations in it, so a decade of data pins it only to within a
  few percentage points a year. Simulated outcomes inherit that uncertainty, and the spread between
  simulations run at plausibly different drifts is wider than the spread within any one of them.
- **A path can stop.** A wheel that has committed nearly all of its capital as collateral may reach
  a charge it cannot settle from uncommitted cash. This is a real property of a cash-secured
  strategy rather than an artefact of simulating it, and the cash buffer is the lever against it.
- **Every run is reproducible or it is worthless.** A distribution that changes between runs cannot
  be compared with another, cannot be regression-tested and cannot be explained. The random stream
  is therefore seeded, and the same seed reproduces the same outcomes exactly.

## Alternatives considered

- **Simulating the price rather than its logarithm.** Rejected: it admits negative prices, needs
  small steps to control its bias, and offers nothing in return, since the log increment is available
  exactly.
- **A daily grid regardless of what the strategy observes.** Rejected for the strategy runs: it
  multiplies the arithmetic by two orders of magnitude and changes no reported figure, because the
  strategy reads the price on a handful of dates. A daily grid is still generated when the purpose
  is to feed the estimators, which do read every observation.
- **Interpolated percentiles.** Rejected: they report a portfolio value no simulated path reached,
  and they break the correspondence between the percentile of the equity and the other figures
  reported alongside it.
- **Variance reduction by antithetic or control variates.** Not used. Both would narrow the bands
  above for the same number of paths, and both make the relationship between the seed and the
  outcomes harder to explain. The simulation is currently fast enough that plain sampling is not the
  constraint.

## References

- Hull, *Options, Futures and Other Derivatives* - geometric Brownian motion, the exact solution of
  the price process, and Monte-Carlo valuation.
- Glasserman, *Monte Carlo Methods in Financial Engineering* - exact simulation of the log increment,
  standard errors, and the cost of variance reduction.
