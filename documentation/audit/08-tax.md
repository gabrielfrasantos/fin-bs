# Audit phase 8: `source/tax`

*Date: 2026-10-19. Scope: the Box 3 parameter set, the wealth-tax model and the dividend
withholding model in `source/tax`, their two test files, the tax contract, the Box 3 theory note,
decisions D26–D29, the 2025 configuration file, the validation record and the configuration
reader. Report only.*

## Summary

The arithmetic is right and matches what the documents say it does. The theory note's worked
example reproduces step by step (4,978.00 deemed, 42,316.00 base, 2,106.49 taxable, 758.34
charged), as do the all-investment and all-deposit variants, the two charges met in phase 7, and
the boundary behaviour at and one cent above the allowance. Over two million random portfolios
through the built library, and every cent of wealth over a 3,000.00 band above the allowance, the
decimal proportion of D27 gives the same taxable return as exact integer arithmetic; it differs
only on exact half-cent ties, which exist but are rare, and then by one cent. The charge is
monotone cent by cent. The shipped 2025 figures equal the validation record. All 26 tests pass.

The findings are about what surrounds the arithmetic. The split of wealth into deposits and
investments, which the contract says "is an input" the investor may need to override, is fixed in
the strategy layer with no setting; on a cash-heavy book of 200,000.00 the two classifications
differ by about 2,200.00 of tax a year (TAX-1). The dividend model caps the credit for withholding
at the annual charge and never repays the excess, which is the treatment of foreign withholding;
domestic withholding is a refundable prepayment under the rules the model names (TAX-2). Neither
reaches a production number today: the first because the default classification is the likely
right one for this broker, the second because nothing calls the dividend model (STRAT-2). The
Belastingdienst pages cited by the validation record could not be fetched from this environment,
so the 2025 figures are checked against the record, not against the source.

| Severity | Count |
|---|---|
| Critical | 0 |
| High | 0 |
| Medium | 2 |
| Low | 4 |
| Info | 4 |

## What was executed

**Build route.** Same harness as phase 1 (scratchpad CMake project, GCC 13.3.0, Ninja, Debug),
executed 2026-10-19.

```text
$ cmake --build build --target fin.tax_test
$ (cd build && ctest -R fin.tax_test --output-on-failure)
1/1 Test #9: fin.tax_test .....................   Passed    0.01 sec
100% tests passed, 0 tests failed out of 1
$ build/source/tax/test/fin.tax_test
[==========] Running 26 tests from 2 test suites.
[==========] 26 tests from 2 test suites ran. (3 ms total)
[  PASSED  ] 26 tests.
```

One experiment ran against the built library (appendix B) and two scripts against a replica
(appendix A):

| # | Experiment | Expected from the reading | Observed |
|---|---|---|---|
| E1 | 2,000,000 random portfolios, each class up to 50,000,000.00, library taxable return against an exact 128-bit integer half-to-even proportion | equal except on half-cent ties | 0 differences, 0 exact ties met; the worked example through the library gives 497,800 / 4,231,600 / 210,649 / 75,834 cents |
| A1 | replica of the model: worked examples, sequential-allowance variant, 400,000 random portfolios, 300,000 consecutive cents of wealth, 200,000-cent monotonicity sweep, overflow bounds, classification and dividend cases | as the documents state | all reproduced; 0 differences; 0 decreases |
| A2 | search for exact half-cent ties over 4,000,000 cents of wealth above the allowance at three splits | ties exist; some fall the wrong way through a double | 5,001 ties; 231 differ from half-to-even by one cent |

Three fetches of the validation record's Belastingdienst pages (the 2025 calculation page, the
allowance page and the rate page) failed with the host unresolved; the environment's network
policy does not reach `belastingdienst.nl`, as it did not reach `degiro.nl` in phase 2.

Files read in full: the three source and three header files, the two test files, the two
`CMakeLists.txt`, `design/tax.md`, `theory/box3-taxation.md`, decisions D26–D29,
`config/tax/nl_2025.json`, `validation/sources.md`, `validation/README.md`, `io/TaxConfig.cpp`,
and the strategy-side callers `strategy/Book.cpp:11-17` and `strategy/Projection.cpp:34-40`.

