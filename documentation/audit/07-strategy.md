# Audit phase 7: `source/strategy`

*Date: 2026-10-19. Scope: the portfolio, ledger, account, sizing policy, roll rules, wheel state
machine, collateral allocation, single-wheel projection and the book in `source/strategy`, their
nine test files, the strategy and book design contracts, the four theory notes they implement, and
decisions D21–D25, D32–D34, D41–D44, D46, D47 and D49. Report only.*

## Summary

The settlement arithmetic is right. Every worked example in the four theory notes, the two design
notes and the report contract was reproduced to the cent, including the 920.35 wealth-tax charge of
the report example (which the document does not give a spot for; it reproduces for 45.00). The
shipped golden projection satisfies the cash identity cycle by cycle, with the three gaps between
cycles equal to the Box 3 charge on an all-cash book to the cent. A randomised run of 399 monthly
cycles over three names with rolls, assignments and assessments held every invariant the design
states: the ledger explains the cash, collateral is never spent, nothing is open at a boundary, the
record stays in date order and the last row of a cycle carries the book's equity. All 116 tests
pass.

Two High findings. First, a charge the free cash cannot meet aborts the process, and that is
reachable with realistic settings: a single-name book with no buffer, fully assigned at the boundary
before 1 January, holds about one month's premium in cash against an annual charge of the same size
(STRAT-1). The documents call this the model behaving correctly; in a Monte-Carlo run one such path
ends the run with no message. Second, the model prices every option with a dividend yield and never
pays a dividend: the ledger has a kind for it, the tax layer has a withholding model for it, and
nothing posts one, so a wheel holding shares of a 4% yielder forgoes income of the same order as its
premiums, and the limits table does not say so (STRAT-2). Two Medium findings: the cash and equity
on a cycle row are a mid-settlement snapshot that depends on the order the book lists its
underlyings (STRAT-3), and a rolled position leaves its capital idle until the next cycle boundary
(STRAT-4).

| Severity | Count |
|---|---|
| Critical | 0 |
| High | 2 |
| Medium | 2 |
| Low | 7 |
| Info | 4 |

## What was executed

**Build route.** Same harness as phase 1 (scratchpad CMake project, GCC 13.3.0, Ninja, Debug),
executed 2026-10-19.

```text
$ cmake --build build --target fin.strategy_test
$ (cd build && ctest -R fin.strategy_test --output-on-failure)
1/1 Test #8: fin.strategy_test ................   Passed    0.03 sec
100% tests passed, 0 tests failed out of 1
$ build/source/strategy/test/fin.strategy_test
[==========] Running 116 tests from 10 test suites.
[==========] 116 tests from 10 test suites ran. (25 ms total)
[  PASSED  ] 116 tests.
```

Nine experiments were run against the built library (appendix B):

| # | Experiment | Expected from the reading | Observed |
|---|---|---|---|
| E1 | one name, 200,000.00, no buffer: 50 puts at 40.00 for 0.50, assigned at 35.00, assessed on 2026-01-01 at 35.00 | charge above the 2,459.50 of cash: abort | abort in `Portfolio::Pay` from `Book.cpp:86`; charge 2,508.46 |
| E1b | the same with a 5% buffer | affordable | 47 contracts; charge 2,406.63 paid; cash 11,905.12 |
| E2a | a wheel whose assessment is dated one day after expiry | dropped without a word | row tax 0.00, no ledger entry, no message |
| E2b | assessment dated one day before the period opened | refused | abort at `Projection.cpp:232` |
| E3 | premium per share 0.8049, 0.8050, 0.8051 on 50 contracts | 4,000.00, 4,000.00, 4,050.00 | as expected (exact: 4,024.50, 4,025.00, 4,025.50) |
| E4 | book listed [A, B] then [B, A]; B rolled at its observation, A writes nothing | A's row differs | A's equity 18,197.00 in one listing, 20,474.00 in the other; same date, same holdings |
| E5 | three names, 400 monthly cycles, random strikes, premiums, spots and roll observations, 3% buffer, assessed every twelfth cycle | invariants hold | abort at cycle 35: charge 2,522.40 against 1,380.93 free cash, 2,900 shares held |
| E5g | E5 with an unaffordable charge skipped and counted | invariants hold | 0 violations over 399 cycles: 1,132 options written, 205 rolled, 463 assigned, 32 charges, 1 unaffordable |
| E5b | E5g with a 10% buffer | invariants hold | 0 violations: 1,182 written, 211 rolled, 469 assigned, 32 charges, 1 unaffordable |
| E6 | 4,000.00 capital, one put at 40.00 for 0.004 per share | premium rounds to nothing, 0.75 fee unpayable: abort | abort in `Portfolio::Pay` from `Projection.cpp:153` |

E5's random premiums (0.5–4% of the strike per month) make the balance compound absurdly; the run
checks identities, not plausibility.

Files read in full: the nine source and nine header files, the nine test files, the strategy
`CMakeLists.txt`, `design/strategy.md`, `design/book.md`, `theory/wheel-strategy.md`,
`theory/position-sizing-and-compounding.md`, `theory/rolling-a-short-option.md`,
`theory/running-several-wheels-at-once.md`, decisions D7, D21–D25, D32–D34, D41–D49, the limits
table of `theory/methodology.md`, the report example of `design/io.md`, `tax/Box3TaxModel.cpp` and
`tax/Box3Parameters.cpp` (to reproduce two charges), `pricing/AssignmentModel.cpp`,
`costs/BrokerFees.cpp`, `config/scenarios/example.json`, `data/golden/example-projection.csv`, and,
to establish how the layer is driven, `sim/WheelScript.cpp`, `sim/BookScript.cpp` and the
projection, simulation and search paths of `present/RunController.cpp`.

## What was verified independently

The script in appendix A produced every figure below.

| Check | Method | Result |
|---|---|---|
| Six-cycle wheel (`wheel-strategy.md`, `TestProjection`) | replay with integer cents | all six rows; premiums 1,340.00, charges 18.00, assignments −10,000.00 |
| Roll example (`rolling-a-short-option.md`) | floor of 10⁴(1 − v/p); close cost and charges | 7,500 bp, 117.00 kept; 1,250 bp when observed at 0.70 |
| Sizing table (`position-sizing-and-compounding.md`) | retained half-to-even, floor of the ratio | all five rows; 15% buffer at 4,500.00 writes one |
| Two-name, two-cycle book (`running-several-wheels-at-once.md`, `TestBook`) | replay in service order | 5 puts, BETA nothing, 593.25; 5 calls, BETA nothing, 20,036.50 |
| Richer premium served first (`TestBook`) | replay | 5,199.25; the poorer name sees 1,199.25 |
| Report example (`design/io.md`) | replay with a 10% buffer at every sizing | all four rows, including 45 and 4 contracts, 203,963.25 and 186,779.15 |
| The 920.35 charge in that example | Box 3 replayed on 183,960.25 of cash and 400 shares, spot solved | reproduces for a spot of 45.00 only |
| `TestProjection` charge of 726.38 | Box 3 replayed on 204,962.50 of deposits | 726.38 |
| Golden projection cash identity | per cycle: last row's cash = previous + Σ(premium − fees + assignment − tax) | exact in the three cycles without a reference date; gaps of 741.44, 849.31 and 714.90 in the others, each equal to Box 3 on an all-cash book of the previous cash |
| Transition table | `TestWheelEngine` table against the theory note's | the eight legal moves and nothing else |
| Charges on each leg | `Projection.cpp:152, 192, 216` against `BrokerFees::For` | 0.75 per contract on writing and on closing; 3.00 per settlement whatever the contract count; nothing on expiry; connectivity never (COSTS-1) |
| Invariants under random scripts | E5g, E5b | 0 violations in 399 cycles × 3 names, each configuration |
| Rounding at the one cashflow point | 200,000 random premiums on 50 contracts | largest error 25.00, mean −0.008 (STRAT-5) |
| Break-even for the boundary charge | single name, 200,000.00, strike 40.00, assigned at 35.00 | no buffer: needs 0.51 per share (1.27% of strike) with no remainder; 1% buffer: covered from nothing |
| Premium at the 30-delta strike (σ 25%, r 2.5%) | Black–Scholes | 30 days 1.31%, 60 days 2.31%, 91 days 2.68%, 182 days 3.95% of the strike |

