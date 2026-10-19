# Audit phase 6: `source/pricing`

*Date: 2026-10-19. Scope: the normal distribution, Black-Scholes-Merton valuation, the Greeks,
implied volatility and the assignment rules in `source/pricing`, their tests, the valuation design
contract and the four theory notes, and the way the simulation and strategy call the layer. Report
only.*

## Summary

The valuation layer is correct. Every published fixture and every worked example in the four theory
notes was reproduced independently to the last printed digit: the Hull and Haug prices, the
at-the-money identity, the eight Greeks of the 42/40 contract, the two Newton steps of the
implied-volatility example and the vega of the deep in-the-money case. The normal CDF is
`std::erfc` in double precision, the formulas are the textbook ones written on the forward, the
degenerate limits are handled without branches that could disagree with the general case, the
implied-volatility search is a safeguarded Newton that inverts to 10⁻¹² in a few steps, and all 41
tests pass. All four caller requirements are asserted and pinned by death tests.

One Medium finding and nothing above it. The dividend-driven early assignment of a covered call,
which the theory note calls one of two errors "worth spending effort to avoid" because omitting it
"overstates dividend income and understates turnover, systematically", is implemented and tested
but never called: the projection assigns at expiry only (PRICE-1). Three Low findings concern the
sign of the extrinsic value, the solver's behaviour on a price below its absolute tolerance, and an
untyped year fraction. The fast-math exposure from phase 3 reaches the normal CDF here, and was
measured at one unit in the last place on 6.5% of values with no effect on a rounded price.

| Severity | Count |
|---|---|
| Critical | 0 |
| High | 0 |
| Medium | 1 |
| Low | 3 |
| Info | 2 |

## What was executed

**Build route.** Same harness as phase 1. GCC 13.3.0, Ninja, Debug configuration, executed
2026-10-19.

```text
$ cmake --build build --target fin.pricing_test
$ (cd build && ctest -R fin.pricing_test --output-on-failure)
1/1 Test #6: fin.pricing_test .................   Passed    0.01 sec
100% tests passed, 0 tests failed out of 1
$ build/source/pricing/test/fin.pricing_test
[==========] Running 41 tests from 5 test suites.
[==========] 41 tests from 5 test suites ran. (5 ms total)
[  PASSED  ] 41 tests.
```

Experiments against the built library (sources and output in appendix B):

| Experiment | What | Result |
|---|---|---|
| X1 | implied volatility on eight hard cases: recovered σ, repricing error, speed | exact except where the price is below the tolerance (PRICE-3); 0.4 µs per inversion |
| X2 | extrinsic value and the early-assignment rule on deep in-the-money calls with carry | negative extrinsic value assigns on a one-cent dividend (PRICE-2) |
| X3 | normal CDF over 65 537 points and at ±∞ and NaN, and expiry prices, in the Debug library, a Release build, a Release build with the pragma neutralised, and Clang | one ulp on 6.5% of points under the pragma; limits correct in every build (PRICE-5) |

Files read in full: the eleven source and header files, the five test files and the scenario
header, `design/valuation.md`, decisions D8–D10, `theory/black-scholes.md`,
`theory/option-greeks.md`, `theory/implied-volatility.md`,
`theory/assignment-and-early-exercise.md`, the fixture table of `validation/sources.md`, and the
toolbox's `Erfc` in `Math.hpp` and its platform override. To establish how the layer is used, the
call sites in `sim/WheelScript.cpp:84-150` and `strategy/Projection.cpp:204`.

## What was verified independently

The Python script in the appendix produced every figure below, using `math.erfc` and nothing from
the project.

