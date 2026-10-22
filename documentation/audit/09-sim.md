# Audit phase 9: `source/sim`

*Date: 2026-10-19. Scope: the seeded random stream, the price-path generator, the single-wheel and
book scripts, the two Monte-Carlo drivers and the outcome distribution in `source/sim`, their eight
test files, the simulation contract, the Monte-Carlo theory note and decisions D30, D31, D35 and
D45. Report only.*

## Summary

The random stream and the path law are exactly what the documents say. An independent Python
implementation of the C++ standard's 64-bit Mersenne Twister and of the project's Box–Muller
transform reproduces the library's first eight normals bit for bit, and a quarterly path from the
exact log-increment law reproduces the library's closes to the cent, so the Runge–Kutta step is
the plain product it is documented to be. Every figure in the theory note's worked example and
its standard-error table reproduces, the replica's mean of 100,000 terminal prices sits 0.01
standard errors from the closed form, the percentile rule is the stated rank rule, and the
fast-math pragma the generator inherits changes none of 700,000 closes on this toolchain. Common
random numbers hold across the optimiser's candidates, because the number of draws a path consumes
depends on the schedule and not on the setting. All 61 tests pass.

The High finding is the one phase 7 predicted, now seen in production terms: the shipped example
scenario, simulated at 10,000 paths with its own seed, aborts the process at path 3,470 on a
wealth-tax charge the book cannot pay, and the module's own two-name test configuration aborts at
path 236 of 256 (SIM-1). The run has no notion of a path that fails; "a path can stop" in the
theory note is implemented as the application stopping. Three Medium findings: the assignment
frequency divides by cycles times underlyings, which counts cycles in which a wheel wrote nothing,
so the statistic the optimiser constrains is understated whenever a name is crowded out (SIM-2);
the premium is the option's fair value at the path's own volatility with no spread, so the
simulated strategy has, by construction, no option-selling edge and no dealing cost, and the limits
table does not say so (SIM-3); and the worst drawdown is the maximum over however many paths were
run, a figure that grows with the path count and is used as the optimiser's risk limit (SIM-4).

| Severity | Count |
|---|---|
| Critical | 0 |
| High | 1 |
| Medium | 3 |
| Low | 6 |
| Info | 3 |

## What was executed

**Build route.** Same harness as phase 1 (scratchpad CMake project, GCC 13.3.0, Ninja, Debug),
executed 2026-10-19.

```text
$ cmake --build build --target fin.sim_test
$ (cd build && ctest -R fin.sim_test --output-on-failure)
1/1 Test #7: fin.sim_test .....................   Passed    0.32 sec
100% tests passed, 0 tests failed out of 1
$ build/source/sim/test/fin.sim_test
[==========] Running 61 tests from 8 test suites.
[==========] 61 tests from 8 test suites ran. (343 ms total)
[  PASSED  ] 61 tests.
```

Experiments against the built library (appendix B):