## Findings

| ID | Severity | Where | Title |
|---|---|---|---|
| STRAT-1 | High | `Portfolio.cpp:152`; `Book.cpp:86`; `Projection.cpp:153, 188, 193, 217, 242` | A charge the free cash cannot meet aborts the process, and a fully assigned book at the boundary before 1 January reaches it |
| STRAT-2 | High | `CompoundingLedger.hpp:16`, `Projection.hpp:35-44`, `sim/WheelScript.cpp:84`, `tax/DividendTax.cpp` | Options are priced with a dividend yield but no dividend is ever received, and the limits table does not say so |
| STRAT-3 | Medium | `Book.cpp:73-74`, `Projection.cpp:252` | The cash and equity on a cycle row are a mid-settlement snapshot that depends on the listing order |
| STRAT-4 | Medium | D24; `Projection.cpp:182-200`; `sim/WheelScript.cpp:104-111` | A rolled position leaves its capital idle until the next cycle boundary |
| STRAT-5 | Low | `Projection.cpp:14-17` | The per-share value is rounded to a cent before the multiplier; D7 says together, once |
| STRAT-6 | Low | `Projection.cpp:164, 227` | An assessment dated after the period's expiry is discarded silently |
| STRAT-7 | Low | `RollRules.cpp:31`; `sim/WheelScript.cpp:119`; `io/ScenarioLoader.cpp:177-191` | The time trigger is always true once `roll_days_to_expiry` reaches `observation_days_before_expiry` |
| STRAT-8 | Low | `RollRules.hpp:9-14`, `RollRules.cpp:37`, `Projection.cpp:268` | The roll reason is computed and dropped; the replacement-risk check has no caller |
| STRAT-9 | Low | `Account.cpp:47`; D23 | The reconciliation identity is test-only; D23 says it runs on every projection |
| STRAT-10 | Low | `Projection.cpp:255-269`; D34 | A covered call accepts a roll observation; the guard that makes a call roll unreachable lives in the script |
| STRAT-11 | Low | `design/strategy.md:98, 110-118`; `design/book.md:98-100`; `theory/wheel-strategy.md:35` | Four statements in the documents no longer describe the code |
| STRAT-12 | Info | `present/RunController.cpp:213-214`, `sim/BookScript.cpp:129-146` | The boundary assessment values holdings taken before the cycle's write pass at prices of the reference date |
| STRAT-13 | Info | `CollateralAllocation.cpp:13, 33` | Service order compares floating-point yields; a tie is a tie only if two divisions round alike |
| STRAT-14 | Info | `CompoundingLedger.cpp:16-21` | Each posting is an ordered insert; cheap while entries arrive nearly sorted |
| STRAT-15 | Info | whole layer | Cash earns nothing, including the collateral, and the limits table does not list it |

### STRAT-1 — An unaffordable charge aborts the process, and realistic settings reach it (High)

**Evidence.** Every payment goes through `Portfolio::Pay`, which asserts `FreeCash() >= amount`
(`Portfolio.cpp:152`). The wealth tax is paid there from `Book::Assess` (`Book.cpp:86`) and from
`Projection::Assess` (`Projection.cpp:242`); contract charges, close values and settlement charges
from `Projection.cpp:153, 188, 193, 217`. `design/strategy.md` says "a projection can therefore fail
to afford a charge while apparently holding a large balance. That is the model behaving correctly;
the cash buffer exists to prevent it", and `design/book.md` that "a fully committed book can still
fail to raise the cash. The buffer is the lever." No test specifies the refusal, although D4
promises every refusal is. Experiment E1: a single name with 200,000.00 and no buffer writes fifty
puts at 40.00 for 0.50, is assigned at 35.00 and holds 2,459.50 in cash and 5,000 shares; the Box 3
charge on that at the 2025 parameters is 2,508.46; `Book::Assess` aborts. E5: the randomised book
with a 3% buffer aborted at its third assessment (charge 2,522.40 against 1,380.93 free) after
loss-making rolls had drained the cash. E6: the same assertion from the opening charge when a
premium rounds to nothing and the balance is an exact multiple of the collateral.

**Impact.** The charge on a fully assigned single-name book is about 2.1% of the share value above
the allowance per year (36% of 5.88%); the cash left after a full assignment with no buffer is the
remainder of the division plus one cycle's premium less charges. At a 30-day cycle and a 30-delta
strike with 25% volatility the premium is 1.31% of the strike, and the break-even in appendix A is
1.27%: whether the run survives 1 January depends on the remainder and on a cent of premium. A 1%
buffer covers the charge in that example, but the buffer is taken at sizing from free cash, before
premiums and roll costs move, so it bounds nothing at the boundary; E5 shows 3% failing. What
happens is `std::abort()`: in a Debug build with the assertion text, in a Release build silently
(CORE-6). Inside a Monte-Carlo run of thousands of paths one such path ends the whole run; inside a
search (OPT-1), a candidate with a small buffer ends the search. There is no message, no partial
result and no way to tell the cause from any other defect. The owner's scenario has a 5% buffer and
half-year cycles and is not exposed; a smaller buffer, monthly cycles, or a lower target delta is.

**Proposed fix.** Decide what the model does when the charge cannot be met, because the tax office
does not care: (a) sell shares to raise it, oldest lot first, with a share charge, which is what a
person would do; or (b) end the path, not the process, and report the count of insolvent paths as a
statistic alongside the drawdown; or (c) carry the shortfall as a negative cash balance limited to
the tax entry. Any of them turns an abort into an outcome. Independently, the scenario boundary can
refuse a buffer of zero when an assessment is configured, naming both fields. **Suggested tests.**
Today: `EXPECT_DEATH` on E1 as written, so the refusal is specified. After the change: the chosen
behaviour on E1, and `Reconciles` still holding.

### STRAT-2 — Options are priced with a dividend yield but no dividend is ever received (High)

**Evidence.** `CashflowKind::Dividend` exists (`CompoundingLedger.hpp:16`) and may only be
positive (`CompoundingLedger.cpp:58`); nothing outside the tests posts one. `ProjectionPeriod`
(`Projection.hpp:35-44`) has no field through which a script could supply one, and `Projection`
has no code path that credits a holder of shares. `tax/DividendTax.cpp` models the withholding and
has no caller. The simulated script prices every option with `terms.dividendYield`
(`sim/WheelScript.cpp:84, 94, 150`), which lowers call premiums and raises put premiums as a
dividend-paying stock's forward requires. The limits table in `theory/methodology.md` lists nine
limits and dividends are not among them; `PLAN.md:114` says the withholding "is modeled as a
credit/leakage".

**Impact.** A wheel holds shares between a put assignment and a call assignment, which in the golden
run and the fuzz is a large fraction of the time. During that time the real holder receives the
dividend less 15% withholding; the model receives nothing while having already priced the option it
sold as if the dividend were paid. The owner's three names carry yields of 1.80%, 0.90% and 4.00%.
Sixteen SHELL contracts assigned at 32.00 hold 51,200.00 of shares; at 4.00% that is 2,048.00 a
year gross, 1,740.80 net, against the 1,500–1,700 of premium those contracts earn per half-year in
the golden file. The projection therefore understates the income of the shares leg by an amount of
the same order as the premium, for every dividend-paying name, in one direction. Separately, if the
scenario's drift is fitted on closes (phase 3) it is a price drift, and the holder's total return is
drift plus yield; the model delivers only the drift.

