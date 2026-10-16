# fin-bs - Copilot / AI agent instructions

Task-oriented guide for AI coding agents working in this repository.

## 1. What this project is

An **options income & compounding strategy estimator**: it projects the after-cost, after-tax
growth of a covered-call / cash-secured-put ("wheel") strategy under **Dutch Box 3 taxation** and
**DEGIRO** fees.

- [numerical-toolbox-cpp](https://github.com/embedded-pro/numerical-toolbox-cpp) - fetched by CMake
  at an exact pin, not vendored. An embedded DSP/control library supplying **numerics only**:
  regression, filters, optimization, ODE integrators, `math::Erfc`. It has **no finance primitives**.
- `embedded-infra-lib` (emil) - fetched by CMake too, and declared **before** the toolbox, which only
  fetches emil itself when it is the top-level project. Provides `infra.util`, the `emil_*` CMake
  helpers, and the JSON parser in `infra/syntax/Json.hpp`.
- `source/` - everything finance-specific, written here: pricing, estimation, costs, tax,
  strategy, simulation, optimization, I/O, CLI.

There are no submodules; a plain `git clone` plus a configure with network access is enough.

Read [PLAN.md](../PLAN.md) before proposing design changes: it holds the architecture, the
explicit modelling assumptions, and the Epic 0-9 task breakdown with acceptance criteria.

## 2. Build & test

```sh
cmake --preset host
cmake --build --preset host-Debug
ctest --preset host-Debug
```

Presets are the only supported interface: `host` (multi-config), `host-single-Debug`, `coverage`.
This is a **host-only desktop project** - there are no embedded targets, no MCU toolchains, no
simulator. Do not reintroduce them.

## 3. Project-specific rules (these override general C++ habits)

- **Money is never floating point.** Use `fin::core::Money` (integer cents). `float`/`double` are
  fine for volatility, rates and price paths, but any cash amount, fee or tax figure is `Money`.
  Convert at the boundary.
- **No magic numbers.** Tax rates, fee schedules and strategy parameters live in `config/*.json`
  and are loaded at runtime. Box 3 rules change yearly, so they are versioned per tax year
  (`config/tax/nl_<year>.json`) and must never be hardcoded.
- **No vendor names in type names.** A broker, an exchange or a tax authority may appear in a
  *configuration file* name, because a file holds one vendor's actual data. Code that applies that
  data is generic: fees are applied by a broker-agnostic type, not by one named after a broker.
- **Do not write a parser, and do not wrap one.** Configuration is read with emil's
  `infra/syntax/JsonFileReader.hpp` and `JsonObjectNavigator.hpp`, which already own the document,
  validate it and report a missing or mistyped key by name. A loader catches at the boundary and
  reports; it adds no reading layer of its own. Do not add third-party dependencies.
- **Box 3 is a wealth tax, not a capital-gains tax.** Tax is computed on portfolio value at the
  peildatum, *not* on premium income. Getting this wrong silently invalidates every projection.
- **Toolbox types stay inside their module.** `numerical::`/`math::` types must not appear in a
  public `fin::` API; wrap them.
- One public type per header, PascalCase filenames, `namespace fin::<module>`.
- Headers use `#pragma once`.
- Code carries no comments: names, types and tests are the documentation. The *why* lives in
  `documentation/`, which is written in domain terms and never refers to code.
- Heap use is allowed (this is a desktop tool), but prefer value semantics and bounded containers
  where it costs nothing.

## 4. Layout & dependency direction

```
app -> io -> {strategy, optimize, sim} -> {pricing, estimation, tax, costs} -> core -> numerical-toolbox
```

Each module is a CMake target named `fin.<module>` in `source/<module>/`, with tests in
`source/<module>/test/` registered via `emil_add_test`, mirroring the toolbox's own convention.

## 5. Testing expectations

GoogleTest on the host; every change ships tests. Assert **properties and invariants**, not golden
numbers pulled from your own implementation:

- Pricing: put-call parity, monotonicity in sigma and S, Greeks vs. central finite differences,
  published reference prices.
- Estimators: recover known parameters from synthetic GBM with a fixed seed.
- Ledger/tax: hand-checked worked examples, boundary cases (exactly at the allowance, zero, empty).
- Monte-Carlo: convergence to the closed-form expectation; fixed seed so CI is deterministic.

Tests must be deterministic - no wall-clock time, no unseeded RNG, no network.

## 6. When making changes

- Update the relevant task row in [PLAN.md](../PLAN.md) section 8 when you complete one.
- New tax or fee parameters require a note in `documentation/` recording the authoritative source
  and the date it was checked.
- Changing a dependency pin means changing it in the root `CMakeLists.txt`; the emil pin must stay
  in step with the one the toolbox declares for itself.
