# Position sizing and compounding

## Summary

Sizing is the rule that turns free capital into a number of contracts, and it is the entire
compounding mechanism of this project: premiums raise the capital, and the raised capital is what the
next cycle is sized from. This note states the rule, the buffer that is held back from it, and the
consequences of only being able to sell whole contracts.

## Motivation

A projection that reinvests continuously - taking the premium as a percentage return on the whole
portfolio and compounding it - overstates an option-selling strategy badly, for two reasons that both
run one way.

The first is granularity. Capital is committed in units of one contract's collateral, which for a
40.00 strike is 4,000.00. An account of 10,000.00 can sell two contracts and leave 2,000.00 idle; it
does not sell two and a half. The idle remainder earns no premium, and it is a fifth of the account.

The second is that not all capital can be committed. Charges are payable, an assignment costs a share
transaction, and a position that has to be closed has to be paid for. Money that is reserved as
collateral cannot pay for any of them. A projection that commits every cent is not a projection of a
strategy that can actually be run.

## Definitions & notation

| Quantity | Unit | Meaning |
|---|---|---|
| Free capital | minor units | cash that is not already reserved as collateral |
| Buffer rate | basis points | the fraction of free capital deliberately not committed |
| Retained | minor units | free capital times the buffer rate |
| Investable | minor units | free capital less the retained amount |
| Collateral per contract | minor units | the strike times the contract size |
| Contracts | count | how many contracts are written this cycle |

With $F$ free capital, $b$ the buffer in basis points and $C$ the collateral per contract:

$$
R = \operatorname{round}\!\left(\frac{F b}{10^4}\right), \qquad I = F - R, \qquad n = \left\lfloor \frac{I}{C} \right\rfloor .
$$

The retained amount is rounded to the minor unit half-to-even, and the investable amount is the
remainder, so the two always add back to the free capital exactly.

## Derivation / Rules

- **Only free capital is ever sized from.** Collateral already reserved against an open position is
  not available; it is spoken for until that position ends.
- **The buffer comes off first.** It is a fraction of free capital rather than a fixed amount, so it
  scales with the account instead of becoming irrelevant as the account grows.
- **Contracts are whole.** The number written is the investable amount divided by one contract's
  collateral, rounded down. Any remainder stays in cash and is offered again next cycle.
- **A covered call is not sized from cash at all.** It is sized from shares: as many contracts as
  there are unpromised lots of one contract size.
- **The compounding is the feedback, not a rate.** No growth rate is applied to anything. Premiums
  and settlements change the cash; the next cycle's sizing sees the changed cash. A projection that
  compounds is one where the sizing rule is applied again to a larger balance.

## Worked example

Collateral per contract at a 40.00 strike is 4,000.00.

| Free capital | Buffer | Retained | Investable | Contracts | Committed | Idle |
|---|---|---|---|---|---|---|
| 3,999.99 | 0 | 0.00 | 3,999.99 | 0 | 0.00 | 3,999.99 |
| 4,000.00 | 0 | 0.00 | 4,000.00 | 1 | 4,000.00 | 0.00 |
| 11,999.99 | 0 | 0.00 | 11,999.99 | 2 | 8,000.00 | 3,999.99 |
| 12,000.00 | 0 | 0.00 | 12,000.00 | 3 | 12,000.00 | 0.00 |
| 10,000.00 | 5% | 500.00 | 9,500.00 | 2 | 8,000.00 | 2,000.00 |

At a 45.00 strike the collateral per contract is 4,500.00, and a buffer is enough to cost a contract:
10,000.00 fully committed writes two, while the same 10,000.00 with 15% held back has 8,500.00
investable and writes one.

The step structure is the point. Between 8,000.00 and 11,999.99 of investable capital the strategy
writes exactly two contracts at this strike, so the premium collected does not grow at all across a
50% increase in capital; it then jumps by half at 12,000.00. Averaged over many cycles and rising
strikes this is a drag, not a wash, because the remainder is idle every single cycle.

## Edge cases & pitfalls

- **Free capital below one contract.** Nothing is written. This is normal early in a projection and
  after an assignment has consumed the cash, not an error.
- **A buffer of the whole balance.** Nothing is ever committed and the projection is flat. It is
  permitted so that the degenerate case is a configuration choice rather than a special case.
- **The buffer does not guarantee solvency.** It only holds back a fraction of free capital at the
  moment of sizing. Charges incurred later in the cycle are paid from whatever is uncommitted then.
- **Rounding the retained amount, not the investable one.** Doing it the other way would let a
  fraction of a minor unit vanish between the two, and the pair would no longer add back to the
  balance they were split from.
- **Sizing against a strike, not against the price.** The collateral follows the obligation, which is
  the strike. A model that sizes against the current price commits the wrong amount whenever the
  option is not at the money.

## Alternatives considered

- **A fixed cash buffer in minor units.** Simpler to state, but it is a real constraint on a small
  account and a rounding error on a large one, so a projection over decades would silently stop
  respecting it.
- **Sizing by a target fraction of equity per position.** A better risk rule in general, but it needs
  a mark of the whole portfolio at the moment of sizing and it hides the granularity effect that the
  collateral rule makes visible.
- **Allowing fractional contracts.** Would make the projection smooth and the arithmetic easier, and
  would overstate every result for an account small enough to care.

## See also

- [The wheel](wheel-strategy.md)
- [Money and rounding](money-and-rounding.md)
- [Broker costs](broker-costs.md)