**Proposed fix.** Give `ProjectionPeriod` a dividend leg (ex-dates within the cycle and the amount
per share, or the period's yield applied to the days shares were held), have `Resolve` post
`CashflowKind::Dividend` net of withholding through `tax::DividendTax`, and have the script supply
it from the per-underlying yield. The path generator then has to drop the price by the dividend on
the ex-date or the yield is counted twice; that is a phase 9 point. Until then, add a row to the
limits table: "No dividends are received; options are priced as if they were". **Suggested test.** A
wheel holding 200 shares across an ex-date receives the net dividend, posted on that date, and the
ledger reconciles.

### STRAT-3 — A cycle row's cash and equity are a mid-settlement snapshot (Medium)

**Evidence.** `Book::Over` settles the wheels in listing order (`Book.cpp:73-74`) and each row reads
`portfolio.Cash()` and `Equity(portfolio, marks)` at the instant its own wheel settles
(`Projection.cpp:252`). Experiment E4: with B rolled at its observation and A writing nothing, A's
row reports equity 18,197.00 when listed first and 20,474.00 when listed second, on the same date
with the same holdings; the difference is B's position marked 6.00 in the money at expiry although
B had closed it, chronologically, a month earlier. The golden file shows the same: on 2026-07-06 the
three rows carry 208,831.50, 207,272.25 and 208,016.00, and on 2028-01-03 the first row's equity is
43,653.00 below its own cash with no contracts written. The report example in `design/io.md` has a
first row whose value is 2,000.00 below its cash with no explanation. `design/book.md:98-100` says
settlement order is "so that a run is reproducible. Nothing depends on it", and
`theory/running-several-wheels-at-once.md` the same. `present/RunController.cpp:219` takes the last
row's equity for the cycle series, which is the right one.

**Impact.** No number downstream is wrong today, because the one consumer that aggregates picks the
last row. The cycle table and the CSV publish several equities per date, of which only one is the
book's, and the others are figures that correspond to no moment in time: they mix cash after some
settlements with liabilities of positions that no longer existed. A reader checking a row by hand,
or a future consumer summing or charting the equity column, will get a wrong answer in proportion
to how much was open and in the money at the time; D22's "every published figure is exact" does not
hold for those rows.

**Proposed fix.** After the settle pass, stamp the book's cash and equity on every row of the cycle
(one `Equity` call per cycle), or move both to a per-cycle summary row and leave the per-underlying
rows with their own movements. State in `design/book.md` what the columns mean. **Suggested test.**
Every row of a cycle carries the same cash and equity, whatever the listing order.

### STRAT-4 — A rolled position leaves its capital idle until the next boundary (Medium)

**Evidence.** D24 makes a roll end the cycle, with "the replacement the next cycle's option". The
projection closes the position at the observation date (`Projection.cpp:182-200`) and returns to
cash; the script opens the next cycle at the expiry date (`sim/WheelScript.cpp:104-111`). Between
the observation and the expiry the collateral sits uncommitted. `theory/rolling-a-short-option.md`
defines a roll as "closing a short option before it expires and writing another one in its place"
and does not mention the gap.

**Impact.** The owner's scenario observes 14 days before a 182-day expiry, so every rolled cycle
earns premium for 92% of its length; a monthly cycle observed 10 days out earns for 67%. In the
fuzz, 18% of the options written were rolled, and the golden run rolls both ASML and SHELL in its
first cycle. The direction is conservative for income, but it also understates the time the book is
exposed, so it is not simply a safety margin. It compounds with STRAT-7, where a time trigger that
always fires turns every cycle into a shortened one.

**Proposed fix.** Either state the gap in the limits table and in the roll note, or let a wheel that
rolls write its replacement at the observation date, which needs the script to price a strike at
that date (it already prices the position being closed) and a cycle whose expiry is not shared with
the others, so it is a clock change rather than a small edit. **Suggested test.** None until the
decision is made.

### STRAT-5 — Rounding per share, then the multiplier (Low)

**Evidence.** `PerContract` (`Projection.cpp:14-17`) computes `FromAmount(perShare) × multiplier`,
and the caller multiplies by the contract count. D7 says "the contract multiplier and the rounding
to whole minor units are applied together, once". E3: 0.8050 per share on 50 contracts books
4,000.00 and 0.8051 books 4,050.00; the exact amounts are 4,025.00 and 4,025.50. Over 200,000
random premiums the largest error on 50 contracts is 25.00 and the mean is −0.008. The theory note
says premiums "are quoted per share to the minor unit", which is what a real quote is, but a real
quote is on a tick, not a rounding of a model value.

**Impact.** Unbiased and at most half a cent per share per cashflow; the precision loss is a hundred
times what D7 describes, not a bias. The value paid to close is rounded the same way, so a roll's
net premium carries up to a euro per contract of rounding noise.

**Proposed fix.** Either round once, `FromAmount(perShare × multiplier × contracts)`, to match D7,
or keep per-share rounding, call it the quotation tick, and amend D7 and the strategy contract to
say so. **Suggested test.** The E3 triple, whichever rule is chosen.

### STRAT-6 — An assessment after expiry is discarded silently (Low)

**Evidence.** `Assess` returns early when the reference date is after `upTo`
(`Projection.cpp:227`), and `Settle` then resets the cycle (`Projection.cpp:164`) with the
assessment unused. The range check at `Projection.cpp:232` runs only when the date is reached, so a
date before `opened` is refused (E2b) and a date after `expiry` is not (E2a: no tax, no entry, no
message). `design/strategy.md` lists "a scripted period whose observation falls between writing and
expiry" as a checked precondition.

**Impact.** Unreachable through the script, which places the date inside the period. A hand-written
period, or a future script, could drop a year's charge without a trace.

**Proposed fix.** Check the assessment's range in `Write`, next to the checks on the period itself.
**Suggested test.** `EXPECT_DEATH` on E2a.

### STRAT-7 — The time trigger becomes always-true against the observation schedule (Low)

**Evidence.** A position is observed once per cycle, `observation_days_before_expiry` before
expiry (`sim/WheelScript.cpp:119`), so the days to expiry at the observation is that number. The
time trigger fires when it is at or below `roll_days_to_expiry` (`RollRules.cpp:31`). The scenario
loader bounds both fields to 0–365 independently (`io/ScenarioLoader.cpp:177-191`) and
`IsWellFormed(WheelTerms)` relates the observation to the cycle length but not to the trigger. Once
`roll_days_to_expiry ≥ observation_days_before_expiry`, every observed put is closed at every
observation; IO-7 asked whether a roll could fire "on day one", and the answer is at every
observation. The owner's scenario has 7 against 14 and is not affected.

**Impact.** A configuration the documents call a defect ("a trigger that is always true") is
accepted, and the projection pays a close value and a charge every cycle while leaving the capital
idle for the observation gap (STRAT-4).

**Proposed fix.** Refuse `roll_days_to_expiry ≥ observation_days_before_expiry` at the scenario
boundary when an observation is configured, naming both fields. **Suggested test.** Loader refusal.

### STRAT-8 — The roll reason is dropped; the replacement check has no caller (Low)

**Evidence.** `RollRules::Triggered` returns a `RollReason` that `Projection::Rolls` reduces to
`has_value()` (`Projection.cpp:268`); no row, ledger entry or report carries it, so a rolled put and
a put that expired worthless are both a row in state "cash", distinguishable only by the second
charge. `RollRules::Allows` (`RollRules.cpp:37`) has no caller outside its test. The roll note says
the reason "matters for the account of why a projection did what it did" and that "the replacement
is subject to the same risk limit as the position it replaces". The replacement is sized at the
scenario's target delta, and nothing requires the target to be inside the roll limit.

**Impact.** A reader cannot tell from a report how often each trigger fired, which is the one
question the three triggers exist to answer; and a target delta above the roll limit would write a
position that breaches the limit at its first observation, every cycle.

**Proposed fix.** Carry the reason on the row (optional) and in the cycle report; refuse a target
delta above the roll delta limit at the boundary, or remove `Allows`. **Suggested test.** A rolled
row reports its reason; the loader refuses target 0.60 with limit 0.50.

### STRAT-9 — The reconciliation identity is test-only (Low)

**Evidence.** `Reconciles` (`Account.cpp:47`) is called from the tests and nowhere else. D23:
"the opening balance plus the ledger equals the closing balance at every point, which is a check
that runs on every projection rather than a claim".

**Impact.** The identity held in every experiment here, so this is about the promise, not the
arithmetic. A future change that moved cash without posting would be caught only by a test that
happened to cover it.

**Proposed fix.** `really_assert(Reconciles(account))` at the end of `Book::Over` and
`Book::Assess`. The ledger sum is linear in its entries; a thousand cycles of three names is about
twelve thousand entries, so the cost over a run is of the order of ten million additions. If that
is ever measurable, D23's own compromise applies: a cached total checked against the sum.
**Suggested test.** None beyond the existing ones.

### STRAT-10 — A covered call accepts a roll observation (Low)

**Evidence.** `Projection::Rolls` (`Projection.cpp:255-269`) applies the rules to whatever position
is open; nothing checks that it is a put. D34 says only the cash-secured leg is offered a roll, and
places the guard in the simulated script (`sim/WheelScript.cpp:89`). A call deep in the money that
a hand-written period observes would be closed at a value the shares cannot fund, and `Pay` would
abort as in STRAT-1.

**Proposed fix.** Assert in `Rolls` that an observed position is a put, so D34 is a property of the
layer rather than of one caller; or document that a call observation is permitted. **Suggested
test.** `EXPECT_DEATH` on a call period with an observation.

### STRAT-11 — Four statements no longer describe the code (Low)

- `design/strategy.md:98`: "A cashflow dated before the one already recorded" cannot be expressed.
  D41 withdrew that; the record sorts itself (`CompoundingLedger.cpp:16-21`).
- `design/strategy.md:110-118`, "The annual tax charge": describes a wheel valuing its holdings on
  the reference date inside a cycle, including the open position. Production runs go through
  `Book`, which refuses a per-wheel assessment (`Book.cpp:48`) and strikes the charge at a boundary
  (D44); the in-cycle path survives only in `sim::MonteCarlo`, which `present` does not use.
- `design/book.md:98-100` and `theory/running-several-wheels-at-once.md:49`: settlement order
  affects nothing. It affects every row's cash and equity (STRAT-3).
- `theory/wheel-strategy.md:35`: premiums "are quoted per share to the minor unit". Through the
  script they are model values rounded per share (STRAT-5).

### STRAT-12 — The boundary assessment mixes two dates (Info)

`present/RunController.cpp:213-214` strikes the charge before the write pass of the cycle that
contains the reference date, from `BookScript::AssessmentBefore`, which values the holdings at the
prices of the reference date (`sim/BookScript.cpp:129-146`). The cash and shares are those of the
boundary, up to 182 days earlier in the owner's scenario; the premiums of the straddling cycle,
received at the boundary and in the account on the reference date, escape the taxed wealth. D44
states the open positions are ignored; it does not state the price and cash dates differ. The error
is small and understates the charge. For phase 8.

### STRAT-13 — Floating-point ties in the service order (Info)

`PremiumPerUnitOfCollateral` divides a double by a double (`CollateralAllocation.cpp:13`) and the
sort compares the quotients (`:33`). The test's tie, 0.80 over 40.00 against 1.00 over 50.00, is a
tie because both quotients round to the same double; two yields that are equal as fractions need
not. The listing order then breaks what should have been a tie. Cross-multiplying in integers
(premium cents times the other strike's cents) would be exact. No effect on any number, only on
which of two equal claims is served first.

### STRAT-14 — Ordered insertion into the ledger (Info)

`Post` is an `upper_bound` and a vector insert (`CompoundingLedger.cpp:16-21`). Entries arrive in
date order except for the interleaving of wheels within a cycle, so the insert moves a handful of
entries and the cost is linear over a run. Every Monte-Carlo path copies the book and grows its own
ledger; whether that memory matters at a million paths is a phase 9 question.

### STRAT-15 — Cash earns nothing (Info)

Collateral is the whole strike and is held in cash for the life of every put; nothing credits
interest on it, and the limits table does not list the assumption. The pricing uses a risk-free
rate of 2.5% in the owner's scenario. If the broker remunerates cash at anything near that, the
omission is larger than every charge modelled; if it pays nothing, the assumption is exact and
should be written down. An owner question.

## Observations carried forward to later phases

| For phase | Where | Observation |
|---|---|---|
| 8 tax | `Book.cpp:11-17`, `Projection.cpp:34-40` | Deposits are `min(cash, equity)` and investments the rest: brokerage cash is classed as a bank deposit (1.37%) and a short position's intrinsic value reduces the investment class. Confirm both against the Box 3 rules. |
| 8 tax | STRAT-12 | Holdings of the boundary, prices of the reference date; the straddling cycle's premiums untaxed. |
| 8 tax | `tax/Box3TaxModel.cpp:47` | The proportion of the deemed return is a `Money × double`; check the rounding against the statutory rule. |
| 8 tax | STRAT-2 | `DividendTax` has no caller. |
| 9 sim | STRAT-1 | One insolvent path aborts a whole Monte-Carlo run and a whole search. |
| 9 sim | STRAT-2 | Whether the path generator's drift is a price drift; a dividend leg would need the price to drop on the ex-date. |
| 9 sim | STRAT-3 | Which row's equity the drawdown and the equity distribution read; it must be the last of each cycle. |
| 9 sim | STRAT-4, STRAT-7 | One observation per cycle; the idle gap after a roll; `IsWellFormed(WheelTerms)` relates the observation to the cycle but not to the roll trigger. |
| 9 sim | `sim/WheelScript.cpp:84` | The premium is the model price at the scenario's volatility, with no bid–ask or risk premium; the close value at the observation is the same model. |
| 9 sim | STRAT-14 | Memory of one ledger per path. |

## Open questions for the owner

1. What should happen when a charge cannot be paid from free cash: sell shares, end the path and
   report it, or carry a tax debt (STRAT-1)? Until then, do you want the boundary to refuse a zero
   buffer when an assessment is configured?
2. Are dividends out of scope for now (STRAT-2)? If so the limits table should say so; if not, is
   the drift in your scenarios a price drift fitted on closes, so that the dividend is additional?
3. Which equity should a cycle row carry (STRAT-3): the book's after the whole settlement, or none,
   with a per-cycle summary row?
4. Is the idle gap after a roll acceptable as a stated limit (STRAT-4), or should a roll write its
   replacement on the day?
5. Does the broker pay interest on the cash balance (STRAT-15)? If yes, at what rate, and should
   collateral earn it?

## Not checked

- The Box 3 arithmetic itself beyond reproducing two charges (phase 8).
- The simulated script, the path generator and the Monte-Carlo statistics (phase 9); the fuzz used
  random premiums, not model prices.
- The display of rows and ledgers in `present` and `gui`.
- Memory and time of the ledger at Monte-Carlo scale.
- Early assignment of calls before an ex-dividend date: PRICE-1 established the model is never
  called; with no dividend leg (STRAT-2) there is no date on which it could be.

## Appendix A: reference-check script

Run with `python3 -I check_strategy.py`. Output as produced on 2026-10-19 follows the script.

```python
import csv, math, random
from collections import OrderedDict

# --- integer money helpers replicating core/Money.hpp
def cdiv(a, b):
    q = abs(a) // abs(b); return q if (a >= 0) == (b > 0) else -q
def div_he(n, d):  # DivideRoundHalfToEven
    q = cdiv(n, d); r = n - q * d
    if r == 0: return q
    twice, ad = abs(r) * 2, abs(d); step = -1 if (n < 0) != (d < 0) else 1
    if twice > ad: return q + step
    if twice < ad: return q
    return q if q % 2 == 0 else q + step
def rhe(v):  # RoundHalfToEven(double)
    lower = math.floor(v); frac = v - lower; r = int(lower)
    if frac > 0.5 or (frac == 0.5 and r % 2 != 0): r += 1
    return r
def c(amount): return rhe(amount * 100.0)        # Money::FromAmount
def fmt(cents): return f"{cents / 100:,.2f}"

OPTION_FEE, SHARE_FEE, M = 75, 300, 100

# --- Box 3 charge replicating tax/Box3TaxModel.cpp with 2025 parameters (AtRate assumed (x*bps)/10000 half-to-even)
def at_rate(cents, bps): return div_he(cents * bps, 10000)
def box3(deposits, investments):
    total = deposits + investments
    deemed = at_rate(deposits, 137) + at_rate(investments, 588)
    taxable = max(total - 5768400, 0)
    if taxable == 0: return 0
    taxable_return = rhe(deemed * ((taxable / 100.0) / (total / 100.0)))   # Money * double
    return at_rate(taxable_return, 3600)

# --- 1. six-cycle wheel (theory/wheel-strategy.md, TestProjection)
def wheel(capital, cycles, buffer_bp=0):
    cash, shares, out = capital, 0, []
    for leg, K, p, S in cycles:
        if leg == "put":
            free = cash
            retained = div_he(free * buffer_bp, 10000)
            n = (free - retained) // (c(K) * M)
        else:
            n = shares // M
        premium = c(p) * M * n
        fees = OPTION_FEE * n if n else 0
        cash += premium - fees
        assignment = 0
        if n and ((leg == "put" and c(S) < c(K)) or (leg == "call" and c(S) > c(K))):
            assignment = -c(K) * M * n if leg == "put" else c(K) * M * n
            shares += M * n if leg == "put" else -M * n
            cash += assignment - SHARE_FEE; fees += SHARE_FEE
        equity = cash + shares * c(S)
        out.append((n, premium, fees, assignment, cash, shares, equity))
    return out
six = [("put", 40, 0.80, 45), ("put", 44, 0.90, 40), ("call", 46, 1.10, 48), ("put", 48, 1.20, 52), ("put", 52, 1.30, 50), ("call", 54, 1.40, 53)]
print("six-cycle worked example (contracts, premium, fees, assignment, cash, shares, equity):")
for row in wheel(1000000, six):
    print("  ", row[0], *[fmt(x) for x in row[1:4]], fmt(row[4]), row[5], fmt(row[6]))
rows = wheel(1000000, six)
print("  totals: premiums", fmt(sum(r[1] for r in rows)), "fees", fmt(sum(r[2] for r in rows)), "assignments", fmt(sum(r[3] for r in rows)))

# --- 2. roll worked example (theory/rolling-a-short-option.md)
captured = math.floor(min(max(1 - 0.20 / 0.80, -1), 1) * 10000)
kept = c(0.80) * M * 2 - OPTION_FEE * 2 - c(0.20) * M * 2 - OPTION_FEE * 2
print(f"roll example: captured {captured} bp, kept {fmt(kept)}; observed at 0.70: captured {math.floor((1 - 0.70 / 0.80) * 10000)} bp")

# --- 3. sizing table (theory/position-sizing-and-compounding.md)
print("sizing table (free, buffer bp -> retained, investable, contracts at 4000.00):")
for free, b in [(399999, 0), (400000, 0), (1199999, 0), (1200000, 0), (1000000, 500)]:
    ret = div_he(free * b, 10000); inv = free - ret
    print(f"   {fmt(free)} {b}: {fmt(ret)} {fmt(inv)} {inv // 400000}")
print("  at 4500.00 per contract, 10000.00 with 1500 bp held back:", (1000000 - div_he(1000000 * 1500, 10000)) // 450000, "contract(s)")

# --- 4. book worked example (theory/running-several-wheels-at-once.md, TestBook)
cash = 2000000
n = cash // 400000; cash += c(1.20) * M * n - OPTION_FEE * n           # ACME writes 5
beta_free = cash; beta_n = beta_free // 500000                        # BETA sees 596.25
cash += -c(40) * M * n - SHARE_FEE; shares = M * n                     # ACME assigned at 35
print(f"book cycle 1: ACME {n} puts, BETA {beta_n}, cash {fmt(cash)}, ACME shares {shares}")
calls = shares // M; cash += c(0.90) * M * calls - OPTION_FEE * calls
beta_n2 = cash // 500000
cash += c(38) * M * calls - SHARE_FEE; shares -= M * calls
print(f"book cycle 2: ACME {calls} calls, BETA {beta_n2}, cash {fmt(cash)}, shares {shares}")
rich = 500000 + c(2.00) * M * 1 - OPTION_FEE
print(f"RICH/POOR: RICH writes {500000 // 400000}, cash after {fmt(rich)}, POOR sees {fmt(rich - 400000)} free")

# --- 5. io design worked example: 200,000 with 10% buffer, two underlyings, then the 920.35 charge
cash = 20000000
ret = div_he(cash * 1000, 10000); n1 = (cash - ret) // 400000; cash += c(0.80) * M * n1 - OPTION_FEE * n1
free = cash - n1 * 400000; ret2 = div_he(free * 1000, 10000); n2 = (free - ret2) // 500000; cash += c(1.00) * M * n2 - OPTION_FEE * n2
print(f"io example cycle 1: first {n1} contracts, second sees {fmt(free)} free and writes {n2}; cash after both writes {fmt(cash)}")
cash_first_row = cash; equity_first_row = cash - (c(50) - c(45)) * M * n2
cash += -c(50) * M * n2 - SHARE_FEE
print(f"  first row: cash {fmt(cash_first_row)}, value {fmt(equity_first_row)} (the second's put still open, marked 5.00 in the money at 45.00); second row: cash {fmt(cash)}, value {fmt(cash + 400 * c(45))}")
sols = [S for S in range(3000, 9001) if box3(cash, 400 * S) == 92035]
print(f"  the 920.35 charge on 1 January is reproduced for a spot of {[fmt(s) for s in sols]} (deposits {fmt(cash)}, 400 shares)")
cash -= 92035
calls = 400 // M; cash += c(1.10) * M * calls - OPTION_FEE * calls
n3 = (cash - div_he(cash * 1000, 10000)) // 440000; cash += c(0.90) * M * n3 - OPTION_FEE * n3
print(f"  cycle 2: second writes {calls} calls (served first), first writes {n3} puts at 44.00; first row cash {fmt(cash)}, value {fmt(cash + 400 * c(46))}; second row cash {fmt(cash + c(46) * 400 - SHARE_FEE)}")
print(f"  TestProjection 726.38: deposits 204,962.50 -> charge {fmt(box3(20496250, 0))}")

# --- 6. golden projection: cycle-level cash identity
cycles = OrderedDict()
with open("/home/user/fin-bs/data/golden/example-projection.csv") as f:
    for r in csv.DictReader(f):
        cycles.setdefault(r["date"], []).append(r)
prev = 20000000
print("golden example-projection.csv, per cycle: cash of last row, sum of row movements, implied charge between cycles")
for date, rs in cycles.items():
    moved = sum(c(float(r["premium"])) - c(float(r["fees"])) + c(float(r["assignment"])) - c(float(r["tax"])) for r in rs)
    last = c(float(rs[-1]["cash"]))
    gap = prev + moved - last
    note = f" = Box 3 on an all-cash book of {fmt(prev)}: {fmt(box3(prev, 0))}" if gap else ""
    print(f"  {date}: cash {fmt(last)}, movements {fmt(moved)}, previous cash + movements - cash = {fmt(gap)}{note}; row equities {[r['equity'] for r in rs]}")
    prev = last

# --- 7. break-even premium for the wealth-tax charge on a fully assigned single-name book
def affordable(capital, K, S, p_cents, buffer_bp):
    ret = div_he(capital * buffer_bp, 10000); n = (capital - ret) // (K * M)
    cash = capital + p_cents * M * n - OPTION_FEE * n - K * M * n - SHARE_FEE
    return cash, box3(min(cash, cash + n * M * S), n * M * S), n
print("single name, 200,000.00 capital, strike 40.00 fully assigned, spot 35.00 on 1 January:")
for b in [0, 100, 300, 500]:
    pmin = next(p for p in range(0, 100000) if (lambda r: r[0] >= r[1])(affordable(20000000, 4000, 3500, p, b)))
    cash, charge, n = affordable(20000000, 4000, 3500, pmin, b)
    print(f"  buffer {b} bp: {n} contracts; charge affordable from a premium of {pmin / 100:.2f} per share ({pmin / 4000 * 100:.2f}% of the strike): cash {fmt(cash)} vs charge {fmt(charge)}")
def N(x): return 0.5 * math.erfc(-x / math.sqrt(2))
def put(S, K, r, q, s, T):
    d1 = (math.log(S / K) + (r - q + s * s / 2) * T) / (s * math.sqrt(T)); d2 = d1 - s * math.sqrt(T)
    return K * math.exp(-r * T) * N(-d2) - S * math.exp(-q * T) * N(-d1), -math.exp(-q * T) * N(-d1)
print("Black-Scholes put at the strike whose delta is nearest -0.30 (S=40, r=2.5%, q=0, sigma=25%): premium as % of strike")
for days in [30, 60, 91, 182]:
    T = days / 365
    best = min((abs(put(40, K / 100, 0.025, 0, 0.25, T)[1] + 0.30), K) for K in range(2000, 4001, 50))[1]
    price = put(40, best / 100, 0.025, 0, 0.25, T)[0]
    print(f"  {days} days: strike {best / 100:.2f}, premium {price:.3f} = {price / (best / 100) * 100:.2f}% of strike")

print("the same book written at the 30-day, delta -0.30 strike (38.50, premium 0.504 -> 0.50 per share), assigned, assessed at a spot of:")
for spot in [3000, 3500, 3800]:
    cash, charge, n = affordable(20000000, 3850, spot, 50, 0)
    print(f"  {fmt(spot)}: {n} contracts, cash {fmt(cash)} vs charge {fmt(charge)} -> {'abort' if cash < charge else 'paid'}")

# --- 8. rounding per share before the multiplier (Projection.cpp:16)
random.seed(3)
errs = []
for _ in range(200000):
    p = random.uniform(0.3, 3.0); n = 50
    errs.append(c(p) * M * n - p * 100 * M * n)
print(f"per-share rounding, 50 contracts, 200,000 random premiums: max |error| {max(abs(e) for e in errs) / 100:.2f}, mean {sum(errs) / len(errs) / 100:+.4f}; rounding once would bound it at 0.005")
```

```text
six-cycle worked example (contracts, premium, fees, assignment, cash, shares, equity):
   2 160.00 1.50 0.00 10,158.50 0 10,158.50
   2 180.00 4.50 -8,800.00 1,534.00 200 9,534.00
   2 220.00 4.50 9,200.00 10,949.50 0 10,949.50
   2 240.00 1.50 0.00 11,188.00 0 11,188.00
   2 260.00 4.50 -10,400.00 1,043.50 200 11,043.50
   2 280.00 1.50 0.00 1,322.00 200 11,922.00
  totals: premiums 1,340.00 fees 18.00 assignments -10,000.00
roll example: captured 7500 bp, kept 117.00; observed at 0.70: captured 1250 bp
sizing table (free, buffer bp -> retained, investable, contracts at 4000.00):
   3,999.99 0: 0.00 3,999.99 0
   4,000.00 0: 0.00 4,000.00 1
   11,999.99 0: 0.00 11,999.99 2
   12,000.00 0: 0.00 12,000.00 3
   10,000.00 500: 500.00 9,500.00 2
  at 4500.00 per contract, 10000.00 with 1500 bp held back: 1 contract(s)
book cycle 1: ACME 5 puts, BETA 4, cash 593.25, ACME shares 500
book cycle 2: ACME 5 calls, BETA 0, cash 20,036.50, shares 0
RICH/POOR: RICH writes 1, cash after 5,199.25, POOR sees 1,199.25 free
io example cycle 1: first 45 contracts, second sees 23,566.25 free and writes 4; cash after both writes 203,963.25
  first row: cash 203,963.25, value 201,963.25 (the second's put still open, marked 5.00 in the money at 45.00); second row: cash 183,960.25, value 201,960.25
  the 920.35 charge on 1 January is reproduced for a spot of ['45.00'] (deposits 183,960.25, 400 shares)
  cycle 2: second writes 4 calls (served first), first writes 37 puts at 44.00; first row cash 186,779.15, value 205,179.15; second row cash 205,176.15
  TestProjection 726.38: deposits 204,962.50 -> charge 726.38
golden example-projection.csv, per cycle: cash of last row, sum of row movements, implied charge between cycles
  2026-07-06: cash 208,016.00, movements 8,016.00, previous cash + movements - cash = 0.00; row equities ['208831.50', '207272.25', '208016.00']
  2027-01-04: cash 218,188.06, movements 10,913.50, previous cash + movements - cash = 741.44 = Box 3 on an all-cash book of 208,016.00: 741.44; row equities ['218562.31', '218200.06', '218188.06']
  2027-07-05: cash 229,887.56, movements 11,699.50, previous cash + movements - cash = 0.00; row equities ['229904.81', '229899.56', '229887.56']
  2028-01-03: cash 192,225.25, movements -36,813.00, previous cash + movements - cash = 849.31 = Box 3 on an all-cash book of 229,887.56: 849.31; row equities ['198388.75', '193301.50', '192225.25']
  2028-07-03: cash 202,634.75, movements 10,409.50, previous cash + movements - cash = 0.00; row equities ['202647.50', '202645.25', '202634.75']
  2029-01-01: cash 194,368.85, movements -7,551.00, previous cash + movements - cash = 714.90 = Box 3 on an all-cash book of 202,634.75: 714.90; row equities ['195407.35', '195403.85', '194368.85']
single name, 200,000.00 capital, strike 40.00 fully assigned, spot 35.00 on 1 January:
  buffer 0 bp: 50 contracts; charge affordable from a premium of 0.51 per share (1.27% of the strike): cash 2,509.50 vs charge 2,508.96
  buffer 100 bp: 49 contracts; charge affordable from a premium of 0.00 per share (0.00% of the strike): cash 3,960.25 vs charge 2,449.93
  buffer 300 bp: 48 contracts; charge affordable from a premium of 0.00 per share (0.00% of the strike): cash 7,961.00 vs charge 2,416.81
  buffer 500 bp: 47 contracts; charge affordable from a premium of 0.00 per share (0.00% of the strike): cash 11,961.75 vs charge 2,383.56
Black-Scholes put at the strike whose delta is nearest -0.30 (S=40, r=2.5%, q=0, sigma=25%): premium as % of strike
  30 days: strike 38.50, premium 0.504 = 1.31% of strike
  60 days: strike 38.50, premium 0.890 = 2.31% of strike
  91 days: strike 38.00, premium 1.019 = 2.68% of strike
  182 days: strike 37.50, premium 1.480 = 3.95% of strike
the same book written at the 30-day, delta -0.30 strike (38.50, premium 0.504 -> 0.50 per share), assigned, assessed at a spot of:
  30.00: 51 contracts, cash 6,158.75 vs charge 2,084.26 -> paid
  35.00: 51 contracts, cash 6,158.75 vs charge 2,619.04 -> paid
  38.00: 51 contracts, cash 6,158.75 vs charge 2,940.52 -> paid
per-share rounding, 50 contracts, 200,000 random premiums: max |error| 25.00, mean -0.0080; rounding once would bound it at 0.005
```

## Appendix B: experiments against the built library

Compiled with `g++ -std=c++20 -O0 -g` against the harness archives (`libfin.*.a`, `libinfra.*.a`)
and run one experiment per process with `stdbuf -o0`. Shell job-control lines are omitted from the
output; exit status 134 is `SIGABRT`.

```cpp
// Phase 7 experiments against the built fin.strategy library. One experiment per invocation so an
// abort in one does not hide the others: ./demo_strat E1 | E1b | E2a | E2b | E3 | E4 | E5 | E6
#include "source/costs/BrokerFees.hpp"
#include "source/strategy/Book.hpp"
#include "source/strategy/Projection.hpp"
#include "source/tax/Box3TaxModel.hpp"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace fin;
using core::Currency;
using core::Date;
using core::Delta;
using core::Money;
using strategy::Book;
using strategy::BookPeriod;
using strategy::CashflowKind;
using strategy::Portfolio;
using strategy::Projection;
using strategy::ProjectionPeriod;
using strategy::ReinvestmentPolicy;
using strategy::RollObservation;
using strategy::RollRules;
using strategy::RollTriggers;
using strategy::SpotPrice;
using strategy::TaxObservation;
using strategy::WheelState;

static Money Eur(double a) { return Money::FromAmount(a, Currency::Eur); }
static Date On(int y, unsigned m, unsigned d) { return Date::FromYearMonthDay(y, m, d); }
static std::string T(const Money& m) { char b[64]; std::snprintf(b, sizeof b, "%.2f", m.ToDouble()); return b; }

static costs::BrokerFees Fees()
{
    return costs::BrokerFees(costs::FeeSchedule{ .optionFeePerContract = Eur(0.75), .shareCommissionPerTrade = Eur(2.00), .handlingFeePerTrade = Eur(1.00),
        .connectivityFeeCapPerExchangePerYear = Eur(2.50), .connectivityBasisPoints = 25, .currencyHandlingBasisPoints = 25 });
}
static tax::Box3TaxModel Rules()
{
    return tax::Box3TaxModel(tax::Box3Parameters{ .taxYear = 2025, .taxFreeAllowance = Eur(57684.00), .depositReturnBasisPoints = 137,
        .investmentReturnBasisPoints = 588, .taxBasisPoints = 3600, .dividendWithholdingBasisPoints = 1500 });
}
static RollRules NeverRolls() { return RollRules(RollTriggers{ 0, 10000, Delta::FromValue(1.0) }); }
static RollRules HalfProfit() { return RollRules(RollTriggers{ 7, 5000, Delta::FromValue(0.45) }); }
static ProjectionPeriod P(Date o, Date e, double k, double p, double s) { return { o, e, Eur(k), p, Eur(s), std::nullopt, std::nullopt }; }

static tax::Box3Assets AssetsOf(const Portfolio& pf, std::span<const SpotPrice> prices)
{
    const auto equity = strategy::Equity(pf, prices);
    const auto deposits = std::min(pf.Cash(), equity);
    return { deposits, equity - deposits };
}

static void TaxAbort(int bufferBp)
{
    Book book({ "ACME" }, Portfolio::Funded(Eur(200000.00)), ReinvestmentPolicy::WithCashBuffer(bufferBp), NeverRolls(), Fees(), Rules());
    const std::vector<BookPeriod> cycle{ { "ACME", P(On(2025, 12, 1), On(2025, 12, 31), 40.00, 0.50, 35.00) } };
    const auto rows = book.Over(cycle);
    const std::vector<SpotPrice> marks{ { "ACME", Eur(35.00) } };
    const auto charge = Rules().Charge(AssetsOf(book.Holdings(), marks));
    std::cout << "buffer " << bufferBp << " bp: contracts " << rows[0].row.contracts << ", premium " << T(rows[0].row.premium) << ", assignment " << T(rows[0].row.assignment)
              << ", cash " << T(book.Holdings().Cash()) << ", free cash " << T(book.Holdings().FreeCash()) << ", shares " << rows[0].row.shares
              << ", equity at 35.00 " << T(strategy::Equity(book.Holdings(), marks)) << ", Box 3 charge " << T(charge) << std::endl;
    std::cout << "calling Book::Assess on 2026-01-01 ..." << std::endl;
    const auto paid = book.Assess({ On(2026, 1, 1), marks });
    std::cout << "assessed: paid " << T(paid) << ", cash after " << T(book.Holdings().Cash()) << std::endl;
}

static void AssessmentOutsidePeriod(bool after)
{
    auto wheel = Projection("ACME", Portfolio::Funded(Eur(200000.00)), ReinvestmentPolicy::WithCashBuffer(0), NeverRolls(), Fees(), Rules());
    auto period = P(On(2025, 7, 18), On(2026, 1, 16), 40.00, 1.00, 45.00);
    period.assessed = TaxObservation{ after ? On(2026, 1, 17) : On(2025, 7, 17), Eur(50.00) };
    std::cout << "assessment dated " << (after ? "2026-01-17 (one day after expiry)" : "2025-07-17 (one day before opened)") << " ..." << std::endl;
    const auto rows = wheel.Over(std::span<const ProjectionPeriod>(&period, 1));
    const auto entries = wheel.Ledger().Entries();
    const auto taxed = std::count_if(entries.begin(), entries.end(), [](const auto& e) { return e.kind == CashflowKind::Tax; });
    std::cout << "returned: row tax " << T(rows[0].tax) << ", tax entries in ledger " << taxed << ", cash " << T(rows[0].cash) << std::endl;
}

static void Rounding()
{
    for (const double p : { 0.8049, 0.8050, 0.8051, 0.80, 0.81 })
    {
        auto wheel = Projection("ACME", Portfolio::Funded(Eur(200000.00)), ReinvestmentPolicy::WithCashBuffer(0), NeverRolls(), Fees(), Rules());
        const auto period = P(On(2025, 7, 18), On(2026, 1, 16), 40.00, p, 45.00);
        const auto rows = wheel.Over(std::span<const ProjectionPeriod>(&period, 1));
        std::printf("premium per share %.4f: contracts %lld, row premium %s, exact p x 100 x n = %.2f\n", p, static_cast<long long>(rows[0].contracts), T(rows[0].premium).c_str(), p * 100.0 * static_cast<double>(rows[0].contracts));
    }
}

static void ListOrder()
{
    for (const auto listing : { std::vector<std::string>{ "A", "B" }, std::vector<std::string>{ "B", "A" } })
    {
        Book book(listing, Portfolio::Funded(Eur(20000.00)), ReinvestmentPolicy::WithCashBuffer(0), HalfProfit(), Fees(), Rules());
        auto a = P(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 45.00);
        auto b = P(On(2025, 1, 3), On(2025, 7, 18), 50.00, 1.50, 44.00);
        b.observed = RollObservation{ On(2025, 6, 18), 0.30, Delta::FromValue(-0.20) };
        std::vector<BookPeriod> cycle;
        for (const auto& u : listing)
            cycle.push_back({ u, u == "A" ? a : b });
        const auto rows = book.Over(cycle);
        std::cout << "book listed [" << listing[0] << ", " << listing[1] << "]:";
        for (const auto& row : rows)
            std::cout << "  " << row.underlying << ": contracts " << row.row.contracts << " premium " << T(row.row.premium) << " cash " << T(row.row.cash) << " equity " << T(row.row.equity) << ";";
        const std::vector<SpotPrice> marks{ { "A", Eur(45.00) }, { "B", Eur(44.00) } };
        std::cout << "  book equity after the cycle " << T(strategy::Equity(book.Holdings(), marks)) << std::endl;
    }
}

static void Fuzz(int bufferBp, bool guardTax)
{
    std::mt19937 rng(7);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    const std::vector<std::string> names{ "A", "B", "C" };
    Book book(names, Portfolio::Funded(Eur(150000.00)), ReinvestmentPolicy::WithCashBuffer(bufferBp), HalfProfit(), Fees(), Rules());
    std::vector<SpotPrice> lastMarks;
    int unaffordable = 0;
    for (const auto& n : names)
        lastMarks.push_back({ n, Eur(50.00) });
    int cycles = 0, rolled = 0, assigned = 0, wrote = 0, taxed = 0, violations = 0;
    Money taxPaid = Money::Zero(Currency::Eur);
    for (int c = 0; c != 400; ++c)
    {
        const auto opened = On(2025, 1, 3).AddDays(c * 30);
        const auto expiry = opened.AddDays(30);
        if (c % 12 == 11)
        {
            const auto before = book.Holdings().Cash();
            const auto due = Rules().Charge(AssetsOf(book.Holdings(), lastMarks));
            if (guardTax && due > book.Holdings().FreeCash())
            {
                ++unaffordable;
                std::cerr << "cycle " << c << ": charge " << T(due) << " exceeds free cash " << T(book.Holdings().FreeCash()) << " (shares held: " << book.Holdings().Shares("A") + book.Holdings().Shares("B") + book.Holdings().Shares("C") << "); skipped" << std::endl;
                continue;
            }
            const auto paid = book.Assess({ expiry.AddDays(-5), lastMarks });
            taxPaid += paid;
            if (paid.Cents() > 0) ++taxed;
            if (book.Holdings().Cash() != before - paid) ++violations;
        }
        std::vector<BookPeriod> cycle;
        std::vector<SpotPrice> marks;
        for (const auto& n : names)
        {
            const double strike = 20.0 + std::floor(u(rng) * 61.0);
            const double premium = std::round(strike * (0.005 + 0.035 * u(rng)) * 10000.0) / 10000.0;
            const double spot = std::round(strike * (0.80 + 0.40 * u(rng)) * 100.0) / 100.0;
            auto p = P(opened, expiry, strike, premium, spot);
            if (book.StateOf(n) == WheelState::Cash && u(rng) < 0.6)
                p.observed = RollObservation{ expiry.AddDays(-10), std::round(premium * (u(rng) * 2.0) * 10000.0) / 10000.0, Delta::FromValue(-(0.05 + 0.6 * u(rng))) };
            cycle.push_back({ n, p });
            marks.push_back({ n, Eur(spot) });
        }
        const auto cashBefore = book.Holdings().Cash();
        std::cerr << "cycle " << c << " cash " << T(cashBefore) << "\r";
        const auto rows = book.Over(cycle);
        ++cycles;
        auto moved = Money::Zero(Currency::Eur);
        for (const auto& r : rows)
        {
            moved += r.row.premium - r.row.fees + r.row.assignment - r.row.tax;
            if (r.row.contracts > 0) ++wrote;
            if (r.row.assignment.Cents() != 0) ++assigned;
            if (r.row.contracts > 0 && r.row.assignment.Cents() == 0 && r.row.fees == Fees().For(costs::Trade::Options(r.row.contracts)) * 2) ++rolled;
        }
        const auto& pf = book.Holdings();
        if (!strategy::Reconciles(book.Holds())) ++violations;
        if (pf.Cash().Cents() < 0 || pf.Collateral().Cents() != 0 || !pf.ShortPositions().empty()) ++violations;
        if (pf.Cash() != cashBefore + moved) ++violations;
        if (rows.back().row.cash != pf.Cash() || rows.back().row.equity != strategy::Equity(pf, marks)) ++violations;
        const auto entries = book.Ledger().Entries();
        if (!std::is_sorted(entries.begin(), entries.end(), [](const auto& l, const auto& r) { return l.on < r.on; })) ++violations;
        lastMarks = marks;
    }
    std::cout << "fuzz (buffer " << bufferBp << " bp): " << cycles << " cycles x 3 underlyings; options written " << wrote << ", rolled " << rolled << ", assigned " << assigned << ", assessments charged " << taxed
              << " (total " << T(taxPaid) << "); final cash " << T(book.Holdings().Cash()) << ", ledger entries " << book.Ledger().Entries().size() << "; charges skipped as unaffordable " << unaffordable << "; invariant violations " << violations << std::endl;
}

static void FeeAbort()
{
    auto wheel = Projection("ACME", Portfolio::Funded(Eur(4000.00)), ReinvestmentPolicy::WithCashBuffer(0), NeverRolls(), Fees(), Rules());
    const auto period = P(On(2025, 7, 18), On(2026, 1, 16), 40.00, 0.004, 45.00);
    std::cout << "capital 4000.00, strike 40.00, premium 0.004 per share (rounds to 0.00): writing ..." << std::endl;
    const auto rows = wheel.Over(std::span<const ProjectionPeriod>(&period, 1));
    std::cout << "returned: contracts " << rows[0].contracts << " premium " << T(rows[0].premium) << " fees " << T(rows[0].fees) << std::endl;
}

int main(int argc, char** argv)
{
    const std::string which = argc > 1 ? argv[1] : "";
    if (which == "E1") TaxAbort(0);
    else if (which == "E1b") TaxAbort(500);
    else if (which == "E2a") AssessmentOutsidePeriod(true);
    else if (which == "E2b") AssessmentOutsidePeriod(false);
    else if (which == "E3") Rounding();
    else if (which == "E4") ListOrder();
    else if (which == "E5") Fuzz(300, false);
    else if (which == "E5g") Fuzz(300, true);
    else if (which == "E5b") Fuzz(1000, true);
    else if (which == "E6") FeeAbort();
    else { std::cerr << "unknown experiment\n"; return 2; }
    return 0;
}
```

```text
### E1
buffer 0 bp: contracts 50, premium 2500.00, assignment -200000.00, cash 2459.50, free cash 2459.50, shares 5000, equity at 35.00 177459.50, Box 3 charge 2508.46
calling Book::Assess on 2026-01-01 ...
demo_strat: /home/user/fin-bs/source/strategy/Portfolio.cpp:152: void fin::strategy::Portfolio::Pay(const fin::core::Money&): Assertion `FreeCash() >= amount' failed.
exit status 134
### E1b
buffer 500 bp: contracts 47, premium 2350.00, assignment -188000.00, cash 14311.75, free cash 14311.75, shares 4700, equity at 35.00 178811.75, Box 3 charge 2406.63
calling Book::Assess on 2026-01-01 ...
assessed: paid 2406.63, cash after 11905.12
exit status 0
### E2a
assessment dated 2026-01-17 (one day after expiry) ...
returned: row tax 0.00, tax entries in ledger 0, cash 204962.50
exit status 0
### E2b
assessment dated 2025-07-17 (one day before opened) ...
demo_strat: /home/user/fin-bs/source/strategy/Projection.cpp:232: void fin::strategy::Projection::Assess(const fin::strategy::ProjectionPeriod&, const fin::core::Date&): Assertion `open->assessment->on >= period.opened && open->assessment->on <= period.expiry' failed.
exit status 134
### E3
premium per share 0.8049: contracts 50, row premium 4000.00, exact p x 100 x n = 4024.50
premium per share 0.8050: contracts 50, row premium 4000.00, exact p x 100 x n = 4025.00
premium per share 0.8051: contracts 50, row premium 4050.00, exact p x 100 x n = 4025.50
premium per share 0.8000: contracts 50, row premium 4000.00, exact p x 100 x n = 4000.00
premium per share 0.8100: contracts 50, row premium 4050.00, exact p x 100 x n = 4050.00
exit status 0
### E4
book listed [A, B]:  A: contracts 0 premium 0.00 cash 20597.00 equity 18197.00;  B: contracts 4 premium 480.00 cash 20474.00 equity 20474.00;  book equity after the cycle 20474.00
book listed [B, A]:  B: contracts 4 premium 480.00 cash 20474.00 equity 20474.00;  A: contracts 0 premium 0.00 cash 20474.00 equity 20474.00;  book equity after the cycle 20474.00
exit status 0
### E5
demo_strat: /home/user/fin-bs/source/strategy/Portfolio.cpp:152: void fin::strategy::Portfolio::Pay(const fin::core::Money&): Assertion `FreeCash() >= amount' failed.
exit status 134
### E6
capital 4000.00, strike 40.00, premium 0.004 per share (rounds to 0.00): writing ...
demo_strat: /home/user/fin-bs/source/strategy/Portfolio.cpp:152: void fin::strategy::Portfolio::Pay(const fin::core::Money&): Assertion `FreeCash() >= amount' failed.
exit status 134
### E5g
cycle 35: charge 2522.40 exceeds free cash 1380.93 (shares held: 2900); skipped
fuzz (buffer 300 bp): 399 cycles x 3 underlyings; options written 1132, rolled 205, assigned 463, assessments charged 32 (total 364902199.05); final cash 1331673930.95, ledger entries 3632; charges skipped as unaffordable 1; invariant violations 0
exit status 0
### E5b
cycle 119: charge 172093.81 exceeds free cash 82636.93 (shares held: 180600); skipped
fuzz (buffer 1000 bp): 399 cycles x 3 underlyings; options written 1182, rolled 211, assigned 469, assessments charged 32 (total 189581266688.46); final cash 511082481013.29, ledger entries 3756; charges skipped as unaffordable 1; invariant violations 0
exit status 0
```