| Check | Method | Result |
|---|---|---|
| Hull fixture, 42/40, 10%, 0, 0.5 y, 20% | closed form | call 4.75942, put 0.80860 (published 4.76, 0.81) |
| Haug fixtures | closed form | put 2.4648; put 4.0870 (published 2.4648, 4.0870) |
| At-the-money identity 100(2Φ(0.1)−1) | closed form | 7.9655674554 (published 7.9655674554) |
| Design note worked example | closed form and bisection | 4.75942, delta 0.77913, time value 2.75942, implied 22.65% at 5.00 |
| Greeks worked example, call and put | closed forms of the theory note | 0.77913129, −0.22086871, 0.04996267, 8.81341506, −4.55909219, −0.75417450, 13.98204591, −5.04254258: all eight exact |
| Implied-volatility worked example | Newton from Brenner-Subrahmanyam | 0.25066283 → 0.23868824 → 0.23868887, residuals +0.2376, −1.2e−5, 0 |
| Deep in-the-money conditioning claim | K = 20, τ = 0.75, 30% | value 78.96, vega 5.8e−8 |
| Normal table and quantiles | `math.erfc` | all ten values to 10 digits |
| Formula on the forward | `BlackScholes.cpp:24-35` vs the note | identical; limits ±∞ and 0 at zero deviation |
| Normal CDF in double | `Math.hpp:221-225`: `Erfc` is `std::erfc`; the only override is for ARM | full precision |
| Caller requirements | same currency, positive spot and strike, τ ≥ 0, q ≥ 0 | each asserted (`BlackScholes.cpp:18-22`) and death-tested |
| Delta range | `Delta::FromValue` asserts [−1, 1]; e^(−qτ) ≤ 1 because q ≥ 0 | enforced by construction |
| Implied-volatility round trips (X1) | eight hard cases | |Δσ| ≤ 5e−8 and repricing error 0 wherever the price exceeds the tolerance |

## Findings

| ID | Severity | Where | Title |
|---|---|---|---|
| PRICE-1 | Medium | `AssignmentModel.cpp:15-27`; no caller outside `pricing` | Early assignment before an ex-dividend date is never applied by the projection |
| PRICE-2 | Low | `BlackScholes.cpp:71-82`, `test/TestBlackScholes.cpp:158-169` | Extrinsic value can be negative with carry; the test asserts non-negativity only at zero carry; the early-assignment rule then assigns on any dividend |
| PRICE-3 | Low | `ImpliedVolatility.cpp:44-52` | A price below the absolute tolerance returns the first candidate as a confident volatility |
| PRICE-4 | Low | `OptionParameters.hpp:15` | The year fraction to expiry is an untyped `double` |
| PRICE-5 | Info | `NormalDistribution.cpp:2` | Fast-math exposure: one ulp on 6.5% of CDF values on GCC, none on Clang; no effect on a rounded price |
| PRICE-6 | Info | `ImpliedVolatility`, `Greeks` beyond `Delta` | Implied volatility, gamma, vega, theta and rho have no production caller |

### PRICE-1 — Early assignment is never applied (Medium)

**Evidence.** `AssignmentModel::BeforeExDividend` implements the rule of
`theory/assignment-and-early-exercise.md` and is tested against that note's worked example. A
search of every non-test file outside `pricing` finds one assignment call in production,
`strategy/Projection.cpp:204`, and it is `AtExpiry`. Decisions D8 and D10 describe the
dividend-driven rule as the way the American feature is handled "outside the valuation".

**Impact.** In the note's own words, a model that only assigns at expiry "overstates dividend
income and understates turnover, and it does so systematically, in the same direction, every
year". For a covered-call wheel on a dividend-paying share, the projection keeps dividends that
would have been taken by assignment and avoids the share turnover and fees that follow. The size
depends on how often the rule would fire, which D8 says is the thing to measure before deciding
whether the European approximation matters; today nothing measures it.

Two hazards wait for whoever wires it up. The simulation prices with the scenario's continuous
yield `q`; the note says the valuation inside the early-exercise test "should not also carry a
dividend yield for the same payment, or the dividend is counted twice". And with carry, the
European value of a deep in-the-money call falls below its intrinsic value, so `ExtrinsicValue` is
negative and the rule assigns on a one-cent dividend (X2: 120/100, q 4%, r 2%, one year: extrinsic
−0.2286, assigned). That is arguably the right answer for an American call whose European value
is below parity, but it is not the dividend race the rule describes.

**Proposed fix.** Decide and document. Either (a) model discrete dividends on the path (a date and
an amount per underlying), call the rule the day before each ex-date with `q = 0` in that
valuation and the cum-dividend spot, and let the strategy settle the assignment; or (b) record in
`theory/methodology.md`'s table of limits that early assignment is not modelled and that dividend
income and turnover are biased accordingly. **Suggested test.** For (a), a projection test in
which a deep in-the-money covered call is assigned the day before a large dividend and the
dividend does not appear in the ledger.

