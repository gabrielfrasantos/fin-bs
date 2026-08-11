# Searching for a setting

## Summary

A wheel is run with three numbers: the delta it writes options at, the fraction of free cash it
holds back, and the delta at which it rolls a position away from assignment. This document sets out
how those three are chosen - what is being maximised, how a risk limit enters the quantity being
maximised, and why a search over a simulated strategy has to be conducted differently from a search
over a smooth function.

## Motivation

The obvious procedure is to score each candidate setting by simulating it, then follow the slope of
the score downhill. Each half of that sentence hides a problem.

Simulating each candidate afresh means that the score of a setting is a random variable. With $N$
paths, the standard error of a mean growth rate is of order $\sigma_g/\sqrt{N}$, and for a wheel on
a 25% underlying over three years that is around one percentage point at $N = 64$. The difference in
growth between two neighbouring settings is closer to one *tenth* of a percentage point. Scored
against independent samples, the comparison is noise by a factor of ten, and no amount of careful
searching recovers a signal that was never measured.

Following the slope assumes there is one. A run of the strategy writes a whole number of contracts,
at a strike drawn from a listed grid, and settles amounts in whole minor units. Change the cash
buffer by one basis point and, almost always, nothing whatsoever happens: the same number of
contracts is written at the same strikes and the score is bit-for-bit identical. Change it by enough
and one more contract becomes affordable, and the score jumps. The score is a staircase, and the
derivative of a staircase is zero almost everywhere and undefined at the rest.

Both problems have the same fix applied twice: fix the randomness, and take differences over steps
that are large enough to matter.

## Definitions & notation

| Symbol | Meaning | Units |
|---|---|---|
| $\delta$ | target delta - the option written is the nearest listed strike whose delta magnitude does not exceed it | dimensionless, $0 < \delta < 1$ |
| $b$ | cash buffer - the fraction of free cash held back rather than committed as collateral | basis points, $0 \le b \le 10\,000$ |
| $\rho$ | roll delta limit - a position whose delta magnitude reaches this is closed | dimensionless, $\rho > 0$ |
| $s = (\delta, b, \rho)$ | a setting | |
| $\underline{s}, \overline{s}$ | the lowest and highest setting of the range | |
| $x \in [0,1]^3$ | a setting expressed as a fraction of the range | dimensionless |
| $g(s)$ | mean growth rate, after costs and after tax, over the simulated paths | per year |
| $D(s)$ | deepest drawdown reached on any simulated path | fraction of a peak |
| $A(s)$ | assignments as a fraction of cycles written | dimensionless |
| $D^\star, A^\star$ | the limits placed on those two | |
| $w$ | the charge per unit of excess over a limit | per year, per unit of excess |
| $f(x)$ | the cost being minimised | per year |
| $h$ | the step over which a difference is taken | fraction of the range |
| $\eta$ | the length of a descent step | |
| $\lambda_1, \lambda_2$ | the charges on changing the setting | |

Growth rates are per year and compounded annually; a drawdown and an assignment rate are pure
fractions. Cash amounts never appear in this document: they have all been reduced to the growth rate
before the search sees them.

## Derivation / Rules

### Three levers, not four

A cash-secured wheel writes as many contracts as its uncommitted cash can collateralise:

$$n = \left\lfloor \frac{(1 - b/10\,000)\,C}{K \cdot m} \right\rfloor$$

for free cash $C$, strike $K$ and contract multiplier $m$. Choosing $n$ and choosing $b$ are
therefore the same choice written two ways, and stating both would be stating one answer twice. The
lever kept is the buffer, because it is the one that stays meaningful as the portfolio compounds: a
contract count fixed at the start of a projection stops binding as soon as the capital has grown,
whereas a fraction held back describes the same policy at every size.

The third lever is the roll limit. It is genuinely independent of the other two, and it is the one
that trades premium against assignment directly: rolling early gives up the rest of a premium in
exchange for not taking delivery.

### The unit box

The three levers differ in scale by four orders of magnitude - a delta of $0.3$ against a buffer of
$500$ basis points. A single step length applied to all three would be either useless on one or
catastrophic on another. The search therefore runs on fractions of the stated range,

$$x_i = \frac{s_i - \underline{s}_i}{\overline{s}_i - \underline{s}_i}, \qquad
s_i = \underline{s}_i + x_i\,(\overline{s}_i - \underline{s}_i),$$

with $x$ clamped to $[0,1]$ before a setting is read back from it. Clamping rather than rejecting is
what makes a bound behave like a bound: a step that would leave the range arrives at its edge, the
score stops changing, and the search settles there.

Because the buffer is a whole number of basis points, reading a setting back off the range rounds it
to the nearest. The round trip from a setting to a fraction and back is therefore exact for any
setting that was expressible in the first place.

### Common random numbers

Every candidate is simulated from the same seed. The paths are not merely drawn from the same
distribution; they are the same paths. What is compared is then

$$g(s_2) - g(s_1) = \frac{1}{N}\sum_{j=1}^{N}\Big(g_j(s_2) - g_j(s_1)\Big),$$

