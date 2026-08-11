# Implied volatility

## Summary

Implied volatility is the volatility that makes the
[Black-Scholes-Merton value](black-scholes.md) of an option equal to its observed market price. It
is found by inverting the pricing formula numerically. This document states when that inverse
exists, how it is found, and - the part that matters most in practice - how accurately it can be
known at all.

## Motivation

Volatility is the one pricing input that cannot be read off a screen. Historical volatility says
what the underlying *did*; implied volatility says what the market currently *charges*, and it is
the market's price that a wheel is actually paid. Two uses drive the requirement:

1. **Quoting.** A chain snapshot gives premiums, not volatilities. Turning premiums into a
   volatility per strike is what makes strikes comparable and what lets the projection reprice a
   position at a later date.
2. **Consistency.** Pricing an option at an estimated volatility and then valuing it against a
   market premium mixes two different numbers. Inverting the market premium keeps a single number in
   play.

The inversion is easy in the middle and badly behaved at the edges, and the edges are where a
projection meets bad data. The design consequence is that this operation **may have no answer**, and
must say so rather than return one.

## Definitions & notation

Notation follows [Black-Scholes-Merton option valuation](black-scholes.md). Let $V(\sigma)$ be the
model value of the option as a function of volatility, all other inputs held fixed, and let $P$ be
the observed market price, per share, in the same currency. The problem is to find $\sigma^\ast > 0$
with $V(\sigma^\ast) = P$.

## Derivation / Rules

### The inverse exists and is unique when it exists

$V$ is **strictly increasing** in $\sigma$ for $\tau > 0$, because its derivative is vega,

$$
\nu = S e^{-q\tau}\varphi(d_1)\sqrt{\tau} > 0
$$

so the equation $V(\sigma) = P$ has at most one solution. Its range is bounded at both ends:

$$
\lim_{\sigma \to 0^+} V(\sigma) = \underbrace{\max\!\left(S e^{-q\tau} - K e^{-r\tau},\ 0\right)}_{\text{call}}
\qquad
\lim_{\sigma \to \infty} V(\sigma) = \underbrace{S e^{-q\tau}}_{\text{call}},\quad \underbrace{K e^{-r\tau}}_{\text{put}}
$$

so a solution exists **if and only if** the observed price lies strictly between the discounted
intrinsic value of the forward and that upper limit. Those are exactly the no-arbitrage bounds: a
price outside them is not a mispriced option, it is bad data - a stale quote, a crossed market, a
wrong multiplier, a mismatched currency. There is no volatility that explains it, and inventing one
would launder a data error into a projection.

The boundaries themselves are meaningful rather than errors: a price exactly at the lower bound is
explained by zero volatility, and that is the answer returned.

### Finding it

Two properties of $V$ decide the method. It is smooth with a cheaply available derivative, which
favours **Newton's method**; but it is far from linear - close to exponential in $\sigma$ deep out of
the money, and nearly flat deep in the money - which makes unguarded Newton diverge or stall
precisely where the data is worst.

The method used is therefore Newton's method **safeguarded by bisection**:

1. Maintain a bracket $[\sigma_{\text{lo}}, \sigma_{\text{hi}}]$ known to contain the root. It starts
   as the full admissible range and is tightened at every step using the sign of
   $V(\sigma) - P$, which is unambiguous because $V$ is monotone.
2. Start from the Brenner-Subrahmanyam approximation
   $\sigma_0 \approx \sqrt{2\pi/\tau}\,\cdot P / S$, which is exact for an at-the-money forward and
   is a good starting point for anything near the money.
3. Take the Newton step $\sigma_{n+1} = \sigma_n - (V(\sigma_n) - P) / \nu(\sigma_n)$. If it leaves
   the bracket, or vega is too small to divide by, **bisect instead**.
4. Stop when the price residual is within the arithmetic's own resolution, or when the step - or the
   bracket - is narrower than the tolerance.
5. Stop unconditionally after a fixed number of iterations and report no result.

Every step either takes a Newton step that stays inside the bracket or halves the bracket, so the
method inherits bisection's guaranteed convergence and Newton's speed wherever the function is
well behaved. Near the money it typically converges in two or three steps.

### How accurately volatility can be known

This is the part that is easy to get wrong, because it is a property of the *problem*, not of the
solver. Perturbing the recovered volatility by $\delta$ changes the price by $\nu\,\delta$. If the
price is only representable to an absolute accuracy $\eta$, then

$$
\delta_{\min} = \frac{\eta}{\nu}
$$

