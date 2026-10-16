# Option Greeks

## Summary

The Greeks are the first- and second-order sensitivities of an option's value to the things that
move it: the spot price, volatility, the passage of time and the interest rate. This document fixes
their definitions, their **units**, their sign conventions, and the closed forms that follow from
the [Black-Scholes-Merton valuation](black-scholes.md).

## Motivation

A wheel strategy is sized and steered by Greeks rather than by prices:

- **Delta** is how the strategy expresses "how likely am I to be assigned, and how much share
  exposure am I already carrying?".
- **Theta** is the income the strategy exists to harvest. A short option position earns time value;
  the projection's premium yield is a theta integral.
- **Vega** is the dominant risk. Volatility is the least observable input, so the sensitivity to it
  is the honest measure of how uncertain a projection is.
- **Gamma** is why a short option position is not a bond: it measures how quickly a comfortable
  delta becomes an uncomfortable one.

The failure mode this document exists to prevent is a **unit error**. Every Greek is a rate of
change with respect to something, and market convention quietly rescales several of them - vega per
volatility *point*, theta per *day*. A projection that mixes the raw derivative with the quoted
convention is wrong by a factor of 100 or 365 and still looks like a plausible income figure.

## Definitions & notation

Notation follows [Black-Scholes-Merton option valuation](black-scholes.md): $S$, $K$, $\tau$, $r$,
$q$, $\sigma$, $s = \sigma\sqrt{\tau}$, $d_1$, $d_2$, with $\Phi$ the standard normal cumulative
distribution function and $\varphi$ its density. $V$ is the value of the option, per share.

| Greek | Definition | Unit | Convention used here |
|---|---|---|---|
| Delta $\Delta$ | $\partial V / \partial S$ | dimensionless | raw derivative |
| Gamma $\Gamma$ | $\partial^2 V / \partial S^2$ | per unit of currency | raw derivative |
| Vega $\nu$ | $\partial V / \partial \sigma$ | currency per unit of annualised volatility | raw derivative, **not** per volatility point |
| Theta $\Theta$ | $\partial V / \partial t = -\,\partial V / \partial \tau$ | currency per year | raw derivative, **not** per day |
| Rho $\rho$ | $\partial V / \partial r$ | currency per unit of continuously compounded rate | raw derivative |

Two decisions are embedded in that table and are worth stating plainly:

1. **Every Greek is the raw partial derivative in the model's own units.** Per-point vega and
   per-day theta are *presentation*, applied once in a report, never in a calculation.
2. **Theta is the derivative with respect to calendar time, not with respect to time to expiry.**
   The two differ by a sign, and the sign is the entire content of "an option decays". A long option
   without carry has $\Theta < 0$.

All Greeks here are per share, and all are for a **long** position. A short position - which is what
a wheel holds - negates every one of them.

## Derivation / Rules

$$
\Delta_{\text{call}} = e^{-q\tau}\,\Phi(d_1)
\qquad
\Delta_{\text{put}} = -e^{-q\tau}\,\Phi(-d_1) = \Delta_{\text{call}} - e^{-q\tau}
$$

$$
\Gamma = \frac{e^{-q\tau}\,\varphi(d_1)}{S\,\sigma\sqrt{\tau}}
\qquad
\nu = S e^{-q\tau}\,\varphi(d_1)\sqrt{\tau}
$$

$$
\Theta_{\text{call}} = -\frac{S e^{-q\tau}\varphi(d_1)\,\sigma}{2\sqrt{\tau}}
    - r K e^{-r\tau}\Phi(d_2) + q S e^{-q\tau}\Phi(d_1)
$$

$$
\Theta_{\text{put}} = -\frac{S e^{-q\tau}\varphi(d_1)\,\sigma}{2\sqrt{\tau}}
    + r K e^{-r\tau}\Phi(-d_2) - q S e^{-q\tau}\Phi(-d_1)
$$

$$
\rho_{\text{call}} = K \tau e^{-r\tau}\,\Phi(d_2)
\qquad
\rho_{\text{put}} = -K \tau e^{-r\tau}\,\Phi(-d_2)
$$

### Properties that hold for every input

| Property | Statement | Why it matters |
|---|---|---|
| Delta range | $0 < \Delta_{\text{call}} \le e^{-q\tau} \le 1$ and $-1 \le -e^{-q\tau} \le \Delta_{\text{put}} < 0$ | a delta outside $[-1,1]$ is a defect, and this is why $q \ge 0$ is required |
| Delta parity | $\Delta_{\text{call}} - \Delta_{\text{put}} = e^{-q\tau}$ | differentiating put-call parity in $S$ |
| Gamma and vega are type-free | a call and a put on the same contract have identical $\Gamma$ and $\nu$ | differentiating parity twice in $S$, and once in $\sigma$; parity has no $\sigma$ in it |
| Non-negativity | $\Gamma \ge 0$ and $\nu \ge 0$ | option value is convex in $S$ and increasing in $\sigma$ |
| Rho sign | $\rho_{\text{call}} > 0$, $\rho_{\text{put}} < 0$ | a higher rate lowers the present value of the strike |

