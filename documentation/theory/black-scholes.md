# Black-Scholes-Merton option valuation

## Summary

A European option on a share paying a continuous dividend yield is valued with the
Black-Scholes-Merton formula. This document states the formula, the assumptions it rests on, the
no-arbitrage bounds and parity relation that any correct implementation must satisfy, and the
degenerate cases the model has to answer for.

## Motivation

Every number this project produces downstream is a function of an option value: the premium a wheel
collects, the size of the position that premium justifies, the probability of assignment, and the
after-cost compounding that follows from all three. Getting the valuation wrong does not produce an
obviously broken projection - it produces a plausible one.

The model is chosen for three reasons, in this order:

1. It is **closed-form**, so a Monte-Carlo projection can revalue a book hundreds of millions of
   times without a nested simulation inside each step.
2. It is **invertible**, which is what makes an observed market premium usable as an input
   ([implied volatility](implied-volatility.md)).
3. It comes with **exact invariants** - parity, monotonicity, bounds - so the implementation can be
   tested against properties rather than against numbers copied out of itself.

Its cost is that it is a European model applied to American-style equity options. That gap is
handled deliberately and separately in
[assignment and early exercise](assignment-and-early-exercise.md), not papered over here.

## Definitions & notation

| Symbol | Meaning | Unit |
|---|---|---|
| $S$ | spot price of the underlying, per share | currency |
| $K$ | strike price, per share | currency |
| $\tau$ | year fraction from valuation to expiry | year |
| $r$ | risk-free interest rate, continuously compounded | per year |
| $q$ | dividend yield, continuously compounded, $q \ge 0$ | per year |
| $\sigma$ | annualised volatility of log-returns | per $\sqrt{\text{year}}$ |
| $s = \sigma\sqrt{\tau}$ | standard deviation of the log-return to expiry | dimensionless |
| $F = S e^{(r-q)\tau}$ | forward price of the underlying at expiry | currency |
| $\Phi$ | standard normal cumulative distribution function | dimensionless |
| $c,\ p$ | value of a European call, of a European put, per share | currency |

Every value in this document is **per share**. A contract covers 100 shares; the multiplier is
applied exactly once, where a value becomes a cashflow, and never inside a pricing formula. See
[option contract conventions](option-contract-conventions.md).

## Derivation / Rules

### Assumptions

