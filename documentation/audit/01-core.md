# Audit phase 1: `source/core`

*Date: 2026-10-18. Scope: the seven value types in `source/core` and their tests. Report only.*

## Summary

`source/core` is sound. Every algorithm that turns into money or time was reproduced
independently and agrees exactly: the civil-date arithmetic over 800 years day by day and over
years 1–9999 sampled, business-day counting against brute force, integer half-to-even division
against a decimal reference over 200,000 cases, and every worked-example figure in the four theory
documents. Overflow checks are present on every integer operation and stay active in Release
builds. No finding changes a number for a realistic input.

Two Medium findings concern promises rather than arithmetic: the conversion from a decimal amount to
cents rounds the *binary* product, not the decimal, which the documentation and a test name
suggest otherwise (CORE-2); and the refusals of that conversion for non-finite or out-of-range
input are not specified by any test although decision D4 promises every refusal is (CORE-1).

| Severity | Count |
|---|---|
| Critical | 0 |
| High | 0 |
| Medium | 2 |
| Low | 6 |
| Info | 5 |

## What was executed

**Build route.** This container has no Qt. `apt-get install qt6-base-dev` provides Qt 6.4.2 and
the root `CMakeLists.txt` requires 6.5, so the repository presets cannot configure here. The tests
were built with a harness outside the repository: a CMake project that repeats the root file's
`FetchContent` pins and options for embedded-infra-lib and numerical-toolbox-cpp, sets C++20 and
warnings-as-errors, and adds the nine non-GUI modules (`core costs estimation io optimize pricing
sim strategy tax`) with `add_subdirectory`. Nothing in the repository was changed. GCC 13.3.0,
Ninja, Debug configuration, executed 2026-10-19.

```text
$ cmake -S harness -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build --target fin.core_test
$ (cd build && ctest -R fin.core_test --output-on-failure)
1/1 Test #1: fin.core_test ....................   Passed    0.02 sec
100% tests passed, 0 tests failed out of 1
$ build/source/core/test/fin.core_test
[==========] Running 62 tests from 8 test suites.
[==========] 62 tests from 8 test suites ran. (16 ms total)
[  PASSED  ] 62 tests.
```

Files read in full: `Money.hpp`, `Date.hpp`, `DayCount.hpp`, `Rate.hpp`, `Volatility.hpp`,
`Delta.hpp`, `OptionContract.hpp`, all eight test files, `design/quantities.md`, decisions D1–D7,
`theory/money-and-rounding.md`, `theory/day-count-conventions.md`,
`theory/rates-and-compounding.md`, `theory/option-contract-conventions.md`, and
`infra/util/ReallyAssert.hpp` at the pinned embedded-infra-lib commit `2848dbf`.

## What was verified independently

The script in the appendix produced every figure below.

