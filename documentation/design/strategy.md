# Strategy contracts

## Summary

The strategy layer is where holdings, cashflows and the rules of the wheel meet: it says what a
portfolio may do, what every movement of cash is called, how much capital becomes contracts, which
transitions of the cycle exist, when a position is rolled, and what a projection over a scripted
price path reports. It states what the caller must supply, what the layer guarantees, and what it
deliberately makes impossible to express.

## Motivation

Everything above this layer - taxation, simulation, optimisation, reporting - reads the same two
things: what is held, and what moved. If either can be wrong, every number downstream is wrong in a
way that is invisible, because a projection has no external reference to be checked against.

Two failures are worth designing against specifically. The first is capital that is committed twice:
cash reserved against a short put is not available to secure another one, to pay a charge, or to buy
shares, and a model that forgets this reports a strategy nobody could have run. The second is equity
that drifts away from the cashflows that produced it, which happens as soon as equity is stored and
updated rather than derived; after a few hundred cycles the two disagree and there is no way to tell
which is right.

## Definitions & notation

| Term | Meaning |
|---|---|
| Holdings | cash, share lots, and short option positions, at one moment |
| Collateral | the part of cash reserved against short puts, unavailable for anything else |
| Free cash | cash less collateral: what may actually be spent or committed |
| Lot | a block of shares acquired together, carrying what was paid per share |
| Cashflow | one dated, typed, signed movement of cash |
| Equity | cash, plus shares at a stated price, less what it would cost to settle the short positions |
| Period | one cycle of the wheel, from writing an option to its expiry or early close |

## Derivation / Rules

### What the caller must supply

| Requirement | Why it is a requirement |
|---|---|
| A portfolio funded with a non-negative amount in one currency | a strategy that starts in debt is a different model |
| Enough free cash to secure a put before writing it | an unsecured put is not the strategy being modelled |
| Shares that nothing else covers before writing a call | a call written twice against one holding is naked once |
| An amount to pay that free cash covers | reserved cash is not a source of payment |
| Cashflows dated in non-decreasing order | a ledger is a record of what happened, in the order it happened |
| A charge posted as a charge and a receipt as a receipt | the sign of an entry carries meaning that a later sum relies on |
| Roll triggers that are neither always nor never true | such a trigger is a defect in the configuration, not a strategy |
| A projection started from a portfolio with nothing open | the projection owns the cycle and cannot adopt a position it did not write |
| A scripted period whose observation falls between writing and expiry | an observation outside the cycle is not an observation of it |

Each of these is a precondition, checked wherever it is stated and stopping the run when broken.
None of them is external input: a scenario file is validated at the boundary, and by the time it
reaches this layer a violation is a programming error.

### What the layer guarantees

- **Cash is never negative and collateral never exceeds it.** Reserving, paying, being assigned and
  closing all preserve both, which is what makes "free cash" meaningful at every moment rather than
  only between cycles.
- **Assignment moves value; it does not create or destroy any.** The cash paid on a put assignment is
  exactly the collateral released, and the shares arrive priced at the strike, so equity marked at any
  price is the same immediately before and immediately after. The same holds for a call assignment.
  This is an invariant, not an approximation, and it is the sharpest available check that the
  settlement arithmetic is right.
- **Shares leave in the order they arrived.** Lots are consumed oldest first, so the cost basis of
  what remains is determinate.
- **Equity is derived from holdings and a price, every time it is asked for.** It is never stored,
  never updated incrementally, and therefore cannot drift from the holdings it describes.
- **The ledger explains every movement of cash.** The opening balance plus the sum of the ledger is
  the closing balance, at every point in a projection. Cashflows are typed, so the same identity
  holds per kind: what premiums contributed, what charges cost, what assignments moved.
- **The cycle is a total function of holding and event.** Every combination is either a stated next
  holding or a refusal; there is no default that quietly does nothing.