a mean of $N$ paired differences rather than a difference of two independent means. The variance of
the paired difference is $\operatorname{Var}(g_j(s_2)) + \operatorname{Var}(g_j(s_1)) -
2\operatorname{Cov}$, and for two nearby settings run on identical price paths the covariance is
very nearly the whole of it. The paired comparison is accurate long before either mean is.

This is also what makes the search reproducible: the score is now a deterministic function of the
setting.

### A difference over a step that changes something

The slope of the cost in each coordinate is taken as a central difference,

$$\frac{\partial f}{\partial x_i} \approx \frac{f(x + h e_i) - f(x - h e_i)}{2h},$$

with $h$ a substantial fraction of the range rather than an infinitesimal - a sixteenth. Anything
much smaller returns exactly zero, because the run on either side of the step is the *same run*. At
a face of the box the step is one-sided and the denominator is the width actually used.

This is a secant of a staircase, not a derivative. It points downhill on average over a stretch of
the range, which is all a descent needs, and it is the reason the search is described as walking
towards a better setting rather than as converging to an optimum.

### A limit as a charge

The quantity minimised is the negative growth rate plus a charge for whatever a limit was passed by:

$$f(x) = -g(s) + w\Big(\max\{0,\ D(s) - D^\star\} + \max\{0,\ A(s) - A^\star\}\Big), \qquad s = s(x).$$

A limit with slack costs nothing, so within the feasible region the search is maximising growth
alone. Passing a limit costs in proportion to how far it was passed, so the search is pushed back to
the boundary rather than off a cliff at it. This matters when no setting can meet the limits: the
search still returns the least bad setting, and reports which limit it is against, instead of
failing to return anything.

The charge has to outweigh what it competes with. Growth differences across the whole range are of
order $10^{-2}$ per year; a drawdown breach is of order $10^{-1}$. A charge of $w = 1$ per unit of
excess therefore makes a ten-point breach cost ten times the entire growth range, which is the
intended ordering: a limit is not for sale.

### A charge on changing the setting

The descent is carried out in the displacement $v = x - x_0$ from the setting already in use, and
that displacement can be charged for:

$$\min_v\ f(x_0 + v) + \lambda_1\lVert v\rVert_1 + \tfrac{\lambda_2}{2}\lVert v\rVert_2^2 .$$

The two charges behave differently, and both are useful. The quadratic charge shrinks every change
in proportion: on a quadratic cost of curvature $a$ per coordinate with its minimum at $x^\star$, the
answer moves to

$$x_i = \frac{a\,x^\star_i + \lambda_2\,x_{0,i}}{a + \lambda_2},$$

a weighted average of where the search wanted to go and where it started. The absolute-value charge
instead moves each coordinate a fixed distance *less* than it otherwise would,

$$x_i = x^\star_i \pm \frac{\lambda_1}{a}, \qquad\text{or}\qquad x_i = x_{0,i}\ \text{ if } a\,\lvert x^\star_i - x_{0,i}\rvert \le \lambda_1,$$

so a change that is not worth at least $\lambda_1$ is not recommended at all. That is the more
useful of the two in practice: a recommendation to alter one number is easier to act on, and easier
to argue with, than a recommendation to alter three by amounts that are within the noise.

With both charges set to nothing, the search is simply the descent on $f$.

## Worked example

Take the range

| Lever | Lowest | Highest |
|---|---|---|
| target delta | $0.10$ | $0.50$ |
| cash buffer | $200$ bp | $4200$ bp |
| roll delta limit | $0.40$ | $0.80$ |

**Reading a setting off the range.** The centre of the box, $x = (0.5, 0.5, 0.5)$, is

| Lever | Arithmetic | Setting |
|---|---|---|
| target delta | $0.10 + 0.5\,(0.50-0.10)$ | $0.30$ |
| cash buffer | $200 + 0.5\,(4200-200)$ | $2200$ bp |
| roll delta limit | $0.40 + 0.5\,(0.80-0.40)$ | $0.60$ |

and the point $(-3, 2, 0.5)$, which no bound admits, is read as $(0.10,\ 4200\ \text{bp},\ 0.60)$.

**Charging for a breach.** With limits $D^\star = 0.30$, $A^\star = 0.50$ and $w = 1$:

| Outcome | Drawdown excess | Assignment excess | Charge |
|---|---|---|---|
| $D = 0.20,\ A = 0.25$ | $-0.10$ | $-0.25$ | $0$ |
| $D = 0.30,\ A = 0.50$ | $0$ | $0$ | $0$ |
| $D = 0.45,\ A = 0.25$ | $+0.15$ | $-0.25$ | $0.15$ |
| $D = 0.20,\ A = 0.75$ | $-0.10$ | $+0.25$ | $0.25$ |
| $D = 0.45,\ A = 0.75$ | $+0.15$ | $+0.25$ | $0.40$ |

The third and fourth rows are both infeasible, and both bind exactly one limit; the second binds
both limits without being infeasible, which is the boundary case worth stating explicitly.

**Charging for a change.** On a quadratic cost of curvature $a = 1$ whose minimum lies at
$x^\star = (0.20,\ 0.80,\ 0.35)$, started from $x_0 = (0.5,\ 0.5,\ 0.5)$:

