# Rolling a short option

## Summary

A roll is closing a short option before it expires and writing another one in its place. This note
states the three conditions under which the model rolls - time remaining, premium already earned, and
risk of assignment - what a roll costs, and why a rolled position can no longer be assigned.

## Motivation

Holding a short option to expiry is not free. The last days of an option's life carry very little
premium left to earn but the whole of the assignment risk, and a position that has already given up
most of its premium is being held for the remainder at the worst ratio of reward to risk it will ever
offer. A rule that closes such a position early and writes a fresh one converts the remaining risk
back into premium.

The rule has to be stated as a trigger rather than a judgement, because a projection has to be able
to run it thousands of times without a human looking at it, and because the value of the strategy
depends on which trigger fires and how often.

## Definitions & notation

| Quantity | Unit | Meaning |
|---|---|---|
| Days to expiry | days | calendar days from the observation to the expiry |
| Premium received | per share | what was collected when the position was written |
| Current value | per share | what it would cost to buy the position back now |
| Captured fraction | basis points | the part of the premium that is no longer owed |
| Delta | dimensionless | the option's sensitivity to the underlying, a proxy for the chance of assignment |

With premium received $p$ and current value $v$, the captured fraction in basis points is

$$
c = \left\lfloor 10^4 \left(1 - \frac{v}{p}\right) \right\rfloor ,
$$

clamped to $[-10^4, 10^4]$ and rounded down, so a position is never credited with capture it has not
actually made.

## Derivation / Rules

A position is rolled when any of the following holds.

| Trigger | Condition | What it is protecting against |
|---|---|---|
| Risk | $\lvert \Delta \rvert \ge$ the configured limit | the price has come to the strike and assignment is becoming likely |
| Profit | $c \ge$ the configured capture | holding the remainder for a poor ratio of reward to risk |
| Time | days to expiry $\le$ the configured number | the last days, where little premium remains and assignment risk is concentrated |

When more than one holds, the reason reported is the first in that order: risk before profit, profit
before time. The order is a reporting convention only - the position is rolled either way - but it
matters for the account of *why* a projection did what it did, and risk is the reason that would have
justified the roll on its own.

A roll settles as a close: the current value is paid, a contract charge applies, the collateral or the
share cover is released, and the cycle ends there. **The position no longer exists at expiry, so it
cannot be assigned**, whatever the price does afterwards. That is the whole point of rolling, and it
is also the one thing a projection must not get wrong: a rolled put that is nevertheless assigned
would double-count both the cost of closing and the cost of the shares.

The replacement is subject to the same risk limit as the position it replaces. A roll into a strike
closer to the money would exchange a known cost for a larger obligation, which is not a roll but an
increase in the position.

## Worked example

A put is written for 0.80 per share on two contracts of one hundred shares: 160.00 received, 1.50 of
contract charges. With a capture trigger at 50%, a risk limit of 0.45 delta and a time trigger at
seven days, the position is observed thirty days before expiry at 0.20 per share and a delta of
-0.20.

| Step | Value |
|---|---|
| Captured | $1 - 0.20/0.80 = 75\%$, which is 7,500 basis points |
| Risk | $\lvert -0.20 \rvert < 0.45$, no |
| Time | 30 days $> 7$, no |
| Reason reported | profit captured |
| Paid to close | $0.20 \times 100 \times 2 = 40.00$, plus 1.50 of charges |
| Kept from the cycle | $160.00 - 1.50 - 40.00 - 1.50 = 117.00$ |

The cycle ends with 117.00 kept, no shares, and no assignment - even though the price at that expiry
closed at 35.00, five below the strike, which would otherwise have put 200 shares into the account at
40.00.

The same position observed at 0.70 per share instead has captured only 1,250 basis points, breaches
neither the risk nor the time trigger, and is left alone: it runs to expiry and is assigned.

## Edge cases & pitfalls

- **A position worth more than it was sold for.** The captured fraction is negative and the profit
  trigger cannot fire. Such a position is rolled, if at all, on risk or on time.
- **A position worth nothing.** Capture is the whole premium. Rolling it is nearly free and is the
  usual outcome of a wide capture trigger.
- **A capture trigger of zero.** Would fire the instant the position was written, so it is refused: a
  trigger that is always true is a defect, not a strategy.
- **A risk limit of zero.** Every position breaches it. Refused for the same reason.
- **Rolling is not free money.** Each roll pays the value of the option back plus two contract
  charges over the cycle instead of one. A capture trigger set too low turns a premium-selling
  strategy into a charge-paying one.
- **Delta as a probability.** Treating the magnitude of delta as the chance of finishing in the money
  is an approximation that is good near the money and poor in the tails. It is used as a risk *limit*
  here, never as a probability in a result.

## Alternatives considered

- **Always holding to expiry.** Simplest, and it is what the model does when no observation is
  supplied. It concentrates every assignment at the expiry dates and earns the last, cheapest part of
  every premium.
- **Rolling on a fixed calendar.** Predictable but blind: it closes cheap positions that had nothing
  left to give and holds expensive ones that had already turned against the writer.
- **Rolling into a further expiry at the same strike for a credit.** The common retail rule. It
  requires a chain to price the replacement rather than a trigger to decide, so it belongs with the
  chain data rather than with the decision rule.

## See also

- [The wheel](wheel-strategy.md)
- [Option Greeks](option-greeks.md)
- [Assignment and early exercise](assignment-and-early-exercise.md)
