# Estimation contracts

## Summary

The estimation layer turns a recorded history of closing prices into the handful of model parameters
the rest of the project runs on: a drift, a volatility and a dividend yield. This document states what
it requires of the history it is given, what it guarantees about the parameters it returns, and which
questions it deliberately refuses to answer.

## Motivation

Every projection this project produces is a long extrapolation from a short measurement. The
projection machinery cannot tell the difference between a volatility measured over twenty years and
one measured over twenty days, and it will compound either of them for decades with equal composure.
Two properties therefore matter more than any feature of the estimators themselves.

1. **A series is validated once, where it is created.** Sorted dates, positive closes, a single
   currency and a non-zero horizon are checked at the point a history comes into existence, so no
   estimator has to re-check them and none of them can be violated further down.
2. **The observation frequency is derived, not declared.** The number of periods per year comes from
   the dates and the day-count convention of the series itself. A caller cannot annualise a series of
   monthly closes as though it were daily, because the caller is never asked.

## Definitions & notation

The layer is built from a data structure and four estimators, layered in one direction:

| Capability | Question answered | Depends on |
|---|---|---|
| Price history | what was observed, when, and how often per year | dates, cash amounts, day counts |
| Log returns | how much did the price move each period | price history |
| Realised volatility | how much has it moved, over the whole window | log returns |
| Weighted volatility | how much is it moving now | log returns |
| Line fit | which straight line best explains these points, by some criterion | - |
| Drift | how fast is it growing | price history, a line fit |
| Gathered parameters | all of the above, in one consistent set | all estimators |

A price history is a sequence of dated closing prices together with the day-count convention they
were recorded under. It is the only input any estimator takes; there is no path by which a bare array
of numbers reaches an estimator without first being made into a history.

**A line fit is a capability, not a setting.** Fitting a line to points is a question with more than
one defensible answer, so it is named, given its own contract, and supplied to the drift estimator by
the caller. Each fit states the longest window it will accept and either returns a slope or reports
that the points do not determine one. The drift estimator knows nothing about how the line was
arrived at; it converts a price history into coordinates, asks for a slope, and gives that slope its
unit. Adding a robust or weighted fit later adds a fit, and changes nothing else.

## Derivation / Rules

### What the caller must supply

| Requirement | Why it is a requirement |
|---|---|
| At least two observations | one price is not a series and admits no return |
| Dates strictly increasing | a duplicate or reversed date is a corrupt file, not a market event |
| Closes strictly positive, one currency | the layer works in logarithms of the price, and a series is one instrument |
| A window spanning a non-zero year fraction | otherwise the observation frequency is not defined |
| A non-negative dividend yield | a negative yield is not a yield |

Each of these is a defect rather than a recoverable error: the loader that reads a file is the
boundary at which malformed data is reported, and by the time a history exists the question has
already been settled. See [decision D4](decisions.md).

The one bound that is a property of the implementation rather than of the model is the **length of the
window a fitted drift will accept**. The fit is performed on a fixed-capacity system of equations, so
each fit publishes its capacity - several decades of daily observations - and refuses a longer series
rather than truncating it silently. See [decision D15](decisions.md).

### What the layer guarantees

- **Every estimate carries its unit.** A volatility is a volatility and a drift is a rate; neither is
  a bare decimal, and neither can be passed where the other is expected.
- **The reported drift is the drift of the price, not of its logarithm.** The two differ by half the
  variance, and the correction is applied exactly once, where the estimates are gathered. See
  [decision D12](decisions.md) and [drift estimation](../theory/drift-estimation.md).
- **The reported drift is a price drift, not a total return.** A series of closes cannot distinguish
  a dividend from a fall in the price, so the dividend yield is supplied by the caller and carried
  alongside rather than estimated. See [decision D13](decisions.md).
- **A drift that the series does not determine is reported as absent.** An orthogonal fit can fail to
  resolve a line; when it does the answer is nothing, not a number.
- **A price that never moves has a drift of exactly zero and a volatility of exactly zero.** Neither
  is a special case that a caller has to guard.
- **Estimates are reproducible.** Given the same history, an estimator returns the same numbers, in
  every build. Nothing in the layer reads a clock, a random number or a file.

### What the layer refuses to express

- **An estimator does not consume a live stream.** It is given a completed history and returns either
  a single estimate or the whole track of estimates that history would have produced, one per
  observation. See [decision D11](decisions.md).
- **An estimator does not report its own uncertainty.** The error bounds are in the theory notes and
  they are large, particularly for the drift; expressing them as a value would invite them to be
  carried around and quietly ignored, and the projection reports uncertainty by simulating a
  distribution rather than by propagating an error bar.
- **An estimator does not choose its own window or weighting.** The half-life of a weighted estimator
  and the choice of fit are inputs, because they encode a judgement about how quickly the market
  changes that no series can settle on its own.
- **The layer does not forecast.** Every quantity it reports describes the window it was given. That
  the projection then treats them as constant for decades is an assumption of the projection, made
  visible here rather than hidden inside an estimator.

## Edge cases & pitfalls

- **The estimates are only as good as their window, and the window is not part of the answer.** Two
  sets of parameters that look identical may have been measured over one year and over thirty. The
  history that produced them is the only record of which.
- **The dividend yield travels with the estimates but is not one of them.** It arrives from a
  configuration file or a data source and passes through unchanged; a reader who assumes all three
  numbers were measured from the same file will be wrong about one of them.
- **A weighted estimator and a batch estimator answer different questions and will disagree.** They
  are not two implementations of one quantity, and a test that pins them to each other would be
  testing the market rather than the code.

## Alternatives considered

**Estimating the dividend yield from the series.** Rejected: separating a dividend from a price fall
requires a total-return series or a dividend record, neither of which a close-only file contains. An
estimator that guessed would be indistinguishable from one that worked.

**Letting the caller declare the observation frequency.** Rejected: it is derivable from data the
series already carries, and every value a caller could supply that differs from the derived one is a
mistake.

**Returning the estimates as plain decimals.** Rejected for the reason given in
[decision D6](decisions.md): a drift, a yield and a volatility are all small decimals, and a swap
between them produces a plausible projection rather than a visible failure.
