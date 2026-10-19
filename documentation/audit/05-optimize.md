# Audit phase 5: `source/optimize`

*Date: 2026-10-19. Scope: the setting and range, the objective, the risk limits, the descent and
the recommendation in `source/optimize`, their tests, the design and theory notes, the toolbox
optimizer and regularisers they instantiate, and the way the presentation layer parameterises the
search. Report only.*

## Summary

The pieces are built as the design says: a setting is searched as a fraction of a stated range,
every candidate is simulated on the same seed, a limit is a charge proportional to the breach, and
a recommendation reports the achievement of the setting it recommends. All 46 tests pass, the
worked examples for reading a setting off the range and for charging a breach are pinned by tests,
and a Python replica of the toolbox's descent reproduces the closed forms the theory note gives for
the two anchor charges where they apply.

The High finding is about the search itself. It is a fixed-step gradient descent with no line
search and no convergence test, on a score that is a staircase, with a step of 4.0 in unit-box
coordinates. Measured on the test scenario under a drawdown limit, the recommended cash buffer
moves between 2 584 and 4 200 basis points and the growth between 1.36% and 1.78% as the iteration
count goes from 23 to 48 or the step from 1 to 8; one variant ends infeasible, and the best cost
found by the method's own variants is 15% better than what the default parameters return
(OPT-1). The theory note's published table of what the search finds no longer matches the code:
today an assignment cap is met by driving the target delta to the floor of the range at 0.09%
growth, where the note says it is met by rolling earlier at 1.68% and draws a conclusion from that
(OPT-2). The determinism guarantee holds exactly; what does not hold is that the answer is a
property of the strategy rather than of the step parameters.

| Severity | Count |
|---|---|
| Critical | 0 |
| High | 1 |
| Medium | 1 |
| Low | 4 |
| Info | 2 |

## What was executed

**Build route.** Same harness as phase 1. GCC 13.3.0, Ninja, Debug configuration, executed
2026-10-19.

```text
$ cmake --build build --target fin.optimize_test
$ (cd build && ctest -R fin.optimize_test --output-on-failure)
1/1 Test #5: fin.optimize_test ................   Passed    2.17 sec
100% tests passed, 0 tests failed out of 1
$ build/source/optimize/test/fin.optimize_test
[==========] Running 46 tests from 6 test suites.
[==========] 46 tests from 6 test suites ran. (2379 ms total)
[  PASSED  ] 46 tests.
```

Experiments against the built library (sources and full output in appendix B), all on the test
scenario of `test/StrategyScenario.hpp`: one underlying at 25% volatility, 200 000 of capital, six
half-year cycles, 64 paths, the range of the theory note, started from (0.30, 2 200 bp, 0.60):

| Experiment | What was varied | Result |
|---|---|---|
| X1 | iterations 23, 24, 25, 26, 48 at step 4.0; step 1, 2, 4, 8 at 24 iterations; the start's own score | OPT-1 table |
| X2 | the three rows of the theory note's "What the search finds" table, at the test's step and count | OPT-2 table |
| X3 | a start outside the range, with and without the anchor charge | silently clamped to the face (OPT-4) |
| X4 | a NaN step in Debug and in a `-O2 -DNDEBUG` build of this module | refused in both (OPT-8) |

Files read in full: the twelve source and header files, the six test files and the scenario
header, `design/optimization.md`, decisions D36–D40, `theory/searching-for-a-setting.md`, the
toolbox's `GradientDescent.hpp`, `L1.hpp` and `L2.hpp`, `sim/OutcomeDistribution.hpp/.cpp` for the
quantities the objective reads, and `present/RunController.cpp:21-43, 286-301` for how the
application builds the range, the limits and the descent terms.

## What was verified independently

