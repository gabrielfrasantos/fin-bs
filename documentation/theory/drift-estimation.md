# Drift estimation

## Summary

The drift is the rate at which an instrument is expected to grow. This document defines the two
drifts that a price series can be said to have - the drift of the log price and the drift of the
price itself - shows why they differ by half the variance, and states how the slope of the log price
against time is fitted and how accurately it can be known.

## Motivation

Drift is the hardest quantity in this project to measure and the easiest to state confidently. A
volatility estimated from a year of daily closes is accurate to a few percent of itself; a drift
estimated from the same year is barely distinguishable from zero. The reason is structural rather
than a defect of the method: the volatility estimate improves with the *number* of observations,
while the drift estimate improves only with the *length of time* they span. Sampling a year of prices
every hour instead of every day multiplies the observations by twenty-four and does almost nothing
for the drift.

A model that reports a drift without reporting how little that drift is known would invite exactly
the mistake the whole project is meant to avoid: projecting decades of compounding from a number that
is mostly noise.

## Definitions & notation

| Symbol | Meaning | Unit |
|---|---|---|
| $S_t$ | price at time $t$ | currency, per share |
| $\mu$ | drift of the price, continuously compounded | per year |
| $\nu$ | drift of the log price, continuously compounded | per year |
| $\sigma$ | annualised volatility | per year |
| $T$ | year fraction spanned by the observation window | years |
| $q$ | continuous dividend yield | per year |

Time is measured in year fractions from the first observation, under the day-count convention the
series was recorded with. The observation dates are exact: they are recorded facts, not
measurements.

## Derivation / Rules

### Two drifts, and why they differ

Under the model this project uses, the price follows

$$\mathrm{d}S_t = \mu S_t\,\mathrm{d}t + \sigma S_t\,\mathrm{d}W_t,$$

whose solution is

$$S_t = S_0 \exp\left[\left(\mu - \tfrac{1}{2}\sigma^2\right)t + \sigma W_t\right].$$

So the log price grows at

$$\nu = \mu - \tfrac{1}{2}\sigma^2,$$

while the price itself satisfies $\mathbb{E}[S_t] = S_0 e^{\mu t}$. The gap between the two is the
whole of the difference between the *median* outcome and the *mean* outcome of a compounding process:
a path that is as likely to be above as below grows at $\nu$, while the average over all paths grows
at the larger $\mu$. Confusing the two is a silent error of half the variance per year - a quarter of
a percent per year at 20% volatility, two percent per year at 20% volatility over a decade of
compounding.

**A regression on the log price measures $\nu$.** The quantity a simulation needs in order to
reproduce $\mathbb{E}[S_t] = S_0 e^{\mu t}$ is $\mu$. The conversion between them is applied exactly
once, where the estimates are gathered together, and it needs the volatility - which is why the two
estimates are only ever published as a pair.

**A price series is not a total-return series.** If the instrument pays dividends, the observed
closes drop by the dividend and the fitted drift is the drift of the price, which is lower than the
total return by the yield $q$. The yield is therefore carried alongside the drift rather than being
recovered from the closes: a series of closes contains no information that could separate the two.

### Fitting the slope

The log price is regressed on time with an intercept, by ordinary least squares. On a uniform time
grid the fitted slope is

$$\hat\nu = \frac{\sum_i (t_i - \bar t)(y_i - \bar y)}{\sum_i (t_i - \bar t)^2}, \qquad y_i = \ln S_i.$$

Both axes are centred and scaled to unit length before the fit and the slope is rescaled afterwards.
Centring and scaling leave the least-squares slope unchanged exactly; they exist so that the fit is
conditioned identically whether the window is ten observations or ten thousand, and whether the price
is quoted in units or in thousands.

### How well it can be known

The estimate is a slope fitted through a random walk, so its error does not shrink with the number of
observations at all. For a window of length $T$ years, the standard error of the fitted log drift is
approximately

$$\operatorname{sd}(\hat\nu) \approx \sigma\sqrt{\frac{6}{5T}}.$$

Two consequences are worth stating plainly:

- At $\sigma = 20\%$ per year, ten years of data gives a standard error of about 7 percentage points
  on a drift that is itself perhaps 8 percentage points. **Ten years of daily closes cannot establish
  that an instrument grows at all.**
- To halve the error, the window must be four times longer - and over four times longer the drift is
  much less likely to have stayed constant.

The estimate is still worth making: it is the best available, and stating it with its error is what
allows a projection to be presented as a distribution rather than a promise.

### Orthogonal regression

