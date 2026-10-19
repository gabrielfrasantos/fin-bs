# Audit phase 3: `source/estimation`

*Date: 2026-10-19. Scope: price history, log returns, historical, exponentially weighted and
recursive volatility, line fits, drift and gathered parameters in `source/estimation`, their tests,
and the GBM test double. Report only.*

## Summary

The estimators compute what the theory documents say they compute. Every worked example in the
three theory notes was reproduced to the last printed digit, the two recursive estimators match
their closed forms, the drift standard-error formula was confirmed by simulation, and all 85 tests
pass. The module's own code is careful: histories are validated once, every parameter carries its
unit, and the centroid padding of the fixed-capacity fits is exact as decision D15 claims.

One High finding comes from the dependency. Every numerical-toolbox header begins with
`#pragma GCC optimize("O3", "fast-math")`, which GCC applies to every function defined later in
the same translation unit, including this project's. In a Release build the constructors of the
exponentially weighted and recursive estimators therefore accept a NaN parameter: the refusal is
compiled away (ESTIM-1, demonstrated, with a one-line fix validated). Three Medium findings: a
price file with exactly two closes passes the loader and then aborts the application instead of
being reported (ESTIM-2, demonstrated); the fixed-capacity fits need between 512 KB and 768 KB of
stack, most of a Windows default (ESTIM-3, measured); and the application estimates volatility
under ACT/252 without a holiday calendar and simulates under ACT/365F, which understates every
annualised volatility by about 1.7% for a Euronext series (ESTIM-4, quantified).

| Severity | Count |
|---|---|
| Critical | 0 |
| High | 1 |
| Medium | 3 |
| Low | 4 |
| Info | 1 |

## What was executed

**Build route.** Same harness as phase 1. GCC 13.3.0, Ninja, Debug configuration, executed
2026-10-19.

```text
$ cmake --build build --target fin.estimation_test
$ (cd build && ctest -R fin.estimation_test --output-on-failure)
1/1 Test #3: fin.estimation_test ..............   Passed    0.09 sec
100% tests passed, 0 tests failed out of 1
$ build/source/estimation/test/fin.estimation_test
[==========] Running 85 tests from 9 test suites.
[==========] 85 tests from 9 test suites ran. (72 ms total)
[  PASSED  ] 85 tests.
```

Three experiments were run against the built library; their sources and outputs are in the
appendix: a demonstration program (two-close history; NaN parameters in Debug and Release builds;
the same with the pragma neutralised), a stack-limit sweep of the test binary, and a dump of the
constructor's assembly.

Files read in full: all twenty source and header files, the nine test files, `GbmSeries.hpp/.cpp`,
`design/estimation.md`, decisions D3 and D11–D15, `theory/log-returns-and-volatility.md`,
`theory/drift-estimation.md`, `theory/exponentially-weighted-volatility.md`, the first part of
`theory/volatility-scaling.md`, and the four toolbox headers the module instantiates
(`LinearRegression`, `TotalLeastSquares`, `RecursiveLeastSquares`, `ExponentialMovingAverage`)
plus the storage declarations of `Matrix`, `QrDecomposition` and `SingularValueDecomposition`.
To establish how the module is used: `io/PriceCsvLoader.cpp:161-173`, `present/PriceHistory.cpp`
and `present/RunController.cpp:55-60`.

## What was verified independently

The Python script in the appendix produced every figure below.

| Check | Method | Result |
|---|---|---|
| Log-returns worked example | by hand | 0.019803, −0.009852, 0.029270; sum = ln 1.04; s = 0.020411; σ = 0.3240 |
| Drift worked example | OLS on five closes, ACT/365F | Sxy = 7.222446e−3, Sxx = 7.213361e−2, slope 0.100126 |
| EWMA worked example | α = 1 − 2^(−1/20) | 0.034064; 15.87%, 21.76%, 25.10%, 28.62%, 31.37% |
| Half-life table | n_eff = (2−α)/α | 28.9, 57.7, 173.1, 721.3; relative error 9.3%, 6.6%, 3.8%, 1.9% |
| EWMA recursion vs closed-form weighted sum | 300 random returns | identical to 12 digits |
| RLS with λ = 1 and a diffuse prior vs the running mean of squares | same returns | agree to 3 parts in 10⁹ |
| RLS gain settles at 1 − λ | λ = 0.97 | P → 0.030003 |
| Standard error of the fitted log drift, σ√(6/(5T)) | 2000 random walks, σ = 20%, T = 10 y | measured 0.0676 vs formula 0.0693 vs first-and-last 0.0632 |
| Minor-unit quotation bias 2c²/(12S²) | 400 short series, prices held near S | S = 5: 0.43% vs 0.39%; S = 2: 2.54% vs 2.47% |
| Padding at the centroid leaves the slope unchanged (D15) | algebra: zero rows add nothing to Sxx, Sxy or MᵀM | holds; pinned by `a_fit_uses_only_the_observations_it_was_given` |
| Coefficient indexing | `LinearRegression` design matrix is [1, x], so `.at(1,0)` is the slope; TLS slope is −v₀₁/v₁₁ | correct |
| Precision inside the toolbox | every instantiation is `<double, …>` | double throughout |
| Caller requirements (design/estimation.md) | ≥ 2 observations, increasing dates, positive closes, one currency, non-zero horizon, non-negative yield | each has an assertion and an `EXPECT_DEATH` |
| ACT/252 versus ACT/365F annualisation of a real year | 2024 weekdays less 8 exchange holidays | 245.2 vs 253.7 periods per year; σ ratio 0.9831 |

