# BENELUX backtest

*Research note dated 2026-10-20. Not financial advice: one year of one book replayed over closes
from Euronext, with every limitation of the model behind it.*

## Summary

The ten BENELUX names were run through the wheel from 2025-10-20 to 2026-10-19, two cycles of 182
days, with each name's drift, volatility and implied volatility re-estimated at the opening of each
cycle from the 252 closes before it (exponentially weighted volatility, half-life 60 closes,
least-squares drift, a 200 basis point premium for pricing). The book went from EUR 200,000.00 to
EUR 197,538.60 at the first expiry and EUR 205,477.60 at the second: +2.7% over the year, after
fees and a wealth-tax charge, in a year in which four of the names rose by more than 40% and
Prosus fell by 39%. A simulation from the same start, on the same calibrated terms and the same
spots, put the median a year out at EUR 210,438.10 (+5.2%) with a 5th percentile of
EUR 197,642.35 and a 95th of EUR 218,113.10, so the realised year sits in the lower third of what
the model expected. No name was assigned: every put that went into the money was rolled under the
delta trigger, at a cost (Prosus alone paid EUR 7,405.00 net in rolls). Two names never wrote a
contract, because a 15% cap per name over ten names fills the pot after the first six or seven.

The loudest figure is the calibrated drift, which ranged from -18% to +73% a year across names in
the same cycle; it is estimated from one year of closes and its error is of the order of the
volatility. The volatilities themselves came out below the figures stated in the scenario for
seven of the ten names in the first cycle and above them for most in the second.

## Motivation

The owner asked for the system to be run on the ten names and to calibrate itself to the market as
it goes. The candidate note fixed the names and stated one drift and one volatility per name from
desk estimates; this note replaces those with what the closes say, cycle by cycle, and compares the
year that happened with the spread of years the model expected at the same start.

## Definitions & notation

| Term | Meaning here |
|---|---|
| Backtest | the book's rules run over the closes as they were, in schedule order, with no sight of later closes |
| Window | the 252 closes dated strictly before a cycle's opening |
| Realised volatility | exponentially weighted, half-life 60 closes, annualised by the window's own trading days |
| Drift | least-squares line through the log closes of the window, plus half the variance |
| Implied | realised plus 200 basis points, the volatility the puts and calls were priced at |
| Cap | at most 15% of the book's opening equity reserved per name ([decision D54](../design/decisions.md)) |

The scenario is `config/scenarios/benelux-backtest.json`: capital EUR 200,000, cash buffer 5%,
target delta 30%, roll at delta 50% or 7 days before expiry or at 80% of the premium earned, 2%
risk-free rate, Box 3 assessed on 1 January, DEGIRO fees, and spots stated as the closes of
2025-10-20 so that a simulation from the same scenario opens where the backtest does.

## Method

- **Data.** `data/prices/<TICKER>.csv`, unadjusted daily closes fetched from Euronext on 2026-10-20
  (2024-10-21 to 2026-10-19, 509 closes each; Unilever under its ISIN since December 2025, Air
  France-KLM from the Paris line). The year before the backtest is the first window.
- **Replay.** Each name's path is its close on every scheduled date: the two openings, the two
  observation dates 14 days before expiry, the two expiries, and 1 January 2026 for the assessment
  (a Thursday; the close used is that of 31 December). Strikes, premiums and the roll decisions
  are the model's, priced at the implied volatility; no option quote was used.
- **Re-estimation.** The rule of [decision D55](../design/decisions.md) at each opening. Nothing
  after an opening reaches the cycle that starts there.
- **Comparison.** A projection and a 256-path simulation of the same scenario from the same start,
  which take their terms from the rule once at 2025-10-20 ([decision D56](../design/decisions.md)).

Everything else is as the model stands: dividends accrued at the stated yields, no bid-ask spread,
assignment only at expiry, every name independent in the simulation.

## Results

**The book.**

