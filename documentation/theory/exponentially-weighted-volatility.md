# Exponentially weighted volatility

## Summary

Volatility is not constant: it clusters, and a figure computed from ten years of history describes
none of those ten years. This document defines the two weighted estimators this project uses - a
fixed exponential weighting and a recursive least-squares fit with a forgetting factor - shows that
they are the same estimator with different startup behaviour, and gives the exact relationship
between the weight parameter, the half-life and the effective sample size.

## Motivation

The equally weighted estimator in [log returns and realised volatility](log-returns-and-volatility.md)
answers the question *how much did this instrument move, on average, over the whole window*. A
strategy that sells options next week needs a different question answered: *how much is it moving
now*. Those differ whenever volatility clusters, which is to say always.

The obvious fix - shorten the window - trades one problem for another. A twenty-day window is
responsive and noisy: with twenty observations the estimate of the deviation carries a standard error
of about 16% of itself, so a third of the movement in the estimate is the estimator, not the market.
Worse, an extreme return enters the window with full weight, sits there for twenty days, and then
falls out of it in one step, producing a cliff in the estimate on a day when nothing happened.
Exponential weighting removes the cliff: an observation's influence decays smoothly and never
terminates.

## Definitions & notation

| Symbol | Meaning | Unit |
|---|---|---|
| $r_i$ | log return of period $i$ | per period |
| $v_i$ | estimated variance of a period, after observing $r_i$ | per period |
| $\alpha$ | smoothing factor | dimensionless, in $(0, 1]$ |
| $\lambda$ | forgetting factor | dimensionless, in $(0, 1]$ |
| $h$ | half-life, in observation periods | periods |
| $m$ | observation frequency | per year |

## Derivation / Rules

### Exponentially weighted variance

$$v_i = v_{i-1} + \alpha\left(r_i^2 - v_{i-1}\right), \qquad v_0 = r_0^2 .$$

Expanding the recursion shows what the estimate is: a weighted mean of past squared returns whose
weights decay geometrically,

$$v_i = \alpha\sum_{k=0}^{i-1}(1-\alpha)^k r_{i-k}^2 + (1-\alpha)^i r_0^2 ,$$

with weights that sum to one, so the estimator is unbiased for a constant variance. The annual figure
is $\sigma_i = \sqrt{v_i\,m}$, exactly as for the equally weighted estimator.

The estimate measures the variance **about zero rather than about a mean**. At the frequencies this
layer is used at, the mean return over a period is smaller than its deviation by a factor of the
order of the square root of the number of periods in a year, so the squared mean is smaller than the
variance by a factor of the order of that number - a fraction of a percent at daily frequency.
Subtracting a mean that must itself be tracked would cost more than it removes.

### The weight parameter, stated three ways

The smoothing factor is not an interpretable number, so it is derived from one that is. An
observation's weight halves after $h$ periods when

$$1 - \alpha = 2^{-1/h}, \qquad \text{equivalently} \qquad \alpha = 1 - 2^{-1/h}.$$

The estimator's effective sample size - the number of equally weighted observations that would give
the same estimation variance - is

$$n_{\text{eff}} = \frac{2-\alpha}{\alpha},$$

and the relative standard error of the variance estimate is $\sqrt{\alpha/(2-\alpha)}$, which is half
that on the deviation.

| Half-life $h$ | $\alpha$ | $n_{\text{eff}}$ | relative error of $\sigma$ |
|---|---|---|---|
| 10 | 0.0670 | 28.9 | 9.3% |
| 20 | 0.0341 | 57.7 | 6.6% |
| 60 | 0.0115 | 173.1 | 3.8% |
| 250 | 0.00277 | 721.3 | 1.9% |

A step change in the true variance is not tracked instantly but decays towards the new level
geometrically: after $k$ periods the remaining gap is $(1-\alpha)^k$ of the original, so one half-life
closes half the gap, two half-lives three quarters, and so on. **Responsiveness and precision are the
same dial turned in opposite directions**; there is no setting that gives both.

### Recursive least squares with a forgetting factor

The second estimator fits, recursively, the single constant that best explains the squared returns,
discounting each past observation by $\lambda$ per period. Writing $P$ for the estimator's own measure
of how uncertain that constant is, the update is

