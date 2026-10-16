# Optimisation contracts

## Summary

The optimisation layer answers one question: given a way of running the wheel and a description of
how much risk is acceptable, which setting should be used? It states what a caller must supply
before a search can start, what the layer guarantees about every answer, and what it deliberately
refuses to express - chiefly a setting that was never inside the stated range, a score that a second
run would not reproduce, and a size of position chosen independently of the cash held back to
support it.

## Motivation

A search over a strategy is easy to make meaningless in three ways.

The first is comparing settings against different futures. If each candidate is scored against its
own fresh set of simulated paths, the difference between two scores is mostly the difference between
two samples, and a search will happily walk towards whichever candidate got lucky. Every candidate
is therefore scored against the same paths.

The second is optimising a quantity nobody would accept. The setting that grows a portfolio fastest
is the one that commits every last unit of cash at the widest strike it is allowed, and the price of
that is a drawdown, and an assignment rate, that no one would sit through. A score that ignores this
recommends it every time.

The third is answering with more confidence than the search has. The objective is the average of a
finite number of simulated runs of a strategy that trades whole contracts at listed strikes. It is
not smooth, it is not convex, and its slope is not a derivative in any but a rough sense. The layer
is arranged so that the shape of the answer - a setting inside a stated range, together with which
limits it is up against - carries no more precision than that.

## Definitions & notation

| Term | Meaning |
|---|---|
| Setting | the three choices a run makes: which delta to write at, how much cash to hold back, and at what delta a position is rolled away |
| Range | the lowest and the highest setting worth considering, one bound per choice |
| Experiment | everything held fixed while the setting varies: the capital, the underlying's model, the fee schedule, the tax rules, the cycle length, the horizon, the seed and the number of paths |
| Score | the mean growth rate, after costs and after tax, of the simulated runs of one setting |
| Limit | the deepest drawdown and the highest assignment rate that are acceptable |
| Excess | how far an outcome passed a limit; negative when it stayed inside |
| Charge | what an excess costs the score |
| Descent | the walk from a starting setting towards a cheaper one |
| Recommendation | the setting the descent arrived at, what it achieved, and which limits it is up against |

## Derivation / Rules

### What the caller must supply

| Requirement | Why it is a requirement |
|---|---|
| A range that grows in every choice | a choice whose bounds coincide is not being searched, and dividing by the width of that range has no answer |
| A target delta strictly between zero and one at both ends | at one, every strike qualifies; at zero, none does |
| A cash buffer between nothing and everything | a buffer is a fraction of free cash, so it cannot be negative nor exceed the whole |
| A positive roll delta at both ends | a limit of zero rolls every position the moment it is written |
| Limits between nothing and everything | a drawdown and an assignment rate are both fractions |
| A non-negative charge per unit of excess | a limit that pays a strategy for passing it is not a limit |
| At least one path | a score is a mean, and a mean of nothing has no value |
| A positive step and at least one step | a descent with no step length or no steps has not searched |
| A starting setting inside the range | the search reports a distance from where it started, and a start outside the range has none |

Each of these is a precondition, checked where it is stated and stopping the search when broken.

### What the layer guarantees

- **The same experiment produces the same recommendation, exactly.** Every setting is simulated from
  the same seed, so two scorings of one setting agree to the last digit, and two searches over one
  experiment arrive at the same setting.
- **Every setting is scored against the same futures.** The paths do not change from candidate to
  candidate. A difference between two scores is therefore a difference between two strategies, not
  between two samples, and the comparison stays meaningful even when the number of paths is far too
  small for either score to be trusted on its own.
- **A recommended setting is one the range could have produced.** The search is carried out on
  fractions of the stated range, so no coordinate can leave it: a step that would walk past a bound
  arrives at the bound instead, and the setting reported is read back off the range.
- **The score of a run that stays within every limit is its growth rate and nothing else.** A limit
  with slack costs nothing at all; only the amount by which a limit was passed is charged for, and
  the charge grows with the breach.