| Check | Method | Result |
|---|---|---|
| Reading a setting off the range | centre → (0.30, 2 200 bp, 0.60); (−3, 2, 0.5) → (0.10, 4 200 bp, 0.60) | pinned by tests, matches the note |
| Charging a breach | the five-row table of the note | pinned by `TestRiskLimits`; boundary row binds without being infeasible |
| Quadratic anchor closed form | Python replica of the toolbox descent, step 0.1, 500 iterations | (0.425, 0.575, 0.4625) exactly |
| Absolute anchor closed form, coordinates that move | same | (0.300, 0.700, 0.450) exactly |
| Absolute anchor, a coordinate that should not move | same, gain 0.05 below a charge of 0.1 | does not stay put: 0.4969 after 50 steps, 0.5122 after 51 (OPT-3) |
| Fixed-step stability | replica, curvature 1, anchor 3 | step 0.5 oscillates back to the start, step 1.5 diverges to 10³³ (OPT-3) |
| Central difference at a face | `StrategyObjective.cpp:85-93` | one-sided with the width actually used, as the note says |
| Common random numbers | `StrategyObjective.cpp:63` seeds a fresh generator per evaluation from the experiment's seed | every candidate sees the same paths |
| Determinism | X1 rows for identical parameters, and `TestRecommendation` | identical to the last digit |
| Score within limits is growth alone | `TestStrategyObjective` | holds |
| Toolbox instantiated in double | `GradientDescent<double, 3>`, `L1/L2<double, 3>` | yes |

## Findings

| ID | Severity | Where | Title |
|---|---|---|---|
| OPT-1 | High | `StrikeSizingOptimizer.cpp:64-75`, toolbox `GradientDescent.hpp`, `present/RunController.cpp:24` | The recommendation depends on the step count and step length by more than the differences it is meant to resolve |
| OPT-2 | Medium | `theory/searching-for-a-setting.md` "What the search finds", `test/TestRecommendation.cpp:120-129` | The published search results and their conclusion are stale; an assignment cap now collapses the target delta to the floor |
| OPT-3 | Low | `StrikeSizingOptimizer.cpp:23-55`, toolbox `L1.hpp`, `L2.hpp` | The absolute anchor is a subgradient step, not the soft threshold the note describes; the descent has no step control |
| OPT-4 | Low | `StrategySetting.cpp:17-20, 52-60`, `test/TestStrategySetting.cpp:86-89` | A start outside the range is silently clamped; the design lists it as a precondition that stops the search |
| OPT-5 | Low | `StrikeSizingOptimizer.cpp:13-21, 34-37` | The anchor charge is computed on the raw displacement while the objective sees the clamped point |
| OPT-6 | Low | `present/RunController.cpp:33-38, 286-296` | The application derives the range from the current setting, half to double, so the "risk decision" of the range is made implicitly |
| OPT-7 | Info | `StrikeSizingOptimizer.hpp:16-21`, toolbox `GradientDescent.hpp` | `Descent.iterations` always equals the configured count; there is no convergence test to report on |
| OPT-8 | Info | `StrikeSizingOptimizer.cpp:3-5` | Same fast-math exposure as ESTIM-1; in this unit the NaN refusal survived the Release test |

### OPT-1 — The recommendation depends on the step parameters (High)

**Evidence.** `StrikeSizingOptimizer::From` runs the toolbox's `GradientDescent::Minimize`, which
takes exactly `maxIterations` steps of `learningRate × gradient` with no line search, no
acceptance test and no stopping rule (`GradientDescent.hpp`, `Minimize`). The gradient is a secant
over a sixteenth of the range on a score that is a staircase (decision D38). The application fixes
the step at 4.0 (`present/RunController.cpp:24`) and takes the iteration count from the user. On
the test scenario under a drawdown limit of 40%, started from (0.30, 2 200 bp, 0.60), which is
itself infeasible at 41.7% drawdown and 1.60% growth:

| Step | Iterations | Target delta | Buffer | Roll limit | Growth | Drawdown | Feasible |
|---|---|---|---|---|---|---|---|
| 4.0 | 23 | 0.3026 | 4 200 bp | 0.7828 | 1.356% | 32.6% | yes |
| 4.0 | 24 | 0.3162 | 4 200 bp | 0.7769 | 1.393% | 32.5% | yes |
| 4.0 | 25 | 0.3231 | 4 059 bp | 0.7870 | 1.463% | 32.9% | yes |
| 4.0 | 26 | 0.3304 | 3 881 bp | 0.7889 | 1.433% | 34.1% | yes |
| 4.0 | 48 | 0.3331 | 3 299 bp | 0.7745 | 1.603% | 37.7% | yes |
| 1.0 | 24 | 0.3062 | 2 742 bp | 0.6275 | 1.499% | 38.9% | yes |
| 2.0 | 24 | 0.3077 | 2 833 bp | 0.6533 | 1.538% | 38.7% | yes |
| 8.0 | 24 | 0.3651 | 2 584 bp | 0.7825 | 1.783% | 41.4% | **no**, drawdown binds |