is the finest volatility difference that is visible in the price at all. Near the money vega is
large and $\delta_{\min}$ is negligible. Deep in or out of the money vega collapses, and the price
becomes a **plateau** in $\sigma$: many volatilities produce the identical price, and no solver can
distinguish them.

The same bound applies, far more coarsely, to real market data. Premiums are quoted in whole ticks,
so $\eta$ is a tick, not an arithmetic rounding. A one-cent tick on an option with a vega of 0.05
per volatility point means the implied volatility is only known to about a fifth of a point - and
that is the good case.

The rule this imposes on everything downstream: **implied volatility from a low-vega quote carries
no information**, and should be weighted or discarded accordingly, never used at face value.

## Worked example

An at-the-money three-month call: $S = K = 100$, $\tau = 0.25$, $r = 2\%$, $q = 0$, observed price
$P = 5.00$.

The starting point is $\sigma_0 = \sqrt{2\pi/0.25} \times 5.00 / 100 = 0.25066283$.

| Step | $\sigma_n$ | $V(\sigma_n)$ | $\nu(\sigma_n)$ | $V - P$ |
|---|---|---|---|---|
| 0 | 0.25066283 | 5.23760546 | 19.842482 | +0.23760546 |
| 1 | 0.23868824 | 4.99998755 | 19.844492 | -0.00001245 |
| 2 | 0.23868887 | 5.00000000 | 19.844492 | 0 |

Two Newton steps. The quadratic convergence is visible: the residual falls from $2\times10^{-1}$ to
$1\times10^{-5}$ to below the resolution of the arithmetic.

Now the ill-conditioned case, same three-month horizon replaced by nine months and the strike moved
deep below the spot: $S = 100$, $K = 20$, $\tau = 0.75$, $\sigma = 30\%$. The call is worth 78.96,
almost all of it intrinsic, and its vega is $5.8\times10^{-8}$ per unit of volatility. The smallest
change in value that double-precision arithmetic can represent at that magnitude is about
$1.4\times10^{-14}$, so volatilities within roughly

$$
\delta_{\min} = \frac{1.4\times10^{-14}}{5.8\times10^{-8}} \approx 2\times10^{-7}
$$

of each other price identically. The solver converges - it just converges to somewhere on a plateau
two ten-millionths of a volatility point wide. Demanding more precision than that of an inversion is
demanding information the price does not contain.

## Edge cases & pitfalls

| Input | Response |
|---|---|
| Price below the discounted intrinsic value | no result; the quote violates no-arbitrage |
| Price exactly at the discounted intrinsic value | zero volatility |
| Price above the upper no-arbitrage limit | no result |
| Zero time to expiry | no result; the value does not depend on volatility, so nothing is implied |
| Price not a finite number | no result |
| Iteration budget exhausted | no result |

"No result" is deliberately a **value**, not an exception and not a sentinel volatility. A quote
that cannot be inverted is a normal event in a chain snapshot - it happens on every illiquid strike -
and the caller has to decide whether to skip the strike or fall back to an estimate. See
[decision D9](../design/decisions.md).

**Do not invert both sides of a pair.** A call and a put on the same strike and expiry imply the
same volatility only if their quotes satisfy put-call parity, which real quotes do not. Inverting
each separately gives two different answers; that difference is a data-quality signal, not a model
result.

**The seed is not the answer.** The Brenner-Subrahmanyam approximation is only good at the money. It
is used to start the search, never as a shortcut in place of it.

## Alternatives considered

**Bisection alone.** Guaranteed and trivially correct, but it needs roughly fifty evaluations for
full precision on every quote. Rejected on cost once a chain snapshot has hundreds of strikes and a
projection reprices them repeatedly.

**Newton alone.** Fast where it works, and it does not work on the deep wings, which is where bad
data lives. Rejected because "usually converges" is not a contract.

**A closed-form rational approximation.** Fast and non-iterative, but its accuracy is fixed at
construction and it cannot report failure - it returns a number for an arbitrage-violating price
just as happily as for a valid one. Rejected for that reason rather than for accuracy.

## References

- Brenner, M. and Subrahmanyam, M. (1988), *A Simple Formula to Compute the Implied Standard
  Deviation*, Financial Analysts Journal 44(5) - the starting approximation.
- Press, W. H. et al., *Numerical Recipes*, on safeguarding Newton's method with bisection.
- Jaeckel, P. (2015), *Let's Be Rational* - on the conditioning of the inversion in the wings.
