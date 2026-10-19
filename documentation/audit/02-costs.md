# Audit phase 2: `source/costs`

*Date: 2026-10-19. Scope: fee schedule, broker fees, trades and the FX model in `source/costs`,
their tests, the DEGIRO configuration and the validation record. Report only.*

## Summary

The arithmetic in `source/costs` is exact and was reproduced to the cent: the five-trade worked
example, the connectivity charge at, below and above its cap, the direction-free handling cost,
and the full currency round trip (10,000.00 EUR out and back loses 49.94 against a predicted
49.9375). Every requirement the design document places on a caller is enforced by an assertion and
pinned by a death test, and the file boundary refuses a malformed schedule by key name. The
configured DEGIRO figures match the validation record.

Three Medium findings. The annual connectivity charge is computed by this layer but no projection
ever strikes it, which is only correct for the exempt home market and is not recorded as a
limitation (COSTS-1). The spot conversion holds the exchange rate as a binary double and rounds the
product three times before the half-to-even step, so it does not meet the rule the theory document
states for it; at a rate of 1.10 one conversion in forty lands on the wrong side of a tie and the
net effect is 2.4 cents per hundred conversions away from zero (COSTS-2). And the charge for an
assignment is not a parameter of the schedule at all: the strategy charges a share transaction,
which is an assumption no validation row supports (COSTS-3). None of these changes a projected
number today for the default EUR, Euronext Amsterdam configuration.

| Severity | Count |
|---|---|
| Critical | 0 |
| High | 0 |
| Medium | 3 |
| Low | 5 |
| Info | 2 |

## What was executed

**Build route.** Same harness as phase 1 (the repository presets need Qt 6.5; the container has
6.4.2). GCC 13.3.0, Ninja, Debug configuration, executed 2026-10-19.

```text
$ cmake --build build --target fin.costs_test
$ (cd build && ctest -R fin.costs_test --output-on-failure)
1/1 Test #2: fin.costs_test ...................   Passed    0.01 sec
100% tests passed, 0 tests failed out of 1
$ build/source/costs/test/fin.costs_test
[==========] Running 32 tests from 4 test suites.
[==========] 32 tests from 4 test suites ran. (8 ms total)
[  PASSED  ] 32 tests.
```

Files read in full: `FeeSchedule.hpp/.cpp`, `BrokerFees.hpp/.cpp`, `FxModel.hpp/.cpp`,
`Trade.hpp/.cpp`, the four test files, `design/costs.md`, decisions D16–D20,
`theory/broker-costs.md`, `theory/currency-conversion.md`, `config/broker/degiro.json`,
`validation/sources.md`, `validation/README.md`, and, to establish how the layer is used,
`io/BrokerConfig.cpp` and the fee-related lines of `present/RunController.cpp` and
`strategy/Projection.cpp`.

## What was verified independently

The script in the appendix produced every figure below.

| Check | Method | Result |
|---|---|---|
| Five-trade worked example | hand sum | 0.75 + 2.25 + 3.00 + 1.50 + 3.00 = 10.50, as the test asserts |
| Connectivity at 400, 900, 1 000, 1 000 000 | `(V × 25) / 10 000` half-to-even, then the cap | 1.00, 2.25, 2.50 (exactly at the cap), 2.50 |
| Connectivity ties | 200.00 → 0.5 c, 600.00 → 1.5 c | 0 c and 2 c (nearest even) |
| Handling cost on 2.00, 6.00, 10.00, 14.00, ±400.00 | `(\|A\| × 25) / 10 000` | 0, 2, 2, 4, 100, 100 c; direction-free |
| FX worked example | replicate `AtSpot`, `HandlingCost`, `Convert` at 1.10 and 25 bp | 11 000.00 / 27.50 / 10 972.50 out; 9 975.00 / 24.94 / 9 950.06 back; loss 49.94 |
| Round-trip identity | every start amount up to 20 000.00 | residue against `2h − h²` at most 0.0185 EUR, i.e. within two minor units of rounding |
| Configured figures vs validation record | `degiro.json` vs `sources.md` | 75 c, 200 c, 100 c, 250 c cap, 25 bp, 25 bp, EUR, reference date 2025-10-01: identical |
| Boundary checks | `io/BrokerConfig.cpp:23-41` | negative amount and rate outside [0, 10 000] refused by key name |
| Caller requirements (design/costs.md) | one currency, non-negative fees, rate ≤ whole, trade ≥ 1, portfolio ≥ 0 | each has an assertion and an `EXPECT_DEATH` |
| Currency of schedule vs scenario | `present/RunController.cpp:138` | refused at the boundary with a message |
| Overflow headroom | `portfolioValue × 25` | checked multiply; first refusal above 9.2 × 10¹⁴ c |