### PRICE-2 — Extrinsic value can be negative (Low)

**Evidence.** `IntrinsicValue` is the undiscounted intrinsic value (`BlackScholes.cpp:71-77`) and
`ExtrinsicValue` is the price less that (`:79-82`). With carry, a European value can lie below
the undiscounted intrinsic value: a deep in-the-money put at r = 5%, one year, is worth 16.98
against an intrinsic 20.00; a deep in-the-money call at q 4%, r 2% is worth 19.77 against 20.00.
The test `splits_the_price_into_a_non_negative_intrinsic_and_extrinsic_part` uses r = q = 0 only.
The theory note defines E = V − I without stating that E may be negative; it says only that E is
"a difference of large, nearly equal numbers".

**Impact.** None in production (only `BeforeExDividend` reads it, PRICE-1). The quantity does not
mean "time value" when it is negative, and a future caller could divide by it or treat it as a
premium.

**Proposed fix.** State in the note and the header that extrinsic value is signed and negative
exactly when early exercise of an American option would dominate; or define it against the
discounted intrinsic value of the forward, which is never exceeded by the European price.
**Suggested test.** The two carry cases above, pinning the sign.

### PRICE-3 — A price below the absolute tolerance returns a confident volatility (Low)

**Evidence.** The solver accepts a candidate when |V(σ) − P| ≤ 16 ε S, about 3.6e−13 for a
spot of 100 (`ImpliedVolatility.cpp:44, 51`). For a quote whose price is itself below that, every
candidate qualifies and the first one, the clamped Brenner-Subrahmanyam seed, is returned. X1: a
seven-day call 50% out of the money at 30% volatility prices at 3.4e−22; inverting that price
returns 0.0001, thirty points off, with a repricing error of 3e−24. The same holds for the put.
D9 and the theory note say that an ill-conditioned inversion is not defended against; the plateau
they describe is a band of nearby volatilities, not a seed thirty points away.

**Impact.** Nil for quotes in ticks, which are never that small; real for any synthetic or model
price fed back into the solver, and for a chain loader that passes a zero or near-zero premium.

**Proposed fix.** Treat P − V(0) ≤ tolerance as "no result" (the price carries no information),
or make the tolerance relative to max(P, one tick). Also record that the upper bound used is the
price at 1 000% volatility (`maxVolatility`, line 14), which is tighter than the no-arbitrage
limit the note states; a seven-day at-the-money call quoted above about 52% of spot is refused
although a volatility above 1 000% would explain it. **Suggested test.** A price below the
tolerance yields no result.

### PRICE-4 — The year fraction is an untyped `double` (Low)

`OptionParameters::timeToExpiry` is a bare `double` in the one aggregate every valuation takes
(`OptionParameters.hpp:15`). Decision D6 gives every quantity its own type precisely so that a
number in the wrong unit cannot be passed; a caller handing days instead of years would price a
365-times longer option and nothing would notice. The simulation derives it from `YearFraction`,
so no misuse exists today. A `YearFraction` type, or at least an upper bound in the constructor
(a century), would close it.

### PRICE-5 — Fast-math exposure on the normal CDF (Info)

`NormalDistribution.cpp` includes the toolbox's `Math.hpp` at line 2, so `StandardNormalCdf` and
`StandardNormalPdf` are compiled under `-O3 -ffast-math` on GCC (ESTIM-1). Measured against the
IEEE evaluation of the same expression in the same binary over 65 537 points in [−8, 8]: the
Debug library and a Release build both differ on 4 272 points (6.5%) by one unit in the last
place, worst 1.1e−16; a Release build with the pragma neutralised, and a Clang build, differ on
none. The limits Φ(±∞), Φ(NaN) and φ(∞) and the expiry-day prices are correct in every build, and
a sample put price is bit-identical across all four. The exposure is real and the effect is
below anything that reaches a rounded cashflow; the `push_options` guard recommended in ESTIM-1
removes it.

