# fin-bs

**Options income & compounding strategy estimator.**

Estimates the compounded, after-cost, after-tax growth of a covered-call / cash-secured-put
("wheel") options-income strategy, with **Dutch (Box 3) taxation** and **DEGIRO** cost modelling.

Numerics come from [`embedded-pro/numerical-toolbox-cpp`](https://github.com/embedded-pro/numerical-toolbox-cpp)
(regression, filtering, optimization, ODE integrators, `math::Erfc`). Everything finance-specific
(option pricing, Greeks, assignment, ledger, tax, fees) is built in this repository on top of it.

> **Not financial or tax advice.** This is a modelling tool. Every tax parameter must be validated
> against current Belastingdienst rules for your tax year before the output means anything.
> See the validation checklist in [PLAN.md](PLAN.md).

## Status

Early development. [PLAN.md](PLAN.md) holds the architecture, the assumptions behind the model,
and the task breakdown (Epics 0-9). Epic 0 (build foundation) is the only one in progress.

## Prerequisites

- CMake >= 3.24 and Ninja
- A C++20 compiler (GCC 12+, Clang 15+, or MSVC 19.3x)
- Git

A ready-made toolchain is available via the dev container in `.devcontainer/`.

## Getting started

```sh
git clone --recursive https://github.com/gabrielfrasantos/fin-bs.git
cd fin-bs

cmake --preset host
cmake --build --preset host-Debug
ctest --preset host-Debug
```

Already cloned without `--recursive`:

```sh
git submodule update --init --recursive
```

`embedded-infra-lib` is fetched automatically by CMake at configure time; only
`numerical-toolbox` is vendored as a submodule.

## Presets

| Preset | Purpose |
|---|---|
| `host` | Multi-config (Ninja Multi-Config) host build with tests |
| `host-single-Debug` | Single-config Debug, for generators that need it |
| `coverage` | Instrumented build used by the SonarCloud workflow |

## Layout

```
config/              tax, broker and scenario configuration (no rates in code)
data/                user-provided price history and options-chain snapshots
numerical-toolbox/   git submodule: numerical-toolbox-cpp
source/              the project, one CMake target per module (fin.core, fin.pricing, ...)
documentation/       methodology, tax notes, validation log
```

Each module carries its own `test/` directory; tests are GoogleTest and run on the host.

## License

See [LICENSE](LICENSE).