## Findings

| ID | Severity | Where | Title |
|---|---|---|---|
| COSTS-1 | Medium | `BrokerFees.cpp:36-44`; no caller outside `costs` | The connectivity charge is never struck by any projection, and the gap is undocumented |
| COSTS-2 | Medium | `FxModel.hpp:24`, `FxModel.cpp:28-34` | Spot conversion uses a binary rate and three roundings; it does not honour the documented half-to-even rule |
| COSTS-3 | Medium | `FeeSchedule.hpp`, `strategy/Projection.cpp:216`, `validation/sources.md` | What an assignment costs is an unsourced assumption, not a schedule parameter |
| COSTS-4 | Low | `FeeSchedule.hpp:15`, `io/BrokerConfig.cpp:59`, `config/broker/degiro.json:11` | The currency-handling rate is read, validated and never used; `FxModel` has no production caller |
| COSTS-5 | Low | `FeeSchedule.cpp:15-18`, `io/BrokerConfig.cpp:37`, `FxModel.cpp:57` | Boundary and point of use disagree on the admissible handling rate (≤ 10 000 vs < 10 000) |
| COSTS-6 | Low | `FxModel.cpp:28-34` | `AtSpot` treats any non-base amount as foreign without checking |
| COSTS-7 | Low | `test/TestBrokerFees.cpp`, `test/TestFxModel.cpp` | The documented half-minor-unit tie rule for proportional charges has no test in this module |
| COSTS-8 | Low | `test/TestFxModel.cpp:86-95` | Round-trip test tolerance (0.01) is tighter than the rounding residue the theory allows (0.02) |
| COSTS-9 | Info | `validation/sources.md`, `config/broker/degiro.json:6` | The option contract charge remains unconfirmed; the tariff site is unreachable from this environment |
| COSTS-10 | Info | `FxModel.cpp:9` | The base currency is a compile-time constant |

### COSTS-1 — The connectivity charge is never struck (Medium)

**Evidence.** `BrokerFees::ConnectivityFor` (`BrokerFees.cpp:36-44`) is correct and tested. A search
of every non-test file under `source/` finds no caller outside `costs`. `theory/broker-costs.md`
states the charge "is struck once per calendar year for each exchange that was traded on or that
holds a position", `design/costs.md` says the number of exchanges "is supplied by the caller", and
PLAN.md task T4.2 records it as done. `theory/methodology.md`'s table of limits lists "One
currency" but nothing about connectivity.

**Impact.** Every projection behaves as if zero chargeable exchanges were used. For the default
configuration that is the right answer, because the tariff exempts Euronext Amsterdam. For any
other venue the yearly cost is understated by the capped amount, 2.50 EUR per exchange per year for
European venues and, per `validation/sources.md`, 5.00 EUR per exchange per *month* for US options,
which the schedule cannot even express. The money is small; the problem is that a documented,
tested cost silently does not exist in the chain, so a reader of the documentation believes it is
modelled.

**Proposed fix.** Decide where it belongs. Either (a) the scenario states the number of chargeable
exchanges and the strategy strikes `ConnectivityFor` at the same yearly boundary as the wealth tax
(decision D44), or (b) the limitation is recorded in `theory/methodology.md` and `design/costs.md`
("connectivity is computed but not applied; the home market is exempt"). **Suggested test.** For
(a), a projection test that one non-home exchange posts exactly the cap once per year and never
twice; for (b), none.

### COSTS-2 — Spot conversion does not honour the documented rounding rule (Medium)