| Boundary | Equity | Change |
|---|---|---|
| 2025-10-20 | 200,000.00 | |
| 2026-04-20 | 197,538.60 | -1.23% |
| 2026-10-19 | 205,477.60 | +4.02% (+2.74% over the year) |

The Box 3 charge struck for 2026-01-01 was EUR 701.90: the book was entirely in cash at the turn
of the year, which the model classes as a deposit (1.37% deemed return, taxed at 36% above the
EUR 57,684 allowance). No dividend was received, since no share was ever held, and nothing was
borrowed. Option fees came to EUR 145.50 in the first cycle and EUR 141.00 in the second.

**Per name.** Contracts written at each opening and the premium received net of rolls and the
option fees of the cycle; the price path is the close at each opening and at the last expiry.

| Name | Cycle 1 contracts | Cycle 1 premium | Cycle 2 contracts | Cycle 2 premium | Close 2025-10-20 → 2026-04-20 → 2026-10-19 |
|---|---|---|---|---|---|
| SHELL | 0 | 0.00 | 1 | 127.00 | 31.27 → 37.60 → 46.86 (+50%) |
| INGA | 15 | 1,095.00 | 13 | 1,547.00 | 20.60 → 24.50 → 29.50 (+43%) |
| MT | 9 | 1,602.00 | 6 | 2,118.00 | 33.35 → 52.90 → 56.52 (+70%) |
| AD | 0 | 0.00 | 0 | 0.00 | 36.74 → 41.30 → 32.61 (-11%) |
| PHIA | 13 | 1,092.00 | 13 | -39.00 | 24.45 → 24.82 → 22.91 (-6%) |
| UNA | 2 | -124.00 | 2 | 344.00 | 53.98 → 49.31 → 58.23 (+8%) |
| ABN | 13 | 1,001.00 | 10 | 1,140.00 | 24.83 → 30.10 → 40.30 (+62%) |
| PRX | 5 | -5,760.00 | 7 | -1,645.00 | 58.63 → 43.99 → 35.70 (-39%) |
| AF | 28 | -1,960.00 | 32 | 3,008.00 | 11.72 → 10.26 → 10.34 (-12%) |
| AALB | 12 | 1,440.00 | 10 | 1,480.00 | 27.46 → 31.66 → 41.18 (+50%) |

A negative premium is a cycle in which the put was rolled down at a net debit larger than the
premium first received. Every name ended both cycles in cash: no put was assigned, because the
delta trigger fired first on each of the names that fell. Shell and Ahold Delhaize wrote nothing in
the first cycle and Ahold nothing in the second: collateral is offered richest premium first
([decision D43](../design/decisions.md)), the first six or seven names each took their 15% cap, and
the pot (95% of equity after the buffer) was gone before the two lowest-volatility names were
reached.

**The estimates the rule used.** Realised volatility, drift and implied, against the scenario's
stated volatility (realised, before the premium).

| Name | Stated vol | Cycle 1 vol | Cycle 1 drift | Cycle 2 vol | Cycle 2 drift |
|---|---|---|---|---|---|
| SHELL | 22% | 18.82% | 0.57% | 24.38% | 25.35% |
| INGA | 28% | 22.10% | 42.91% | 31.31% | 37.70% |
| MT | 38% | 34.68% | 36.29% | 45.27% | 86.12% |
| AD | 20% | 15.33% | 11.52% | 21.42% | 11.59% |
| PHIA | 30% | 25.85% | -11.33% | 28.52% | 23.95% |
| UNA | 18% | 18.78% | -5.66% | 26.49% | 6.34% |
| ABN | 27% | 25.21% | 72.99% | 26.72% | 39.69% |
| PRX | 32% | 26.44% | 49.09% | 33.61% | 1.60% |
| AF | 42% | 43.18% | 62.33% | 55.23% | 27.82% |
| AALB | 30% | 33.39% | -17.72% | 33.12% | 10.78% |

The windows end on 2025-10-17 and 2026-04-17, the last closes before each opening; the implied
volatility is each realised figure plus two points.