## What was verified independently

| Check | Method | Result |
|---|---|---|
| Worked example (`box3-taxation.md`, `TestBox3TaxModel`) | replica, step by step | 100,000.00 / 274.00 + 4,704.00 / 42,316.00 / 0.42316 / 2,106.49 / 758.34 |
| All investments, all deposits, half and half | replica | 895.74, 208.70, 552.23 |
| At the allowance and one cent above | replica | 0.00; base 0.01, taxable return 0.00, charge 0.00; 49.02 at 60,000.00 |
| Phase 7 charges | replica | 726.38 and 920.35 |
| "The difference is not small" (allowance off the assets instead of the return) | replica, deposits exempted first | 895.74 against 758.34 on the example |
| D27: decimal proportion vs exact integers | E1 (library), A1 (replica) | 0 differences in 2,700,000 cases; differs only on exact ties (A2) |
| Monotone in wealth | every cent of 5,700,000–5,900,000 at a quarter deposits | 0 decreases |
| Overflow bounds D27 cites | arithmetic | integer product overflows at 125,243,790.00 (all investments) or 259,468,463.00 (all deposits); `AtRate` itself at 1.6 × 10¹⁴ |
| Shipped figures vs validation record | `nl_2025.json` vs `sources.md` | 5,768,400 c, 137, 588, 3,600, 1,500 bp: identical |
| Shipped figures vs source | three fetches of `belastingdienst.nl` | host unreachable; not re-checked |
| Caller requirements (`design/tax.md`) | each has an assertion | yes; only one has a death test (TAX-5) |
| Boundary refusals | `io/TaxConfig.cpp:23-51` | negative amount, rate outside 0–10,000, year ≤ 0 refused by key (phase 4) |
| Currency agreement | `present/RunController.cpp:137-139` | refused with a message |

## Findings

| ID | Severity | Where | Title |
|---|---|---|---|
| TAX-1 | Medium | `strategy/Book.cpp:11-17`, `strategy/Projection.cpp:34-40`; `design/tax.md:130-132, 141-143` | The deposit-or-investment classification of cash is fixed in code; the override the contract promises has no setting |
| TAX-2 | Medium | `DividendTax.cpp:36-47`; `design/tax.md:75-76, 112-116`; `theory/box3-taxation.md:86-87` | Domestic withholding is capped at the annual charge and never repaid; the rules make it a refundable prepayment |
| TAX-3 | Low | `Box3TaxModel.cpp:47`; D27 | The decimal proportion is not half-to-even on exact ties; an exact integer route is available |
| TAX-4 | Low | `Box3Parameters.hpp:8-16`; `io/TaxConfig.cpp:61-70`; `config/tax/nl_2025.json` | A parameter set carries no provenance: the `checked` date, filing status and regime are not read |
| TAX-5 | Low | `test/TestBox3TaxModel.cpp:132-135`, `test/TestDividendTax.cpp` | Most of the layer's refusals are not specified by a test |
| TAX-6 | Low | `strategy/Projection.cpp:34-40`; D29 | An open short option is netted against the investment class at intrinsic value; the rules treat a written option as a debt |
| TAX-7 | Info | `Box3TaxModel.cpp` | Official rounding conventions (whole euros; possibly the effective percentage) are not applied |
| TAX-8 | Info | `validation/sources.md`, `config/tax/nl_2025.json` | The figures could not be re-checked; which of them are at risk |
| TAX-9 | Info | `sim/BookScript.cpp:119` | Shares are valued at the simulated price of 1 January, not the last close of the previous year |
| TAX-10 | Info | `io/TaxConfig.cpp:12-21` | A tax file in US dollars is accepted although the regime is euro-only |

### TAX-1 — The classification of cash is fixed in code, with no override (Medium)