| Check | Method | Result |
|---|---|---|
| Date ↔ year/month/day, every day 1600-01-01 to 2400-12-31 | replicate `DaysFromCivil`/`CivilFromDays` with C++ truncating division, compare with Python `datetime` | 0 mismatches in 292,547 days |
| Same, sampled over years 1–9999 plus every leap day | as above | 0 mismatches |
| Weekday phase, every day 1600–2400 | `((d+3)%7+7)%7` vs `datetime.weekday()` | 0 mismatches |
| Supported date range for `int32_t` days | arithmetic | years −5,877,641 to 5,881,580 |
| `BusinessDaysBetween`, 3,000 random half-open pairs up to 400 days | whole-weeks formula vs day-by-day count | 0 mismatches |
| Day-count worked examples | 2025-01-17→02-21: 35 days, 25 business; 2024: 262; 2023: 260; 2025-01-17→03-21: 63 | all reproduced |
| Weekend-only business days per year, 1970–2069 | brute force | 260.89 average, consistent with the documented "roughly 261" |
| `DivideRoundHalfToEven`, 200,000 random signed pairs | vs `decimal.ROUND_HALF_EVEN` | 0 mismatches |
| `CheckedAdd`/`CheckedSubtract`/`CheckedMultiply` conditions | inspection against the standard pre-checks | correct; `Multiply` is conservative only at `INT64_MIN × ±1` |
| `FromAmount(c/100) == c` for every cent count up to ±2,000,000 | replicate `RoundHalfToEven` | 0 mismatches |
| Money worked example | 12.345→1234; ×1.5%→19; 5/3→2; 5/2→2; 7/2→4; −5/2→−2 | all reproduced |
| Rate figures | ln 1.04 = 0.0392207131532813; ln 1.20 = 0.1823215567939546; 100·e^(−ln1.04·45/365) = 99.51762 | match tests and docs |
| Volatility figures | 0.0125·√252 = 0.198431; over 30/365 of a year: 0.056888 | match |
| Quantities worked example | 63/365 = 0.172603; 0.24·√0.172603 = 0.099709; discount 0.993253 | match |
| `really_assert` survives Release builds | emil `ReallyAssert.hpp` at `2848dbf`: with `NDEBUG` it expands to `if (!(c)) std::abort();` | yes, in every build |
| Documentation links from `design/quantities.md` | files exist | all present |

## Findings

| ID | Severity | Where | Title |
|---|---|---|---|
| CORE-1 | Medium | `Money.hpp:68-84`, `test/TestMoney.cpp:145-152` | Refusals of `FromAmount` and `Money × double` are not specified by a test |
| CORE-2 | Medium | `Money.hpp:97-100`, `design/quantities.md`, `theory/money-and-rounding.md` | `FromAmount` rounds the binary product, not the decimal amount; docs and a test name claim decimal half-to-even |
| CORE-3 | Low | `Money.hpp:152-156` | `Money × integral` silently narrows any integer type to `int64_t` |
| CORE-4 | Low | `Rate.hpp:12-15`, `Volatility.hpp:12-16` | `Rate::FromContinuous` accepts NaN and ±∞; `Volatility::FromAnnualized` accepts +∞ |
| CORE-5 | Low | `Date.hpp:84-87, 119-127` | Unchecked `int32_t` arithmetic in `AddDays`, `operator-` and `FromDaysSinceEpoch` |
| CORE-6 | Low | `ReallyAssert.hpp` (emil), all of `core` | In Release builds an invariant failure aborts with no message |
| CORE-7 | Low | `test/TestDate.cpp:33-42` | Date round trip is sampled every 13th day; negative-era branches never executed |
| CORE-8 | Low | `test/TestOptionContract.cpp` | Cross-currency contract comparison is documented as a defect but not pinned by a test |
| CORE-9 | Info | `Money.hpp:39-43` | `CheckedMultiply` rejects two representable products at `INT64_MIN` |
| CORE-10 | Info | `Money.hpp:68-84` | Tie detection in `RoundHalfToEven` is inexact for values in (−1, 0) within one ulp of a half |
| CORE-11 | Info | `Money.hpp:90` | One minor-unit scale for all currencies |
| CORE-12 | Info | `OptionContract.hpp:70` | `std::string` underlying makes every copy allocate; the type is compared in hot loops |
| CORE-13 | Info | `.clang-tidy`, `test/TestToolboxLinkage.cpp` | Static analysis is one check; the toolbox smoke test is `float` only |

### CORE-1 — Refusals of `FromAmount` and `Money × double` are not tested (Medium)

**Evidence.** `detail::RoundHalfToEven` refuses a non-finite input and an input outside the
`int64_t` range (`Money.hpp:72`, `:75`). It is reached from `Money::FromAmount` (`:99`) and from
`operator*(floating_point)` (`:161`). `DivideRoundHalfToEven` refuses `INT64_MIN / −1` (`:48`).
The death tests in `TestMoney.cpp:145-152` cover addition, subtraction, integer multiplication,
negation and division by zero only. There is no `EXPECT_DEATH` for `Money::FromAmount(NaN)`,
`Money::FromAmount(1e17)`, `Eur(x) * 1e30`, or `Eur(INT64_MIN) / -1`.