### PRICE-6 — Most of the layer has no production caller (Info)

The simulation uses `BlackScholes::Price` and `Greeks::Delta` (`sim/WheelScript.cpp:84-150`) and
the strategy uses `AssignmentModel::AtExpiry`. `ImpliedVolatility::From`, gamma, vega, theta, rho
and `BeforeExDividend` are tested and unused. That is fine for a library, but D9's rule that
"every caller is forced to have a policy for an uninvertible quote" has no caller, and the
validation checklist's confirmation of "prices/Greeks" against a reference is complete for
quantities the projection does not consume. Worth stating in the README which parts of the model
a projection actually exercises.

## Observations carried forward to later phases

| For phase | Where | Observation |
|---|---|---|
| 7 strategy | `strategy/Projection.cpp:204` | Assignment at expiry only; see PRICE-1. Check how a dividend reaches the ledger (`CashflowKind::Dividend`) and whether the continuous yield in pricing and any cash dividend in the ledger describe the same payment once. |
| 9 sim | `sim/WheelScript.cpp:84` | The premium a wheel collects is the model price at the scenario's volatility, not a quote; the simulated σ is both the path's σ and the pricing σ, so the projection assumes the market prices options at realised volatility, with no risk premium. Confirm this is stated in `methodology.md`. |
| 9 sim | `sim/WheelScript.cpp:150` | Strike selection by delta magnitude ≤ target (D33); check tie handling and the grid walk. |

## Open questions for the owner

1. Is early assignment meant to be modelled in version one (PRICE-1)? If not, the methodology's
   limits table should say so; if so, dividends need dates and amounts in the scenario.
2. Should the solver refuse a quote whose price is within tolerance of intrinsic rather than
   return the seed (PRICE-3)?

## Not checked

- The American early-exercise gap itself (how far the European value is below an American one
  for the strikes a wheel writes); D8 accepts not quantifying it.
- Behaviour for year fractions above a few decades or volatilities above 1 000%.
- The GUI's presentation of Greeks (per point, per day); out of scope.

## Appendix A: reference-check script

Run with `python3 -I check_pricing.py`. Output as produced on 2026-10-19 follows the script.

