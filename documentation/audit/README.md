# Source audit

A phased, report-only review of `source/`, one module per phase, written for the point at which
this tool starts to inform real-money decisions. Each report states what was executed, what was
verified independently, every finding with evidence and a proposed fix, and what was *not* checked.
No code was changed by the audit itself; fixes are a separate decision per finding.

## Severity scale

| Severity | Meaning |
|---|---|
| Critical | A wrong amount or wrong number reaches a report, or data is silently corrupted |
| High | Wrong under realistic inputs, or a documented guarantee is not enforced |
| Medium | Robustness, precision or maintainability risk; a promise that is not pinned by a test |
| Low | Clarity, hygiene, a defensive check that is cheap and currently unreachable |
| Info | A note for the owner; no action implied |

## Phases

| # | Module | Report | Status | Critical | High | Medium | Low | Info |
|---|---|---|---|---|---|---|---|---|
| 1 | `source/core` | [01-core.md](01-core.md) | done 2026-10-18 | 0 | 0 | 2 | 6 | 5 |
| 2 | `source/costs` | [02-costs.md](02-costs.md) | done 2026-10-19 | 0 | 0 | 3 | 5 | 2 |
| 3 | `source/estimation` | [03-estimation.md](03-estimation.md) | done 2026-10-19 | 0 | 1 | 3 | 4 | 1 |
| 4 | `source/io` | [04-io.md](04-io.md) | done 2026-10-19 | 0 | 0 | 2 | 5 | 1 |
| 5 | `source/optimize` | [05-optimize.md](05-optimize.md) | done 2026-10-19 | 0 | 1 | 1 | 4 | 2 |
| 6 | `source/pricing` | [06-pricing.md](06-pricing.md) | done 2026-10-19 | 0 | 0 | 1 | 3 | 2 |
| 7 | `source/strategy` | [07-strategy.md](07-strategy.md) | done 2026-10-19 | 0 | 2 | 2 | 7 | 4 |
| 8 | `source/tax` | [08-tax.md](08-tax.md) | done 2026-10-19 | 0 | 0 | 2 | 4 | 4 |
| 9 | `source/sim` | [09-sim.md](09-sim.md) | done 2026-10-19 | 0 | 1 | 3 | 6 | 3 |

`present`, `gui` and `app` are display code and are out of scope.

## Where to start

All nine phases are done. Across them: 0 Critical, 5 High, 19 Medium, 44 Low, 24 Info. The
arithmetic of every layer reproduced independently; the findings that matter for real money are
about what the chain does not do and what stops it.

1. A charge the free cash cannot meet aborts the process. The shipped example scenario dies at
   path 3,470 of a 10,000-path run (SIM-1, STRAT-1). Decide what an insolvent path means.
2. No dividend is ever received while options are priced with a yield (STRAT-2); the dividend
   model that exists caps domestic withholding wrongly (TAX-2).
3. Premiums are fair value at the path's own volatility with no spread (SIM-3), the roll leaves
   capital idle until the next boundary (STRAT-4), and the optimiser's result depends on its step
   size (OPT-1) and on the path count through the worst drawdown (SIM-4).
4. The numerical toolbox's `fast-math` pragma reaches seven project files; in Release builds the
   estimators accept NaN parameters (ESTIM-1). The push/pop wrap is a two-line fix per file.
5. The classification of brokerage cash for Box 3 is fixed in code (TAX-1), the assignment charge
   is an assumption (COSTS-3), and two validation figures remain unconfirmed (COSTS-9, TAX-8).

## Method, applied identically in every phase

1. Build and run the module's own tests; record the result verbatim.
2. Read every file of the module and the design, theory and decision-log entries it implements.
3. Review against a fixed checklist: financial correctness (formulas, units, signs, day counts,
   rounding points); every documented invariant enforced and tested; numeric robustness (overflow,
   precision, NaN, division by zero, degenerate inputs); C++ safety (undefined behaviour,
   narrowing, lifetime, const-correctness, assertions versus reported errors); test adequacy;
   documentation drift; correct use of the two external libraries.
4. Reproduce every closed-form or externally known value independently in Python, and attach the
   script and its output so the check can be re-run.
5. Record findings with `file:line`, evidence, impact in money terms where possible, a proposed fix
   and a suggested test.