## Findings

| ID | Severity | Where | Title |
|---|---|---|---|
| ESTIM-1 | High | `EmaVolatilityEstimator.cpp:3,54-60`, `RlsVolatilityEstimator.cpp:3,73-79`, `OrdinaryLeastSquaresFit.cpp:2`, `TotalLeastSquaresFit.cpp:2` | Toolbox headers switch the rest of the translation unit to `-O3 -ffast-math`; Release builds accept NaN parameters |
| ESTIM-2 | Medium | `HistoricalVolatility.cpp:9`, `PriceSeries.cpp:10`, `io/PriceCsvLoader.cpp:167`, `present/PriceHistory.cpp:33-39` | A history of exactly two closes passes the boundary and aborts the application |
| ESTIM-3 | Medium | `OrdinaryLeastSquaresFit.cpp:26-32`, `TotalLeastSquaresFit.cpp:26-35` | The fits need 512–768 KB of stack per call |
| ESTIM-4 | Medium | `present/PriceHistory.cpp:23`, `present/RunController.cpp:59`, `PriceSeries.cpp:56-59` | Volatility is annualised under ACT/252 without holidays and consumed under ACT/365F: about 1.7% low |
| ESTIM-5 | Low | `HistoricalVolatility.hpp:11`, `EmaVolatilityEstimator.hpp:12-17`, `RlsVolatilityEstimator.hpp:12-16`, `design/estimation.md` | Three estimators take a bare array and a declared frequency, which the design document says cannot happen; two of them and the orthogonal fit have no production caller |
| ESTIM-6 | Low | `theory/log-returns-and-volatility.md` | The quotation-bias example figure ("about 2%" near 5 units) does not follow from its own formula (0.4%) |
| ESTIM-7 | Low | `design/decisions.md` D3 | D3 still says single precision is used inside external fits; the code instantiates the toolbox in double |
| ESTIM-8 | Low | `DriftEstimator.hpp:18` | The drift estimator keeps a reference to the fit it was given |
| ESTIM-9 | Info | `LineFit.cpp:54-60`, `TotalLeastSquaresFit.cpp` | The orthogonal fit on unit-length axes is the reduced-major-axis regression, available in closed form |

### ESTIM-1 — Toolbox headers put the rest of the unit under `-O3 -ffast-math` (High)

**Evidence.** All 99 headers under `numerical/` in numerical-toolbox-cpp v3.0.0 begin with
`#pragma GCC optimize("O3", "fast-math")` without `push_options`/`pop_options`, among them
`Math.hpp` and `Matrix.hpp`. GCC documents this pragma as setting options "for functions defined
later in the source file", and warns that the optimize attribute "is not suitable in production
code". In this module four translation units include such a header before defining their own
functions: `EmaVolatilityEstimator.cpp` (include at line 3, constructor at 54–60),
`RlsVolatilityEstimator.cpp` (3, 73–79), `OrdinaryLeastSquaresFit.cpp` (2) and
`TotalLeastSquaresFit.cpp` (2). Three more exist elsewhere: `optimize/StrikeSizingOptimizer.cpp`,
`pricing/NormalDistribution.cpp`, `sim/GbmPathGenerator.cpp`.

Demonstrated with the program in the appendix, built against this module's sources:

| Build | `WithSmoothingFactor(NaN, 252)` | `WithForgettingFactor(NaN, 252)` | `WithSmoothingFactor(0.1, NaN)` |
|---|---|---|---|
| Debug (`assert` form of `really_assert`) | refused | refused | refused |
| `-O2 -DNDEBUG` (abort form) | **accepted**, `SmoothingFactor() = nan` | **accepted** | **accepted** |
| `-O2 -DNDEBUG -fno-fast-math` on the command line | accepted (the pragma overrides the command line) | | |
| `-O2 -DNDEBUG`, include wrapped in `push_options`/`pop_options` | refused | | |
| `-O2 -DNDEBUG`, a unit without a toolbox include (`HistoricalVolatility::From(r, NaN)`) | refused | | |