**Evidence.** `theory/currency-conversion.md` defines the spot amount as `[A·r]`, "rounded to the
nearest minor unit, halves going to the even one", and decision D18 argues that proportional
parameters are held as whole basis points because "multiplying an exact count of minor units by an
inexact fraction gives a charge that depends on the platform's rounding". The exchange rate is the
one proportional parameter in this layer held as a `double` (`FxModel.hpp:24`), and `AtSpot`
computes `Money::FromAmount(amount.ToDouble() * foreignPerBase)` (`FxModel.cpp:31`, and `/` on
line 33): three binary roundings (cents ÷ 100, × r, × 100) before the half-to-even step. Measured
over every amount in ±2,000.00 against the decimal rule:

| Rate | Exact decimal ties | Conversions that differ | Net effect |
|---|---|---|---|
| 1.10 | 40 000 (10.0%) | 9 724 (2.4%) | 2.4 c away from zero per 100 conversions |
| 1.25 | 100 000 (25.0%) | 14 662 (3.7%) | none (symmetric) |
| 1.0837 | 40 | 14 | nil |
| 0.92 | 0 | 0 | nil |

Concrete cases at 1.10: 0.55 EUR → 0.605 USD is stored as 61 c (rule: 60); 1.35 → 1.485 as 149 c
(rule: 148); 1.55 → 1.705 as 171 c (rule: 170). The same amounts held as an integer rate in
millionths (1 100 000) and divided with `DivideRoundHalfToEven` give 0 differences.

**Impact.** None today, because no production code converts (COSTS-4). Once it does, the effect at
a two-decimal rate is a drift of the order of 2 c per 100 conversions, i.e. well under a euro per
simulated path over thirty years: immaterial as money, but exactly the "fake drag" that
`theory/money-and-rounding.md` is written to exclude, dependent on the platform's floating-point
contraction, and in breach of a stated rule. The rule as stated is unachievable with a binary rate
at all: a single rounding of `cents × r` still differs in 11 056 of the same 400 001 cases.

**Proposed fix.** Hold the rate exactly, as an integer count of a fixed sub-unit (for example
micro-units of foreign per base, `std::int64_t foreignPerBaseMillionths`), construct it from a
decimal at the boundary under the same refusal rule as D16/D20, and compute the spot amount with
integer arithmetic: `DivideRoundHalfToEven(cents × rate, 10⁶)` out, and
`DivideRoundHalfToEven(cents × 10⁶, rate)` back. Alternatively, restate the rule in the theory
document as "nearest representable, ties unspecified" and drop the half-to-even claim.
**Suggested test.** A property test over a range of amounts comparing `AtSpot` with an exact
integer reference at two or three rates, including a two-decimal one.

### COSTS-3 — What an assignment costs is an unsourced assumption (Medium)

**Evidence.** The schedule (`FeeSchedule.hpp:8-16`) has per-contract, per-share-trade and
proportional entries, and nothing for the exercise, assignment or expiry of an option. The
strategy charges settlement as a share transaction, `brokerFees.For(costs::Trade::Shares(...))`
(`strategy/Projection.cpp:216`), i.e. 3.00 EUR flat. `validation/sources.md` has no row for what
the broker charges on assignment; its handling-charge row records only that the charge is not
levied on option *trades*.

**Impact.** Every assigned cycle carries a cost the tariff may state differently (brokers variously
charge per contract on exercise, the share transaction, both, or nothing). `design/costs.md` calls
costs "the one input to a compounding projection that is known exactly"; this one is not known at
all. Being wrong by a few euro per assignment is of the same order as the per-contract charge the
validation record calls the single most influential cost.

**Proposed fix.** Add an explicit settlement entry to the schedule (`settlementFeePerContract` or
`settlementChargedAsShareTrade`), read it from the configuration by key, and add a sourced row to
`validation/sources.md`. Until then, note in `theory/broker-costs.md` that settlement is charged as
a share trade by assumption. **Suggested test.** Once the entry exists, a `BrokerFees::For`
case for an assignment trade kind.

### COSTS-4 — Currency-handling rate read but unused; `FxModel` has no caller (Low)

**Evidence.** `currency_handling_basis_points` is parsed (`io/BrokerConfig.cpp:59`), validated,
carried in `FeeSchedule` and never read by `BrokerFees`. `FxModel` is constructed only in its own
test. `theory/methodology.md:220` records "One currency — conversion costs are modelled but no run
mixes currencies", and `present/RunController.cpp:138-139` refuses a broker or tax configuration in
another currency than the scenario, so the limitation is enforced, not merely stated.

