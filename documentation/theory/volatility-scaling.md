# Volatility scaling

## Summary

Volatility here is always the **annualised standard deviation of log-returns**, $\sigma$, and it is
scaled to any other horizon with the square-root-of-time rule $\sigma\sqrt{\tau}$. This document
derives that rule, states the assumptions that make it exact, and lists the situations in which it
is known to be wrong.

## Motivation

$\sigma$ is the input to a wheel projection that is both the most influential and the least
observable. Premium income is roughly proportional to it; assignment probability is driven by it;
and it is the parameter every estimator in this project exists to recover. Two conventions must be
nailed down before any of that means anything:

1. **Per what?** A "20% volatility" is 20% *per year*. Quoting a per-day or per-period figure
   without saying so is the most common unit error in options work.
2. **How does it move between horizons?** Option prices depend on $\sigma\sqrt{\tau}$, never on
   $\sigma$ alone, so the scaling rule is baked into every formula downstream.

## Definitions & notation

| Symbol | Meaning | Unit |
|---|---|---|
| $S_t$ | spot price | currency |
| $R_i = \ln(S_i / S_{i-1})$ | log-return over one sampling period | dimensionless |
| $\sigma$ | annualised volatility | per $\sqrt{\text{year}}$ |
| $\sigma_p$ | volatility over one sampling period | dimensionless |
| $N$ | sampling periods per year (252 trading days, 12 months, ...) | 1/year |
| $\tau$ | year fraction | year |

Volatility is non-negative by construction. It is a standard deviation, not a variance, and not a
percentage of a percentage.

## Derivation / Rules

### Where $\sqrt{\tau}$ comes from

Under geometric Brownian motion,

$$
dS_t = \mu S_t\, dt + \sigma S_t\, dW_t
\quad\Longrightarrow\quad
\ln \frac{S_T}{S_0} \sim \mathcal{N}\!\left(\left(\mu - \tfrac{\sigma^2}{2}\right)T,\ \sigma^2 T\right)
$$

The variance of the log-return is **linear in time**, $\operatorname{Var} = \sigma^2 T$, so the
standard deviation grows as $\sqrt{T}$. The reason is more general than geometric Brownian motion:
if returns over disjoint intervals are independent and identically distributed with finite variance,
variances add,

$$
\operatorname{Var}\!\left(\sum_{i=1}^{n} R_i\right) = \sum_{i=1}^{n}\operatorname{Var}(R_i) = n\sigma_p^2
$$

and taking the square root gives $\sigma_p\sqrt{n}$. The cross terms vanish *only* because the
covariances are zero - that is the entire content of the assumption, and it is exactly what fails
when returns are autocorrelated.

Working in **log**-returns rather than simple returns is what makes the sum telescope:
$\sum_i \ln(S_i/S_{i-1}) = \ln(S_n/S_0)$. Simple returns compound multiplicatively and do not add,
so the same argument does not go through for them.

### Annualisation and de-annualisation

$$
\sigma = \sigma_p \sqrt{N}, \qquad \sigma_p = \frac{\sigma}{\sqrt{N}}, \qquad
\sigma_\tau = \sigma\sqrt{\tau}
$$

With $N = 252$ trading days, a daily $\sigma_p$ of $1\%$ is an annualised $\sigma$ of
$0.01\sqrt{252} = 15.87\%$. The choice of $N$ must agree with the day-count convention used for
$\tau$ - see [day-count conventions](day-count-conventions.md).

### Variance is the natural domain for addition

Because variances add and standard deviations do not, anything that combines volatilities does so in
variance space:

$$
\sigma_{\text{total}} = \sqrt{\sigma_1^2 + \sigma_2^2}
$$

Both the deviation and the variance are therefore first-class ways to state a volatility, so an
estimator that naturally produces a variance never has to round-trip through a square root the
caller might forget.

### Why volatility is its own concept

$\sigma$ is dimensionally *not* a rate: it has units of $\text{year}^{-1/2}$, so adding it to an
interest rate is meaningless even though both are small numbers near 0.2. Treating it as a distinct
quantity makes that addition inexpressible and makes the scaling $\sigma\sqrt{\tau}$ an explicit,
named step.

## Worked example

Daily closes give a sample standard deviation of log-returns of $\sigma_p = 0.0125$.

| Quantity | Computation | Value |
|---|---|---|
| Annualised $\sigma$ | $0.0125\sqrt{252}$ | 0.198431 |
| Annual variance | $\sigma^2$ | 0.039375 |
| 30-day $\sigma_\tau$ (ACT/365F) | $0.198431\sqrt{30/365}$ | 0.056888 |
| 30-day one-standard-deviation move on $S = 100$ | $100(e^{\pm\sigma_\tau} - 1)$ | +5.85 / -5.53 |

The asymmetry in the last row is not a rounding artefact: a log-normal price has a bigger up-move
than down-move for the same log-return magnitude, which is why an evenly spaced strike ladder is
*not* evenly spaced in probability.

## Edge cases & pitfalls

- **Negative volatility does not exist.** It is rejected on the spot; an estimator that produces a
  negative variance has a defect, and clamping it silently would hide that.
- **Zero volatility is legal** and useful as a degenerate test case: option value collapses to
  discounted intrinsic, and a simulated path becomes deterministic drift.
- **Square-root-of-time fails when returns are autocorrelated.** Real equity returns show volatility
  clustering and mean reversion at long horizons, both of which break the zero-covariance
  assumption. Empirically, scaling a daily $\sigma$ to a one-year horizon *over-estimates* realised
  annual volatility for indices. Monte-Carlo simulation is the honest answer; the closed-form
  scaling is the tractable approximation.
- **Realised is not implied.** Volatility estimated from history is a backward-looking statistic; the
  volatility that reproduces a market option price is a forward-looking, risk-neutral quantity, and
  it is systematically higher - the variance risk premium, which is incidentally the edge a
  premium-selling strategy harvests. Both are volatilities; do not mix them without saying which is
  which.
- **The annualisation factor must match the sampling.** Weekly data annualised with $N = 252$ is a
  factor $\sqrt{5}$ error, that is, a volatility 2.2 times too large.
- **The mean is usually not subtracted.** Over daily horizons the drift contributes $O(\Delta t)$ to
  the mean and $O(\sqrt{\Delta t})$ to the standard deviation, so the sample mean is noise;
  estimators state which convention they use.

## Alternatives considered

| Option | Verdict |
|---|---|
| A bare decimal for volatility | Rejected: unit confusion with rates and with per-period figures |
| Hold the variance instead of the deviation | Rejected: humans and configuration files speak in $\sigma$; the variance stays available as a derived value |
| Per-period volatility as the canonical form | Rejected: it would embed the sampling frequency in the value, so two volatilities would not be comparable |
| A term structure of volatility | Deferred: the first version uses a single flat $\sigma$; a surface belongs with the options-chain data |

## Verification

Checked: the deviation-to-variance round trip; the annualisation factor; the square-root-of-time
scaling against the hand-computed figure above; the additivity of variance over consecutive
horizons; and the refusal of negative inputs.

## References

- J. Hull, *Options, Futures, and Other Derivatives*, chapter 15 - volatility estimation and the
  $\sqrt{T}$ rule.
- R. F. Engle, *Autoregressive Conditional Heteroskedasticity*, Econometrica 50(4), 1982 - why the
  independence assumption fails in practice.
- P. Carr and L. Wu, *Variance Risk Premiums*, Review of Financial Studies 22(3), 2009.