The constructor's Release assembly compares with `comisd` and branches with `ja` and no parity
check: the unordered (NaN) outcome is not tested, which is what `-ffinite-math-only` permits.

**Impact.** Decision D4 promises that impossible states stop the run "in every build". In a Release
build of the desktop application, a NaN smoothing factor, forgetting factor or observation
frequency reaching these constructors is stored and propagates as NaN volatilities. No current
caller passes one (the configuration loaders bound their inputs), so no number is wrong today; the
guarantee is what is broken, and it is broken silently and only on one compiler. Two further
effects are certain rather than potential: the same source compiles to different arithmetic on
GCC (fast-math: reassociation, reciprocal approximation, flush-to-zero) than on Clang and MSVC,
which both ignore this pragma, so the three CI platforms do not run the same numerics in these
units; and the affected functions are compiled at `-O3` even in Debug and coverage builds, which
distorts coverage and debugging. In this module the fast-math units are the fits and the recursive
estimators, so a fitted drift can differ in its last digits between platforms. In the pricing and
simulation units the same mechanism touches the normal CDF and the path generator; those are
assessed in phases 6 and 9.

**Proposed fix.** In each of the seven translation units, wrap the toolbox includes:

```cpp
#pragma GCC push_options
#include "numerical/filters/passive/ExponentialMovingAverage.hpp"
#pragma GCC pop_options
```

(Clang and MSVC ignore both pragmas, so no guard is needed.) Better, one wrapper header per toolbox
header the project uses, so the rule cannot be forgotten. Upstream, the toolbox already defines an
`OPTIMIZE_FOR_SPEED` attribute macro for its hot functions; the file-level pragma could be dropped
in favour of it. **Suggested test.** Death tests for a NaN smoothing factor, forgetting factor and
observation frequency in both estimators, run in CI on a Release configuration as well as Debug,
so that the refusal is pinned in the build where it currently fails.

### ESTIM-2 — Two closes pass the boundary and abort the application (Medium)

**Evidence.** `PriceSeries::From` accepts two observations (`PriceSeries.cpp:10`) and
`io/PriceCsvLoader.cpp:167` reports fewer than two. `HistoricalVolatility::From` requires two
*returns* (`HistoricalVolatility.cpp:9`), which two closes do not give. `present/PriceHistory.cpp`
loads the file (line 23), calls `MarketParameters::From` (33) and handles a `nullopt` with the
message "The price history does not hold enough closes to estimate from" (35–39). That branch is
only reached when a fit fails to resolve a line; with two closes the run instead stops in the
estimator. Demonstrated: a two-close series is accepted and `MarketParameters::From` aborts with
`Assertion 'logReturns.size() >= 2' failed`.

**Impact.** A user who loads a two-row file crashes the application instead of reading a message,
which is the boundary-versus-defect split of decision D4 drawn in the wrong place. No wrong number.
`design/estimation.md` lists "at least two observations" as the requirement, so the document and
the volatility estimator disagree by one.

**Proposed fix.** Require three closes at `PriceSeries::From` and in the CSV loader (by message),
and state three in the design note; or make the historical estimator's requirement part of
`MarketParameters::From` and return `nullopt`. **Suggested tests.** A loader test that a two-row
document is reported, and a `MarketParameters` test with two closes.

### ESTIM-3 — The fits need 512–768 KB of stack per call (Medium)

**Evidence.** The toolbox's fixed-capacity types are `std::array`-backed, so every matrix lives
on the stack. With the capacity of 8 192 observations (`OrdinaryLeastSquaresFit.hpp:11`), GCC's
`-fstack-usage` reports frames of 131 216 B for `OrdinaryLeastSquaresFit::StandardisedSlope`,
262 240 B for `LinearRegression::Fit`, 131 200 B for `QrDecomposition::Decompose` and 65 664 B
for `ApplyQtranspose`; the orthogonal path is 131 200 + 262 288 + 262 288 + 65 680 B. Running
the line-fit and drift tests under a reduced stack limit: they pass at 768 KB and crash at 512 KB.

**Impact.** The Linux and macOS defaults (8 MB) are ample. The Windows default for the main thread
and for threads created without an explicit size is 1 MB, so a fit consumes between half and
three quarters of it, below whatever Qt and the caller have already used. CI passes on Windows
because the test binary calls the fit from a shallow stack; the desktop application, or any
future worker thread with a smaller stack, has a narrow margin that no test measures. An overflow
is a silent crash, not a wrong number.