| Charge | Coordinate 1 | Coordinate 2 | Coordinate 3 |
|---|---|---|---|
| none | $0.200$ | $0.800$ | $0.350$ |
| quadratic, $\lambda_2 = 3$ | $(0.2 + 1.5)/4 = 0.425$ | $(0.8+1.5)/4 = 0.575$ | $(0.35+1.5)/4 = 0.4625$ |
| absolute, $\lambda_1 = 0.1$ | $0.2 + 0.1 = 0.300$ | $0.8 - 0.1 = 0.700$ | $0.35 + 0.1 = 0.450$ |

Every entry in the last two rows lies between where the search started and where it wanted to go,
which is the property both charges are there to provide.

**What the search finds.** For a wheel of six half-year cycles on a 25% underlying, started with
EUR 200,000 and simulated over 64 paths, the answers are:

| Limits | Target delta | Buffer | Roll limit | Growth | Drawdown | Assignments |
|---|---|---|---|---|---|---|
| none in reach | $0.32$ | $200$ bp | $0.78$ | $2.43\%$ | $50.7\%$ | $15.6\%$ |
| drawdown $\le 0.40$ | $0.32$ | $2998$ bp | $0.77$ | $1.74\%$ | $38.4\%$ | $16.4\%$ |
| assignments $\le 0.10$ | $0.32$ | $200$ bp | $0.40$ | $1.68\%$ | $50.3\%$ | $5.5\%$ |

Unconstrained, the search commits everything it is allowed to and accepts a drawdown of half the
portfolio. A drawdown limit is met by holding back cash - the buffer moves from the bottom of the
range to three quarters of the way up it - and costs about seven tenths of a percentage point of
growth per year. An assignment limit is met on a different lever entirely, by rolling earlier, and
costs about the same. Neither limit is met by lowering the target delta, which is the result worth
noticing: writing further out of the money reduces the premium collected faster than it reduces the
risk taken.

## Edge cases & pitfalls

- **A step of nothing measures nothing.** The most likely way to break this search is to reduce the
  difference step in the belief that a smaller step gives a better derivative. It gives exactly zero,
  the descent never leaves its starting point, and the recommendation is silently the setting it was
  handed.
- **The deepest drawdown of a small sample is not the deepest drawdown.** It is the maximum over
  $N$ paths and grows with $N$. A drawdown limit that appears comfortably met at 16 paths can be
  breached at 64 by the same setting, as it is above: the number of paths is part of the statement,
  not an implementation detail.
- **The best setting is usually on a bound.** Growth is monotone in how much cash is committed, so
  with no limit in reach the search runs to the edge of the range. The range is therefore itself a
  risk decision, and a range whose bounds are never reached is the only one that was purely
  descriptive.
- **A fixed number of steps overshoots as easily as it undershoots.** There is no convergence test.
  A step length that crosses the range in a handful of steps will oscillate around the best setting
  and can report a worse one than a shorter step would, which is why the achievement of a
  recommendation is worth comparing with the achievement of the setting the search started from.
- **Growth is compounded, and a drawdown is not.** The two quantities in the cost are not in the same
  units, and the charge per unit of excess is the exchange rate between them. It is a stated
  preference, not a derived quantity, and changing it changes the answer.
- **An assignment is not a loss.** The assignment rate is limited because taking delivery changes
  what the strategy does next, not because the assignment itself destroys value; a wheel that is
  never assigned is not thereby a better wheel.

## Alternatives considered

- **Independent samples per candidate with more paths.** Reaching the accuracy that pairing gives at
  64 paths would need of order a hundred times as many, because pairing removes almost all of the
  variance rather than averaging it down.
- **A smoothed score.** Averaging the score over a small neighbourhood of each setting would give a
  differentiable surface, at the cost of several more simulations per evaluation and of optimising a
  strategy nobody runs.
- **A derivative-free search - pattern search, or a simplex.** A better fit to a staircase than a
  gradient is, and worth revisiting. The difference-based descent was chosen because the step is
  already large enough to behave like a pattern move, and because it reuses machinery that is
  available and tested.
- **A hard feasibility test instead of a charge.** Rejected: it leaves the search with nothing to
  report when every setting breaks a limit, which is exactly when a caller most needs an answer.
- **Maximising terminal wealth rather than the growth rate.** Equivalent here, since every candidate
  starts from the same capital over the same horizon, and the growth rate is the more comparable of
  the two across experiments.

## References

- Glasserman, P., *Monte Carlo Methods in Financial Engineering*, Springer 2004, §4.2 on common
  random numbers as a variance-reduction technique for comparing systems.
- Nocedal, J. and Wright, S., *Numerical Optimization*, 2nd ed., Springer 2006, §8.1 on
  finite-difference derivatives and the choice of step, and §17.1 on penalty methods.
- Tibshirani, R., "Regression Shrinkage and Selection via the Lasso", *Journal of the Royal
  Statistical Society B* 58(1), 1996, for the soft-threshold form of the absolute-value charge.