The formula is the discounted risk-neutral expectation of the payoff under the assumption that the
underlying follows geometric Brownian motion with constant $\sigma$, that $r$ and $q$ are constant
and continuously compounded, that the underlying can be traded continuously without cost, and that
the option can only be exercised at expiry. Real markets violate every one of these; the ones that
matter for a wheel strategy, and what is done about them, are listed under
[edge cases and pitfalls](#edge-cases--pitfalls).

### The formula

$$
d_1 = \frac{\ln(F/K)}{s} + \frac{s}{2},
\qquad
d_2 = d_1 - s
$$

$$
c = S e^{-q\tau}\,\Phi(d_1) - K e^{-r\tau}\,\Phi(d_2)
\qquad
p = K e^{-r\tau}\,\Phi(-d_2) - S e^{-q\tau}\,\Phi(-d_1)
$$

Writing $d_1$ in terms of the forward rather than the spot is algebraically identical to the
textbook form

$$
d_1 = \frac{\ln(S/K) + \left(r - q + \tfrac{\sigma^2}{2}\right)\tau}{\sigma\sqrt{\tau}}
$$

but it separates the two things that move the price: **where the forward sits relative to the
strike**, and **how wide the distribution is**. It also makes the degenerate cases below fall out
without a separate derivation.

The dividend yield enters in exactly one way: it replaces $S$ by $S e^{-q\tau}$ everywhere. A
dividend paid before expiry accrues to the holder of the share, not to the holder of the option, so
the option is written on the *ex-dividend* value of the share. This is Merton's extension of the
original formula, and it is the reason $q$ is required to be non-negative here: a negative dividend
yield would mean the share paid its holder to hold it, which is not a situation this project models,
and it would allow a call delta above 1.

### Interpreting the two probability factors

$\Phi(d_2)$ is the risk-neutral probability that the option finishes in the money. $\Phi(d_1)$ is
that same probability re-weighted by the value of the share in those outcomes; it is also the call's
delta divided by $e^{-q\tau}$. The gap between them, $s$, is precisely the amount of uncertainty
between now and expiry. When $s \to 0$ the two collapse onto each other and the value collapses onto
the discounted intrinsic value of the forward.

### Invariants any correct valuation satisfies

These are the properties this project tests against, because they are true independently of the
implementation.

| Invariant | Statement |
|---|---|
| Put-call parity | $c - p = S e^{-q\tau} - K e^{-r\tau}$, exactly, for every input |
| Lower bound | $c \ge \max(S e^{-q\tau} - K e^{-r\tau},\, 0)$, and symmetrically for the put |
| Upper bound | $c \le S e^{-q\tau}$ and $p \le K e^{-r\tau}$ |
| Monotone in volatility | $\partial c / \partial\sigma > 0$ and $\partial p / \partial\sigma > 0$ for $\tau > 0$ |
| Monotone in spot | $\partial c / \partial S > 0$, $\partial p / \partial S < 0$ |
| Zero volatility | $c = \max(S e^{-q\tau} - K e^{-r\tau},\, 0)$, the discounted intrinsic value of the forward |
| Zero time | $c = \max(S-K,\,0)$, the intrinsic value |

Parity is the strongest of these: it holds *exactly*, not approximately, and it is the invariant
that catches a sign error, a swapped discount factor or a mis-scaled dividend yield in a single
assertion.

## Worked example

Spot 42.00, strike 40.00, six months to expiry, risk-free rate 10% per annum continuously
compounded, no dividend, volatility 20% per annum. This is the standard textbook example and its
published values are 4.76 and 0.81.

| Step | Expression | Value |
|---|---|---|
| Deviation to expiry | $s = 0.20\sqrt{0.5}$ | 0.14142136 |
| Log moneyness | $\ln(42/40)$ | 0.04879016 |
| Carry term | $(r - q + \sigma^2/2)\tau = 0.12 \times 0.5$ | 0.06000000 |
| Standardised moneyness | $d_1 = (0.04879016 + 0.06)/0.14142136$ | 0.76926263 |
| | $d_2 = d_1 - s$ | 0.62784127 |
| In-the-money weights | $\Phi(d_1)$ | 0.77913129 |
| | $\Phi(d_2)$ | 0.73494604 |
| Discount factor | $e^{-0.10 \times 0.5}$ | 0.95122942 |
| Discounted strike | $K e^{-r\tau}$ | 38.04917698 |
| **Call** | $42 \times 0.77913129 - 38.04917698 \times 0.73494604$ | **4.75942239** |
| **Put** | $38.04917698 \times 0.26505396 - 42 \times 0.22086871$ | **0.80859937** |

Parity check: $4.75942239 - 0.80859937 = 3.95082302$, and
$S - K e^{-r\tau} = 42 - 38.04917698 = 3.95082302$. The two agree to the last digit carried, which
is what parity guarantees.

A second example that can be checked without any tables: at the money, with no interest rate and no
dividend, the formula collapses to $c = p = S\left(2\Phi(\sigma\sqrt{\tau}/2) - 1\right)$. For
$S = K = 100$, $\sigma = 20\%$, $\tau = 1$ this is $100(2\Phi(0.1) - 1) = 7.9655674554$.

## Edge cases & pitfalls

**Zero deviation.** When $\sigma = 0$ or $\tau = 0$, $s = 0$ and $\ln(F/K)/s$ is undefined at the
money and infinite elsewhere. The model treats the limit as the definition: the standardised
moneyness is $+\infty$ when the forward is above the strike, $-\infty$ when it is below, and $0$
when they coincide. All three give the discounted intrinsic value of the forward, including the
at-the-money case, where the two half-weights cancel exactly.

**Expiry is not a special case.** At $\tau = 0$ the discount factors are 1 and the value is the
intrinsic value. This falls out of the same limit and needs no separate branch, which matters
because expiry day is the single most-executed path in a wheel projection.

**Constant volatility.** Real option markets price a volatility *smile*: different strikes on the
same expiry imply different $\sigma$. This model has one $\sigma$ per valuation. The project does
not pretend otherwise - it takes the implied volatility of the strike actually being traded, rather
than a single at-the-money number applied to every strike.

**American exercise.** Equity options are American. For a put, and for a call on a share paying no
dividend, early exercise is never optimal and the European value is exact. For a call on a
dividend-paying share it can be optimal, and the European value is then a lower bound. See
[assignment and early exercise](assignment-and-early-exercise.md).

**Continuous dividend yield.** Real dividends are discrete. Modelling them as a continuous yield is
accurate for an index or a broadly diversified fund and approximate for a single share, where the
timing of a single large payment matters. The early-exercise rule is the one place where the
discrete payment is reinstated.

**Precision.** Deep in-the-money values are a difference of two nearly equal large numbers, so their
*relative* accuracy degrades even though their absolute accuracy does not. Nothing downstream should
divide by a deep in-the-money time value; the consequence for inverting the formula is discussed in
[implied volatility](implied-volatility.md).

## Alternatives considered

**Binomial or trinomial trees.** They price American exercise directly instead of approximating it.
Rejected for v1: they are one to three orders of magnitude slower per valuation, which is the whole
budget of a Monte-Carlo projection, and they are not analytically invertible. The cases where early
exercise actually binds for a wheel are narrow enough to be handled by an explicit rule.

**Bjerksund-Stensland or Barone-Adesi-Whaley American approximations.** Closed-form and fast, but
they add a second calibration surface and their error is largest exactly where the wheel operates -
near the money, short dated. Revisit if the early-exercise rule turns out to be the dominant model
error.

**A stochastic-volatility model.** Correct about the smile and wrong about the effort: it needs
calibration data this project does not have, and it would make every downstream result depend on
parameters that cannot be validated from a retail price history.

## References

- Black, F. and Scholes, M. (1973), *The Pricing of Options and Corporate Liabilities*, Journal of
  Political Economy 81(3).
- Merton, R. C. (1973), *Theory of Rational Option Pricing*, Bell Journal of Economics and
  Management Science 4(1) - the continuous dividend yield extension used here.
- Hull, J. C., *Options, Futures, and Other Derivatives* - the worked example above and the
  no-arbitrage bounds.
- Reference prices used as test fixtures are logged in
  [external parameter sources](../validation/sources.md).