**Proposed fix.** Allocate the regression and solver objects on the heap in `StandardisedSlope`
(`std::make_unique`), or keep them in a per-fit buffer; or lower the capacity to what the project
needs (8 192 daily closes is 32 years; 4 096 halves the footprint). **Suggested test.** A CI step
that runs `fin.estimation_test` under `ulimit -s 1024` on Linux, which reproduces the Windows
budget.

### ESTIM-4 — ACT/252 estimation consumed under ACT/365F understates σ by about 1.7% (Medium)

**Evidence.** `present/PriceHistory.cpp:23` builds the series with `Act252`;
`present/RunController.cpp:59` runs the path generator with `Act365Fixed`. Under ACT/252 the
business-day count ignores holidays, so a real year of closes spans about 260/252 "years" while
holding only about 253 observations; `PriceSeries::PeriodsPerYear` (`PriceSeries.cpp:56-59`) then
reports 245 periods per year and the annualised deviation is s√245. The same closes under
ACT/365F give 254 periods per year, s√254, which is the figure a calendar-time projection needs.
Computed for 2024 with eight Euronext holidays: ratio 0.9831. `theory/day-count-conventions.md`
itself says "Use ACT/252 only when the volatility in hand was itself estimated in trading time",
and decision D14's claim that an average frequency "is the right answer for a series with
holidays in it" holds under ACT/365F but not under ACT/252.

**Impact.** Every projection run from a loaded price history uses a volatility 1.7% below the
calendar-time realised figure (3.4% in variance). Premium income is roughly proportional to σ,
so projected income is about 1.7% low and the simulated paths are correspondingly too calm;
assignment frequencies and drawdowns are slightly understated. Small, but systematic, and in the
optimistic direction on the risk side. The estimation module itself is correct for the convention
it is handed; the mismatch is in the choice made by the presentation layer.

**Proposed fix.** Build loaded histories with `Act365Fixed`, which annualises by the actual number
of closes per calendar year and matches the simulation; or add a holiday calendar to ACT/252; or
carry the series' convention into the path generator so one convention is used end to end.
**Suggested test.** A synthetic daily series with holidays removed, estimated and then re-simulated
through the production convention, must recover its own σ within the sampling error.

### ESTIM-5 — Bare-array estimators and unused estimators (Low)

**Evidence.** `design/estimation.md` says "there is no path by which a bare array of numbers
reaches an estimator without first being made into a history" and that the observation frequency
"is derived, not declared". `HistoricalVolatility::From`, `EmaVolatilityEstimator` and
`RlsVolatilityEstimator` all take `std::span<const double>` and a caller-supplied
`periodsPerYear`. Only `MarketParameters::From` derives the frequency from a series. A search of
non-test code finds one production caller of the module, `present/PriceHistory.cpp`, which uses
`MarketParameters` with the ordinary fit; the exponentially weighted estimator, the recursive
estimator and the orthogonal fit have none.

**Impact.** The production path honours the design rule, so no number is affected. The rule as
written is not what the API enforces, and three documented capabilities are dead code with the
same risk profile as the FX model in phase 2: they are maintained and tested but never exercised
by a projection.

**Proposed fix.** Either give the three estimators an overload that takes a `PriceSeries` and make
the span overloads private to the module, and reword the design note; or state in the design note
that the span forms exist for the gathered-parameters path only. Decide whether the weighted
estimators are meant to feed a projection; if so, that is a feature, if not, say so in the README.

### ESTIM-6 — A documented figure does not follow from its formula (Low)

**Evidence.** `theory/log-returns-and-volatility.md` derives the variance inflation from quoting
closes in whole minor units as 2c²/(12S²) per period and then says that "for an instrument quoted
near 5 units of currency the same calculation gives about 2%". With c = 0.01, S = 5 and a daily
deviation of 1.3% the formula gives 0.39%; simulation with the price held near 5 gives 0.43%. The
figure of 2% corresponds to a price near 2.

**Impact.** Documentation only; the formula, which is what a reader would use, is right.

**Proposed fix.** Replace "near 5 units … about 2%" by "near 2 units … about 2.5%", or "near 5
units … about 0.4%".

### ESTIM-7 — D3 is out of date, in the right direction (Low)

Decision D3 says single precision "appears only inside a call into the external library" and
accepts a documented precision loss inside fits. Every toolbox instantiation in this module is
`<double, …>`, so there is no single-precision step. The decision should record that the library
is used in double and that the accepted loss no longer exists; the `1e-6f` degeneracy threshold in
the toolbox's total-least-squares fit is the only float literal on the path and is harmless.