$$g_i = \frac{P_{i-1}}{\lambda + P_{i-1}}, \qquad v_i = v_{i-1} + g_i\left(r_i^2 - v_{i-1}\right), \qquad P_i = \frac{P_{i-1}}{\lambda + P_{i-1}} .$$

Two limits make its behaviour clear.

- **No forgetting, $\lambda = 1$.** Started from an uninformative prior, $P_i = 1/i$ and $g_i = 1/i$,
  so the estimate is exactly the running mean of the squared returns. It coincides with the equally
  weighted estimator, up to the Bessel correction and the subtraction of the mean.
- **Forgetting, $\lambda < 1$.** The uncertainty settles at $P \to 1-\lambda$, so the gain settles at
  $g \to 1-\lambda$ and the update becomes the exponentially weighted recursion with
  $\alpha = 1 - \lambda$.

So the two estimators agree in the long run, and differ only in how they start: the exponentially
weighted estimator applies its full weighting from the first observation, while the recursive fit
begins as a plain average and tightens into the exponential weighting as its own uncertainty falls.
On a short window - the first few dozen observations - the recursive fit is the better behaved of the
two, because it does not pretend to a precision it has not yet earned.

## Worked example

Suppose the daily return magnitude has been a steady 1% for a long time and then steps permanently to
2%. With a half-life of 20 days, $\alpha = 1 - 2^{-1/20} = 0.034064$, and the variance estimate
converges from $10^{-4}$ towards $4\times10^{-4}$:

| Days after the step | remaining gap $(1-\alpha)^k$ | $v_k$ | $\sigma_k$ at 252 periods per year |
|---|---|---|---|
| 0 | 1.0000 | $1.0000\times10^{-4}$ | 15.87% |
| 10 | 0.7071 | $1.8787\times10^{-4}$ | 21.76% |
| 20 | 0.5000 | $2.5000\times10^{-4}$ | 25.10% |
| 40 | 0.2500 | $3.2500\times10^{-4}$ | 28.62% |
| 100 | 0.0313 | $3.9062\times10^{-4}$ | 31.37% |

Reading the third row: after exactly one half-life the estimate has closed exactly half the gap in
*variance*, which is less than half the gap in volatility, because the square root compresses the
upper half of the range.

## Edge cases & pitfalls

- **A smoothing factor of one keeps nothing.** The estimate is then the latest squared return alone,
  which is a legitimate but useless setting: a single squared return is an unbiased estimate of the
  variance with a relative standard error of 100%.
- **The first estimate is one observation old.** Both estimators start from the first squared return,
  so the first few values of a track are the first few returns and nothing more. A track should be
  discarded for the first few half-lives before it is read.
- **Halving the gap in variance is not halving the gap in volatility.** Every statement about
  half-lives above is a statement about variance. Reporting the responsiveness of the estimator in
  terms of volatility overstates how quickly it reacts to a fall and understates how quickly it
  reacts to a rise.
- **Neither estimator has a mean to revert to.** They are weighted averages, not models of volatility,
  and so they will happily extrapolate the current level forever. A model with mean reversion would
  predict a return towards a long-run level; these do not, and a projection that runs for years on a
  volatility measured over the last few weeks inherits that limitation.
- **The forgetting factor is per observation period, not per year.** A factor of 0.99 discards half
  the weight in 69 daily observations but in 69 monthly observations too.

## Alternatives considered

**A rolling fixed-length window.** Rejected for the cliff described in the motivation: an extreme
observation leaves the window all at once, moving the estimate on a day when nothing happened.

**A generalised autoregressive conditional heteroskedasticity model.** The standard fitted model of
clustering volatility, and the natural successor to exponential weighting - which is the special case
of it with no mean reversion and no fitted parameters. Rejected for now because it must be fitted by
numerical likelihood maximisation, has parameters that are badly determined from a single instrument's
history, and would make the projection's dependence on the estimation window harder rather than easier
to explain. It is the first thing to reach for if the projection turns out to be sensitive to the
volatility path rather than to its level.

**Estimating volatility from option prices instead of from history.** The implied volatility of a
traded option is the market's forecast, and it is a better one. It is already available in this
project, from inverting a quoted price. The two are complementary rather than competing: the implied
figure prices the options the strategy sells, while the historical figure describes the paths the
underlying takes, and using the implied figure for both would assume the market's forecast is correct.