```python
import math

def N(x):
    return 0.5 * math.erfc(-x / math.sqrt(2))

def phi(x):
    return math.exp(-0.5 * x * x) / math.sqrt(2 * math.pi)

def d12(S, K, r, q, T, sig):
    d1 = (math.log(S / K) + (r - q + 0.5 * sig * sig) * T) / (sig * math.sqrt(T))
    return d1, d1 - sig * math.sqrt(T)

def bs(S, K, r, q, T, sig, call=True):
    if sig * math.sqrt(T) == 0:
        F, Kd = S * math.exp(-q * T), K * math.exp(-r * T)
        return max(F - Kd, 0.0) if call else max(Kd - F, 0.0)
    d1, d2 = d12(S, K, r, q, T, sig)
    if call:
        return S * math.exp(-q * T) * N(d1) - K * math.exp(-r * T) * N(d2)
    return K * math.exp(-r * T) * N(-d2) - S * math.exp(-q * T) * N(-d1)

def vega(S, K, r, q, T, sig):
    return S * math.exp(-q * T) * phi(d12(S, K, r, q, T, sig)[0]) * math.sqrt(T)

def iv(S, K, r, q, T, price, call=True):
    lo, hi = 0.0, 10.0
    for _ in range(200):
        mid = 0.5 * (lo + hi)
        if bs(S, K, r, q, T, mid, call) > price:
            hi = mid
        else:
            lo = mid
    return 0.5 * (lo + hi)

print(f"Hull 42/40/10%/0/0.5/20%: call {bs(42, 40, .1, 0, .5, .2):.5f} put {bs(42, 40, .1, 0, .5, .2, False):.5f} (published 4.76 / 0.81)")
print(f"Haug 100/95/10%/5%/0.5/20% put: {bs(100, 95, .1, .05, .5, .2, False):.4f} (published 2.4648)")
print(f"Haug 75/70/10%/carry 5% (q 5%)/0.5/35% put: {bs(75, 70, .1, .05, .5, .35, False):.4f} (published 4.0870)")
print(f"ATM identity 100/100/0/0/1/20%: {bs(100, 100, 0, 0, 1, .2):.10f} = 100(2N(0.1)-1) = {100 * (2 * N(0.1) - 1):.10f} (published 7.9655674554)")
c = bs(42, 40, .1, 0, .5, .2)
d1, d2 = d12(42, 40, .1, 0, .5, .2)
print(f"design example: price {c:.5f} delta {N(d1):.5f} time value {c - 2:.5f} implied at 5.00 {100 * iv(42, 40, .1, 0, .5, 5.0):.2f}% (published 4.75942 / 0.77913 / 2.75942 / 22.65%)")
S, K, r, q, T, s = 42.0, 40.0, 0.10, 0.0, 0.5, 0.20
e_q, e_r = math.exp(-q * T), math.exp(-r * T)
gamma = e_q * phi(d1) / (S * s * math.sqrt(T))
theta_c = -S * e_q * phi(d1) * s / (2 * math.sqrt(T)) - r * K * e_r * N(d2) + q * S * e_q * N(d1)
theta_p = -S * e_q * phi(d1) * s / (2 * math.sqrt(T)) + r * K * e_r * N(-d2) - q * S * e_q * N(-d1)
print(f"Greeks example: delta {e_q * N(d1):.8f} / {-e_q * N(-d1):.8f} gamma {gamma:.8f} vega {vega(S, K, r, q, T, s):.8f} theta {theta_c:.8f} / {theta_p:.8f} rho {K * T * e_r * N(d2):.8f} / {-K * T * e_r * N(-d2):.8f}")
print("published:      0.77913129 / -0.22086871 gamma 0.04996267 vega 8.81341506 theta -4.55909219 / -0.75417450 rho 13.98204591 / -5.04254258")
sig = math.sqrt(2 * math.pi / 0.25) * 5.0 / 100
for n in range(3):
    v, nu = bs(100, 100, 0.02, 0, 0.25, sig), vega(100, 100, 0.02, 0, 0.25, sig)
    print(f"implied-volatility example step {n}: sigma {sig:.8f} V {v:.8f} vega {nu:.6f} V-P {v - 5:+.8f}")
    sig -= (v - 5) / nu
print("published: 0.25066283 -> 0.23868824 -> 0.23868887")
print(f"deep ITM K=20, 0.75 y, 30%: call {bs(100, 20, 0.03, 0.02, 0.75, 0.3):.2f} vega {vega(100, 20, 0.03, 0.02, 0.75, 0.3):.1e} (published 78.96, 5.8e-8)")
print(f"normal table: N(-1)={N(-1):.10f} N(1)={N(1):.10f} N(1.96)={N(1.96):.10f} N(2.5)={N(2.5):.10f} N(3)={N(3):.10f} phi(0)={phi(0):.10f} phi(1)={phi(1):.10f} phi(2)={phi(2):.10f} phi(3)={phi(3):.10f}")
for S_, K_, r_, q_, T_, call, label in [(120, 100, .02, .04, 1, True, "deep ITM call, q 4% > r 2%, 1 y"), (80, 100, .05, 0, 1, False, "deep ITM put, r 5%, 1 y"), (120, 100, .03, 0, .02, True, "note's example 120/100, 7 days")]:
    p = bs(S_, K_, r_, q_, T_, .2, call)
    intr = max(S_ - K_, 0) if call else max(K_ - S_, 0)
    print(f"{label}: price {p:.4f} intrinsic {intr:.2f} extrinsic {p - intr:+.4f}")
print(f"7-day call 50% OTM at 30%: price {bs(100, 150, .03, .02, .02, .3):.3e}; absolute tolerance 16 eps S = {16 * 2.220446049250313e-16 * 100:.2e}")
```