### ESTIM-8 — The drift estimator holds a reference (Low)

`DriftEstimator` stores `const LineFit& fit` (`DriftEstimator.hpp:18`). `LineFit` is
non-copyable by design, so the reference is the natural shape, but `DriftEstimator` itself is
copyable and can be constructed from a temporary fit, after which it dangles. Every current use
builds the estimator and uses it in one expression (`MarketParameters.cpp:12`). Making
`DriftEstimator` a function taking the fit per call, or deleting its copy, removes the trap.

### ESTIM-9 — The orthogonal fit is the reduced-major-axis regression (Info)

After both axes are scaled to unit length (`LineFit.cpp:43-55`) the two-dimensional cloud has
equal variances, so the total-least-squares direction is ±45° and the rescaled slope is
sign(ρ)·s_y/s_x, which `theory/drift-estimation.md` states as ν/|ρ|. The singular value
decomposition therefore computes a quantity with a one-line closed form. Not a defect; worth
naming in the theory note so a reader knows it is the geometric-mean regression and not a
different estimator.

## Observations carried forward to later phases

| For phase | Where | Observation |
|---|---|---|
| 5 optimize | `optimize/StrikeSizingOptimizer.cpp:3-5` | Same pragma exposure as ESTIM-1; functions defined after the includes run under fast-math. |
| 6 pricing | `pricing/NormalDistribution.cpp:2` | Same pragma exposure; the normal CDF and anything after it in that unit are fast-math on GCC. Check any `isfinite` or NaN refusal there, and the accuracy of `math::Erfc`. |
| 9 sim | `sim/GbmPathGenerator.cpp:3` | Same pragma exposure on the path generator; cross-platform bit-reproducibility of paths is not given on GCC. Also ESTIM-4: the generator runs under ACT/365F. |
| 4 io | `io/PriceCsvLoader.cpp:167` | Minimum of two closes; see ESTIM-2. |

## Open questions for the owner

1. Are the exponentially weighted and recursive estimators meant to drive a projection, or are
   they diagnostics (ESTIM-5)? The answer decides whether their span-based API is a design gap or
   a convenience.
2. Which convention should a loaded history be annualised under (ESTIM-4)? ACT/365F matches the
   simulation and needs no holiday calendar; ACT/252 needs one to be right.
3. Is a Windows build of the desktop application run with the default 1 MB stack (ESTIM-3)?

## Not checked

- The numerical accuracy of the toolbox's QR and SVD on the padded 8 192-row system beyond what
  the tests pin (slopes to 10⁻⁹ on exact lines); a conditioning study was not done.
- Behaviour on series longer than a few thousand points other than the capacity refusal.
- The `volatility-scaling.md` note beyond its first section.

## Appendix A: reference-check script

Run with `python3 -I check_estimation.py`; takes about a minute. Output as produced on
2026-10-19 follows the script.