**Impact.** A reviewer comparing `degiro.json` with the tariff sees a rate that looks modelled and
is not. A USD scenario is accepted by the loader (`io/ScenarioLoader.cpp:33`) but would need a USD
tax configuration, which Box 3 does not have, so in practice only EUR runs happen and the
foreign-listed underlyings the currency-conversion note is motivated by cannot be projected.

**Proposed fix.** Either mark the key as reserved in the configuration and the design note, or
remove it until the FX leg exists. State in the README's status that projections are
single-currency. **Suggested test.** None.

### COSTS-5 — Boundary and point of use disagree on the handling rate (Low)

**Evidence.** `IsFraction` accepts `0 ≤ bps ≤ 10 000` (`FeeSchedule.cpp:15-18`) and
`BrokerConfig::BasisPointsAt` the same (`io/BrokerConfig.cpp:37`); `FxModel` requires
`bps < 10 000` (`FxModel.cpp:57`). Decision D17 says the same conditions are expressed twice, once
as a report and once as an assertion.

**Impact.** A configured 10 000 (a handling cost of the whole amount) passes the file check and,
once `FxModel` is wired up (COSTS-4), aborts inside the model instead of being reported by key.
Unreachable today.

**Proposed fix.** Make the two bounds identical; the strict one is right (a cost equal to the whole
amount is not a conversion). **Suggested test.** A `BrokerConfig` test that 10 000 is refused for
this key and accepted for `connectivity_basis_points`.

### COSTS-6 — `AtSpot` assumes any non-base amount is foreign (Low)

**Evidence.** `FxModel.cpp:30-33` converts to foreign when the amount is in the base currency and
to base otherwise, without asserting the amount's currency equals `foreign`.

**Impact.** Exact with the current two-currency set. A third currency would be converted at the
wrong rate silently, which is the class of error decision D2 exists to stop.

**Proposed fix.** `really_assert(amount.GetCurrency() == foreign)` on the second branch.
**Suggested test.** Not expressible until a third currency exists; a comment will do.

### COSTS-7 — The tie rule for proportional charges is not tested here (Low)

**Evidence.** Both `design/costs.md` and `theory/broker-costs.md` list "a proportional charge that
lands exactly on a half minor unit rounds to the nearest even minor unit" among the cases they
cover, and the theory note says every row of its tables is checked. No test in `costs` exercises a
tie: the connectivity cases are 400, 900, 1 000 and 1 000 000; the handling cases 400 and −400.

**Impact.** Coverage only. The behaviour is inherited from `Money::operator/`, which is tested in
`core`, but a future change to use `Money × double` here would not be caught.

**Proposed fix.** None in code. **Suggested tests.** `ConnectivityFor(200.00, 1) == 0.00`,
`ConnectivityFor(600.00, 1) == 0.02`, `HandlingCost(2.00) == 0.00`, `HandlingCost(6.00) == 0.02`.

### COSTS-8 — Round-trip test tolerance is tighter than the documented residue (Low)

**Evidence.** `a_round_trip_loses_twice_the_handling_cost_less_its_square`
(`test/TestFxModel.cpp:86-95`) asserts the loss within 0.01 EUR of `2h − h²`. The theory note
says the residue "is at most one minor unit per leg". Over every start amount up to 20 000.00 EUR
the largest residue is 0.0185 EUR (at 14 594.55 EUR); the test passes at 10 000.00 only because its
residue happens to be 0.0025.

**Impact.** A fragile test: changing the example amount could fail it for the wrong reason.

**Proposed fix.** Tolerance 0.02, or assert the residue is at most two minor units.

### COSTS-9 — Option contract charge still unconfirmed (Info)

`validation/sources.md` marks the 0.75 EUR per contract figure as not yet confirmed against the
full tariff document and calls it the single most influential cost. This audit tried to read
`degiro.nl` and could not: the environment's network policy refuses the host. The figure therefore
stays unconfirmed, and so does the reference date `2025-10-01` in `degiro.json`, now more than a
year old relative to this audit's date.

### COSTS-10 — The base currency is a compile-time constant (Info)

`FxModel.cpp:9` fixes EUR as the base. Consistent with "the project reports in one base currency";
noted because `FxModel::From` takes `foreign` as a parameter, which suggests more generality than
exists.

## Observations carried forward to later phases