| # | Experiment | Expected from the reading | Observed |
|---|---|---|---|
| E1 | first eight normals for seed 20250811, three uniforms for seed 7, a quarterly path for seed 1 | equal to the Python replica | normals identical to 17 digits; uniforms identical (printed in reverse by the demo's argument order); closes identical to the cent |
| E2 | book of ACME at 40.00 and BIG at 950.00 on 50,000.00: BIG can never write | `positions` counts BIG's cycles | 12 positions for 6 options written; frequency halved |
| E3 | the module's two-name test configuration at 64, 256, 1,024 and 4,096 paths | worst drawdown grows with the count | 64 paths: 0.357; 256 paths: abort at path 236 in `Book::Assess` → `Portfolio::Pay` |
| E4 | one name, four 182-day cycles, started 2026-01-01 and 2026-01-02 | the opening-day reference date is not struck | both runs strike only 2027-01-01; the 2026 charge is never paid |
| E5 | σ 100%, 365-day cycle, target delta 0.05, spot 40.00, holding shares | call search bound of 8 × spot fails | abort at `WheelScript.cpp:156`; the put at 13.00 was fine |
| E6 | S0 0.02, σ 100%, one ten-year step | a close rounds to nothing | abort at `PriceSeries.cpp:18` on the first path |
| E7 | the shipped example scenario's terms, 256, 2,000 and 10,000 paths, seed 20250811 | survives or aborts | 256 and 2,000 complete (median 211,564.12 / 213,336.80; worst drawdown 0.505 / 0.574); 10,000 aborts at path 3,470 |
| F | 100,000 seven-point paths from the generator compiled as shipped and with the toolbox pragma neutralised, both at -O2 | identical or not | 0 of 700,000 closes differ; neither build emits fused multiply-adds |

The first attempt at F compared garbage: my harness iterated a range-for over the span returned by
a temporary path, a dangling view. The library has no such pattern (its range-fors bind returned
vectors, which live for the loop); the corrected harness is in appendix B.

Files read in full: the seven source and eight header files, the eight test files, the two
`CMakeLists.txt`, `design/simulation.md`, `theory/monte-carlo-simulation.md`, decisions D30, D31,
D35, D45, the head of the toolbox's `RungeKuttaIntegrators.hpp`, `estimation/PriceSeries.cpp:7-21`,
and the callers `optimize/StrategyObjective.cpp`, `optimize/RiskLimits.cpp`,
`present/RunController.cpp` (simulation and search paths), `present/ResultTable.cpp:120-126` and
`present/FanChartSeries.cpp:80-95`.

## What was verified independently

The scripts in appendix A produced every figure below.

| Check | Method | Result |
|---|---|---|
| The random stream (D30) | independent `mt19937_64` and Box–Muller in Python vs `Rng` | first eight normals for seed 20250811 identical to 17 significant digits |
| The path law (D31) | `log S + νh + σ√h Z` with the replica's draws vs `GbmPathGenerator` | 10000, 11562, 13655, 15718, 16226 cents: identical |
| Worked example (`monte-carlo-simulation.md`) | closed forms | ν 0.0600, expected 108.33, median 106.18, sd 21.88, log variance 0.0400, gap 2.15 |
| Standard errors and bands | sd/√N | 0.692 / 2.076, 0.219 / 0.657, 0.069 / 0.208 |
| Convergence (the test's claim) | replica mean of 100,000 one-step prices | 108.3284 vs 108.3287, 0.01 standard errors |
| Percentile rank rule (D35) | ⌈pN/100⌉ on three outcomes | ranks 1, 1, 2, 3 for 1, 33, 34, 67 |
| Common random numbers across settings | draws per path = (schedule − 1) × underlyings; schedule from cycles, observation and reference date only | a candidate's target delta, buffer and roll limit cannot change the draws; CRN holds (resolves the phase 5 note) |
| Fast-math pragma (ESTIM-1 carry-forward) | experiment F | no observable effect on closes here |
| Rounding of a close (phase 1 carry-forward) | `GbmPathGenerator.cpp:61-64` | once per close; the log price continues unrounded |
| Drawdown and equity read the book's last row (STRAT-3) | `BookMonteCarlo.cpp:97-107` | yes; the row loop leaves the last underlying's row, which is the book's |
| Per-path ledger (STRAT-14) | `BookMonteCarlo.cpp:79` | discarded with the path; the equity series is retained (SIM-9) |
| Price on the reference date (TAX-9) | `BookScript.cpp:119` | from the schedule; the date is on it |

## Findings

| ID | Severity | Where | Title |
|---|---|---|---|
| SIM-1 | High | `BookMonteCarlo.cpp:71-108`; `strategy/Book.cpp:86`; `strategy/Portfolio.cpp:152` | An insolvent path aborts the whole run; the shipped example reaches one before 3,500 paths |
| SIM-2 | Medium | `BookMonteCarlo.cpp:112`; `OutcomeDistribution.cpp:96-108`; D46 | The assignment frequency counts cycles in which nothing was written |
| SIM-3 | Medium | `WheelScript.cpp:84, 94`; `theory/methodology.md` limits | The premium is fair value at the path's own volatility, with no spread; the consequence is not stated |
| SIM-4 | Medium | `OutcomeDistribution.cpp:87-94`; `optimize/RiskLimits.cpp:20`; `present/ResultTable.cpp:124` | The worst drawdown is the maximum over the paths run, a figure that grows with the path count and is used as a limit |
| SIM-5 | Low | `BookMonteCarlo.cpp:105-106`; D31 | Drawdown is measured at cycle boundaries only; not in the limits table |
| SIM-6 | Low | `BookScript.cpp:141`, `WheelScript.cpp:134`; `design/simulation.md:106-108` | A run started on the reference date never pays that year's charge |
| SIM-7 | Low | `WheelScript.cpp:145, 153-157` | The strike search's bounds are assertions, reachable at high volatility or a low target |
| SIM-8 | Low | `GbmPathGenerator.cpp:64`; `estimation/PriceSeries.cpp:18` | A simulated close that rounds to nothing aborts the run |
| SIM-9 | Low | `OutcomeDistribution.hpp:17`; `BookMonteCarlo.cpp:89, 107` | Every path's equity series is retained; 16 GB at the scenario bounds |
| SIM-10 | Low | `test/TestRng.cpp`, `test/TestGbmPathGenerator.cpp` | No test pins the stream's values; a libm or compiler change would pass every test and change every result |
| SIM-11 | Info | `GbmPathGenerator.cpp:3` | The toolbox pragma applies to the generator; no effect measured here |
| SIM-12 | Info | `MonteCarlo.cpp`; `present/RunController.cpp` | The single-wheel driver has no production caller |
| SIM-13 | Info | `GbmPathGenerator.cpp:56`; `WheelScript.cpp:12`; ESTIM-4 | ACT/365F throughout the simulation; the estimator's ACT/252 is the one mismatch |

### SIM-1 — An insolvent path aborts the whole run (High)

**Evidence.** `BookMonteCarlo::Once` runs the book over a path (`BookMonteCarlo.cpp:71-108`) with no
handling for a path on which a payment cannot be made; the assertion in `Portfolio::Pay`
(`strategy/Portfolio.cpp:152`) ends the process. The theory note says "a path can stop ... the
cash buffer is the lever against it"; the contract says "a buffer of a few percent of capital
covers an annual wealth charge on the same capital with room to spare". Experiment E7 builds the
terms of `config/scenarios/example.json` (three names, 200,000.00, 5% buffer, 182-day cycles,
target delta 0.30, roll limit 0.50, assessed on 1 January) and simulates with the scenario's own
seed: 256 and 2,000 paths complete; 10,000 paths abort at path 3,470 in `Book::Assess`. E3 runs the
module's own two-name test configuration (5% buffer): 64 paths complete, 256 abort at path 236.
The test `no_path_runs_out_of_the_cash_it_needs_to_settle_what_it_owes` covers a single wheel at
256 paths over 40 seeds; the book tests stop at 64 paths.

**Impact.** The scenario file permits up to 1,000,000 paths. A user who raises the example's path
count to 10,000, a modest number for a distribution whose 5th percentile they intend to read, sees
the application die, in a Release build without a message (CORE-6). A search (OPT-1) evaluates
hundreds of settings, including smaller buffers; its first insolvent path ends the search. The
mechanism and the strategy-layer fix are STRAT-1; what belongs here is that the simulation has no
outcome for a failed path. The phase 7 break-even applies: a book fully assigned at the boundary
before 1 January holds the remainder of its sizing plus one cycle's premium against a charge of
about 2.1% of the share value above the allowance.

**Proposed fix.** Give `PathOutcome` a failure state, make the strategy layer report an unpayable
charge instead of asserting (STRAT-1's options: sell shares, or stop the path), and have
`OutcomeDistribution` carry the count of insolvent paths as a reported figure and a risk limit the
optimiser can constrain. **Suggested tests.** E7 at 10,000 paths completes and reports its
insolvent paths; a book test at 256 paths.

### SIM-2 — The assignment frequency counts cycles in which nothing was written (Medium)

**Evidence.** `positions = script.Cycles() × underlyings.size()` (`BookMonteCarlo.cpp:112`) and the
frequency divides assignments by it (`OutcomeDistribution.cpp:96-108`). D46 defines the
denominator as "the number of options written over the run - for a book, the number of cycles
times the number of underlyings"; the two are equal only if every wheel writes every cycle. The
service rule (D43) routinely leaves the poorest-yield name with nothing: AEX writes nothing in any
of the six cycles of the shipped golden projection. E2: a book whose second name can never write
reports 12 positions for 6 options written, and a frequency half the true one.

**Impact.** The figure the result table labels "assignments per option written"
(`present/ResultTable.cpp:125`) is not that, and the optimiser's assignment limit
(`optimize/RiskLimits.cpp:21`, OPT-2) is met more easily than the strategy deserves, more so the
more names are crowded out. A reader comparing two books of different sizes, which D46 says the
measure is for, is comparing different denominators.

**Proposed fix.** Count a position when a row's contracts are positive, in both drivers, and amend
D46. **Suggested test.** E2's book reports 6 positions.

### SIM-3 — Fair-value premiums at the path's volatility, no spread (Medium)

**Evidence.** The premium is the Black–Scholes price of the option at `terms.volatility`
(`WheelScript.cpp:84`), the same volatility that generated the path, and the close value at the
observation is the same model (`:94`). There is no bid–ask, no implied volatility distinct from
the realised one, and no risk premium. The contract states it as a guarantee ("the premium is what
the model says the option is worth"); the limits table in `theory/methodology.md` lists constant
volatility and no jumps, not this.

**Impact.** Under this model the option is sold for exactly its expected cost, so the simulated
strategy earns the drift exposure of its positions and nothing from option selling as such; what a
wheel earns in practice, if anything, is the gap between implied and realised volatility, which
the model sets to zero. In the other direction, an option on a mid-cap name is sold at the bid,
some percent below mid, which the model ignores. The two errors have opposite signs and neither is
small; the example's mean growth of about 1% a year (E7) is what fair-value selling of a 30-delta
put on these names looks like.

**Proposed fix.** Separate the pricing volatility from the path volatility in the scenario (an
implied-volatility term per underlying, or a spread in basis points of premium), and add the
assumption to the limits table until then. **Suggested test.** A scenario with pricing volatility
above path volatility earns more than the same with equal volatilities.

### SIM-4 — The worst drawdown grows with the path count and is used as a limit (Medium)

**Evidence.** `WorstDrawdown` is the maximum of the per-path maximum drawdowns
(`OutcomeDistribution.cpp:87-94`). It is reported (`present/ResultTable.cpp:124`) and is the figure
the optimiser's drawdown limit binds on (`optimize/RiskLimits.cpp:20`). E3 and E7: 0.357 at 64
paths, 0.505 at 256, 0.574 at 2,000, for the same seed and strategy.

**Impact.** The maximum of N draws from a fixed distribution rises with N without bound, so the
same strategy passes a 50% drawdown limit at 256 paths and fails it at 2,000; a search run at a
different path count recommends a different setting. The percentile figures beside it are stable
in N by construction (D35).

**Proposed fix.** Report and constrain a percentile of the drawdown distribution (the drawdown of
the path at the 95th or 99th percentile of drawdown, which is an outcome that occurred, in the
spirit of D35), and keep the maximum as an informational figure labelled with the path count.
**Suggested test.** The constrained figure is the same to within sampling error at 256 and 4,096
paths.

### SIM-5 — Drawdown at cycle boundaries only (Low)

The drawdown is updated once per cycle from the book's equity after settlement
(`BookMonteCarlo.cpp:105-106`). A put written for 182 days that is deep in the money at its
observation and recovers by expiry shows no drawdown at all. D31 states the cost; the limits table
does not, and the optimiser's drawdown limit is a limit on this coarse series. Worth a row in the
table, and an owner decision on whether a mark at the observation date (the path already carries
a price there, and the position's value is computed there) should enter the series.

### SIM-6 — A run started on the reference date never pays that year's charge (Low)

`ReferenceDateWithin` takes a date in the half-open interval after the cycle's opening day
(`BookScript.cpp:141`, `WheelScript.cpp:134`), and the contract records that "a reference date
falling on the day a projection opens is not struck". E4: a run from 2026-01-01 over four
half-year cycles pays one charge, dated 2027-01-01, for two years of holding. The investor holds
the capital on 1 January 2026 and owes tax on it; starting the projection on that day loses one
annual charge, about 2,000.00 to 3,000.00 on 200,000.00. Including the opening day in the first
cycle only (`on >= opened` when `cycle == 0`) keeps consecutive cycles disjoint. **Suggested test.**
A run from 2026-01-01 is assessed on 2026-01-01.

### SIM-7 — The strike search's bounds are assertions (Low)

`StrikeFor` asserts that one increment is within the target and eight times the spot is not, in
the appropriate direction (`WheelScript.cpp:153-157`), and that the spot is at least one increment
(`:145`). E5: at 100% volatility over a 365-day cycle with a target delta of 0.05, a call at eight
times the spot still has delta 0.060, and the search aborts; the loader admits volatility to
1,000% and a target of one basis point, so the combination is expressible. A path that falls
below one strike increment (a 5.00 grid on a name that has collapsed) trips the other assertion.
Both should be refusals at the boundary or a wider, computed bound (a strike at which the delta is
certainly below any target, from the volatility and horizon). **Suggested test.** E5 refused by
name, or succeeding.

### SIM-8 — A close that rounds to nothing aborts the run (Low)

`FromAmount(exp(logPrice))` (`GbmPathGenerator.cpp:64`) can round to zero cents, and
`PriceSeries::From` refuses a non-positive close (`PriceSeries.cpp:18`). E6: at 0.02 with 100%
volatility over ten years, the first path aborts; the replica puts the probability at 84% per path
for that case, 4% for 0.50 at 60% volatility, and nil for any name priced in whole euros. The
theory note says a single-minor-unit underlying is "not a case this project models", but the
loader admits a spot of one cent. Refuse a spot below, say, 1.00 at the boundary, or floor the
simulated close at one cent and count the event.

### SIM-9 — Every path's equity series is retained (Low)

`PathOutcome::equityByCycle` holds one `Money` per cycle boundary for every path
(`BookMonteCarlo.cpp:89, 107`), consumed by the fan chart (`present/FanChartSeries.cpp:93`). At the
loader's bounds of 1,000,000 paths and 1,000 cycles that is 16 GB of amounts before vector
overhead; 100,000 paths over 1,000 cycles is 1.6 GB. The failure is an allocation error, not a
refusal. Keep the series only for the paths the chart draws (the percentile paths, chosen after
sorting), or bound paths × cycles at the boundary.

### SIM-10 — No test pins the stream's values (Low)

The stream tests check mean, variance, range, pairing and seed sensitivity; none asserts a
value. D30 exists so that "a seed identifies a run exactly, in minor units, on any platform that
has the same arithmetic"; a change of libm (the `log`, `sin` and `cos` of Box–Muller), of the
engine's seeding, or of the compiler's treatment of the generator TU would pass every current test
and silently change every published figure. **Suggested tests.** The eight normals and the five
closes of E1 as exact expectations, with a comment that a failure means results are no longer
comparable with earlier runs.

### SIM-11 — The toolbox pragma applies to the generator (Info)

`GbmPathGenerator.cpp:3` includes the Runge–Kutta header, whose first line turns on `O3` and
`fast-math` for the rest of the translation unit (ESTIM-1), including the inlined `FromAmount` and
its finiteness check. Experiment F finds no difference in 700,000 closes between the shipped build
and one with the pragma neutralised, and neither emits fused multiply-adds, so there is no
observable effect on this toolchain. The push/pop wrap recommended in phase 3 is still the right
hygiene, and the integrator is, as D31 says, a constant-drift product with a general name.

### SIM-12 — The single-wheel driver has no production caller (Info)

`MonteCarlo` and the in-cycle assessment path it exercises are reached only by tests;
`present/RunController.cpp` builds a `Book` for one name as for many. Either retire it or state
that it exists for the hand-checkable single-name case.

### SIM-13 — Day-count consistency (Info)

The generator, the script's time to expiry and the horizon all use ACT/365F
(`GbmPathGenerator.cpp:56`, `WheelScript.cpp:12`, `present/RunController.cpp:59`,
`optimize/StrategyObjective.cpp:24`); the only mismatch in the chain is the estimator's ACT/252
(ESTIM-4), which makes a volatility fitted from a history and fed to this generator about 1.7% low.

## Observations resolved from earlier phases

| From | Observation | Resolution |
|---|---|---|
| Phase 1 | `FromAmount(exp(logPrice))` rounds once per close and is not fed back | confirmed |
| Phase 5 | common random numbers across candidate settings | confirmed: draws per path depend on the schedule only |
| Phase 7 | which row's equity the drawdown reads | the book's last row; correct |
| Phase 7 | a per-path ledger's memory | discarded with the path; the equity series is what is retained (SIM-9) |
| Phase 7 | one observation per cycle; the idle gap after a roll | as described in STRAT-4 and STRAT-7; nothing in this layer changes it |
| Phase 7 | the drift is a price drift and no dividend is paid | the path has no ex-date drop; a dividend leg (STRAT-2) must add one |
| Phase 8 | price on the reference date | taken from the schedule, which carries the date |

## Open questions for the owner

1. How many paths do you intend to run? At the example's settings the run dies between 2,000
   and 10,000 (SIM-1). Until the strategy layer reports an unpayable charge, a larger buffer is the
   only lever.
2. Should the pricing volatility be a separate input from the path volatility (SIM-3)? The answer
   decides whether the simulation can show any option-selling edge at all.
3. Which drawdown figure should the search constrain: the worst path, or a percentile (SIM-4)?

## Not checked

- Run time at large path counts; the 10,000-path run was not timed.
- The fan chart and result table in `present` and `gui` beyond the lines cited.
- The quality of `mt19937_64` as a source of uniforms beyond the tests' mean and variance checks.
- Whether the volatility risk premium or the bid–ask spread of Euronext single-name options is
  of the size assumed in SIM-3; both are empirical and outside the code.

## Appendix A: reference-check script

Run with `python3 -I check_sim.py`. Output as produced on 2026-10-19 follows the script.

```python
import math, datetime as dt

M64 = (1 << 64) - 1
class MT19937_64:  # the C++ standard's mt19937_64, written out independently
    def __init__(self, seed):
        self.mt = [0] * 312; self.i = 312
        self.mt[0] = seed & M64
        for k in range(1, 312):
            self.mt[k] = (6364136223846793005 * (self.mt[k - 1] ^ (self.mt[k - 1] >> 62)) + k) & M64
    def __call__(self):
        if self.i >= 312:
            for k in range(312):
                x = (self.mt[k] & 0xFFFFFFFF80000000) | (self.mt[(k + 1) % 312] & 0x7FFFFFFF)
                xa = x >> 1
                if x & 1: xa ^= 0xB5026F5AA96619E9
                self.mt[k] = self.mt[(k + 156) % 312] ^ xa
            self.i = 0
        y = self.mt[self.i]; self.i += 1
        y ^= (y >> 29) & 0x5555555555555555
        y ^= (y << 17) & 0x71D67FFFEDA60000
        y ^= (y << 37) & 0xFFF7EEE000000000
        y ^= y >> 43
        return y & M64

class Rng:  # sim/Rng.cpp
    def __init__(self, seed):
        self.e = MT19937_64(seed); self.spare = None
    def uniform(self):
        return ((self.e() >> 11) + 0.5) * 2.0 ** -53
    def normal(self):
        if self.spare is not None:
            z, self.spare = self.spare, None; return z
        r = math.sqrt(-2.0 * math.log(self.uniform())); a = 2.0 * math.pi * self.uniform()
        self.spare = r * math.sin(a)
        return r * math.cos(a)

def rhe(v):
    lower = math.floor(v); frac = v - lower; r = int(lower)
    if frac > 0.5 or (frac == 0.5 and r % 2 != 0): r += 1
    return r

# --- 1. the stream, for comparison with the library (appendix B, E1)
rng = Rng(20250811)
print("first eight normals for seed 20250811:", " ".join(f"{rng.normal():.17g}" for _ in range(8)))
u = Rng(7)
print("first three uniforms for seed 7:", " ".join(f"{u.uniform():.17g}" for _ in range(3)))

# --- 2. a path from the exact law with the same draws (compare with E1)
rng = Rng(1)
S0, mu, sigma = 100.0, 0.08, 0.20
nu = mu - 0.5 * sigma * sigma
schedule = [dt.date(2025, 1, 2), dt.date(2025, 4, 2), dt.date(2025, 7, 2), dt.date(2025, 10, 2), dt.date(2026, 1, 2)]
logS = math.log(S0); closes = [10000]
for a, b in zip(schedule, schedule[1:]):
    h = (b - a).days / 365.0
    logS += nu * h + sigma * math.sqrt(h) * rng.normal()
    closes.append(rhe(math.exp(logS) * 100.0))
print("path for seed 1, S0 100, mu 8%, sigma 20%, ACT/365F, quarterly:", closes, "cents")

# --- 3. the theory note's worked example and bands
H = 1.0
print(f"log drift {nu:.4f}; expected price {S0 * math.exp(mu * H):.2f}; median {S0 * math.exp(nu * H):.2f}; sd {S0 * math.exp(mu * H) * math.sqrt(math.expm1(sigma * sigma * H)):.2f}; log variance {sigma * sigma * H:.4f}; gap {S0 * math.exp(mu * H) - S0 * math.exp(nu * H):.2f}")
for N in (1000, 10000, 100000):
    se = S0 * math.exp(mu * H) * math.sqrt(math.expm1(sigma * sigma * H)) / math.sqrt(N)
    print(f"  {N} paths: standard error {se:.3f}, three-sigma band {3 * se:.3f}")
print("percentile ranks on three outcomes:", {p: math.ceil(p * 3 / 100) for p in (1, 33, 34, 67, 100)})

# --- 4. the mean of 100,000 one-step terminal prices from the replica (the convergence test's check)
rng = Rng(20250811); total = 0.0; N = 100000
for _ in range(N):
    total += rhe(math.exp(math.log(S0) + nu + sigma * rng.normal()) * 100.0) / 100.0
se = S0 * math.exp(mu) * math.sqrt(math.expm1(sigma * sigma)) / math.sqrt(N)
print(f"replica mean of {N} terminal prices: {total / N:.4f} vs {S0 * math.exp(mu):.4f}, {abs(total / N - S0 * math.exp(mu)) / se:.2f} standard errors")

# --- 5. the strike search's outer bound: call delta at eight times the spot
def N_(x): return 0.5 * math.erfc(-x / math.sqrt(2))
def call_delta(S, K, r, q, s, T):
    d1 = (math.log(S / K) + (r - q + s * s / 2) * T) / (s * math.sqrt(T)); return math.exp(-q * T) * N_(d1)
for s, T in [(1.00, 1.0), (0.60, 1.0), (0.60, 0.5), (0.34, 0.5), (0.80, 1.0)]:
    print(f"  call delta at K = 8 S, sigma {s:.2f}, T {T}: {call_delta(40, 320, 0.025, 0, s, T):.4f}")

# --- 6. memory of retained equity series at the scenario bounds
for paths, cycles in [(256, 6), (10000, 120), (100000, 1000), (1000000, 1000)]:
    print(f"  {paths} paths x {cycles + 1} cycle equities: {paths * (cycles + 1) * 16 / 1e9:.2f} GB of Money alone")

# --- 7. a close that rounds to nothing: probability for a cheap, volatile underlying over one step
for S0_, s, T in [(0.02, 1.0, 10.0), (0.50, 0.6, 10.0), (1.00, 0.8, 5.0), (32.00, 0.26, 3.0)]:
    nu_ = 0.05 - 0.5 * s * s
    z = (math.log(0.005 / S0_) - nu_ * T) / (s * math.sqrt(T))
    print(f"  S0 {S0_:.2f}, sigma {s:.2f}, {T:.0f} years in one step: P(close < 0.005) = {N_(z):.4f}")
```

```text
first eight normals for seed 20250811: 0.20555135791986839 -0.37569432986146745 0.16732616243353135 -0.7276489592281139 -0.096171754069307613 -1.4051657831416127 -0.086858928211284089 0.94558564913912102
first three uniforms for seed 7: 0.75438530415285809 0.94930120289264419 0.11741428103451806
path for seed 1, S0 100, mu 8%, sigma 20%, ACT/365F, quarterly: [10000, 11562, 13655, 15718, 16226] cents
log drift 0.0600; expected price 108.33; median 106.18; sd 21.88; log variance 0.0400; gap 2.15
  1000 paths: standard error 0.692, three-sigma band 2.076
  10000 paths: standard error 0.219, three-sigma band 0.657
  100000 paths: standard error 0.069, three-sigma band 0.208
percentile ranks on three outcomes: {1: 1, 33: 1, 34: 2, 67: 3, 100: 3}
replica mean of 100000 terminal prices: 108.3284 vs 108.3287, 0.01 standard errors
  call delta at K = 8 S, sigma 1.00, T 1.0: 0.0600
  call delta at K = 8 S, sigma 0.60, T 1.0: 0.0009
  call delta at K = 8 S, sigma 0.60, T 0.5: 0.0000
  call delta at K = 8 S, sigma 0.34, T 0.5: 0.0000
  call delta at K = 8 S, sigma 0.80, T 1.0: 0.0151
  256 paths x 7 cycle equities: 0.00 GB of Money alone
  10000 paths x 121 cycle equities: 0.02 GB of Money alone
  100000 paths x 1001 cycle equities: 1.60 GB of Money alone
  1000000 paths x 1001 cycle equities: 16.02 GB of Money alone
  S0 0.02, sigma 1.00, 10 years in one step: P(close < 0.005) = 0.8376
  S0 0.50, sigma 0.60, 10 years in one step: P(close < 0.005) = 0.0408
  S0 1.00, sigma 0.80, 5 years in one step: P(close < 0.005) = 0.0137
  S0 32.00, sigma 0.26, 3 years in one step: P(close < 0.005) = 0.0000
```

## Appendix B: experiments against the built library

Compiled with `g++ -std=c++20 -O1 -g` against the harness archives and run one experiment per
process with `stdbuf -o0`; progress lines and shell job-control lines are omitted. The three
uniforms in E1 are printed in reverse because the demo evaluates printf's arguments right to left;
the values are the replica's. Exit status 134 is `SIGABRT`.

```cpp
// Phase 9 experiments. One per invocation: E1 stream and path fixture, E2 assignment denominator,
// E3 worst drawdown against path count, E4 reference date on the opening day, E5 strike bound,
// E6 a close that rounds to nothing.
#include "source/sim/BookMonteCarlo.hpp"
#include "source/sim/GbmPathGenerator.hpp"
#include "source/sim/Rng.hpp"
#include "source/sim/WheelScript.hpp"
#include "source/sim/RunObserver.hpp"
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

using namespace fin;
using core::Currency;
using core::Date;
using core::DayCountConvention;
using core::Delta;
using core::Money;
using core::Rate;
using core::Volatility;

static Money Eur(double a) { return Money::FromAmount(a, Currency::Eur); }
static Date On(int y, unsigned m, unsigned d) { return Date::FromYearMonthDay(y, m, d); }
static std::string T(const Money& m) { char b[64]; std::snprintf(b, sizeof b, "%.2f", m.ToDouble()); return b; }
static std::string D(const Date& d) { char b[32]; std::snprintf(b, sizeof b, "%d-%02u-%02u", d.Year(), d.Month(), d.Day()); return b; }

static costs::BrokerFees Fees()
{
    return costs::BrokerFees(costs::FeeSchedule{ Eur(0.75), Eur(2.00), Eur(1.00), Eur(2.50), 25, 25 });
}
static tax::Box3TaxModel Rules()
{
    return tax::Box3TaxModel(tax::Box3Parameters{ 2025, Eur(57684.00), 137, 588, 3600, 1500 });
}
static sim::UnderlyingTerms Market(std::string u, double price, double vol)
{
    return { std::move(u), Eur(price), Rate::FromContinuous(0.07), Volatility::FromAnnualized(vol), Rate::Zero(), Eur(1.00) };
}
static sim::BookTerms Terms(std::vector<sim::UnderlyingTerms> us, Date start = On(2025, 1, 2), int cycles = 6)
{
    return { start, cycles, 182, 30, Delta::FromValue(0.30), Rate::FromContinuous(0.02), sim::AnnualReference{ 1, 1 }, std::move(us) };
}
static std::vector<sim::GbmPathGenerator> Markets(const sim::BookTerms& t)
{
    std::vector<sim::GbmPathGenerator> g;
    for (const auto& u : t.underlyings)
        g.push_back(sim::GbmPathGenerator(sim::GbmParameters{ u.initialPrice, u.drift, u.volatility, DayCountConvention::Act365Fixed }));
    return g;
}
static strategy::Book BookOf(const sim::BookScript& script, double capital, int bufferBp = 500)
{
    return strategy::Book(script.Underlyings(), strategy::Portfolio::Funded(Eur(capital)), strategy::ReinvestmentPolicy::WithCashBuffer(bufferBp),
        strategy::RollRules(strategy::RollTriggers{ 7, 8000, Delta::FromValue(0.60) }), Fees(), Rules());
}

class Progress : public sim::RunObserver
{
public:
    void Completed(std::int32_t path, std::int32_t of) override { last = path; std::cerr << "  path " << path << " of " << of << "\r"; }
    bool Continues() const override { return true; }
    std::int32_t last{ 0 };
};

static void Example(int paths)
{
    const sim::BookTerms terms{ On(2026, 1, 5), 6, 182, 14, Delta::FromValue(0.30), Rate::FromContinuous(0.025), sim::AnnualReference{ 1, 1 },
        { { "AEX", Eur(95.00), Rate::FromContinuous(0.06), Volatility::FromAnnualized(0.22), Rate::FromContinuous(0.018), Eur(1.00) },
            { "ASML", Eur(680.00), Rate::FromContinuous(0.09), Volatility::FromAnnualized(0.34), Rate::FromContinuous(0.009), Eur(5.00) },
            { "SHELL", Eur(32.00), Rate::FromContinuous(0.04), Volatility::FromAnnualized(0.26), Rate::FromContinuous(0.04), Eur(0.50) } } };
    const sim::BookScript script(terms);
    const strategy::Book book(script.Underlyings(), strategy::Portfolio::Funded(Eur(200000.00)), strategy::ReinvestmentPolicy::WithCashBuffer(500),
        strategy::RollRules(strategy::RollTriggers{ 7, 8000, Delta::FromValue(0.50) }), Fees(), Rules());
    const sim::BookMonteCarlo simulation(book, script, Markets(terms));
    sim::Rng rng(20250811u);
    Progress progress;
    std::cout << "example scenario (config/scenarios/example.json terms), " << paths << " paths, seed 20250811 ..." << std::endl;
    const auto d = simulation.Over(paths, rng, progress);
    std::cout << "  finished: median equity " << T(d.Equity(50)) << ", 5th " << T(d.Equity(5)) << ", 95th " << T(d.Equity(95)) << ", mean growth " << d.MeanGrowthRate() << ", worst drawdown " << d.WorstDrawdown() << ", assignments per position " << d.AssignmentFrequency() << std::endl;
}

static void Fixture()
{
    sim::Rng rng(20250811u);
    std::printf("first eight normals for seed 20250811:");
    for (int i = 0; i != 8; ++i) std::printf(" %.17g", rng.NextStandardNormal());
    std::printf("\n");
    sim::Rng u(7u);
    std::printf("first three uniforms for seed 7: %.17g %.17g %.17g\n", u.NextUniform(), u.NextUniform(), u.NextUniform());
    sim::Rng one(1u);
    const sim::GbmPathGenerator g(sim::GbmParameters{ Eur(100.00), Rate::FromContinuous(0.08), Volatility::FromAnnualized(0.20), DayCountConvention::Act365Fixed });
    const std::vector<Date> schedule{ On(2025, 1, 2), On(2025, 4, 2), On(2025, 7, 2), On(2025, 10, 2), On(2026, 1, 2) };
    const auto path = g.Along(schedule, one);
    std::printf("path for seed 1, S0 100, mu 8%%, sigma 20%%, ACT/365F, quarterly: [");
    for (const auto& c : path.Closes()) std::printf("%lld, ", static_cast<long long>(c.Cents()));
    std::printf("] cents\n");
}

static void Denominator()
{
    const auto terms = Terms({ Market("ACME", 40.00, 0.25), Market("BIG", 950.00, 0.22) });
    const sim::BookScript script(terms);
    const sim::BookMonteCarlo simulation(BookOf(script, 50000.00), script, Markets(terms));
    sim::Rng rng(20250811u);
    const auto distribution = simulation.Over(256, rng);
    // count the options actually written by replaying one book per path is not exposed; count from a projection instead
    auto book = BookOf(script, 50000.00);
    sim::Rng replay(20250811u);
    std::vector<estimation::PriceSeries> paths;
    for (const auto& g : Markets(terms)) paths.push_back(g.Along(script.Schedule(), replay));
    int written = 0, assigned = 0, rows = 0;
    for (int c = 0; c != script.Cycles(); ++c)
    {
        if (const auto a = script.AssessmentBefore(c, paths)) book.Assess(*a);
        for (const auto& row : book.Over(script.CycleFor(c, book, paths)))
        {
            ++rows;
            if (row.row.contracts > 0) ++written;
            if (row.row.assignment.Cents() != 0) ++assigned;
        }
    }
    std::printf("first path replayed: %d underlying-cycles, %d options written, %d assigned; PathOutcome.positions = %d\n", rows, written, assigned, distribution.Outcomes().front().positions);
    std::printf("256 paths: AssignmentFrequency() = %.4f with positions = cycles x underlyings; BIG (950.00) never writes against 50,000.00, so the denominator is twice the options written\n", distribution.AssignmentFrequency());
}

static void Worst()
{
    const auto terms = Terms({ Market("ACME", 40.00, 0.25), Market("BETA", 55.00, 0.30) });
    const sim::BookScript script(terms);
    const sim::BookMonteCarlo simulation(BookOf(script, 200000.00), script, Markets(terms));
    for (const int paths : { 64, 256, 1024, 4096 })
    {
        sim::Rng rng(20250811u);
        Progress progress;
        const auto d = simulation.Over(paths, rng, progress);
        std::printf("%5d paths: worst drawdown %.4f, drawdown of the 5th-percentile path %.4f, of the median path %.4f, mean growth %.4f\n", paths, d.WorstDrawdown(), d.At(5).maximumDrawdown, d.At(50).maximumDrawdown, d.MeanGrowthRate());
    }
}

static void OpeningDay()
{
    for (const auto start : { On(2026, 1, 1), On(2026, 1, 2) })
    {
        const auto terms = Terms({ Market("ACME", 40.00, 0.25) }, start, 4);
        const sim::BookScript script(terms);
        sim::Rng rng(1u);
        std::vector<estimation::PriceSeries> paths;
        for (const auto& g : Markets(terms)) paths.push_back(g.Along(script.Schedule(), rng));
        std::printf("start %s, four 182-day cycles to %s: reference dates struck:", D(start).c_str(), D(start.AddDays(4 * 182)).c_str());
        for (int c = 0; c != script.Cycles(); ++c)
            if (const auto a = script.AssessmentBefore(c, paths)) std::printf(" %s (before cycle %d)", D(a->on).c_str(), c);
        std::printf("\n");
    }
}

static void StrikeBound()
{
    const sim::WheelTerms terms{ On(2025, 1, 2), 1, 365, 0, Delta::FromValue(0.05), Eur(1.00), Rate::FromContinuous(0.025), Rate::Zero(), Volatility::FromAnnualized(1.00), std::nullopt };
    const sim::WheelScript script(terms);
    const auto schedule = script.Schedule();
    const auto path = estimation::PriceSeries::From(schedule, std::vector<Money>(schedule.size(), Eur(40.00)), DayCountConvention::Act365Fixed);
    std::cout << "sigma 100%, 365-day cycle, target delta 0.05, spot 40.00: a put ..." << std::endl;
    const auto put = script.PeriodFor(0, strategy::WheelState::Cash, path);
    std::cout << "  put strike " << T(put.strike) << " premium " << put.premiumPerShare << "; now a call (search bounded at 8 x spot = 320.00) ..." << std::endl;
    const auto call = script.PeriodFor(0, strategy::WheelState::HoldingShares, path);
    std::cout << "  call strike " << T(call.strike) << std::endl;
}

static void ZeroClose()
{
    const sim::GbmPathGenerator g(sim::GbmParameters{ Eur(0.02), Rate::FromContinuous(0.05), Volatility::FromAnnualized(1.00), DayCountConvention::Act365Fixed });
    const std::vector<Date> schedule{ On(2025, 1, 2), On(2035, 1, 2) };
    sim::Rng rng(3u);
    std::cout << "S0 0.02, sigma 100%, one ten-year step: generating paths ..." << std::endl;
    for (int p = 0; p != 100; ++p)
    {
        std::cout << "  path " << p << std::flush;
        const auto path = g.Along(schedule, rng);
        std::cout << " close " << T(path.Closes().back()) << std::endl;
    }
}

int main(int argc, char** argv)
{
    const std::string which = argc > 1 ? argv[1] : "";
    if (which == "E1") Fixture();
    else if (which == "E2") Denominator();
    else if (which == "E3") Worst();
    else if (which == "E4") OpeningDay();
    else if (which == "E5") StrikeBound();
    else if (which == "E6") ZeroClose();
    else if (which == "E7") Example(256);
    else if (which == "E7b") Example(2000);
    else if (which == "E7c") Example(10000);
    else return 2;
    return 0;
}
```

```text
### E1
first eight normals for seed 20250811: 0.20555135791986839 -0.37569432986146745 0.16732616243353135 -0.7276489592281139 -0.096171754069307613 -1.4051657831416127 -0.086858928211284089 0.94558564913912102
first three uniforms for seed 7: 0.11741428103451806 0.94930120289264419 0.75438530415285809
path for seed 1, S0 100, mu 8%, sigma 20%, ACT/365F, quarterly: [10000, 11562, 13655, 15718, 16226, ] cents
exit status 0
### E2
first path replayed: 12 underlying-cycles, 6 options written, 0 assigned; PathOutcome.positions = 12
256 paths: AssignmentFrequency() = 0.0436 with positions = cycles x underlyings; BIG (950.00) never writes against 50,000.00, so the denominator is twice the options written
exit status 0
### E3
   64 paths: worst drawdown 0.3573, drawdown of the 5th-percentile path 0.2695, of the median path 0.0937, mean growth 0.0233
demo_sim: /home/user/fin-bs/source/strategy/Portfolio.cpp:152: void fin::strategy::Portfolio::Pay(const fin::core::Money&): Assertion `FreeCash() >= amount' failed.
exit status 134
### E4
start 2026-01-01, four 182-day cycles to 2027-12-30: reference dates struck: 2027-01-01 (before cycle 2)
start 2026-01-02, four 182-day cycles to 2027-12-31: reference dates struck: 2027-01-01 (before cycle 1)
exit status 0
### E5
sigma 100%, 365-day cycle, target delta 0.05, spot 40.00: a put ...
  put strike 13.00 premium 1.29038; now a call (search bounded at 8 x spot = 320.00) ...
demo_sim: /home/user/fin-bs/source/sim/WheelScript.cpp:156: fin::core::Money fin::sim::WheelScript::StrikeFor(const fin::core::Money&, fin::core::OptionType, double) const: Assertion `withinTarget(put ? 1 : furthest)' failed.
exit status 134
### E6
S0 0.02, sigma 100%, one ten-year step: generating paths ...
exit status 134

### E3
   64 paths: worst drawdown 0.3573, drawdown of the 5th-percentile path 0.2695, of the median path 0.0937, mean growth 0.0233
demo_sim: /home/user/fin-bs/source/strategy/Portfolio.cpp:152: void fin::strategy::Portfolio::Pay(const fin::core::Money&): Assertion `FreeCash() >= amount' failed.
last progress line:   path 235 of 256
### E7
example scenario (config/scenarios/example.json terms), 256 paths, seed 20250811 ...
  finished: median equity 211564.12, 5th 140721.00, 95th 265402.84, mean growth 0.00758958, worst drawdown 0.504605, assignments per position 0.0236545
last progress line:   path 256 of 256
### E7b
example scenario (config/scenarios/example.json terms), 2000 paths, seed 20250811 ...
  finished: median equity 213336.80, 5th 142432.85, 95th 263799.04, mean growth 0.0118512, worst drawdown 0.574496, assignments per position 0.0266389
last progress line:   path 2000 of 2000
### E7c
example scenario (config/scenarios/example.json terms), 10000 paths, seed 20250811 ...
demo_sim: /home/user/fin-bs/source/strategy/Portfolio.cpp:152: void fin::strategy::Portfolio::Pay(const fin::core::Money&): Assertion `FreeCash() >= amount' failed.
last progress line:   path 3469 of 10000
```

Call stack of the E3 abort (gdb, frames inside the project):

```text
#5  0x00007ffff782881b in __assert_fail_base at ./assert/assert.c:96
#6  0x00007ffff783b517 in __assert_fail at ./assert/assert.c:105
#7  0x0000555555568036 in fin::strategy::Portfolio::Pay at /home/user/fin-bs/source/strategy/Portfolio.cpp:152
#8  0x000055555556466b in fin::strategy::Book::Assess at /home/user/fin-bs/source/strategy/Book.cpp:86
#9  0x000055555555c04f in fin::sim::BookMonteCarlo::Once at /home/user/fin-bs/source/sim/BookMonteCarlo.cpp:95
#10 0x000055555555c64e in fin::sim::BookMonteCarlo::Over at /home/user/fin-bs/source/sim/BookMonteCarlo.cpp:62
```

Experiment F: `GbmPathGenerator.cpp` compiled as shipped and with the include of the Runge–Kutta
header wrapped in `#pragma GCC push_options` / `#pragma GCC pop_options`, each linked ahead of the
harness archives into the program below at `-O2`; 100,000 seven-point paths; outputs compared line
by line.

```cpp
#include "source/sim/GbmPathGenerator.hpp"
#include <cstdio>
#include <vector>
using namespace fin;
int main()
{
    const sim::GbmPathGenerator g(sim::GbmParameters{ core::Money::FromAmount(100.00, core::Currency::Eur), core::Rate::FromContinuous(0.08), core::Volatility::FromAnnualized(0.20), core::DayCountConvention::Act365Fixed });
    std::vector<core::Date> schedule;
    for (int i = 0; i != 7; ++i) schedule.push_back(core::Date::FromYearMonthDay(2025, 1, 2).AddDays(i * 61));
    sim::Rng rng(20250811u);
    for (int p = 0; p != 100000; ++p)
    {
        const auto path = g.Along(schedule, rng);
        for (const auto& c : path.Closes()) std::printf("%lld\n", static_cast<long long>(c.Cents()));
    }
}
```

```text
closes compared: 700000 lines; differing lines: 0
first eight closes (cents): 10000 10272 10062 10303 9806 9827 8849 10000
fused multiply-add instructions in run_fast: 0; in run_strict: 0
```