- **Sizing is the only place capital becomes contracts.** No other part of the layer invents a
  position, which is what makes the compounding mechanism a single reviewable rule.
- **A rolled position cannot be assigned.** Closing ends the cycle; expiry is then a date with nothing
  outstanding.
- **A projection is deterministic.** The same script produces the same rows, with no clock, no
  randomness and no hidden state carried between runs.

### What cannot be expressed

- A short position without the cash or the shares to cover it.
- Spending money that is reserved.
- A fee that is a receipt, a tax that is a refund, or a dividend that is a charge.
- Selling a fraction of a contract.
- Writing a put while holding shares, or a call while holding only cash.
- A cashflow dated before the one already recorded.
- Equity as an independent number that can disagree with the holdings.

### Where the numbers come from

The layer prices nothing. Premiums, the value of a position at an observation, and the price at
expiry are supplied by the script that drives the projection; assignment is decided by the moneyness
rule of the valuation layer; charges are computed by the cost layer from a schedule read from a file.
This is deliberate: a projection whose premiums came from a model would be a projection of the model,
and its agreement with itself would prove nothing.

## Worked example

Two contracts of a 44.00 put are written against 10,158.50 in cash: 8,800.00 is reserved, 180.00 of
premium is received and 1.50 of contract charges is paid, leaving 10,337.00 in cash of which
1,537.00 is free. The price closes at 40.00, the put is assigned, 8,800.00 is paid for 200 shares and
3.00 of share transaction charges is paid.

| Check | Value |
|---|---|
| Cash | 1,534.00 |
| Collateral | 0.00 |
| Shares | 200, one lot, at 44.00 |
| Ledger: premium | 180.00 |
| Ledger: charges | -4.50 |
| Ledger: assignment | -8,800.00 |
| Opening cash plus ledger | $10{,}158.50 - 8{,}624.50 = 1{,}534.00$ |
| Equity at 40.00 | $1{,}534.00 + 200 \times 40.00 = 9{,}534.00$ |

## Edge cases & pitfalls

- **A period that writes nothing.** When free capital does not secure one contract, or no share lot
  is uncovered, the period still reports a row: nothing happened, and that is a result.
- **Equity while a position is open.** A short option is carried at what it would cost to settle it
  at the stated price, which is its intrinsic value. Rows are reported at expiry, where that is the
  whole of the option's value, so the figure is exact wherever it is published. Between those dates
  it understates the liability by the extrinsic value. See [D22](decisions.md).
- **The order of releasing and paying.** A position is released before its settlement is paid, so the
  payment is made from cash the position itself had reserved. Reversing the order would break the
  free-cash precondition on an assignment that is in fact fully funded.
- **Charges are paid from free cash, not from collateral.** A projection can therefore fail to afford
  a charge while apparently holding a large balance. That is the model behaving correctly; the cash
  buffer exists to prevent it.
- **A premium can be negative.** Buying a position back is a premium paid. Only charges, taxes and
  currency handling have a fixed direction.

## Alternatives considered

- **Storing equity and updating it.** Cheaper to read and impossible to audit. Rejected: the derived
  figure is the only one that can be checked against the ledger.
- **Margin rather than reserved cash.** Closer to what a broker actually requires, and it would make
  the collateral rule depend on a broker's risk model, on the price, and on the day. The strategy
  being modelled is the cash-secured one; see [D21](decisions.md).
- **A ledger of untyped amounts.** Simpler, and it would make "how much did charges cost over twenty
  years" unanswerable, which is one of the questions the project exists to answer.
- **Letting the projection price its own options.** Would remove the need for a script and would make
  every projection a projection of the pricing model rather than of the strategy.

## See also

- [The wheel](../theory/wheel-strategy.md)
- [Position sizing and compounding](../theory/position-sizing-and-compounding.md)
- [Rolling a short option](../theory/rolling-a-short-option.md)
- [Cost contracts](costs.md)
- [Decision log](decisions.md)