- **The reported achievement belongs to the reported setting.** The growth rate, the median balance,
  the deepest drawdown and the assignment rate quoted with a recommendation are all read from one
  simulation of the setting being recommended.
- **A recommendation says what it is up against.** Each limit is reported as binding or not, so a
  setting that is merely the best available under an impossible constraint is distinguishable from
  one that had room to spare.
- **A recommendation can be held to the setting already in use.** The search is expressed as a
  distance from the setting the caller runs today, and that distance can be charged for in two ways:
  one that shrinks every change towards nothing, and one that leaves a change out altogether unless
  it earns more than it costs. With neither charge applied, the search is unconstrained in this
  respect.

### What cannot be expressed

- **A number of contracts chosen on its own.** How many contracts a cycle writes follows from how
  much cash is free after the buffer is set aside and what one contract must collateralise. There is
  no separate choice of a contract count, because there is no freedom left for one to describe.
- **A setting outside the range.** Every point the search visits is read back through the range
  before it is simulated, so there is no route by which a setting the caller excluded can be scored,
  recommended, or even evaluated once.
- **A score computed from a fresh stream.** The seed belongs to the experiment, not to the scoring,
  so no candidate can be given a different set of paths from its rivals.
- **A limit that stops the search.** A limit is a charge on the score. A search under limits that no
  setting can meet still returns the least bad setting, reported as infeasible, rather than nothing
  at all.

## Edge cases & pitfalls

- **The score is a staircase, not a slope.** Contracts are whole, strikes are listed, and money is
  counted in minor units, so the score is constant across small changes of setting and jumps at the
  point where one more contract becomes affordable. A difference taken over a step small enough to
  be a derivative is therefore exactly zero everywhere. The step used is a substantial fraction of
  the range - large enough that the run on either side of it is a genuinely different run.
- **The descent takes a fixed number of steps and does not stop early.** It has no convergence test,
  so the number of steps and the step length have to be chosen together: too short a step and the
  search never crosses the range; too long and it overshoots the best setting and reports a worse
  one. Both failures are visible, because the achievement reported alongside a recommendation can be
  compared with the achievement of the setting the search started from.
- **A search is only as good as its number of paths.** With few paths the deepest drawdown is the
  deepest of a small sample and understates what the strategy can do, so a limit that looks
  comfortably met may not be. The number of paths belongs to the experiment for exactly this reason:
  it is a stated assumption, not a hidden one.
- **The best setting is usually on a bound.** With no limits in reach, growth is monotone in how much
  cash is committed, so the search runs to the edge of the range and stops there. That is a correct
  answer to the question asked, and a reminder that the range is itself a risk decision.
- **A charge has to be worth more than the growth it is competing with.** Growth rates over a few
  years are a few percent; a drawdown breach is a few tens of percent. A charge per unit of excess
  of around one therefore dominates, which is the intent - a limit should not be traded away for a
  fraction of a percent of return.

## Alternatives considered

- **Choosing a contract count directly.** Rejected: it is the same choice as the cash buffer, and
  offering both would let a caller state two answers to one question, with nothing to say which
  wins.
- **Ranking a grid of settings instead of walking downhill.** Reasonable, and cheaper to explain, but
  a grid fine enough to be useful in three choices costs far more simulations than a walk, and its
  answer is still a grid point rather than a setting.
- **Refusing a setting that breaks a limit outright.** Rejected: a hard refusal leaves a search with
  nothing to say when every setting breaks a limit, which is precisely the case where a caller most
  needs to be told how close it is possible to get and which limit is the obstacle.
- **Scoring on the median outcome rather than the mean.** Deferred. The median of a small number of
  paths moves in steps of one path and is therefore an even coarser staircase than the mean, which
  makes it a poor thing to take a difference of.
- **Charging for the size of a setting rather than for the change to it.** Rejected: a charge on the
  setting itself would pull a target delta towards zero, which is not a preference anybody expressed.
  A charge on the change expresses one that is real - a recommendation to leave things alone is worth
  more than a recommendation to alter three things for a hundredth of a percent.