The first three are *tests*, not remarks: they are exact identities, so an implementation that
violates them is wrong regardless of how close its numbers look.

### Verifying a Greek without trusting the closed form

A closed-form Greek and a hand-derived formula can be wrong in the same way. The independent check
is a **central difference** of the price function itself,

$$
\frac{\partial V}{\partial x} \approx \frac{V(x + h) - V(x - h)}{2h},
\qquad
\frac{\partial^2 V}{\partial x^2} \approx \frac{V(x+h) - 2V(x) + V(x-h)}{h^2}
$$

whose error is $O(h^2)$ from truncation and $O(\epsilon V / h)$ from round-off, where $\epsilon$ is
the relative precision of the arithmetic. The two error terms pull in opposite directions, so $h$
must be chosen, not defaulted: too large and the curvature of $V$ shows up in the answer, too small
and the subtraction loses its significant digits. A bump of a few cents in the spot, and of
$10^{-4}$ to $10^{-6}$ in the dimensionless parameters, keeps both terms far below the accuracy the
model is used at.

One constraint is specific to this project: the spot is a cash amount and therefore lives on a grid
of whole minor units. A finite-difference bump in the spot must be a **whole number of minor units**,
or the bump that is actually applied is not the bump that is divided by.

## Worked example

The same contract as in [Black-Scholes-Merton option valuation](black-scholes.md): spot 42.00,
strike 40.00, $\tau = 0.5$, $r = 10\%$, $q = 0$, $\sigma = 20\%$, so $d_1 = 0.76926263$,
$d_2 = 0.62784127$, $\Phi(d_1) = 0.77913129$, $\Phi(d_2) = 0.73494604$.

| Greek | Call | Put | Reading |
|---|---|---|---|
| Delta | 0.77913129 | -0.22086871 | difference is $e^{-q\tau} = 1$ exactly |
| Gamma | 0.04996267 | 0.04996267 | identical, as required |
| Vega | 8.81341506 | 8.81341506 | per unit of volatility; **0.08813415 per volatility point** |
| Theta | -4.55909219 | -0.75417450 | per year; the call is **-0.01249066 per calendar day** |
| Rho | 13.98204591 | -5.04254258 | per unit of rate; 0.13982 per percentage point |

Reading the call's vega as "0.088" without saying *per point* understates the risk of a
10-point volatility move by a factor of 100. Reading its theta as "-4.56" without saying *per year*
overstates one day's decay by a factor of 365. Both are the same mistake.

A finite-difference sanity check on the call's delta, bumping the spot by five cents either way:
$(V(42.05) - V(41.95)) / 0.10 = 0.77912810$, against the closed form's $0.77913129$ - agreeing to
five decimal places, with the residual being the expected $O(h^2)$ truncation.

## Edge cases & pitfalls

**At expiry.** With $\tau = 0$ the density term vanishes and gamma, vega and rho are all zero, while
delta becomes a step at the strike. The decay term in theta is a division by $\sqrt{\tau}$ and is
genuinely unbounded as expiry approaches; the model reports zero decay exactly at expiry rather than
an infinity, because there is no time left to decay over.

**Zero volatility.** Same situation: gamma is a delta function at the strike in the limit, not a
number. The model reports zero.

**Gamma near expiry.** Just before expiry an at-the-money gamma is enormous, and it is real, not an
artefact. Any sizing rule that treats gamma as bounded will be wrong in exactly the region a wheel
spends most of its time in.

**Vega and the smile.** These are sensitivities to *the* volatility used in the valuation. Because
real markets have a smile, a book's true sensitivity to a market-wide volatility move is not the sum
of the per-strike vegas. Aggregating vega across strikes is an approximation and should be labelled
as one.

**Sign of a short position.** A wheel is short options. Short theta is positive - that is the
income. Short gamma is negative - that is the risk. Nothing in the pricing layer knows about
position direction; the negation belongs to the position, not to the Greek.

## References

- Hull, J. C., *Options, Futures, and Other Derivatives*, the chapter on the Greek letters.
- Haug, E. G., *The Complete Guide to Option Pricing Formulas*, for the generalised
  Black-Scholes-Merton sensitivities with a dividend yield.
