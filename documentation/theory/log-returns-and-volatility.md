# Log returns and realised volatility

## Summary

Everything the estimation layer measures is derived from one transformation: the change in the
logarithm of the price from one observation to the next. This document defines that quantity, states
how a deviation measured over one observation period is converted into an annual figure, and gives
the error bounds that follow from prices being quoted in whole minor units.

## Motivation

A strategy is projected forward as a compounding process, and a compounding process is additive in
the logarithm of the price. Working in simple percentage changes instead has two costs. A gain of
50% followed by a loss of 50% is not a wash but a 25% loss, so simple changes cannot be added across
periods; and the estimator of the annual figure would then depend on how often the price happened to
be sampled. Log changes add exactly across consecutive periods, and their sum over a window is the
total log change of that window - a property every test in this layer relies on.

## Definitions & notation

| Symbol | Meaning | Unit |
|---|---|---|
| $S_i$ | closing price of observation $i$ | currency, per share |
| $r_i$ | log return of period $i$, $\ln(S_i / S_{i-1})$ | dimensionless, per period |
| $n$ | number of log returns, one fewer than the number of observations | - |
| $m$ | observation frequency, periods per year | per year |
| $s$ | sample standard deviation of the log returns | per period |
| $\sigma$ | annualised volatility | per year |

The observation frequency is *derived*, never configured: it is the number of observation intervals
divided by the year fraction that the whole series spans, under the day-count convention the series
was recorded with. A series of daily closes recorded on trading days under a business-day convention
therefore reports 252 periods per year by construction, and a series of month-end closes reports 12,
without either number being written down anywhere.

## Derivation / Rules

A log return is

$$r_i = \ln \frac{S_i}{S_{i-1}} = \ln S_i - \ln S_{i-1}.$$

Log returns are additive: the return over a window is the sum of the returns of its periods, because
the intermediate log prices cancel.

$$\sum_{i=a+1}^{b} r_i = \ln S_b - \ln S_a.$$

The per-period deviation is the sample standard deviation with the Bessel correction, so that it is
an unbiased estimator of the variance of a single period:

$$s^2 = \frac{1}{n-1}\sum_{i=1}^{n}\left(r_i - \bar r\right)^2 .$$

Under the model this layer estimates, the log price is a random walk with independent increments of
equal variance, so variance accumulates linearly in time and deviation accumulates in the square root
of time. The annual figure is therefore

$$\sigma = s\sqrt{m}.$$

The consequences of that square root - that it is a modelling assumption and not an identity, and
that it understates the volatility of a series whose returns are serially correlated - are set out in
[volatility scaling](volatility-scaling.md).

## Worked example

Four daily closes of an instrument observed on consecutive trading days, so $m = 252$:

| $i$ | $S_i$ | $r_i$ |
|---|---|---|
| 0 | 100.00 | - |
| 1 | 102.00 | 0.019803 |
| 2 | 101.00 | -0.009852 |
| 3 | 104.00 | 0.029270 |

The returns sum to $0.039221$, which is exactly $\ln(104.00/100.00)$ - the additivity property. Their
mean is $\bar r = 0.013074$, the sum of squared deviations about that mean is $8.3321\times10^{-4}$,
so $s^2 = 4.1661\times10^{-4}$ and $s = 0.020411$. Annualised, $\sigma = 0.020411\sqrt{252} = 0.3240$,
or about 32% per year.

Three observations is far too few to say anything about an instrument; the example exists so the
arithmetic can be checked by hand.

## Edge cases & pitfalls

- **A series that never moves has no volatility, not an undefined one.** Every return is zero, the
  sample deviation is zero, and the annual figure is zero.
- **A price of zero or below has no logarithm.** Prices are contractual figures that are always
  positive, so a non-positive close is a defect in the data rather than a market event, and it stops
  the run where the series is created.
- **Quotation in whole minor units adds a small upward bias.** A price rounded to the nearest minor
  unit carries an error uniform on half a unit either side, so the log price carries an independent
  error of standard deviation $c/(S\sqrt{12})$, where $c$ is the size of a minor unit. Two such
  errors enter each return, so the measured variance exceeds the true variance by
  $2c^2/(12S^2)$ per period. For an instrument quoted near 100 units of currency in cents and a daily
  volatility of about 1.3%, that inflates the variance by roughly one part in $10^5$ - negligible.
  For an instrument quoted near 5 units of currency the same calculation gives about 2%, which is
  not. **The bias grows as the square of the reciprocal of the price level.**
- **The mean is subtracted, and over short windows that matters.** Removing the sample mean removes
  the drift from the deviation. Over a window of a few observations the mean is itself mostly noise,
  which is why the Bessel correction is applied and why a window of a handful of returns should not
  be trusted.
- **Deviation is not the same as expected absolute movement.** For a normal distribution the mean
  absolute deviation is $\sqrt{2/\pi} \approx 0.798$ times the standard deviation.

## Alternatives considered

**Simple returns.** Rejected: they do not add across periods, so the annual figure would depend on
the sampling frequency, and the projection is a compounding process which is naturally additive in
logarithms.

**Estimators using the daily high, low and open.** The Parkinson, Garman-Klass and Rogers-Satchell
estimators are several times more efficient per observation than the close-to-close deviation.
Rejected for now because the data this project ingests is a close-only series; the close-to-close
estimator is the only one that can be computed from it. It is the estimator to revisit first if
richer data becomes available.

**Not subtracting the mean.** Estimating the variance about zero rather than about the sample mean is
marginally more efficient when the true drift is small, which it is at daily frequency. Rejected for
the batch estimate because the bias it introduces is proportional to the square of the drift and
grows with the length of the window, and because an estimator that is correct for any window is worth
more than one that is slightly better for short ones. The exponentially weighted estimators do
measure about zero, for the reason given in
[exponentially weighted volatility](exponentially-weighted-volatility.md).