| For phase | Where | Observation |
|---|---|---|
| 4 io | `io/BrokerConfig.cpp:37` | Inclusive upper bound on both basis-point keys; align with COSTS-5. |
| 4 io | `io/ScenarioLoader.cpp:33` | A USD scenario is accepted; with EUR-only tax and broker files it is refused later by the run controller. Consider refusing non-EUR at the loader with a clearer message. |
| 7 strategy | `strategy/Projection.cpp:152,192,216` | Fees are charged on open, close and settlement; settlement as a share trade is COSTS-3. Check that a roll charges a close and an open, and that an expiry worthless charges nothing. |
| 7 strategy | — | Where the yearly connectivity charge would be struck (COSTS-1). |
| 8 tax | `present/RunController.cpp:139` | Tax configuration currency must equal the scenario currency; Box 3 is EUR-only by nature. |

## Open questions for the owner

1. Is the connectivity charge intentionally never struck because the home market is exempt
   (COSTS-1)? If so it should be written down; if not, which layer should strike it, and where does
   the number of exchanges come from?
2. What does DEGIRO charge on assignment or exercise of a Euronext option (COSTS-3)? The model
   currently charges 3.00 EUR as a share transaction; please confirm from the tariff document, and
   confirm the 0.75 EUR per contract figure while there (COSTS-9).
3. Should `currency_handling_basis_points` stay in the configuration while nothing uses it
   (COSTS-4)?

## Not checked

- The tariff itself: `degiro.nl` is unreachable from this environment (DNS refused), so the option
  fee, the assignment charge and whether the 2025-10-01 reference date is still current could not
  be confirmed.
- Whether the share commission for Euronext Amsterdam depends on account type or on the "core
  selection"; the configuration states it for a Basic account only.
- How fees are posted into the ledger and on which dates (phase 7).

## Appendix: reference-check script

Run with `python3 -I check_costs.py`. It takes about a minute. Output as produced on 2026-10-19
follows the script.

```python
import math
from decimal import Decimal as D, ROUND_HALF_EVEN

def rhe(v):  # Money.hpp RoundHalfToEven(double)
    lower = math.floor(v); frac = v - lower; r = int(lower)
    if frac > 0.5 or (frac == 0.5 and r % 2 != 0): r += 1
    return r
def cdiv(a, b):  # C++ truncating division
    q = abs(a) // abs(b); return q if (a >= 0) == (b > 0) else -q
def div_he(n, d):  # Money.hpp DivideRoundHalfToEven
    q = cdiv(n, d); r = n - q * d
    if r == 0: return q
    twice, ad = abs(r) * 2, abs(d); step = -1 if (n < 0) != (d < 0) else 1
    if twice > ad: return q + step
    if twice < ad: return q
    return q if q % 2 == 0 else q + step
def from_amount(a): return rhe(a * 100.0)   # Money::FromAmount
def to_double(c): return c / 100.0           # Money::ToDouble

print("connectivity, 25 bps, cap 250 c (BrokerFees::ConnectivityFor)")
for v in [40000, 90000, 100000, 100000000, 20000, 60000]:
    prop = div_he(v * 25, 10000)
    print(f"  V={v / 100:.2f}: proportional {prop} c -> charge {min(prop, 250)} c")
print("handling cost, 25 bps (FxModel::HandlingCost)")
for c in [200, 600, 1000, 1400, 40000, -40000]:
    print(f"  {c} c -> {div_he(abs(c) * 25, 10000)} c")

r = 1.10
def at_spot(c, out): return from_amount(to_double(c) * r) if out else from_amount(to_double(c) / r)
def handling(c): return div_he(abs(c) * 25, 10000)
def convert(c, out):
    s = at_spot(c, out); return s - handling(s), s, handling(s)
out, s1, h1 = convert(1000000, True)
back, s2, h2 = convert(out, False)
print(f"FX worked example: out spot {s1} c, handling {h1} c, net {out} c; back spot {s2} c, handling {h2} c, net {back} c; loss {1000000 - back} c; predicted {10000 * (2 * 0.0025 - 0.0025 ** 2):.4f} EUR")

print("AtSpot EUR->USD against decimal half-to-even, every amount in [-2000.00, 2000.00]")
for rate_s in ["1.10", "0.92", "1.0837", "1.25"]:
    rate, rd = float(rate_s), D(rate_s)
    ties = mism = bias = 0
    for c in range(-200000, 200001):
        exact = D(c) * rd
        want = int(exact.quantize(D(1), rounding=ROUND_HALF_EVEN))
        got = from_amount(to_double(c) * rate)
        ties += exact - exact.to_integral_value(rounding="ROUND_FLOOR") == D("0.5")
        if got != want:
            mism += 1; bias += (got - want) if c >= 0 else -(got - want)
    print(f"  r={rate_s}: decimal ties {ties}, mismatches {mism}, net away-from-zero {bias} c = {bias / 4000.01:.3f} c per 100 conversions")
mism = sum(div_he(c * 1100000, 1000000) != int((D(c) * D("1.10")).quantize(D(1), rounding=ROUND_HALF_EVEN)) for c in range(-200000, 200001))
print(f"  integer rate in millionths at 1.10: mismatches {mism}")
mism1 = sum(rhe(c * r) != int((D(c) * D("1.10")).quantize(D(1), rounding=ROUND_HALF_EVEN)) for c in range(-200000, 200001))
print(f"  single rounding of cents x r at 1.10: mismatches {mism1}")
shown = 0
for c in range(1, 2_000_001):
    want = int((D(c) * D("1.10")).quantize(D(1), rounding=ROUND_HALF_EVEN))
    got = from_amount(to_double(c) * r)
    if got != want:
        print(f"  example: {c / 100:.2f} EUR x 1.10 = {D(c) * D('1.10') / 100} USD -> half-to-even {want} c, code {got} c")
        shown += 1
        if shown == 4: break

h = 0.0025
worst = (0.0, 0)
for start in range(1, 2_000_001):
    loss = (start - convert(convert(start, True)[0], False)[0]) / 100.0
    dev = abs(loss - start / 100.0 * (2 * h - h * h))
    if dev > worst[0]: worst = (dev, start)
print(f"round trip residue vs 2h-h^2, every start up to 20,000.00 EUR: max {worst[0]:.4f} EUR at {worst[1] / 100:.2f} EUR (test tolerance 0.01)")
```