```text
Hull 42/40/10%/0/0.5/20%: call 4.75942 put 0.80860 (published 4.76 / 0.81)
Haug 100/95/10%/5%/0.5/20% put: 2.4648 (published 2.4648)
Haug 75/70/10%/carry 5% (q 5%)/0.5/35% put: 4.0870 (published 4.0870)
ATM identity 100/100/0/0/1/20%: 7.9655674554 = 100(2N(0.1)-1) = 7.9655674554 (published 7.9655674554)
design example: price 4.75942 delta 0.77913 time value 2.75942 implied at 5.00 22.65% (published 4.75942 / 0.77913 / 2.75942 / 22.65%)
Greeks example: delta 0.77913129 / -0.22086871 gamma 0.04996267 vega 8.81341506 theta -4.55909219 / -0.75417450 rho 13.98204591 / -5.04254258
published:      0.77913129 / -0.22086871 gamma 0.04996267 vega 8.81341506 theta -4.55909219 / -0.75417450 rho 13.98204591 / -5.04254258
implied-volatility example step 0: sigma 0.25066283 V 5.23760546 vega 19.842482 V-P +0.23760546
implied-volatility example step 1: sigma 0.23868824 V 4.99998755 vega 19.844492 V-P -0.00001245
implied-volatility example step 2: sigma 0.23868887 V 5.00000000 vega 19.844492 V-P -0.00000000
published: 0.25066283 -> 0.23868824 -> 0.23868887
deep ITM K=20, 0.75 y, 30%: call 78.96 vega 5.8e-08 (published 78.96, 5.8e-8)
normal table: N(-1)=0.1586552539 N(1)=0.8413447461 N(1.96)=0.9750021049 N(2.5)=0.9937903347 N(3)=0.9986501020 phi(0)=0.3989422804 phi(1)=0.2419707245 phi(2)=0.0539909665 phi(3)=0.0044318484
deep ITM call, q 4% > r 2%, 1 y: price 19.7714 intrinsic 20.00 extrinsic -0.2286
deep ITM put, r 5%, 1 y: price 16.9824 intrinsic 20.00 extrinsic -3.0176
note's example 120/100, 7 days: price 20.0600 intrinsic 20.00 extrinsic +0.0600
7-day call 50% OTM at 30%: price 3.379e-22; absolute tolerance 16 eps S = 3.55e-13
```

## Appendix B: experiments against the built library

Built from the harness in the session scratchpad against `libfin.pricing.a` and emil's
`libinfra.util.a`; the Release variants compile the module's sources directly with
`-O2 -DNDEBUG`. `demo_pr.cpp`:

```cpp
#include "source/pricing/AssignmentModel.hpp"
#include "source/pricing/BlackScholes.hpp"
#include "source/pricing/Greeks.hpp"
#include "source/pricing/ImpliedVolatility.hpp"
#include "source/pricing/NormalDistribution.hpp"
#include <chrono>
#include <cstring>
#include <tuple>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace fin; using namespace fin::pricing;
static OptionParameters P(double S, double K, double r, double q, double T, core::OptionType type)
{ return { core::Money::FromAmount(S, core::Currency::Eur), core::Money::FromAmount(K, core::Currency::Eur), core::Rate::FromContinuous(r), core::Rate::FromContinuous(q), T, type }; }
int main(int argc, char** argv)
{
    const int which = argc > 1 ? std::atoi(argv[1]) : 0;
    if (which == 1)  // implied-volatility solver on hard cases: recovered vol, repricing error
    {
        std::printf("case                                   sigma_in  recovered     |dsigma|   |dprice|/S\n");
        for (auto [S, K, r, q, T, s, call, label] : { std::tuple{100.0, 100.0, 0.03, 0.02, 0.02, 0.01, true, "ATM, 7 days, 1% vol"}, std::tuple{100.0, 150.0, 0.03, 0.02, 0.02, 0.30, true, "50% OTM call, 7 days"}, std::tuple{100.0, 50.0, 0.03, 0.02, 0.02, 0.30, false, "50% OTM put, 7 days"}, std::tuple{100.0, 100.0, 0.03, 0.02, 10.0, 3.0, true, "ATM, 10 years, 300% vol"}, std::tuple{100.0, 20.0, 0.03, 0.02, 0.75, 0.30, true, "deep ITM call (K=20)"}, std::tuple{100.0, 500.0, 0.03, 0.02, 0.75, 0.30, false, "deep ITM put (K=500)"}, std::tuple{100.0, 100.0, -0.01, 0.0, 1.0, 0.20, true, "negative rate"}, std::tuple{5.0, 5.0, 0.03, 0.0, 0.1, 0.60, false, "5.00 share, 60% vol"} })
        {
            const auto p = P(S, K, r, q, T, call ? core::OptionType::Call : core::OptionType::Put);
            const double price = BlackScholes(p, core::Volatility::FromAnnualized(s)).Price();
            const auto got = ImpliedVolatility::From(p, price);
            if (got) std::printf("%-38s %8.4f  %9.6f  %10.2e  %10.2e\n", label, s, got->Annualized(), std::abs(got->Annualized() - s), std::abs(BlackScholes(p, *got).Price() - price) / S);
            else std::printf("%-38s %8.4f  NO RESULT (price %.6g)\n", label, s, price);
        }
        const auto p = P(100.0, 100.0, 0.03, 0.02, 0.5, core::OptionType::Call);
        const auto t0 = std::chrono::steady_clock::now(); double acc = 0;
        for (int i = 0; i < 20000; ++i) acc += ImpliedVolatility::From(p, 5.0 + 1e-6 * i)->Annualized();
        std::printf("20000 inversions: %.1f us each (checksum %.3f)\n", std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / 20000, acc);
    }
    if (which == 2)  // negative extrinsic value and the early-assignment rule
    {
        for (auto [S, K, r, q, T, label] : { std::tuple{120.0, 100.0, 0.02, 0.04, 1.0, "deep ITM call, q 4% > r 2%, one year"}, std::tuple{150.0, 100.0, 0.03, 0.0, 1.0, "deep ITM call, q 0, r 3%, one year"}, std::tuple{120.0, 100.0, 0.03, 0.0, 0.02, "test case 120/100, 7 days"} })
        {
            const auto p = P(S, K, r, q, T, core::OptionType::Call);
            const BlackScholes m(p, core::Volatility::FromAnnualized(0.2));
            const auto a = AssignmentModel::BeforeExDividend(p, core::Volatility::FromAnnualized(0.2), core::Money::FromCents(1, core::Currency::Eur));
            std::printf("%-40s price %.4f intrinsic %.2f extrinsic %+.4f -> one cent of dividend: %s\n", label, m.Price(), m.IntrinsicValue(), m.ExtrinsicValue(), a == AssignmentOutcome::Assigned ? "ASSIGNED" : "not assigned");
        }
    }
    if (which == 3)  // the normal CDF on a grid and at the limits (for bitwise comparison between builds)
    {
        double acc = 0; unsigned long long h = 1469598103934665603ull;
        for (double x = -8.0; x <= 8.0; x += 1.0 / 1024) { const double v = StandardNormalCdf(x); acc += v; unsigned long long b; std::memcpy(&b, &v, 8); h = (h ^ b) * 1099511628211ull; }
        std::printf("grid: sum %.17g hash %016llx | N(+inf)=%g N(-inf)=%g N(NaN)=%g pdf(inf)=%g | price at expiry ITM call %.4f OTM put %.4f\n", acc, h, StandardNormalCdf(std::numeric_limits<double>::infinity()), StandardNormalCdf(-std::numeric_limits<double>::infinity()), StandardNormalCdf(std::numeric_limits<double>::quiet_NaN()), StandardNormalPdf(std::numeric_limits<double>::infinity()), BlackScholes(P(120, 100, 0.03, 0.02, 0.0, core::OptionType::Call), core::Volatility::FromAnnualized(0.2)).Price(), BlackScholes(P(120, 100, 0.03, 0.02, 0.0, core::OptionType::Put), core::Volatility::FromAnnualized(0.2)).Price());
    }
    return 0;
}
```