**What the model expected at the same start.** The simulation of the same scenario, 256 paths,
seed 20261019, terms calibrated once at 2025-10-20:

| Measure | Value |
|---|---|
| Equity at the 5th percentile | 197,642.35 |
| Equity at the median | 210,438.10 |
| Equity at the 95th percentile | 218,113.10 |
| Mean growth a year | 4.64% |
| Worst drawdown | 9.79% |
| Assignments per option written | 1.50% |
| The one projection on the calibrated drifts | 201,445.85 then 214,998.57 |

The realised 205,477.60 is at about the 25th percentile of the simulated year. The projection,
which walks each name along its calibrated drift with no noise, ends higher than the backtest
because drifts of 40% to 70% on ING, ArcelorMittal, ABN AMRO and Prosus carry those names away
from their strikes, whereas the year that happened took Prosus down by two fifths.

## Reading the result

- **Rolling worked as a stop, and it was paid for.** The names that fell were rolled rather than
  assigned, and four of them ended a cycle with a net debit after the premium first received:
  Prosus 7,405.00 over the year, Air France-KLM 1,960.00 in the first cycle, Unilever 124.00 and
  Philips 39.00, EUR 9,528.00 in all. The book never held a share, so it never earned a dividend
  either. Whether a roll at delta 50% is better than taking
  the shares is the question the roll levers exist for; this year says it kept the drawdown to
  1.2% at the first boundary where an assignment on Prosus would have cost more.
- **The cap and the count do not fit.** Ten names at 15% each is 150% of equity against a pot of
  95%, so the service order decides who writes and the two lowest-volatility names never do. Either
  the cap comes down to about 9% or the list to six names; as it stands the last names on the list
  are passengers.
- **The volatilities moved more than the stated ones allowed for.** Between the two openings the
  realised figure rose on nine of ten names (Shell from 18.8% to 24.4%, ArcelorMittal from 34.7% to
  45.3%, Air France-KLM from 43.2% to 55.2%), and the second cycle's premiums are higher for it. A
  fixed scenario volatility would have priced the second cycle too low; this is the case for the
  rule.
- **The drift is not a figure to lean on.** The same rule gave ABN AMRO 73% a year in October and
  40% in April, Aalberts -18% then +11%. These are not opinions about the names, they are the
  standard error of a one-year line, and a forward projection on them is a projection on noise.
  For a forward run the `stated` drift estimator with a calibrated volatility is the sounder
  setting; the forward scenario `benelux.json` keeps least squares so that the owner sees the
  effect and chooses.

## Limits of this test

- One year, one book, two cycles: it is a worked example of the rule, not evidence about the
  strategy. Two years of closes is what the free download holds; a longer file per name makes the
  window and the replay longer with no other change.
- The names were chosen in October 2026 knowing they were still listed and liquid; the test cannot
  show what a name that had been delisted or had collapsed would have done to the book.
- Closes are unadjusted and carried forward over shut days; no split or return of capital fell in
  the period for these names, but the tool does not check.
- Premiums are the model's at realised volatility plus two points, with no spread. Real fills
  would be lower, and the roll costs higher.
- The wealth tax is struck on the book as it stood at the turn of the year, as the model does for
  every run.

## Next steps

1. Decide the cap against the count of names (9% for ten, or six names at 15%).
2. Set `drift_estimator` to `stated` in `benelux.json` unless a forward drift of the order of the
   calibrated figures is wanted.
3. Supply longer histories where a source allows, and re-run; the scenario needs no other change.
4. A bid-ask spread and an implied volatility from quotes are the two inputs this test most lacks.

## References

- [Calibration from a price history](../theory/calibration.md); decisions D54, D55, D56.
- [BENELUX option candidates](benelux-option-candidates.md) for the names and the stated figures.
- `data/prices/README.md` for the files and their source; `data/golden/benelux-backtest.csv` for
  the cycle table this note reads from.
