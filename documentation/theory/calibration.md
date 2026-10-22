# Calibration from a price history

## Summary

A calibration rule turns the closes before a date into the drift, the volatility and the implied
volatility a wheel is run with from that date. It takes the last $n$ closes dated strictly before
the date, estimates the volatility either from every return alike or with exponentially decaying
weights, estimates the drift either by a least-squares line through the log closes or keeps the
stated figure, and prices the options at the realised volatility plus a stated premium. In a
backtest the rule is applied at the opening of every cycle, so each cycle is run on what was
knowable at its opening and nothing later. The estimates are reported beside the result.

## Motivation

A scenario that states one drift and one volatility per name runs the next thirty years on a
judgement made once. A market does not hold still, and a projection that is to be compared with
what a year actually did has to be allowed to see only what was known as the year went by. The
rule here is deliberately simple and deliberately stated: a window, an estimator, a premium. It is
not a forecast. It is the discipline of writing down what the model may look at before it chooses
a strike.

## Definitions & notation

| Symbol | Meaning |
|---|---|
| $S_1, \dots, S_n$ | the last $n$ closes dated strictly before the date $t_0$ the rule serves |
| $r_i = \ln(S_{i+1} / S_i)$ | the $n - 1$ log returns of the window |
| $p$ | observations per year, the window's count of returns over its span in trading years |
| $\hat\sigma$ | the realised annual volatility of the window |
| $h$ | the half-life of the weighted estimator, in closes |
| $\lambda = 1 - 2^{-1/h}$ | its smoothing factor: a return $h$ closes back weighs half as much as the latest |
| $\hat\mu_{\log}$ | the slope of a least-squares line through $\ln S_i$ against time |
| $\hat\mu = \hat\mu_{\log} + \tfrac{1}{2}\hat\sigma^2$ | the drift of the price, which is what a lognormal path is drawn with |
| $\pi$ | the implied premium, in basis points of annual volatility |
| $\hat\sigma_{\text{impl}} = \hat\sigma + \pi / 10\,000$ | the volatility the options are priced at |

## Derivation / Rules

1. **The window.** Closes are taken from the history in date order; the window is the last $n$
   closes whose date is before $t_0$, not on it. A close on $t_0$ is the day's own result and is
   not known at its opening. A history with fewer than $n$ closes before $t_0$ is refused, naming
   how many it holds.
2. **The volatility.** With every return alike, $\hat\sigma$ is the sample standard deviation of
   the $r_i$ scaled by $\sqrt{p}$ ([log returns and volatility](log-returns-and-volatility.md)).
   Exponentially weighted, it is the square root of the recursion
   $v_i = (1 - \lambda)\, v_{i-1} + \lambda\, r_i^2$ run over the window, scaled the same way
   ([exponentially weighted volatility](exponentially-weighted-volatility.md)). Either way the
   figure is annual and dimensionless, and $p$ comes from the window's own dates under the day count
   the history was read with, so a file of trading days annualises by about 252 and a file of
   calendar days by about 365.
3. **The drift.** Least squares fits $\ln S_i = a + \hat\mu_{\log}\, t_i$ over the window
   ([drift estimation](drift-estimation.md)) and the price drift adds half the variance of the
   volatility estimated in step 2, so the two are consistent with each other. *Stated* keeps the
   underlying's own drift and estimates nothing.
4. **The implied volatility.** $\hat\sigma_{\text{impl}} = \hat\sigma + \pi / 10\,000$; the premium
   may be negative. A result at or below zero is refused: an option priced at no volatility is
   worth nothing and the roll rules would never fire.
5. **What is replaced.** Only the drift, the volatility and the implied volatility. The spot, the
   dividend yield, the withholding rate and the strike grid stay as the scenario states them.
6. **When.** In a backtest, at the opening of every cycle, each name from its own history. In a
   forward run, once at the start ([decision D56](../design/decisions.md)).

## Worked example

Shell on 2025-10-20, the first opening of the published backtest, with a window of 252 closes,
exponential weighting with a half-life of 60 closes, a least-squares drift (shown to illustrate the
line; the published scenarios keep the stated drift) and a premium of 200 basis points. The
window is the 252 closes from 2024-10-21 to 2025-10-17; the close of 2025-10-20 itself is
excluded. The smoothing factor is $\lambda = 1 - 2^{-1/60} = 0.01148$. The rule reports a
volatility of 18.82%, a drift of 0.57% and an implied volatility of 20.82%, against the 22%, 5% and
24% the scenario states. Six months later, at the opening of 2026-04-20, the same rule over the 252
closes to 2026-04-17 reports 24.38%, 25.35% and 26.38%: the share had risen by a fifth over the
window and the weighted estimator had seen the busier spring.

## Edge cases & pitfalls

- **The drift is the loud figure.** Over a window of one year the standard error of an estimated
  drift is of the order of the volatility itself, so a name with a realised volatility of 25%
  reports a drift anywhere within some 25 percentage points of the truth, and the line over the
  BENELUX histories gave drifts from -18% to +73% in the same cycle. The volatility, by contrast,
  is known to within about $\hat\sigma / \sqrt{2(n-1)}$, a twentieth of itself for a year of daily closes.
  Use the *stated* drift when the forward figure matters more than the discipline.
- **A weighted estimator sees fewer closes than the window holds.** With $h = 60$ the effective
  count is about $2/\lambda - 1 \approx 173$ of the 252, and the estimate moves within a season.
- **The window is in closes, not days.** A history with gaps (a thinly traded line, a holiday
  calendar) reaches further back for the same $n$; the reported window end says how far.
- **Carry-forward in a backtest.** A schedule date on which the market was shut uses the last
  close before it, so an expiry on a Sunday settles at Friday's close.
- **Unadjusted closes.** A split or a return of capital shows as a jump the rule reads as
  volatility and the replay reads as a price move. The data tool fetches closes as printed; a file
  spanning such an event has to be adjusted by hand or cut.

## Alternatives considered

- **Re-estimating inside a simulated future.** Rejected: a simulated path is drawn from the terms,
  and re-estimating from it recovers them with noise ([decision D56](../design/decisions.md)).
- **A window in calendar days.** Rejected: it would mean different counts of closes on different
  calendars and in gappy files, and the count is what the error bound depends on.
- **Moving the levers with the market.** Deliberately not yet: a rule that widens the delta or the
  buffer when the volatility rises is a strategy change, to be stated as its own decision.
- **An implied volatility from quotes.** The right source, and not available offline; the premium
  is the stated stand-in until a chain snapshot is ([decision D52](../design/decisions.md)).

## References

- [Log returns and realised volatility](log-returns-and-volatility.md),
  [drift estimation](drift-estimation.md),
  [exponentially weighted volatility](exponentially-weighted-volatility.md).
- Decisions [D55 and D56](../design/decisions.md).
- Research note: [BENELUX backtest](../research/benelux-backtest.md).