```python
import datetime as dt
import math
import random
import statistics

# --- worked examples in the theory documents
closes = [100.00, 102.00, 101.00, 104.00]
r = [math.log(closes[i] / closes[i - 1]) for i in range(1, 4)]
mean = sum(r) / 3
ss = sum((x - mean) ** 2 for x in r)
s = math.sqrt(ss / 2)
print(f"log-returns example: r={[round(x, 6) for x in r]} sum={sum(r):.6f} ln(1.04)={math.log(1.04):.6f} s={s:.6f} sigma={s * math.sqrt(252):.4f}")
t = [31 * i / 365 for i in range(5)]
y = [math.log(v) for v in [100.00, 100.85, 101.71, 102.58, 103.46]]
tb, yb = sum(t) / 5, sum(y) / 5
sxy = sum((a - tb) * (b - yb) for a, b in zip(t, y))
sxx = sum((a - tb) ** 2 for a in t)
print(f"drift example: Sxy={sxy:.6e} Sxx={sxx:.6e} slope={sxy / sxx:.6f}")
a = 1 - 2 ** (-1 / 20)
print(f"EMA alpha for half-life 20: {a:.6f}")
for k in [0, 10, 20, 40, 100]:
    v = 4e-4 + (1e-4 - 4e-4) * (1 - a) ** k
    print(f"  k={k}: remaining {(1 - a) ** k:.4f} v={v:.4e} sigma={math.sqrt(v * 252) * 100:.2f}%")
for h in [10, 20, 60, 250]:
    aa = 1 - 2 ** (-1 / h)
    print(f"  half-life {h}: alpha={aa:.5f} n_eff={(2 - aa) / aa:.1f} rel.err sigma={0.5 * math.sqrt(aa / (2 - aa)) * 100:.1f}%")

# --- the two recursive estimators against their closed forms
random.seed(3)
rs = [random.gauss(0, 0.01) for _ in range(300)]
v = rs[0] ** 2
for x in rs[1:]:
    v = v + a * (x * x - v)
n = len(rs) - 1
closed = a * sum((1 - a) ** k * rs[n - k] ** 2 for k in range(n)) + (1 - a) ** n * rs[0] ** 2
print(f"EMA recursion vs closed form: {v:.12e} vs {closed:.12e}")
P, theta = 1e6, 0.0
for x in rs:
    g = P / (1 + P)
    theta += g * (x * x - theta)
    P = P / (1 + P)
print(f"RLS lambda=1, diffuse prior: {theta:.12e} vs running mean {sum(x * x for x in rs) / len(rs):.12e}")
P, theta, lam = 1e6, 0.0, 0.97
for x in rs:
    g = P / (lam + P)
    theta += g * (x * x - theta)
    P = P / (lam + P)
print(f"RLS lambda=0.97: P settles at {P:.6f} (1-lambda={1 - lam:.6f})")

# --- standard error of a fitted log drift on a random walk: sigma*sqrt(6/(5T))
random.seed(7)
sigma, T, steps, paths = 0.20, 10.0, 2520, 2000
dt_ = T / steps
tt = [i * dt_ for i in range(steps + 1)]
tb = sum(tt) / len(tt)
sxx = sum((x - tb) ** 2 for x in tt)
slopes = []
for _ in range(paths):
    yv = [0.0]
    for _ in range(steps):
        yv.append(yv[-1] + sigma * math.sqrt(dt_) * random.gauss(0, 1))
    yb = sum(yv) / len(yv)
    slopes.append(sum((x - w) * (z - yb) for x, z, w in zip(tt, yv, [tb] * len(tt))) / sxx)
print(f"OLS slope sd over {paths} random walks (sigma={sigma}, T={T}y): measured {statistics.pstdev(slopes):.4f}; sigma*sqrt(6/(5T))={sigma * math.sqrt(6 / (5 * T)):.4f}; sigma/sqrt(T)={sigma / math.sqrt(T):.4f}")

# --- bias from quoting closes in whole minor units: 2c^2/(12 S^2) per period, prices held near S
random.seed(11)
for S0 in [100.0, 5.0, 2.0]:
    dvol, ratios = 0.013, []
    for _ in range(400):
        lp = math.log(S0)
        ex, rd = [lp], [math.log(round(S0, 2))]
        for _ in range(60):
            lp += dvol * random.gauss(0, 1)
            ex.append(lp)
            rd.append(math.log(round(math.exp(lp), 2)))
        re = [ex[i] - ex[i - 1] for i in range(1, len(ex))]
        rr = [rd[i] - rd[i - 1] for i in range(1, len(rd))]
        ratios.append(sum(x * x for x in rr) / sum(x * x for x in re))
    print(f"quotation bias at S={S0:.0f}: measured variance inflation {(statistics.mean(ratios) - 1) * 100:.3f}%; formula {2 * 0.01 ** 2 / (12 * S0 ** 2) / dvol ** 2 * 100:.3f}%")

# --- annualising a real daily series under ACT/252 (weekends only) versus ACT/365F
year = [dt.date(2024, 1, 1) + dt.timedelta(days=i) for i in range(366)]
weekdays = [d for d in year if d.weekday() < 5]
holidays = {dt.date(2024, 1, 1), dt.date(2024, 3, 29), dt.date(2024, 4, 1), dt.date(2024, 5, 1), dt.date(2024, 5, 9), dt.date(2024, 5, 20), dt.date(2024, 12, 25), dt.date(2024, 12, 26)}
observed = [d for d in weekdays if d not in holidays]
def business_days(a, b):
    return sum(1 for i in range((b - a).days) if (a + dt.timedelta(days=i)).weekday() < 5)
n = len(observed)
h252 = business_days(observed[0], observed[-1]) / 252
h365 = (observed[-1] - observed[0]).days / 365
print(f"2024, {len(weekdays)} weekdays, {len(holidays)} exchange holidays removed: {n} closes")
print(f"  periods per year under ACT/252: {(n - 1) / h252:.1f}; under ACT/365F: {(n - 1) / h365:.1f}; realised-vol scale factor ACT/252 vs ACT/365F: {math.sqrt(((n - 1) / h252) / ((n - 1) / h365)):.4f}")
```