```text
$ ./demo_pr 1
case                                   sigma_in  recovered     |dsigma|   |dprice|/S
ATM, 7 days, 1% vol                      0.0100   0.010000    2.50e-15    0.00e+00
50% OTM call, 7 days                     0.3000   0.000100    3.00e-01    3.38e-24
50% OTM put, 7 days                      0.3000   0.000100    3.00e-01    4.49e-63
ATM, 10 years, 300% vol                  3.0000   3.000000    5.70e-12    0.00e+00
deep ITM call (K=20)                     0.3000   0.300000    4.64e-08    0.00e+00
deep ITM put (K=500)                     0.3000   0.300000    7.35e-09    0.00e+00
negative rate                            0.2000   0.200000    1.11e-16    0.00e+00
5.00 share, 60% vol                      0.6000   0.600000    1.17e-14    1.51e-15
20000 inversions: 0.4 us each (checksum 3418.580)
$ ./demo_pr 2
deep ITM call, q 4% > r 2%, one year     price 19.7714 intrinsic 20.00 extrinsic -0.2286 -> one cent of dividend: ASSIGNED
deep ITM call, q 0, r 3%, one year       price 53.0807 intrinsic 50.00 extrinsic +3.0807 -> one cent of dividend: not assigned
test case 120/100, 7 days                price 20.0600 intrinsic 20.00 extrinsic +0.0600 -> one cent of dividend: not assigned
$ ./demo_pr 3; ./demo_pr_rel 3; ./demo_pr_rel_patched 3     # Debug; Release; Release with push/pop
grid: sum 8192.5000000000127 hash c1905639492ec54b | N(+inf)=1 N(-inf)=0 N(NaN)=nan pdf(inf)=0 | price at expiry ITM call 20.0000 OTM put 0.0000
grid: sum 8192.5000000000127 hash c1905639492ec54b | N(+inf)=1 N(-inf)=0 N(NaN)=nan pdf(inf)=0 | price at expiry ITM call 20.0000 OTM put 0.0000
grid: sum 8192.5000000000127 hash 23aeed837feae627 | N(+inf)=1 N(-inf)=0 N(NaN)=-nan pdf(inf)=0 | price at expiry ITM call 20.0000 OTM put 0.0000
```

`cdfdiff.cpp` compares `StandardNormalCdf` with the IEEE evaluation of the same expression in a
unit that is not under the pragma:

```cpp
#include "source/pricing/BlackScholes.hpp"
#include "source/pricing/NormalDistribution.hpp"
#include <cmath>
#include <cstdio>
using namespace fin; using namespace fin::pricing;
int main()
{
    double worst = 0, worstX = 0; long differing = 0, total = 0;
    for (double x = -8.0; x <= 8.0; x += 1.0 / 4096)
    {
        const double ref = 0.5 * std::erfc(-x / std::sqrt(2.0));   // IEEE evaluation, this unit is not under the pragma
        const double got = StandardNormalCdf(x); ++total;
        if (got != ref) { ++differing; if (std::abs(got - ref) > worst) { worst = std::abs(got - ref); worstX = x; } }
    }
    std::printf("normal CDF vs IEEE evaluation over %ld points: %ld differ (%.1f%%), worst |diff| %.3g at x=%.4f\n", total, differing, 100.0 * differing / total, worst, worstX);
    const OptionParameters p{ core::Money::FromAmount(100.0, core::Currency::Eur), core::Money::FromAmount(95.0, core::Currency::Eur), core::Rate::FromContinuous(0.03), core::Rate::FromContinuous(0.02), 0.5, core::OptionType::Put };
    std::printf("put 100/95/3%%/2%%/0.5y/25%%: %.17g\n", BlackScholes(p, core::Volatility::FromAnnualized(0.25)).Price());
}
```

```text
--- Debug library (pragma active)
normal CDF vs IEEE evaluation over 65537 points: 4272 differ (6.5%), worst |diff| 1.11e-16 at x=0.1096
put 100/95/3%/2%/0.5y/25%: 4.4125996130745619
--- Release, pragma active
normal CDF vs IEEE evaluation over 65537 points: 4272 differ (6.5%), worst |diff| 1.11e-16 at x=0.1096
put 100/95/3%/2%/0.5y/25%: 4.4125996130745619
--- Release, pragma neutralised (push/pop)
normal CDF vs IEEE evaluation over 65537 points: 0 differ (0.0%), worst |diff| 0 at x=0.0000
put 100/95/3%/2%/0.5y/25%: 4.4125996130745619
--- clang++ -O2, pragma ignored
normal CDF vs IEEE evaluation over 65537 points: 0 differ (0.0%), worst |diff| 0 at x=0.0000
put 100/95/3%/2%/0.5y/25%: 4.4125996130745619
```