**Evidence.** The tax layer takes a split of wealth into deposits and investments and classifies
nothing itself; `design/tax.md:141-143` records that choice ("the classification depends on the
broker's legal structure ... it is the one part of the calculation the investor may need to
override. The split is an input"), and `:130-132` tells "an investor whose broker does not hold
cash at a bank" to "state it as an investment instead". The only production callers build the
split as deposits = min(cash, equity) and investments = the rest (`strategy/Book.cpp:11-17`,
`strategy/Projection.cpp:34-40`). No scenario or tax-file key, and no code path, lets cash be
stated as an investment.

**Impact.** The two imputed returns differ by a factor of four (1.37% against 5.88%). A book of
200,000.00 holding 190,000.00 in cash on the reference date is charged 817.43 with the cash as a
deposit and 3,012.54 with it as an investment: 2,195.11 a year, about 1.1% of the capital, on a
strategy whose premium income is a few percent. The default is the right one if the broker's
uninvested cash sits in a bank account in the client's name; for this broker that has been the
arrangement since its cash was moved from money-market funds to a bank account, which makes the
default probably right today and silently wrong for any broker, or any future arrangement, where
cash is held otherwise. The contract says the investor can choose; the code says they cannot.

**Proposed fix.** A key in the tax file (`cash_is_deposit: true`) or in the broker file, read at
the boundary and carried into `Box3Assets` construction, so the classification is configuration
next to the rates it interacts with. Record in `validation/sources.md` where this broker holds
cash and when that was checked. **Suggested test.** A book assessed under both settings is charged
the two figures above.

### TAX-2 — Domestic withholding is capped at the charge and never repaid (Medium)

**Evidence.** `CreditAgainst` sums the creditable withholding and returns the minimum of that sum
and the charge (`DividendTax.cpp:36-47`); the test
`a_credit_larger_than_the_wealth_tax_is_capped_at_it_and_is_not_repaid` pins it, and
`design/tax.md:75-76` states it as a guarantee: "a portfolio that owes no tax recovers nothing".
The theory note calls domestic withholding "a prepayment". Under the Dutch rules it is exactly
that: dividend tax withheld from a resident is a *voorheffing* on income tax, credited against the
whole income-tax liability and refunded where it exceeds it. The cap, with a carry-forward of the
excess to later years rather than its loss, is the treatment of *foreign* withholding, where the
credit is limited to the Dutch tax attributable to the income. The model applies the foreign rule
to both and drops the carry-forward.

**Impact.** None in a projection today, because nothing posts a dividend (STRAT-2). Once a
dividend leg exists, a domestic dividend of 10,000.00 on a portfolio charged 758.34 would have
741.66 of its withholding written off (appendix A), money a resident gets back; for a portfolio
below the allowance all of it. The direction is pessimistic for Dutch names and roughly right for
foreign ones held long enough for the carry-forward not to matter. The validation record also
lists the 15% rate as unconfirmed.

**Proposed fix.** Separate the two cases: domestic withholding reduces the net tax without a cap
(a refund is a negative charge in the ledger's terms, which `CashflowKind::Tax` cannot carry, so
it needs its own kind or a net-of-credit posting); foreign withholding credited up to the lower of
the treaty rate and the Box 3 tax attributable to those shares, with the excess carried forward.
Amend `design/tax.md` and the theory note. Confirm both rules against the Belastingdienst pages
and add the rows to `validation/sources.md`. **Suggested tests.** A domestic dividend on a
portfolio charged nothing is refunded in full; a foreign dividend withheld at 26.38% credits 15%
and carries nothing back.

### TAX-3 — The decimal proportion is not half-to-even on exact ties (Low)

**Evidence.** `TaxableReturn` scales the deemed return by `taxable.ToDouble() /
TotalValue(assets).ToDouble()` through `Money × double` (`Box3TaxModel.cpp:47`), which rounds the
binary product half-to-even. D27 says this is "one multiplication followed by the project's usual
rounding rule". On 2,700,000 random and consecutive inputs the result equals the exact integer
proportion (E1, A1). Searching for exact half-cent ties (A2) finds 5,001 among 12,000,000
candidates, and on 231 of them the double product lands a few ulps off the half and rounds the
other way: deposits and investments of 28,980.00 each give a taxable return of exactly 1,000.5
cents, half-to-even 1,000, the library 1,001.

**Impact.** One cent of taxable return, hence at most one cent of charge, on inputs that occur
with frequency of the order of 10⁻⁵. Immaterial as money; the same class as COSTS-2, much rarer.
D27's reason for the decimal is that the integer product overflows "at a portfolio size that is
large and not absurd": with the 2025 rates that size is 125 million euro all in investments.

**Proposed fix.** Compute the proportion in integers with a 128-bit intermediate
(`__int128` on both supported compilers, or two-step long division), which is exact for any
wealth the 64-bit cent count can hold, and amend D27. Otherwise, restate D27 as "nearest, ties
unspecified". **Suggested test.** The 28,980.00 tie above.

### TAX-4 — A parameter set carries no provenance (Low)

**Evidence.** `nl_2025.json` carries `authority`, `jurisdiction`, `regime`, `filing_status`
("without fiscal partner") and `checked` ("2026-08-11"); `TaxConfig` reads the year, currency,
allowance and four rates and nothing else (`io/TaxConfig.cpp:61-70`); `Box3Parameters` has no
field for any of them. `validation/README.md` makes the checked date the thing a user must look at
before trusting a projection, and the record notes that the deposit return was provisional for
part of the year.

**Impact.** A run cannot show which parameter set it used or when its figures were last confirmed,
and a file for a fiscal partner (double allowance) is indistinguishable from one without except
by its number. The staleness the validation folder exists to catch is invisible at the point of
use.

**Proposed fix.** Carry `checked` and `filing_status` into the parameter set and surface them in
the run's report header, next to the tax year. Refuse a file whose `checked` date is more than a
year before the run's start, or warn. **Suggested test.** A file whose `checked` is missing is
refused by key.

### TAX-5 — Most refusals are not specified by a test (Low)

**Evidence.** `design/tax.md:48-56` lists five caller requirements and D4 promises each refusal a
test. Death tests exist for a tax rate of 10,001, negative wealth, wealth in another currency, a
negative dividend and a dividend in another currency. None exists for a negative allowance, a
year of zero, a deposit, investment or withholding rate above the whole, a `Dividend` whose
`withheldBasisPoints` is outside 0–10,000 (asserted inside `AtRate`), or `CreditAgainst` with a
negative charge.

**Proposed fix.** None in code. **Suggested tests.** One `EXPECT_DEATH` per requirement above.

### TAX-6 — An open short is netted against investments at intrinsic value (Low)

**Evidence.** `Projection`'s own assessment values the portfolio with the position open
(`strategy/Projection.cpp:34-40`): equity subtracts the intrinsic liability (D22), deposits are
capped at that equity and investments take the rest, so a put in the money reduces the investment
class. Under the rules a written option is a liability valued at what it would cost to close, in
the debts class, with its own imputed rate (2.70% per the validation record) and a threshold
below which debts are ignored; D29 says debts are not modelled and "wealth is non-negative by
construction". The production path assesses at a boundary with nothing open (D44), so this is
reachable only through a wheel run on its own.

**Impact.** None in production. If the in-cycle path is ever used, the liability is valued without
its extrinsic value (D22), netted at 5.88% instead of imputed at the debt rate, and never subject
to the threshold; the charge is understated by a few tenths of a percent of the liability.

**Proposed fix.** Either remove the in-cycle assessment path now that the book strikes the charge,
or document that an open position is approximated as a reduction of investments.

### TAX-7 — Official rounding conventions are not applied (Info)

The return carries amounts in whole euros, rounded down, and the published worked calculations
present the effective return percentage to two decimals. The model carries cents throughout and
never rounds the proportion. The difference is bounded by a few euro on a charge of hundreds;
whether the assessment itself rounds the percentage could not be confirmed from here. Not a
defect; worth one sentence in the theory note.

### TAX-8 — What could not be re-checked (Info)

The three Belastingdienst pages are unreachable from this environment, so the 2025 figures were
compared with the validation record only. The allowance (57,684), the investment return (5.88%)
and the rate (36%) agree with this auditor's recollection of the published 2025 rules; the deposit
return (1.37%) is the figure the record itself says was provisional and later settled, and the
debt return (2.70%, not in the configuration) is the one most likely to differ from the published
definitive value. The 15% withholding rate is marked unconfirmed in the record. A 2026 file does
not exist; the published 2026 allowance is, from memory, 59,489, to be confirmed. `Year()` is
read by nothing in production, so a 2025 file used for a reference date in 2030 is not remarked
on; the theory note states that a projection uses one year's rules as a forecast.

### TAX-9 — Valuation date of listed shares (Info)

The simulated path supplies a price on 1 January itself (`sim/BookScript.cpp:119`, a date on the
schedule whatever the calendar says). The rules value listed securities at the last closing price
before the reference date. One day of simulated drift on a price series; immaterial.

### TAX-10 — A euro-only regime accepts a dollar file (Info)

`CurrencyNamed` accepts `USD` (`io/TaxConfig.cpp:12-21`) and the run controller refuses the file
only when it disagrees with the scenario. A Box 3 file in dollars is meaningless; refusing it at
the boundary, by name, would be clearer than a later mismatch.

## Observations carried forward to later phases

| For phase | Where | Observation |
|---|---|---|
| 9 sim | `sim/BookScript.cpp:129-146`, STRAT-12 | The straddling cycle's premiums escape the taxed wealth. Quantified: 36% × 1.37% × premium, about 12.00 on 2,500.00 of premium. Immaterial; resolved. |
| 9 sim | TAX-9 | Price on 1 January taken from the simulated schedule. |
| — | IO-1 | "No wealth tax" is expressible by a tax file with all rates zero, which is clearer than omitting the `assessment` block; the owner's choice. |

## Open questions for the owner

1. Where does your broker hold uninvested cash on the reference date: a bank account in your
   name, or a money-market fund (TAX-1)? The answer is worth about 1.1% of the capital a year in
   tax on a cash-heavy book.
2. Do you file with a fiscal partner (TAX-4)? The allowance doubles and nothing in the model
   says which was assumed beyond the number in the file.
3. Should the dividend model be corrected now, while nothing calls it (TAX-2), or removed until
   the dividend leg exists (STRAT-2)?

## Not checked

- The Belastingdienst sources themselves (host unreachable); the exact rounding steps of the
  official assessment (TAX-7); the definitive 2025 deposit return and the 2026 parameters.
- The actual-return option (tegenbewijsregeling) that the validation record describes: a
  projection is an upper bound on the charge in a year the portfolio did worse than the imputed
  return, and nothing here models the lower figure.
- The configuration reader beyond the lines cited (phase 4).

## Appendix A: reference-check scripts

Run with `python3 -I check_tax.py` and `python3 -I check_tax_ties.py` (the second takes about a
minute). Output as produced on 2026-10-19 follows each script.

```python
import random

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
def rhe(v):  # RoundHalfToEven(double), used by Money * double
    import math
    lower = math.floor(v); frac = v - lower; r = int(lower)
    if frac > 0.5 or (frac == 0.5 and r % 2 != 0): r += 1
    return r
def fmt(c): return f"{c / 100:,.2f}"

A, d, i, t = 5768400, 137, 588, 3600   # nl_2025.json

def at_rate(c, bps): return div_he(c * bps, 10000)
def model(D, I):  # Box3TaxModel, step by step
    W = D + I; Y = at_rate(D, d) + at_rate(I, i); G = max(W - A, 0)
    if G == 0: return W, Y, G, 0, 0
    V = rhe(Y * ((G / 100.0) / (W / 100.0)))        # Money * double, one rounding (D27)
    return W, Y, G, V, at_rate(V, t)
def exact_V(D, I):  # the same proportion in integers, half-to-even
    W = D + I; Y = at_rate(D, d) + at_rate(I, i); G = max(W - A, 0)
    return div_he(Y * G, W) if G else 0

# --- 1. the theory note's worked example and the test's figures
W, Y, G, V, T = model(2000000, 8000000)
print(f"worked example: W {fmt(W)}, deemed {fmt(Y)} (= {fmt(at_rate(2000000, d))} + {fmt(at_rate(8000000, i))}), base {fmt(G)}, share {G / W:.5f}, taxable return {fmt(V)}, charge {fmt(T)}")
print(f"  all investments: {fmt(model(0, 10000000)[4])}; all deposits: {fmt(model(10000000, 0)[4])}; half and half: {fmt(model(5000000, 5000000)[4])}")
print(f"  at the allowance: charge {fmt(model(0, 5768400)[4])}; one cent above: base {fmt(model(0, 5768401)[2])}, taxable return {fmt(model(0, 5768401)[3])}, charge {fmt(model(0, 5768401)[4])}; at 60,000: {fmt(model(0, 6000000)[4])}")
print(f"  phase 7 figures: 726.38 -> {fmt(model(20496250, 0)[4])}; 920.35 -> {fmt(model(18396025, 400 * 4500)[4])}")

# --- 2. the sequential formula the theory note warns against
def sequential(D, I):  # take the allowance off the deposits first, then impute to what is left
    freeD = min(D, A); freeI = min(I, A - freeD)
    return at_rate(at_rate(D - freeD, d) + at_rate(I - freeI, i), t)
print(f"allowance taken off the assets instead of the return: {fmt(sequential(2000000, 8000000))} (deposits first) vs {fmt(model(2000000, 8000000)[4])} proportional")

# --- 3. D27: the decimal proportion against the exact integer proportion
random.seed(8)
mism = ties = n = 0; examples = []
for _ in range(400000):
    D = random.randint(0, 50000000); I = random.randint(0, 50000000)
    n += 1
    a, b = model(D, I)[3], exact_V(D, I)
    if a != b:
        mism += 1
        if len(examples) < 5: examples.append((D, I, a, b))
print(f"random wealth up to 500,000 each, {n} cases: decimal proportion differs from the exact half-to-even proportion in {mism} cases; examples {examples}")
mism2 = 0; n2 = 0
for Wc in range(5768401, 5768401 + 300000):
    D = Wc // 5; I = Wc - D
    n2 += 1
    if model(D, I)[3] != exact_V(D, I): mism2 += 1
print(f"every cent of wealth from one cent above the allowance over 300,000 cents (one fifth deposits): {mism2} differences in {n2}")

# --- 4. monotonicity, cent by cent
prev = -1; drops = 0
for Wc in range(5700000, 5900000):
    T = model(Wc // 4, Wc - Wc // 4)[4]
    if T < prev: drops += 1
    prev = T
print(f"charge cent by cent over 5,700,000..5,900,000 cents (quarter deposits): decreases {drops}")

# --- 5. the overflow bound D27 cites, and the model's own bound
import math
print(f"integer proportion Y*G overflows int64 at W = {math.sqrt(2**63 / (i / 10000)) / 100:,.0f} EUR (all investments), {math.sqrt(2**63 / (d / 10000)) / 100:,.0f} EUR (all deposits); AtRate overflows at {2**63 / i / 100:,.0f} EUR")

# --- 6. classification of the collateral: deposit or investment
for cash, shares in [(19000000, 1000000), (5000000, 15000000)]:
    print(f"cash {fmt(cash)}, shares {fmt(shares)}: cash as deposits {fmt(model(cash, shares)[4])}, cash as investments {fmt(model(0, cash + shares)[4])}, difference {fmt(model(0, cash + shares)[4] - model(cash, shares)[4])} per year")

# --- 7. dividend credit as modelled
def withheld(gross, w): return at_rate(gross, w)
def creditable(gross, w): return min(withheld(gross, w), at_rate(gross, 1500))
charge = 75834
for gross, w, label in [(100000, 1500, "domestic 1,000.00"), (1000000, 1500, "domestic 10,000.00"), (100000, 2638, "German 1,000.00 at 26.38%"), (140000, 1500, "domestic 1,400.00 plus 1,000.00")]:
    print(f"  {label}: withheld {fmt(withheld(gross, w))}, creditable {fmt(creditable(gross, w))}, credit against a charge of {fmt(charge)}: {fmt(min(creditable(gross, w), charge))}, not returned by the model: {fmt(creditable(gross, w) - min(creditable(gross, w), charge))}")
```

```text
worked example: W 100,000.00, deemed 4,978.00 (= 274.00 + 4,704.00), base 42,316.00, share 0.42316, taxable return 2,106.49, charge 758.34
  all investments: 895.74; all deposits: 208.70; half and half: 552.23
  at the allowance: charge 0.00; one cent above: base 0.01, taxable return 0.00, charge 0.00; at 60,000: 49.02
  phase 7 figures: 726.38 -> 726.38; 920.35 -> 920.35
allowance taken off the assets instead of the return: 895.74 (deposits first) vs 758.34 proportional
random wealth up to 500,000 each, 400000 cases: decimal proportion differs from the exact half-to-even proportion in 0 cases; examples []
every cent of wealth from one cent above the allowance over 300,000 cents (one fifth deposits): 0 differences in 300000
charge cent by cent over 5,700,000..5,900,000 cents (quarter deposits): decreases 0
integer proportion Y*G overflows int64 at W = 125,243,790 EUR (all investments), 259,468,463 EUR (all deposits); AtRate overflows at 156,860,068,653,993 EUR
cash 190,000.00, shares 10,000.00: cash as deposits 817.43, cash as investments 3,012.54, difference 2,195.11 per year
cash 50,000.00, shares 150,000.00: cash as deposits 2,434.89, cash as investments 3,012.54, difference 577.65 per year
  domestic 1,000.00: withheld 150.00, creditable 150.00, credit against a charge of 758.34: 150.00, not returned by the model: 0.00
  domestic 10,000.00: withheld 1,500.00, creditable 1,500.00, credit against a charge of 758.34: 758.34, not returned by the model: 741.66
  German 1,000.00 at 26.38%: withheld 263.80, creditable 150.00, credit against a charge of 758.34: 150.00, not returned by the model: 0.00
  domestic 1,400.00 plus 1,000.00: withheld 210.00, creditable 210.00, credit against a charge of 758.34: 210.00, not returned by the model: 0.00
```

```python
import math
def cdiv(a, b):
    q = abs(a) // abs(b); return q if (a >= 0) == (b > 0) else -q
def div_he(n, d):  # DivideRoundHalfToEven
    q = cdiv(n, d); r = n - q * d
    if r == 0: return q
    twice, ad = abs(r) * 2, abs(d); step = -1 if (n < 0) != (d < 0) else 1
    if twice > ad: return q + step
    if twice < ad: return q
    return q if q % 2 == 0 else q + step
def rhe(v):  # RoundHalfToEven(double), used by Money * double
    lower = math.floor(v); frac = v - lower; r = int(lower)
    if frac > 0.5 or (frac == 0.5 and r % 2 != 0): r += 1
    return r
A, d, i = 5768400, 137, 588
def at_rate(c, bps): return div_he(c * bps, 10000)
ties, differ = [], []
for W in range(A + 1, A + 4000000):
    for D in (0, W // 2, W):
        I = W - D
        Y = at_rate(D, d) + at_rate(I, i); G = W - A
        if (2 * Y * G) % W == 0 and ((2 * Y * G) // W) % 2 == 1:
            got = rhe(Y * ((G / 100.0) / (W / 100.0))); want = div_he(Y * G, W)
            ties.append((D, I))
            if got != want: differ.append((D, I, Y, G, W, want, got, Y * ((G / 100.0) / (W / 100.0))))
print(f"exact half-cent ties among wealth from the allowance up to +40,000.00 (three splits each): {len(ties)}; the double route differs from half-to-even on {len(differ)}")
for D, I, Y, G, W, want, got, prod in differ[:4]:
    print(f"  deposits {D / 100:,.2f} investments {I / 100:,.2f}: Y*G/W = {Y * G / W} exactly, half-to-even {want}, library {got} (double product {prod!r})")
```

```text
exact half-cent ties among wealth from the allowance up to +40,000.00 (three splits each): 5001; the double route differs from half-to-even on 231
  deposits 28,980.00 investments 28,980.00: Y*G/W = 1000.5 exactly, half-to-even 1000, library 1001 (double product 1000.5000000000001)
  deposits 29,080.00 investments 29,080.00: Y*G/W = 1725.5 exactly, half-to-even 1726, library 1725 (double product 1725.4999999999998)
  deposits 29,108.00 investments 29,108.00: Y*G/W = 1928.5 exactly, half-to-even 1928, library 1929 (double product 1928.5000000000002)
  deposits 29,304.00 investments 29,304.00: Y*G/W = 3349.5 exactly, half-to-even 3350, library 3349 (double product 3349.4999999999995)
```

## Appendix B: experiment against the built library

Compiled with `g++ -std=c++20 -O1` against the harness archives.

```cpp
// Phase 8 experiment: the library's charge against an exact integer half-to-even proportion.
#include "source/tax/Box3TaxModel.hpp"
#include <cstdio>
#include <random>

using namespace fin;

static std::int64_t DivHe(__int128 n, __int128 d)
{
    __int128 q = n / d; __int128 r = n - q * d;
    if (r == 0) return static_cast<std::int64_t>(q);
    __int128 twice = (r < 0 ? -r : r) * 2;
    if (twice > d) return static_cast<std::int64_t>(q + 1);
    if (twice < d) return static_cast<std::int64_t>(q);
    return static_cast<std::int64_t>(q % 2 == 0 ? q : q + 1);
}

int main()
{
    const tax::Box3Parameters p{ 2025, core::Money::FromCents(5768400, core::Currency::Eur), 137, 588, 3600, 1500 };
    const tax::Box3TaxModel model(p);
    std::mt19937_64 rng(11);
    std::uniform_int_distribution<std::int64_t> cents(0, 5000000000LL);
    int n = 0, differs = 0, ties = 0, shown = 0;
    for (int k = 0; k != 2000000; ++k)
    {
        const auto D = cents(rng), I = cents(rng);
        const tax::Box3Assets assets{ core::Money::FromCents(D, core::Currency::Eur), core::Money::FromCents(I, core::Currency::Eur) };
        const auto W = D + I;
        const auto Y = model.DeemedReturn(assets).Cents();
        const auto G = W > 5768400 ? W - 5768400 : 0;
        const auto exact = G ? DivHe(static_cast<__int128>(Y) * G, W) : 0;
        const auto got = model.TaxableReturn(assets).Cents();
        ++n;
        if (G && (static_cast<__int128>(Y) * G * 2) % W == 0) ++ties;
        if (got != exact)
        {
            ++differs;
            if (shown++ < 5) std::printf("  D %lld I %lld: library %lld, exact %lld (Y %lld, G %lld, W %lld)\n", (long long)D, (long long)I, (long long)got, (long long)exact, (long long)Y, (long long)G, (long long)W);
        }
    }
    std::printf("%d random portfolios up to 50,000,000.00 in each class: taxable return differs from the exact integer proportion in %d, exact half-cent ties %d\n", n, differs, ties);
    const tax::Box3Assets example{ core::Money::FromCents(2000000, core::Currency::Eur), core::Money::FromCents(8000000, core::Currency::Eur) };
    std::printf("worked example through the library: deemed %lld, base %lld, taxable return %lld, charge %lld cents\n", (long long)model.DeemedReturn(example).Cents(), (long long)model.TaxableBase(example).Cents(), (long long)model.TaxableReturn(example).Cents(), (long long)model.Charge(example).Cents());
    return 0;
}
```

```text
2000000 random portfolios up to 50,000,000.00 in each class: taxable return differs from the exact integer proportion in 0, exact half-cent ties 0
worked example through the library: deemed 497800, base 4231600, taxable return 210649, charge 75834 cents
```