Every row is a deterministic function of its parameters and every feasible row has a lower cost
than the start, so the design's guarantees hold. But the buffer recommended to a person ranges from
26% to 42% of free cash, the growth from 1.36% to 1.60%, and the drawdown slack from 1 point to
7.5 points, according to an iteration count that two users would choose differently without any
reason to. The default parameters stop at a cost of −0.0139 while 48 iterations reach −0.0160 and
a step of 1.0 reaches −0.0150: the method's own variants find settings 8–15% better in cost than
what it returns by default. The theory note names this failure mode ("overshoots as easily as it
undershoots") and suggests comparing with the start; the application reports neither the start's
score nor the sensitivity.

**Impact.** The number a person acts on, the cash buffer and the delta to write at, is determined
to within about one fifth of the range by the step parameters rather than by the strategy. For a
real-money decision that is the difference between a 26% and a 42% cash reserve. No value in the
simulation is wrong; the search reports one point of a bouncing trajectory as the answer.

**Proposed fix.** Any of the following removes most of the arbitrariness, in increasing order of
change: (a) accept a step only when it lowers the cost (a monotone descent), so the walk cannot end
on an overshoot; (b) replace the gradient step by a pattern search on the sixteenth-of-range grid
the gradient already uses, which the theory note itself lists as the better fit to a staircase;
(c) a coarse grid followed by a local pattern search, cached by setting so that repeated
evaluations cost nothing. Whatever the method, report with every recommendation the start's own
score and the spread between two step sizes, so a person can see when the answer is not settled.
**Suggested test.** A stability test: the setting found with N and with 2N iterations, and with
two step sizes, agree to within the difference step (one sixteenth of the range) in every
coordinate, on the test scenario with and without a limit.

### OPT-2 — The published search results and their conclusion are stale (Medium)

**Evidence.** The theory note's "What the search finds" table, reproduced with the parameters of
`TestRecommendation` (step 4.0, 24 iterations, 64 paths, the same scenario), gives today:

| Limits | Published | Today |
|---|---|---|
| none in reach | 0.32 / 200 bp / 0.78; growth 2.43%, drawdown 50.7%, assignments 15.6% | 0.34 / 200 bp / 0.80; 2.46%, 52.1%, 20.3% |
| drawdown ≤ 40% | 0.32 / 2 998 bp / 0.77; 1.74%, 38.4%, 16.4% | 0.32 / 4 200 bp / 0.78; 1.39%, 32.5%, 15.6% |
| assignments ≤ 10% | 0.32 / 200 bp / 0.40; 1.68%, 50.3%, 5.5% | **0.10** / 200 bp / 0.61; **0.09%**, 42.5%, 1.8% |

The note concludes "Neither limit is met by lowering the target delta, which is the result worth
noticing". Today the assignment limit is met almost entirely by lowering the target delta to the
floor of the range, and the recommendation is a wheel that grows at 0.09% a year. The test
`an_assignment_limit_is_met_by_rolling_sooner_and_is_paid_for_in_growth` still passes because it
asserts only that the roll limit fell and growth fell, not that the delta held.

**Impact.** A reader of the theory note is told the opposite of what the code does, on the one
behavioural conclusion the note draws. Separately, the degenerate answer to an assignment cap is a
symptom of OPT-1: the step of 4.0 on a penalty slope of order one carries the delta coordinate to
its face in a single step, and nothing brings it back.

**Proposed fix.** Regenerate the table from the current code and keep it as a golden test so the
note cannot drift from the code again; after OPT-1 is addressed, rewrite the conclusion from the
new result. Consider reporting a recommendation whose growth is below the start's by most of its
value as "this limit is not worth meeting" rather than as a setting.

### OPT-3 — The absolute anchor is a subgradient step; the descent has no step control (Low)

**Evidence.** The toolbox's `L1::Gradient` returns `±λ` by sign and 0 at exactly zero
(`L1.hpp`); applied inside a fixed-step descent this is a subgradient method, not the proximal
soft-threshold operator the theory note derives (Tibshirani). The replica shows the consequence: a
coordinate whose gain is below the charge, which the note says "is not recommended at all", ends
at 0.4969 after 50 steps and 0.5122 after 51, chattering around the start by ±(step × charge).
The same replica shows the quadratic anchor's fixed step oscillating back to the start when
step × (curvature + charge) = 2 and diverging to 10³³ at 1.5. The tests pin only coordinates that
do move (`sparsity_moves_every_coordinate_the_same_distance_short_of_the_floor_of_the_bowl`).

**Impact.** Nil today: the application sets both charges to zero
(`present/RunController.cpp:300`). If the anchor is ever switched on, the "leave this lever alone"
property it exists for is not delivered exactly, and the step must be chosen with the charge in
mind or the descent diverges.

**Proposed fix.** Implement the absolute charge as a proximal step (soft-threshold the
displacement after each gradient step) rather than a subgradient; or drop the charge until a
caller exists. **Suggested test.** A coordinate whose gain is below the charge ends exactly at its
start.

### OPT-4 — A start outside the range is silently clamped (Low)

**Evidence.** `design/optimization.md` lists "a starting setting inside the range" among the
preconditions that are "checked where stated and stopping the search when broken". `PointOf`
clamps (`StrategySetting.cpp:17-20, 57-59`), `From` then sees a point inside the box, and the test
`a_setting_outside_the_range_is_placed_on_the_face_it_left_through` pins the clamping. Experiment
X3: a start of (0.05, 100 bp, 0.90) against the range (0.10–0.50, 200–4 200 bp, 0.40–0.80) runs
without complaint and returns the same recommendation as a start on the face, including with the
anchor charge, whose distance is then measured from the face rather than from the setting in use.

**Impact.** None in production, where the range is built around the start (OPT-6). The design
table and the code disagree on which it is.

**Proposed fix.** Either assert in `Recommend` that the start is inside the range, or change the
design table to say the start is read onto the range. **Suggested test.** Whichever is chosen.

### OPT-5 — Anchor charge on the raw displacement, objective on the clamped point (Low)

**Evidence.** `HeldNearTheStart::Cost` adds the charges on `displacement` as given while the
objective is evaluated at `Displaced(start, displacement)`, which clamps
(`StrikeSizingOptimizer.cpp:13-21, 34-37`). Once a coordinate has crossed a face, further steps in
the same direction change nothing in the objective but keep increasing the charge, so with an anchor
the point where the search leaves the face depends on how long it pressed against it.

**Impact.** Nil today (charges are zero). With a charge it couples the answer to the iteration
count in a second way.

**Proposed fix.** Charge the effective displacement, `Displaced(start, v) − start`.

### OPT-6 — The application derives the range from the current setting (Low)

**Evidence.** `present/RunController.cpp:33-38` builds each lever's range as half to double the
current value, widened to a minimum spread (100 bp for the deltas, 1 000 bp for the buffer) and
clipped to the natural bounds. For the shipped scenario (0.30, 500 bp, 0.50) that is deltas
0.15–0.60, buffer 250–1 250 bp, roll 0.25–1.00. Decision D37 says the range "is itself a risk
decision, made before the search rather than by it".

**Impact.** The decision is made by a formula the user does not see: a scenario started at a 5%
buffer can never be recommended more than 12.5%, and one started at 40% never less than 20%.
Since the best setting is usually on a bound (the note's own observation), the answer is shaped
by the start more than by the strategy.

**Proposed fix.** Show the range with the recommendation, or let the scenario state it.

### OPT-7 — `Descent.iterations` is uninformative (Info)

The toolbox descent always runs `maxIterations` steps, so the reported count equals the input and
says nothing about convergence; there is no convergence to report. Harmless; it becomes useful
once OPT-1 introduces a stopping rule.

### OPT-8 — Fast-math exposure, refusal intact here (Info)

`StrikeSizingOptimizer.cpp` includes three toolbox headers at lines 3–5, so `Displaced`,
`HeldNearTheStart`, `From` and `IsWellFormed(DescentTerms)` are compiled under
`-O3 -ffast-math` on GCC (ESTIM-1). Tested: a NaN learning rate is refused by `IsWellFormed` and
the constructor aborts in both the Debug library and a `-O2 -DNDEBUG` build of this module, with
and without the `push_options` guard. The exposure is the same; in this unit's code shape the
compiler did not remove the check. The fix recommended in ESTIM-1 applies here too.

## Observations carried forward to later phases

| For phase | Where | Observation |
|---|---|---|
| 7 strategy | the staircase | The score's steps come from whole contracts at listed strikes; the strategy phase should state the step size in buffer terms (one contract's collateral as a fraction of free cash), which bounds how fine any search can usefully be. |
| 9 sim | `sim/OutcomeDistribution.cpp:87-108` | `WorstDrawdown` is the maximum over paths, which grows with the path count (the note warns of this); `AssignmentFrequency` is assignments over positions pooled across paths; `Equity(50)` is a rank statistic (D35). Check the drawdown's definition per path. |
| 9 sim | `StrategyObjective.cpp:63` | A fresh `Rng(seed)` per evaluation gives common random numbers only if the generator consumes draws in a setting-independent order; verify that a setting that writes fewer contracts or rolls earlier does not shift the draw sequence of later cycles or other underlyings. |

## Open questions for the owner

1. Is the search meant to be a decision input today, or an exploration aid (OPT-1)? If the former,
   the step parameters need to stop mattering before it is used with money.
2. Should the range be part of the scenario rather than derived from the current setting (OPT-6)?
3. Should a recommendation that gives up most of the growth to meet a limit be presented as a
   setting or as "the limit costs this much" (OPT-2)?

## Not checked

- The quality of the simulated score itself: drawdown and assignment definitions, percentile
  semantics and the common-random-numbers property are phase 9.
- Runtime at the application's path counts; the test scenario's 170 simulations per
  recommendation took about 0.3 s at 64 paths and one underlying.
- The toolbox descent's behaviour with fixed-point types; only `double` is used.

## Appendix A: replica of the fixed-step descent with the anchor charges

Run with `python3 -I check_optimize.py`. Output as produced on 2026-10-19 follows the script.

```python
# Replica of numerical-toolbox GradientDescent (fixed step, no stopping test) with the L1 and L2
# charges, on the theory note's quadratic worked example: f(x) = 1/2 * sum (x - x*)^2.
xstar = [0.20, 0.80, 0.35]
x0 = [0.5, 0.5, 0.5]

def run(l1, l2, eta, iters):
    v = [0.0, 0.0, 0.0]
    for _ in range(iters):
        g = []
        for i in range(3):
            gf = x0[i] + v[i] - xstar[i]                                   # quadratic, curvature 1
            gl1 = l1 * (1 if v[i] > 0 else -1 if v[i] < 0 else 0)          # toolbox L1::Gradient
            gl2 = l2 * v[i]                                                # toolbox L2::Gradient
            g.append(gf + gl1 + gl2)
        v = [v[i] - eta * g[i] for i in range(3)]
    return [round(x0[i] + v[i], 5) for i in range(3)]

def closed_l2(l2):
    return [round((xstar[i] + l2 * x0[i]) / (1 + l2), 5) for i in range(3)]

def closed_l1(l1):
    return [x0[i] if abs(xstar[i] - x0[i]) <= l1 else round(xstar[i] + (l1 if xstar[i] < x0[i] else -l1), 5) for i in range(3)]

print("closed forms: quadratic charge 3 ->", closed_l2(3), "| absolute charge 0.1 ->", closed_l1(0.1))
for eta, iters in [(0.1, 50), (0.1, 500), (0.2, 512), (0.5, 50), (0.5, 500), (1.5, 50)]:
    print(f"step {eta} x {iters}: quadratic 3 -> {run(0, 3, eta, iters)} | absolute 0.1 -> {run(0.1, 0, eta, iters)}")
xstar = [0.55, 0.5, 0.5]
for eta, iters in [(0.1, 50), (0.1, 51), (0.5, 200), (0.5, 201)]:
    print(f"gain 0.05 below a charge of 0.1, step {eta} x {iters}: absolute -> {run(0.1, 0, eta, iters)} (closed form: unchanged, 0.5)")
```

```text
closed forms: quadratic charge 3 -> [0.425, 0.575, 0.4625] | absolute charge 0.1 -> [0.3, 0.7, 0.45]
step 0.1 x 50: quadratic 3 -> [0.425, 0.575, 0.4625] | absolute 0.1 -> [0.30097, 0.69903, 0.4502]
step 0.1 x 500: quadratic 3 -> [0.425, 0.575, 0.4625] | absolute 0.1 -> [0.3, 0.7, 0.45]
step 0.2 x 512: quadratic 3 -> [0.425, 0.575, 0.4625] | absolute 0.1 -> [0.3, 0.7, 0.45]
step 0.5 x 50: quadratic 3 -> [0.5, 0.5, 0.5] | absolute 0.1 -> [0.3, 0.7, 0.45]
step 0.5 x 500: quadratic 3 -> [0.5, 0.5, 0.5] | absolute 0.1 -> [0.3, 0.7, 0.45]
step 1.5 x 50: quadratic 3 -> [6.661338147750942e+33, -6.661338147750942e+33, 3.330669073875471e+33] | absolute 0.1 -> [0.3, 0.7, 0.65]
gain 0.05 below a charge of 0.1, step 0.1 x 50: absolute -> [0.4969, 0.5, 0.5] (closed form: unchanged, 0.5)
gain 0.05 below a charge of 0.1, step 0.1 x 51: absolute -> [0.51221, 0.5, 0.5] (closed form: unchanged, 0.5)
gain 0.05 below a charge of 0.1, step 0.5 x 200: absolute -> [0.47857, 0.5, 0.5] (closed form: unchanged, 0.5)
gain 0.05 below a charge of 0.1, step 0.5 x 201: absolute -> [0.56429, 0.5, 0.5] (closed form: unchanged, 0.5)
```

## Appendix B: experiments against the built library

Built from the harness in the session scratchpad against the `fin.*` and `infra.*` archives of
`build/`, including the module's own test scenario header. `demo_opt.cpp`:

```cpp
#include "source/optimize/Recommendation.hpp"
#include "source/optimize/test/StrategyScenario.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace fin;
using namespace fin::optimize;
using namespace fin::optimize::test;
static void Print(const char* label, const Recommendation& r)
{
    std::printf("%-44s delta %.4f buffer %5d bp roll %.4f | growth %.4f%% drawdown %.4f assign %.4f %s%s\n", label, r.setting.targetDelta.Value(), r.setting.cashBufferBasisPoints, r.setting.rollDeltaLimit.Value(), 100 * r.growthRate, r.worstDrawdown, r.assignmentFrequency, r.drawdownBinds ? "[drawdown binds]" : "", r.assignmentBinds ? "[assignment binds]" : "");
}
int main(int argc, char** argv)
{
    const int which = argc > 1 ? std::atoi(argv[1]) : 0;
    const StrategyObjective unconstrained(Experiment(64), SearchedRange(), UnreachableLimits());
    const StrategyObjective drawdownLimited(Experiment(64), SearchedRange(), RiskLimits{ .maximumDrawdown = 0.40, .maximumAssignmentFrequency = 1.0, .penaltyWeight = 1.0 });
    const auto start = Setting(0.30, 2200, 0.60);
    if (which == 1)
    {
        char label[64];
        for (std::size_t iterations : { 23, 24, 25, 26, 48 })
        {
            std::snprintf(label, sizeof label, "drawdown<=0.40, rate 4.0, %2zu iterations", iterations);
            Print(label, Recommend(start, drawdownLimited, StrikeSizingOptimizer({ .learningRate = 4.0, .iterations = iterations, .sparsityWeight = 0.0, .shrinkageWeight = 0.0 })));
        }
        for (double rate : { 1.0, 2.0, 4.0, 8.0 })
        {
            std::snprintf(label, sizeof label, "drawdown<=0.40, rate %.1f, 24 iterations", rate);
            Print(label, Recommend(start, drawdownLimited, StrikeSizingOptimizer({ .learningRate = rate, .iterations = 24, .sparsityWeight = 0.0, .shrinkageWeight = 0.0 })));
        }
        Print("start itself (no search), drawdown<=0.40", Recommendation{ start, drawdownLimited.GrowthRate(start), drawdownLimited.Outcome(start).Equity(50), drawdownLimited.Outcome(start).WorstDrawdown(), drawdownLimited.Outcome(start).AssignmentFrequency(), false, false });
    }
    if (which == 2)
    {
        const auto outside = Setting(0.05, 100, 0.90);  // every coordinate outside SearchedRange
        std::printf("start outside the range: PointOf = (%.2f, %.2f, %.2f); Recommend ...\n", PointOf(SearchedRange(), outside)[0], PointOf(SearchedRange(), outside)[1], PointOf(SearchedRange(), outside)[2]);
        Print("from a start outside the range, shrinkage 3", Recommend(outside, unconstrained, StrikeSizingOptimizer({ .learningRate = 4.0, .iterations = 24, .sparsityWeight = 0.0, .shrinkageWeight = 3.0 })));
        Print("from the clamped start (0.10, 200, 0.80), shrinkage 3", Recommend(Setting(0.10, 200, 0.80), unconstrained, StrikeSizingOptimizer({ .learningRate = 4.0, .iterations = 24, .sparsityWeight = 0.0, .shrinkageWeight = 3.0 })));
    }
    if (which == 3)
    {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        std::printf("IsWellFormed(DescentTerms{learningRate = NaN}) = %d\n", int(IsWellFormed(DescentTerms{ .learningRate = nan, .iterations = 10, .sparsityWeight = 0.0, .shrinkageWeight = 0.0 })));
        const StrikeSizingOptimizer built({ .learningRate = nan, .iterations = 10, .sparsityWeight = 0.0, .shrinkageWeight = 0.0 });
        std::printf("StrikeSizingOptimizer with a NaN learning rate was ACCEPTED\n");
        (void)built;
    }
    return 0;
}
```

```text
$ ./demo_opt 1
drawdown<=0.40, rate 4.0, 23 iterations      delta 0.3026 buffer  4200 bp roll 0.7828 | growth 1.3563% drawdown 0.3262 assign 0.1562
drawdown<=0.40, rate 4.0, 24 iterations      delta 0.3162 buffer  4200 bp roll 0.7769 | growth 1.3932% drawdown 0.3250 assign 0.1562
drawdown<=0.40, rate 4.0, 25 iterations      delta 0.3231 buffer  4059 bp roll 0.7870 | growth 1.4629% drawdown 0.3291 assign 0.1641
drawdown<=0.40, rate 4.0, 26 iterations      delta 0.3304 buffer  3881 bp roll 0.7889 | growth 1.4329% drawdown 0.3408 assign 0.1771
drawdown<=0.40, rate 4.0, 48 iterations      delta 0.3331 buffer  3299 bp roll 0.7745 | growth 1.6028% drawdown 0.3766 assign 0.1797
drawdown<=0.40, rate 1.0, 24 iterations      delta 0.3062 buffer  2742 bp roll 0.6275 | growth 1.4991% drawdown 0.3885 assign 0.1016
drawdown<=0.40, rate 2.0, 24 iterations      delta 0.3077 buffer  2833 bp roll 0.6533 | growth 1.5379% drawdown 0.3867 assign 0.1094
drawdown<=0.40, rate 4.0, 24 iterations      delta 0.3162 buffer  4200 bp roll 0.7769 | growth 1.3932% drawdown 0.3250 assign 0.1562
drawdown<=0.40, rate 8.0, 24 iterations      delta 0.3651 buffer  2584 bp roll 0.7825 | growth 1.7827% drawdown 0.4140 assign 0.1901 [drawdown binds]
start itself (no search), drawdown<=0.40     delta 0.3000 buffer  2200 bp roll 0.6000 | growth 1.5992% drawdown 0.4166 assign 0.0938
$ ./demo_opt 2
start outside the range: PointOf = (0.00, 0.00, 1.00); Recommend ...
from a start outside the range, shrinkage 3  delta 0.1000 buffer   200 bp roll 0.8000 | growth 0.0134% drawdown 0.4339 assign 0.0312
from the clamped start (0.10, 200, 0.80), shrinkage 3 delta 0.1000 buffer   200 bp roll 0.8000 | growth 0.0134% drawdown 0.4339 assign 0.0312
$ ./demo_opt 3            # Debug library
demo_opt: source/optimize/StrikeSizingOptimizer.cpp:61: ... Assertion `IsWellFormed(terms)' failed.
$ g++ -std=c++20 -O2 -DNDEBUG ... demo_opt.cpp source/optimize/*.cpp <other archives>
--- demo_opt_rel 3 (stdout unbuffered)
IsWellFormed(DescentTerms{learningRate = NaN}) = 0
/bin/bash: line 1:  1455 Aborted                 stdbuf -o0 ./$b 3
[exit code 134]
--- demo_opt_rel_patched 3 (stdout unbuffered)
IsWellFormed(DescentTerms{learningRate = NaN}) = 0
/bin/bash: line 1:  1456 Aborted                 stdbuf -o0 ./$b 3
[exit code 134]
```

`doc_rows.cpp` reproduces the theory note's table with the test's parameters:

```cpp
#include "source/optimize/Recommendation.hpp"
#include "source/optimize/test/StrategyScenario.hpp"
#include <cstdio>
using namespace fin; using namespace fin::optimize; using namespace fin::optimize::test;
static void Print(const char* label, const Recommendation& r)
{
    std::printf("%-22s delta %.2f buffer %4d bp roll %.2f | growth %.2f%% drawdown %.1f%% assignments %.1f%%%s%s\n", label, r.setting.targetDelta.Value(), r.setting.cashBufferBasisPoints, r.setting.rollDeltaLimit.Value(), 100 * r.growthRate, 100 * r.worstDrawdown, 100 * r.assignmentFrequency, r.drawdownBinds ? " [drawdown binds]" : "", r.assignmentBinds ? " [assignment binds]" : "");
}
int main()
{
    const auto start = Setting(0.30, 2200, 0.60);
    const StrikeSizingOptimizer optimizer({ .learningRate = 4.0, .iterations = 24, .sparsityWeight = 0.0, .shrinkageWeight = 0.0 });
    Print("none in reach", Recommend(start, StrategyObjective(Experiment(64), SearchedRange(), UnreachableLimits()), optimizer));
    Print("drawdown <= 0.40", Recommend(start, StrategyObjective(Experiment(64), SearchedRange(), RiskLimits{ .maximumDrawdown = 0.40, .maximumAssignmentFrequency = 1.0, .penaltyWeight = 1.0 }), optimizer));
    Print("assignments <= 0.10", Recommend(start, StrategyObjective(Experiment(64), SearchedRange(), RiskLimits{ .maximumDrawdown = 1.0, .maximumAssignmentFrequency = 0.10, .penaltyWeight = 1.0 }), optimizer));
    std::printf("published in theory/searching-for-a-setting.md: none in reach: 0.32 / 200 bp / 0.78 / 2.43%% / 50.7%% / 15.6%%; drawdown: 0.32 / 2998 bp / 0.77 / 1.74%% / 38.4%% / 16.4%%; assignments: 0.32 / 200 bp / 0.40 / 1.68%% / 50.3%% / 5.5%%\n");
}
```

```text
$ ./doc_rows
none in reach          delta 0.34 buffer  200 bp roll 0.80 | growth 2.46% drawdown 52.1% assignments 20.3%
drawdown <= 0.40       delta 0.32 buffer 4200 bp roll 0.78 | growth 1.39% drawdown 32.5% assignments 15.6%
assignments <= 0.10    delta 0.10 buffer  200 bp roll 0.61 | growth 0.09% drawdown 42.5% assignments 1.8%
published in theory/searching-for-a-setting.md: none in reach: 0.32 / 200 bp / 0.78 / 2.43% / 50.7% / 15.6%; drawdown: 0.32 / 2998 bp / 0.77 / 1.74% / 38.4% / 16.4%; assignments: 0.32 / 200 bp / 0.40 / 1.68% / 50.3% / 5.5%
```