```text
connectivity, 25 bps, cap 250 c (BrokerFees::ConnectivityFor)
  V=400.00: proportional 100 c -> charge 100 c
  V=900.00: proportional 225 c -> charge 225 c
  V=1000.00: proportional 250 c -> charge 250 c
  V=1000000.00: proportional 250000 c -> charge 250 c
  V=200.00: proportional 50 c -> charge 50 c
  V=600.00: proportional 150 c -> charge 150 c
handling cost, 25 bps (FxModel::HandlingCost)
  200 c -> 0 c
  600 c -> 2 c
  1000 c -> 2 c
  1400 c -> 4 c
  40000 c -> 100 c
  -40000 c -> 100 c
FX worked example: out spot 1100000 c, handling 2750 c, net 1097250 c; back spot 997500 c, handling 2494 c, net 995006 c; loss 4994 c; predicted 49.9375 EUR
AtSpot EUR->USD against decimal half-to-even, every amount in [-2000.00, 2000.00]
  r=1.10: decimal ties 40000, mismatches 9724, net away-from-zero 9460 c = 2.365 c per 100 conversions
  r=0.92: decimal ties 0, mismatches 0, net away-from-zero 0 c = 0.000 c per 100 conversions
  r=1.0837: decimal ties 40, mismatches 14, net away-from-zero 14 c = 0.003 c per 100 conversions
  r=1.25: decimal ties 100000, mismatches 14662, net away-from-zero 2 c = 0.000 c per 100 conversions
  integer rate in millionths at 1.10: mismatches 0
  single rounding of cents x r at 1.10: mismatches 11056
  example: 0.55 EUR x 1.10 = 0.605 USD -> half-to-even 60 c, code 61 c
  example: 1.35 EUR x 1.10 = 1.485 USD -> half-to-even 148 c, code 149 c
  example: 1.55 EUR x 1.10 = 1.705 USD -> half-to-even 170 c, code 171 c
  example: 1.75 EUR x 1.10 = 1.925 USD -> half-to-even 192 c, code 193 c
round trip residue vs 2h-h^2, every start up to 20,000.00 EUR: max 0.0185 EUR at 14594.55 EUR (test tolerance 0.01)
```