```text
log-returns example: r=[0.019803, -0.009852, 0.02927] sum=0.039221 ln(1.04)=0.039221 s=0.020411 sigma=0.3240
drift example: Sxy=7.222446e-03 Sxx=7.213361e-02 slope=0.100126
EMA alpha for half-life 20: 0.034064
  k=0: remaining 1.0000 v=1.0000e-04 sigma=15.87%
  k=10: remaining 0.7071 v=1.8787e-04 sigma=21.76%
  k=20: remaining 0.5000 v=2.5000e-04 sigma=25.10%
  k=40: remaining 0.2500 v=3.2500e-04 sigma=28.62%
  k=100: remaining 0.0313 v=3.9062e-04 sigma=31.37%
  half-life 10: alpha=0.06697 n_eff=28.9 rel.err sigma=9.3%
  half-life 20: alpha=0.03406 n_eff=57.7 rel.err sigma=6.6%
  half-life 60: alpha=0.01149 n_eff=173.1 rel.err sigma=3.8%
  half-life 250: alpha=0.00277 n_eff=721.3 rel.err sigma=1.9%
EMA recursion vs closed form: 1.006762108013e-04 vs 1.006762108013e-04
RLS lambda=1, diffuse prior: 1.002517158827e-04 vs running mean 1.002517162169e-04
RLS lambda=0.97: P settles at 0.030003 (1-lambda=0.030000)
OLS slope sd over 2000 random walks (sigma=0.2, T=10.0y): measured 0.0676; sigma*sqrt(6/(5T))=0.0693; sigma/sqrt(T)=0.0632
quotation bias at S=100: measured variance inflation 0.005%; formula 0.001%
quotation bias at S=5: measured variance inflation 0.431%; formula 0.394%
quotation bias at S=2: measured variance inflation 2.543%; formula 2.465%
2024, 262 weekdays, 8 exchange holidays removed: 254 closes
  periods per year under ACT/252: 245.2; under ACT/365F: 253.7; realised-vol scale factor ACT/252 vs ACT/365F: 0.9831
```

## Appendix B: experiments against the built library

Built from the harness in the session scratchpad (`build/` configured as in phase 1; toolbox and
emil sources under `build/_deps/`). `demo.cpp`:

```cpp
#include "source/estimation/EmaVolatilityEstimator.hpp"
#include "source/estimation/HistoricalVolatility.hpp"
#include "source/estimation/MarketParameters.hpp"
#include "source/estimation/OrdinaryLeastSquaresFit.hpp"
#include "source/estimation/RlsVolatilityEstimator.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
using namespace fin;
int main(int argc, char** argv)
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const int which = argc > 1 ? std::atoi(argv[1]) : 0;
    if (which == 1)
    {
        std::vector<core::Date> dates{ core::Date::FromYearMonthDay(2025, 1, 6), core::Date::FromYearMonthDay(2025, 1, 7) };
        std::vector<core::Money> closes{ core::Money::FromCents(10000, core::Currency::Eur), core::Money::FromCents(10100, core::Currency::Eur) };
        const auto series = estimation::PriceSeries::From(dates, closes, core::DayCountConvention::Act252);
        std::printf("two-close series accepted; calling MarketParameters::From ...\n"); std::fflush(stdout);
        const estimation::OrdinaryLeastSquaresFit fit;
        const auto p = estimation::MarketParameters::From(series, core::Rate::Zero(), fit);
        std::printf("returned: has_value=%d\n", int(p.has_value()));
    }
    if (which == 2)
    {
        const auto e = estimation::EmaVolatilityEstimator::WithSmoothingFactor(nan, 252.0);
        std::printf("EmaVolatilityEstimator::WithSmoothingFactor(NaN) ACCEPTED, SmoothingFactor()=%g\n", e.SmoothingFactor());
    }
    if (which == 3)
    {
        const auto e = estimation::RlsVolatilityEstimator::WithForgettingFactor(nan, 252.0);
        std::printf("RlsVolatilityEstimator::WithForgettingFactor(NaN) ACCEPTED, ForgettingFactor()=%g\n", e.ForgettingFactor());
    }
    if (which == 4)
    {
        const std::vector<double> r{ 0.01, -0.01, 0.02 };
        const auto v = estimation::HistoricalVolatility::From(r, nan);
        std::printf("HistoricalVolatility::From(returns, NaN periods) ACCEPTED, %g\n", v.Annualized());
    }
    if (which == 5)
    {
        const auto e = estimation::EmaVolatilityEstimator::WithSmoothingFactor(0.1, nan);
        std::printf("EmaVolatilityEstimator::WithSmoothingFactor(0.1, NaN periods) ACCEPTED\n"); (void)e;
    }
    return 0;
}
```