**Impact.** Decision D4 states that "every one of those refusals is specified by a test, so 'this
stops the run' is a promise rather than a comment". These four are comments today. The production
callers of these paths are exactly the places a NaN or an overflow would originate: an FX
conversion (`costs/FxModel.cpp:31,33`), a simulated close (`sim/GbmPathGenerator.cpp:64`) and a
model value becoming a cashflow (`strategy/Projection.cpp:16`). A future edit that weakened the
check would not be caught.

**Proposed fix.** None in code. **Suggested tests.** Add to `rejects_overflowing_arithmetic` or a
new `rejects_amounts_that_are_not_finite_or_not_representable`:
`FromAmount(std::numeric_limits<double>::quiet_NaN())`, `FromAmount(infinity)`,
`FromAmount(1e17)` (1e19 cents), `FromAmount(-1e17)`, `Eur(1) * 1e30`, `Eur(int64Min) / -1`.

### CORE-2 — `FromAmount` rounds the binary product, not the decimal amount (Medium)

**Evidence.** `Money::FromAmount(double amount, …)` computes `RoundHalfToEven(amount * 100.0)`
(`Money.hpp:99`). The input is already a binary double, so a decimal such as 1.015 is
1.01499999999999990230…, the product is 101.49999999999998579 and the result is 101 cents. Decimal
half-to-even gives 102. Across all 40,001 exact half-cent decimals in ±200.00, the two rules
disagree in 2,292 cases (5.7%), in a direction that depends on the binary expansion, not on parity.
The test `converts_an_amount_to_cents_with_half_to_even_rounding` (`TestMoney.cpp:48-57`) passes
because its five literals (12.345, 0.005, 0.015, −0.005, −0.015) all happen to produce a product
that is exactly representable as `.5`. The table in `design/quantities.md` ("Multiplication by a
fraction rounds half-to-even") and the worked example in `theory/money-and-rounding.md`
("EUR 12.345 as cents → 1234, tie") describe the decimal rule.

**Impact.** No wrong number today. All three production callers pass continuous model values,
for which a true tie has probability zero and the binary rule is the right one (decision D7). The
risk is twofold. First, the rationale given for half-to-even in `money-and-rounding.md` is that
quoted amounts carry "a genuine atom at exactly one half"; through this entry point that atom is
*not* resolved by parity, so the stated zero-bias argument does not apply to it. Second, the
function reads as a general decimal-to-cents conversion and a future caller could use it on a
quoted premium, a strike, or a configuration value, reintroducing exactly the error decision D16
exists to prevent. The io loader is documented as the exact decimal route; whether it is the only
one is checked in phase 4.

**Proposed fix.** Rename or document: either `FromAmount` → `FromModelValue` with a comment that
ties are resolved on the binary product, or keep the name and add one sentence to
`money-and-rounding.md` and `quantities.md` stating that only cashflows derived from whole cents
and integer or basis-point factors are rounded under the decimal rule, and that a decimal literal
or string must never enter through this function. **Suggested test.** Replace the decimal literals
in the tie test with values exact in binary (0.125, 0.375, −0.625, 2.5), and add an explicit
non-tie case (1.015 → 101) with a comment, so that the test documents the real behaviour instead
of relying on coincidence.

### CORE-3 — `Money × integral` narrows any integer type silently (Low)

**Evidence.** `template<std::integral T> Money operator*(T quantity)` does
`static_cast<int64_t>(quantity)` (`Money.hpp:155`). `std::integral` admits `bool`, `char`,
`uint64_t` and `size_t`; a `uint64_t` of 2^63 or more becomes negative without any check.

**Impact.** None today: the three production call sites pass `std::int64_t`
(`strategy/Projection.cpp:21`, `strategy/Portfolio.cpp:162,172`). A future `contracts.size()`
would compile and, for an absurd count, flip the sign rather than stop the run.

**Proposed fix.** Constrain to signed types (`std::signed_integral`) or add
`really_assert(std::in_range<int64_t>(quantity))`. **Suggested test.** A compile-time check that
`Money * size_t` is ill-formed, or a death test for `uint64_t(1) << 63`.

### CORE-4 — `Rate::FromContinuous` accepts NaN and infinities (Low)

**Evidence.** `Rate::FromContinuous` stores its argument unchecked (`Rate.hpp:12-15`).
`FromEffectiveAnnual` rejects NaN as a side effect of `> -1.0` (`Rate.hpp:19`); `Delta::FromValue`
rejects NaN and ±∞ through its range check; `Volatility::FromAnnualized` rejects NaN but admits
+∞ (`Volatility.hpp:14`).

**Impact.** A NaN rate would propagate as NaN discount factors and NaN option values without
tripping any assertion in `core`. Current callers are bounded (`io/ScenarioLoader.cpp:162-191`,
`present/ScenarioDocument.cpp:256-272`) or arithmetic on finite inputs
(`sim/GbmPathGenerator.cpp:37`, `estimation/MarketParameters.cpp:19`). One caller takes a fitted
slope (`estimation/DriftEstimator.cpp:29`); whether that can be NaN is checked in phase 3.

**Proposed fix.** `really_assert(std::isfinite(x))` in `FromContinuous`, `FromAnnualized` and
`FromVariance` (and `OverHorizon`, `GrowthFactor`, `DiscountFactor` for the year fraction).
Decision D6 says admissible values are stated at creation; "finite" is part of that.
**Suggested test.** Death tests for NaN and ±∞ in each factory.

### CORE-5 — Unchecked `int32_t` arithmetic in `Date` (Low)

**Evidence.** `AddDays` computes `daysSinceEpoch + days` (`Date.hpp:121`), `operator-` computes a
difference (`:126`) and `FromDaysSinceEpoch` accepts any `int32_t` (`:84-87`); `CivilFromDays`
adds 719,468 to the day count (`:43`), which overflows for day counts above 2,146,764,179.

**Impact.** Signed overflow is undefined behaviour, and the compiler is allowed to assume it does
not happen. Reaching it needs a date about 5.8 million years away, so this is unreachable from
any valid input. It is still the one place in `core` where an impossible state would not stop the
run (decision D4), and the check costs two lines.

**Proposed fix.** `really_assert` the result range in `AddDays` (for example via
`__builtin_add_overflow` or a bound on `|days|`), and bound `FromDaysSinceEpoch` to the range
`CivilFromDays` supports. **Suggested test.** Death tests at the bounds.

### CORE-6 — A Release-build invariant failure is silent (Low)

**Evidence.** With `NDEBUG`, `really_assert` is `if (!(c)) { std::abort(); }`; without it, it is
`assert(c)`, which prints the expression, file and line. The `host-Release` preset defines
`NDEBUG`.

**Impact.** Decision D4 promises a defect stops the run "immediately and loudly". In a Release
build of the desktop application the user sees a process exit with no diagnostic, and cannot tell
a currency mismatch from an overflow or from a crash elsewhere. Not a numbers issue; a usability
and supportability one, which matters once the tool is used in earnest.

**Proposed fix.** Either build the application with assertions that print (keep `NDEBUG`
undefined for `fin.*` targets, which has no performance cost worth measuring here), or install a
`std::set_terminate`/`SIGABRT` handler in `app` that writes the last known step and a request to
report the failure. Upstream, a `really_assert` variant that prints before aborting would be the
cleanest. **Suggested test.** None; verify by provoking a mismatch in a Release build.

### CORE-7 — Date round trip is sampled; negative-era branches never run (Low)

**Evidence.** `round_trips_through_year_month_day_over_two_centuries` steps by 13 days from
1900 to 2100 (`TestDate.cpp:35`). The `shiftedYear < 0` branch in `DaysFromCivil` (`Date.hpp:32`)
and the `shifted < 0` branch in `CivilFromDays` (`:44`) are only taken for dates before 1 March of
year 0 and are never executed by any test. The earliest tested date is 1900-01-01.

**Impact.** The Python replica confirms both branches are correct, so this is a coverage gap, not
a defect. A full daily sweep over 1600–2400 is 292,547 iterations and runs in milliseconds.

**Proposed fix.** None in code. **Suggested test.** Sweep every day over at least 1600–2400
(covers the 1700, 1800, 1900 non-leap centuries and 2000, 2400 leap centuries), and add one date
in a negative year (for example −1-12-31 → 0-01-01) to execute both negative branches.

### CORE-8 — Cross-currency contract comparison not pinned by a test (Low)

**Evidence.** `design/quantities.md` states "Contract comparison inherits the currency rule
through the strike, so a chain containing two currencies cannot be sorted without converting
first". `OptionContract::operator<=>` is defaulted (`OptionContract.hpp:67`) and reaches
`Money::operator<=>`, which asserts. `TestOptionContract.cpp` has no `EXPECT_DEATH` for comparing
or equating a EUR-strike contract with a USD-strike one.

**Impact.** Coverage only. If a future change reordered the members so that `type` or `expiry`
compared before `strike`, a mixed-currency chain could be *partially* sorted without stopping, and
no test would notice.

**Proposed fix.** None in code. **Suggested test.** `EXPECT_DEATH` on `==` and `<` between
contracts whose strikes differ only in currency.

### CORE-9 — `CheckedMultiply` is conservative at `INT64_MIN` (Info)

`Magnitude(INT64_MIN) = 2^63 > INT64_MAX / 1`, so `INT64_MIN × 1` and `1 × INT64_MIN` are refused
although representable. Harmless; noted so that nobody "fixes" the check in a way that loses the
guard for `INT64_MIN × −1`.

### CORE-10 — Inexact tie detection for small negative values (Info)

`fraction = value − floor(value)` is exact for |value| ≥ 1 and for value in [−1, −0.5] (Sterbenz),
but for value in (−0.5, 0) the addition of 1 can round, so a value within one ulp of −0.5 (about
10⁻¹⁶) registers as a tie. The outcome differs from the exact answer by at most one cent and only
for inputs that are themselves meaningless at that precision. No action.

### CORE-11 — One minor-unit scale for all currencies (Info)

`minorUnitsPerUnit = 100` is a class constant (`Money.hpp:90`). Correct for EUR and USD, the only
two currencies in the closed set. Adding a currency with a different scale (JPY, KWD) would need
this to become a property of `Currency`. Worth one line in `quantities.md`.

### CORE-12 — `OptionContract` copies allocate (Info)

The underlying is a `std::string` (`OptionContract.hpp:70`). Every copy of a contract allocates
and every comparison compares strings first. Correctness is unaffected; whether it matters for the
Monte-Carlo inner loop is a phase 9 question (an interned symbol or `std::string_view` into a
registry would remove it).

### CORE-13 — Static analysis is a single check; the toolbox smoke test is `float` (Info)

`.clang-tidy` enables only `modernize-use-override`. For a money library, `bugprone-*`,
`clang-analyzer-*`, `cppcoreguidelines-narrowing-conversions` and `misc-*` would be cheap to run
on `source/`. Separately, `TestToolboxLinkage.cpp` evaluates `math::Erfc` in `float` to 10⁻⁶;
that is fine as a linkage check, but whether the pricing module's normal CDF goes through the same
single-precision function (decision D3 says model arithmetic is double) is checked in phase 6.

## Observations carried forward to later phases

These were seen while mapping how other modules cross the `Money` boundary. They are not `core`
findings and are not yet assessed; they are listed so they are not lost.

| For phase | Where | Observation |
|---|---|---|
| 7 strategy | `strategy/Projection.cpp:16` | `Money::FromAmount(perShare, currency) * multiplier` rounds the per-share value to a cent and *then* multiplies by 100. Decision D7 says the multiplier and the rounding are applied together, once. Rounding first makes the per-contract error up to ±50 cents per cashflow instead of ±0.5 cent. Unbiased, but 100× the documented precision loss. |
| 2 costs | `costs/FxModel.cpp:31,33` | Conversion goes `ToDouble()` → multiply or divide → `FromAmount`. This is the one sanctioned conversion point (D19); check the rounding of the fee and the round-trip cost. |
| 2 costs | `costs/*` | Decision D18 says proportional costs are whole basis points; confirm they are computed as `(cents × bps) / 10000` in integers and not via `Money × double`. |
| 3 estimation | `estimation/DriftEstimator.cpp:29` | `Rate::FromContinuous(*slope)`: can the fitted slope be NaN or infinite (constant series, two points)? See CORE-4. |
| 4 io | `io/*` | Confirm every decimal in a file enters through an exact parser (D16, D48) and never through `FromAmount`. |
| 9 sim | `sim/GbmPathGenerator.cpp:64` | Simulated closes are rounded to cents with `FromAmount(exp(logPrice))`; confirm the rounding happens once per close and is not fed back into the next step's log-price. |

## Open questions for the owner

1. Is `Money::FromAmount` ever meant to take a *quoted* decimal (premium, strike, dividend per
   share from a chain snapshot)? If yes, the exact decimal parser should be the only route and
   `FromAmount` should be renamed (CORE-2). If no, a one-line comment saying so is enough.
2. Should a Release build print which invariant failed before it aborts (CORE-6)? For a desktop
   application used with real money this is the difference between a bug report and a shrug.
3. ACT/252 counts weekends only. Measured over 1970–2069 that is 260.9 business days a year
   against the 252 denominator, a systematic +3.5% on every ACT/252 year fraction, as the theory
   note says. Is a holiday calendar planned before any ACT/252 figure is compared with a broker
   quote?

## Not checked

- Compilation with `-Wall -Wextra -Wconversion` and with sanitizers; the repository builds with
  warnings as errors but the warning set is the compiler default.
- Behaviour on 32-bit or non-two's-complement targets (irrelevant for the stated toolchains).
- `theory/volatility-scaling.md` content beyond the formulas `Volatility` implements.
- Anything in the external libraries beyond `ReallyAssert.hpp` and the `math::Erfc` call.

## Appendix: reference-check script

Run with `python3 -I check_core.py`. Output as produced on 2026-10-18 follows the script.

```python
import datetime as dt, random, math
from decimal import Decimal, ROUND_HALF_EVEN

def cdiv(a, b):  # C++ truncating division
    q = abs(a) // abs(b)
    return q if (a >= 0) == (b > 0) else -q

def days_from_civil(y, m, d):
    y2 = y - (1 if m <= 2 else 0)
    era = cdiv(y2 if y2 >= 0 else y2 - 399, 400)
    yoe = y2 - era * 400
    mp = m - 3 if m > 2 else m + 9
    doy = (153 * mp + 2) // 5 + d - 1
    doe = yoe * 365 + yoe // 4 - yoe // 100 + doy
    return era * 146097 + doe - 719468

def civil_from_days(z):
    z2 = z + 719468
    era = cdiv(z2 if z2 >= 0 else z2 - 146096, 146097)
    doe = z2 - era * 146097
    yoe = (doe - doe // 1460 + doe // 36524 - doe // 146096) // 365
    doy = doe - (365 * yoe + yoe // 4 - yoe // 100)
    mp = (5 * doy + 2) // 153
    d = doy - (153 * mp + 2) // 5 + 1
    m = mp + 3 if mp < 10 else mp - 9
    return (yoe + era * 400 + (1 if m <= 2 else 0), m, d)

def cpp_mod(a, b):
    r = abs(a) % abs(b)
    return r if a >= 0 else -r

epoch = dt.date(1970, 1, 1)
def D(y, m, d): return (dt.date(y, m, d) - epoch).days

start, end = D(1600, 1, 1), D(2400, 12, 31)
bad = wbad = 0
for z in range(start, end + 1):
    d = epoch + dt.timedelta(days=z)
    if civil_from_days(z) != (d.year, d.month, d.day) or days_from_civil(d.year, d.month, d.day) != z:
        bad += 1
    if cpp_mod(cpp_mod(z + 3, 7) + 7, 7) != d.weekday():
        wbad += 1
print("Date daily sweep 1600-2400: mismatches", bad, "weekday mismatches", wbad)
sbad = 0
for y in range(1, 10000, 7):
    for (m, d) in [(1, 1), (2, 28), (3, 1), (12, 31), (7, 15)]:
        z = D(y, m, d)
        if days_from_civil(y, m, d) != z or civil_from_days(z) != (y, m, d):
            sbad += 1
for y in range(4, 10000, 4):
    if (y % 100 != 0) or (y % 400 == 0):
        if civil_from_days(D(y, 2, 29)) != (y, 2, 29):
            sbad += 1
print("Date sampled sweep years 1-9999: mismatches", sbad)
print("int32 range of days covers years", civil_from_days(-2**31)[0], "to", civil_from_days(2**31 - 1)[0])

def biz_cpp(s, e):
    weeks = (e - s) // 7
    n = weeks * 5
    z = s + weeks * 7
    while z < e:
        n += (epoch + dt.timedelta(days=z)).weekday() < 5
        z += 1
    return n
def biz_brute(s, e):
    return sum(1 for z in range(s, e) if (epoch + dt.timedelta(days=z)).weekday() < 5)
random.seed(1)
bbad = 0
for _ in range(3000):
    s = random.randint(start, end - 400)
    e = s + random.randint(0, 400)
    bbad += biz_cpp(s, e) != biz_brute(s, e)
print("BusinessDaysBetween vs brute force (3000 random pairs): mismatches", bbad)
print("2025-01-17..2025-02-21 days", D(2025, 2, 21) - D(2025, 1, 17), "biz", biz_brute(D(2025, 1, 17), D(2025, 2, 21)))
print("2025-01-17..2025-03-21 days", D(2025, 3, 21) - D(2025, 1, 17))
print("2024 biz days", biz_brute(D(2024, 1, 1), D(2025, 1, 1)), "2023 biz days", biz_brute(D(2023, 1, 1), D(2024, 1, 1)))
print("avg business days/year 1970-2069 (no holidays):", biz_brute(D(1970, 1, 1), D(2070, 1, 1)) / 100)

def round_half_even_double(v):
    lower = math.floor(v)
    frac = v - lower
    r = int(lower)
    if frac > 0.5 or (frac == 0.5 and r % 2 != 0):
        r += 1
    return r
def div_half_even_cpp(n, d):
    q = cdiv(n, d); r = n - q * d
    if r == 0: return q
    twice, ad = abs(r) * 2, abs(d)
    step = -1 if (n < 0) != (d < 0) else 1
    if twice > ad: return q + step
    if twice < ad: return q
    return q if q % 2 == 0 else q + step
random.seed(2)
dbad = 0
for _ in range(200000):
    n = random.randint(-10**12, 10**12)
    d = random.choice([random.randint(-10**6, 10**6) or 1, random.choice([2, -2, 3, 4, 5, -5, 7, 10, 100, 10000])])
    ref = int((Decimal(n) / Decimal(d)).quantize(Decimal(1), rounding=ROUND_HALF_EVEN))
    dbad += div_half_even_cpp(n, d) != ref
print("DivideRoundHalfToEven vs Decimal ROUND_HALF_EVEN (200k random): mismatches", dbad)
fbad = sum(round_half_even_double((c / 100) * 100.0) != c for c in range(-2_000_000, 2_000_001))
print("FromAmount(c/100)==c for |c|<=2e6: mismatches", fbad)
mis = []
for k in range(-20000, 20001):
    dec = Decimal(2 * k + 1) / Decimal(200)
    want = int((dec * 100).quantize(Decimal(1), rounding=ROUND_HALF_EVEN))
    got = round_half_even_double(float(dec) * 100.0)
    if got != want:
        mis.append((str(dec), want, got))
print("FromAmount on exact half-cent decimals (40001 cases): differ from decimal half-to-even in", len(mis), "cases; examples:", mis[:6])
for v in [12.345, 0.005, 0.015, -0.005, -0.015, 1.234, 1.236, 1.015, 2.675, 0.125, 0.135]:
    print(f"  FromAmount({v}) -> {round_half_even_double(v * 100.0)}   (product {v * 100.0!r})")
for cents, f in [(1234, 0.015), (5, 0.5), (7, 0.5), (-5, 0.5), (-7, 0.5)]:
    print(f"  {cents} * {f} -> {round_half_even_double(cents * f)}")
print("ln(1.04) =", math.log1p(0.04), " ln(1.20) =", math.log1p(0.20))
print("100*exp(-ln1.04*45/365) =", 100 * math.exp(-math.log1p(0.04) * 45 / 365))
print("0.0125*sqrt(252) =", 0.0125 * math.sqrt(252), " overHorizon(30/365) =", 0.0125 * math.sqrt(252) * math.sqrt(30 / 365))
print("worked example: 63/365 =", 63 / 365, " 0.24*sqrt(63/365) =", 0.24 * math.sqrt(63 / 365), " DF =", math.exp(-math.log1p(0.04) * 63 / 365))
```

```text
Date daily sweep 1600-2400: mismatches 0 weekday mismatches 0
Date sampled sweep years 1-9999: mismatches 0
int32 range of days covers years -5877641 to 5881580
BusinessDaysBetween vs brute force (3000 random pairs): mismatches 0
2025-01-17..2025-02-21 days 35 biz 25
2025-01-17..2025-03-21 days 63
2024 biz days 262 2023 biz days 260
avg business days/year 1970-2069 (no holidays): 260.89
DivideRoundHalfToEven vs Decimal ROUND_HALF_EVEN (200k random): mismatches 0
FromAmount(c/100)==c for |c|<=2e6: mismatches 0
FromAmount on exact half-cent decimals (40001 cases): differ from decimal half-to-even in 2292 cases; examples: [('-163.795', -16380, -16379), ('-163.705', -16370, -16371), ('-163.645', -16364, -16365), ('-163.575', -16358, -16357), ('-163.515', -16352, -16351), ('-163.485', -16348, -16349)]
  FromAmount(12.345) -> 1234   (product 1234.5)
  FromAmount(0.005) -> 0   (product 0.5)
  FromAmount(0.015) -> 2   (product 1.5)
  FromAmount(-0.005) -> 0   (product -0.5)
  FromAmount(-0.015) -> -2   (product -1.5)
  FromAmount(1.234) -> 123   (product 123.4)
  FromAmount(1.236) -> 124   (product 123.6)
  FromAmount(1.015) -> 101   (product 101.49999999999999)
  FromAmount(2.675) -> 268   (product 267.5)
  FromAmount(0.125) -> 12   (product 12.5)
  FromAmount(0.135) -> 14   (product 13.5)
  1234 * 0.015 -> 19
  5 * 0.5 -> 2
  7 * 0.5 -> 4
  -5 * 0.5 -> -2
  -7 * 0.5 -> -4
ln(1.04) = 0.039220713153281295  ln(1.20) = 0.18232155679395462
100*exp(-ln1.04*45/365) = 99.51762414843401
0.0125*sqrt(252) = 0.19843134832984433  overHorizon(30/365) = 0.05688849945167313
worked example: 63/365 = 0.1726027397260274  0.24*sqrt(63/365) = 0.09970916611936527  DF = 0.9932532595967459
```