Ordinary least squares assumes the horizontal axis is exact and attributes every deviation to the
vertical axis. Orthogonal regression - total least squares - instead minimises the perpendicular
distance to the line, which is the right thing to do when *both* coordinates are measured with error.

It is offered as a selectable alternative, but it is not the default, for two reasons.

1. **The time axis is exact.** The horizontal coordinate is a recorded date, not a measurement.
2. **On a noisy path it fits a steeper line.** With the axes scaled to unit length, the orthogonal
   slope equals the least-squares slope divided by the absolute correlation between the two axes:

$$\hat\nu_{\perp} = \frac{\hat\nu}{|\rho|} \ge \hat\nu .$$

   Since a price path is far from a straight line, $|\rho|$ is well below one and the orthogonal fit
   reports a materially larger drift. The two agree only when the path is nearly deterministic.

The situation orthogonal regression corrects for - a horizontal coordinate carrying error, which
biases the least-squares slope *towards zero* by the factor $\tau^2/(\tau^2 + \eta^2)$ where $\eta^2$
is the variance of the error in the coordinate and $\tau^2$ the variance of the coordinate itself -
cannot arise for a dated series in any material size. Recorded dates must be distinct and increasing,
so any error in them is smaller than the spacing between observations, and the ratio $\eta^2/\tau^2$
is then of the order of the reciprocal of the square of the number of observations.

## Worked example

Five closes, one every 31 days, of an instrument whose price grows deterministically at a log drift
of exactly 10% per year, with time in years under a fixed 365-day convention and each close rounded
to whole minor units:

| $i$ | $t_i$ | $S_i$ | $y_i = \ln S_i$ |
|---|---|---|---|
| 0 | 0.00000 | 100.00 | 4.605170 |
| 1 | 0.08493 | 100.85 | 4.613634 |
| 2 | 0.16986 | 101.71 | 4.622126 |
| 3 | 0.25479 | 102.58 | 4.630643 |
| 4 | 0.33973 | 103.46 | 4.639185 |

The mean of $t$ is $0.16986$ and the mean of $y$ is $4.622152$. The sum of cross-products is
$7.222446\times10^{-3}$ and the sum of squared deviations in $t$ is $7.213361\times10^{-2}$, giving

$$\hat\nu = \frac{7.222446\times10^{-3}}{7.213361\times10^{-2}} = 0.100126 \approx 10\%\ \text{per year}.$$

The path was constructed with a log drift of exactly 10%; the residual, about one part in eight
hundred, is the rounding of each close to whole minor units. Because the path has no volatility, the
arithmetic drift equals the log drift here; had the same series carried a volatility of 20% per year,
the arithmetic drift would have been $0.100126 + \tfrac{1}{2}(0.2)^2 = 0.120126$.

## Edge cases & pitfalls

- **A price that never moves has a drift of exactly zero.** This is checked on the quoted minor units
  rather than on the logarithms, because subtracting the mean from a constant series leaves rounding
  noise rather than zeros, and scaling that noise to unit length turns it into a fit of pure noise.
- **The window is bounded.** The fit is performed on a fixed-capacity system of equations, so a
  series longer than that capacity is refused where it enters rather than silently truncated. The
  capacity covers several decades of daily observations.
- **A short window is not an error, it is just uninformative.** Two observations define a line
  exactly; the fit will report its slope without complaint. The standard error above is the only
  thing that says whether the answer means anything.
- **The drift is assumed constant over the window.** Fitting one slope to a series that spans a
  regime change reports the average of the regimes, which describes neither.
- **Weekends and holidays are handled by the day-count convention, not by the fit.** Under a
  business-day convention consecutive trading days are equally spaced whatever the calendar does
  between them; under a calendar convention they are not, and the fit accounts for the gap.

## Alternatives considered

**Using the first and last observation only.** For a pure random walk this is the maximum-likelihood
estimator of the drift and it is *more* efficient than the fitted slope - its standard error is
$\sigma/\sqrt{T}$ rather than $\sigma\sqrt{6/5T}$, about 10% smaller. Rejected because it discards
every observation in between, which makes it defenceless against a single bad quote at either end of
the file, and because it cannot be extended to a weighted or robust fit later. The efficiency lost is
small next to the size of the error itself.

**Estimating the drift from the mean log return.** On a uniform time grid this is identical to the
first-and-last estimator, since the intermediate terms cancel; it is not a separate method.

**Not estimating a drift at all.** Defensible - a projection could be run at an assumed drift
supplied by the user. This is still available, since the estimate is only ever an input to the
projection, but a measured starting point with a stated error is more useful than an unexamined
assumption.