```text
$ g++ -std=c++20 -O0 -I<fin-bs> -I<toolbox> -I<emil> -o demo demo.cpp \
      build/source/estimation/libfin.estimation.a build/_deps/emil-build/infra/util/libinfra.util.a
$ ./demo 1
two-close series accepted; calling MarketParameters::From ...
demo: source/estimation/HistoricalVolatility.cpp:9: ... Assertion `logReturns.size() >= 2' failed.
$ ./demo 2   # Debug library
demo: source/estimation/EmaVolatilityEstimator.cpp:58: ... Assertion `smoothingFactor > 0.0 && smoothingFactor <= 1.0' failed.
$ ./demo 3   # Debug library
demo: source/estimation/RlsVolatilityEstimator.cpp:77: ... Assertion `forgettingFactor > 0.0 && forgettingFactor <= 1.0' failed.
$ ./demo 4   # Debug library
demo: source/estimation/HistoricalVolatility.cpp:10: ... Assertion `periodsPerYear > 0.0' failed.
$ ./demo 5   # Debug library
demo: source/estimation/EmaVolatilityEstimator.cpp:59: ... Assertion `periodsPerYear > 0.0' failed.

$ g++ -std=c++20 -O2 -DNDEBUG -I... -o demo_rel demo.cpp source/estimation/*.cpp
$ ./demo_rel 2
EmaVolatilityEstimator::WithSmoothingFactor(NaN) ACCEPTED, SmoothingFactor()=nan
$ ./demo_rel 3
RlsVolatilityEstimator::WithForgettingFactor(NaN) ACCEPTED, ForgettingFactor()=nan
$ ./demo_rel 5
EmaVolatilityEstimator::WithSmoothingFactor(0.1, NaN periods) ACCEPTED
$ ./demo_rel 4            # unit without a toolbox include: aborts silently (NDEBUG form), nothing printed

$ sed 's|#include "numerical/filters/passive/ExponentialMovingAverage.hpp"|#pragma GCC push_options\n&\n#pragma GCC pop_options|' \
      source/estimation/EmaVolatilityEstimator.cpp > Ema_patched.cpp
$ g++ -std=c++20 -O2 -DNDEBUG -I... -o demo_rel_patched demo.cpp Ema_patched.cpp <other estimation .cpp>
$ ./demo_rel_patched 2    # aborts silently: refused

$ g++ -std=c++20 -O2 -DNDEBUG -fno-fast-math -I... -o demo_rel_nofm demo.cpp source/estimation/*.cpp
$ ./demo_rel_nofm 2
EmaVolatilityEstimator::WithSmoothingFactor(NaN) ACCEPTED, SmoothingFactor()=nan

$ g++ -std=c++20 -O2 -DNDEBUG -I... -S -o - source/estimation/EmaVolatilityEstimator.cpp \
      | awk '/EmaVolatilityEstimatorC2Edd:/,/cfi_endproc/' | grep -E 'comisd|jp|ja'
	comisd	%xmm0, %xmm2
	comisd	.LC7(%rip), %xmm0
	ja	.L75
	comisd	%xmm1, %xmm2

$ for kb in 8192 2048 1024 768 512; do (ulimit -s $kb; \
      build/source/estimation/test/fin.estimation_test --gtest_filter='TestLineFit.every_fit_recovers*:TestDriftEstimator.a_known*' | tail -1); done
stack 8192 KB: [  PASSED  ] 2 tests.
stack 2048 KB: [  PASSED  ] 2 tests.
stack 1024 KB: [  PASSED  ] 2 tests.
stack  768 KB: [  PASSED  ] 2 tests.
stack  512 KB: crashed during TestDriftEstimator.a_known_drift_is_recovered_from_a_synthetic_price_series

$ g++ -std=c++20 -O0 -fstack-usage -I... -c source/estimation/OrdinaryLeastSquaresFit.cpp
$ g++ -std=c++20 -O0 -fstack-usage -I... -c source/estimation/TotalLeastSquaresFit.cpp
SingularValueDecomposition<double,8192,2>::Decompose   262288 static
TotalLeastSquares<double,8192,1>::Fit                   262288 static
LinearRegression<double,8192,1>::Fit                    262240 static
OrdinaryLeastSquaresFit::StandardisedSlope              131216 static
QrDecomposition<double,8192,2>::Decompose               131200 static
TotalLeastSquaresFit::StandardisedSlope                 131200 static
SingularValueDecomposition<double,8192,2>::Bidiagonalize 65680 static
QrDecomposition<double,8192,2>::ApplyQtranspose          65664 static
```
