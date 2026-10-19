# The wheel

## Summary

The wheel is a repeating cycle of short options on one underlying: sell a put against cash set aside
to buy the shares, and if the shares are put to you, sell calls against them until they are called
away. Every leg of the cycle collects a premium, and the premium collected is what the projection
compounds. This note states the cycle, what each leg costs and settles, and what a cycle is worth.

## Motivation

An income strategy has to be written down as a rule before it can be projected, and "sell options for
income" is not a rule. The wheel is the smallest complete rule that produces income and can also lose
money in the way selling options actually loses money: it never leaves an obligation uncovered, so
every outcome is either cash or shares, and both are things a projection can carry forward.

Stating the cycle as a small set of states also fixes what cannot happen. A short put and a short
call are never held at once; a call is never written against shares that are already promised; a
position is never opened while another is open. Any sequence of events that would produce one of
those is a defect, not an unusual market.

## Definitions & notation

| Term | Meaning |
|---|---|
| Contract size | the number of shares one option contract covers, one hundred by convention |
| Cash-secured put | a short put with the full purchase price of the shares set aside in cash |
| Collateral | the cash set aside, equal to the strike times the contract size times the number of contracts |
| Assignment | the exercise of the option against the writer: the put writer buys, the call writer sells |
| Covered call | a short call written against shares already held and not promised elsewhere |
| Cycle | one short option from the day it is written to the day it expires, is assigned, or is closed |
| Premium | the amount received for writing the option, quoted per share and settled per contract |

Let $K$ be the strike, $m$ the contract size, $n$ the number of contracts, $p$ the premium per share
and $S_T$ the price at expiry. All cash amounts are whole minor units; premiums are quoted per share
to the minor unit and settle as $p \, m \, n$.

## Derivation / Rules

### The cycle

| Holding | What happens | Next holding |
|---|---|---|
| Cash | a put is written | short put |
| Short put | expires with $S_T \ge K$ | cash |
| Short put | expires with $S_T < K$, shares are assigned | shares |
| Short put | closed before expiry | cash |
| Shares | a call is written | short call |
| Short call | expires with $S_T \le K$ | shares |
| Short call | expires with $S_T > K$, shares are called away | cash |
| Short call | closed before expiry | shares |

Nothing else is a legal move. In particular a put is never written while shares are held and a call
is never written while only cash is held: those are the two ways a wheel silently turns into a
different strategy.

### What each leg settles

- **Writing.** The writer receives $p \, m \, n$ and pays a charge per contract. Writing a put also
  reserves $K m n$ in cash, which stays reserved until the position ends.
- **Expiring worthless.** Nothing settles. The premium was already received and the collateral or the
  shares are released.
- **Put assignment.** The writer pays $K m n$ - exactly the collateral reserved - and receives $mn$
  shares at a cost of $K$ each. A share transaction charge applies.
- **Call assignment.** The writer delivers $mn$ shares and receives $K m n$. A share transaction
  charge applies.
- **Holding shares.** Shares held at the opening of a cycle earn the dividend the yield accrues over
  it, received at the cycle's end net of the withholding stated for the underlying
  ([decision D51](../design/decisions.md)).

### What a cycle is worth

The premium is received whatever happens; assignment decides what is held afterwards, not whether the
premium was earned. Writing to expiry therefore has three outcomes, all with the premium in hand:

$$
\text{put: } \quad \begin{cases}
\text{cash unchanged plus } p m n, & S_T \ge K \\
\text{shares bought at } K \text{ while worth } S_T, & S_T < K
\end{cases}
$$

$$
\text{call: } \quad \begin{cases}
\text{shares kept, plus } p m n, & S_T \le K \\
\text{shares sold at } K \text{ while worth } S_T, & S_T > K
\end{cases}
$$

The put writer's loss when assigned is not the assignment itself - the cash paid equals the cash
reserved - it is holding shares worth $S_T$ that cost $K$. The call writer's cost when assigned is
the upside above $K$ that was sold. This is why an assignment does not change the value of the
holdings at the moment it happens, and why a projection must mark the shares to see the effect.

## Worked example

Six half-year cycles on one underlying, starting from 10,000.00 in cash, contract size one hundred,
no buffer held back, a charge of 0.75 per contract and 3.00 per share transaction.

| Cycle | Written | Strike | Premium/share | Contracts | Price at expiry | Outcome | Cash after | Shares | Equity |
|---|---|---|---|---|---|---|---|---|---|
| 1 | put | 40.00 | 0.80 | 2 | 45.00 | expires | 10,158.50 | 0 | 10,158.50 |
| 2 | put | 44.00 | 0.90 | 2 | 40.00 | assigned | 1,534.00 | 200 | 9,534.00 |
| 3 | call | 46.00 | 1.10 | 2 | 48.00 | assigned | 10,949.50 | 0 | 10,949.50 |
| 4 | put | 48.00 | 1.20 | 2 | 52.00 | expires | 11,188.00 | 0 | 11,188.00 |
| 5 | put | 52.00 | 1.30 | 2 | 50.00 | assigned | 1,043.50 | 200 | 11,043.50 |
| 6 | call | 54.00 | 1.40 | 2 | 53.00 | expires | 1,322.00 | 200 | 11,922.00 |

Cycle 2 in full: two contracts reserve $44.00 \times 100 \times 2 = 8{,}800.00$; the premium is
$0.90 \times 100 \times 2 = 180.00$ and the contract charge $1.50$, so cash rises from $10{,}158.50$
to $10{,}337.00$. The price closes below the strike, so 8,800.00 is paid for 200 shares and a share
transaction charge of 3.00 is added, leaving 1,534.00 in cash. Equity is
$1{,}534.00 + 200 \times 40.00 = 9{,}534.00$: the 800.00 lost on shares bought at 44.00 and worth
40.00, less the 4.50 of charges, against the 180.00 premium.

Over the six cycles the premiums total 1,340.00, the charges 18.00 and the assignments net to
-10,000.00, which is exactly the change in cash from 10,000.00 to 1,322.00. Equity ends at 11,922.00,
of which 10,600.00 is 200 shares at 53.00.

## Edge cases & pitfalls

- **Not enough cash for one contract.** Nothing is written and nothing settles. A cycle that writes a
  fraction of a contract does not exist.
- **A price exactly at the strike.** Neither leg is assigned: assignment requires the option to be in
  the money, not at the money.
- **Assignment is value-neutral at the moment it happens.** Cash paid equals collateral released and
  the shares arrive at the strike; the profit or loss is in the mark afterwards, never in the
  settlement itself. A model that books a loss at assignment has counted it twice.
- **The premium is not a yield on the equity.** It is a yield on the collateral committed, which is
  never the whole portfolio once a buffer and the granularity of a contract are accounted for.
- **The shares are held, not traded.** Between assignment and call-away the position is a plain long
  holding, exposed to the whole of the downside below the strike that was paid.

## Alternatives considered

- **Naked short options.** More premium per unit of capital, but the loss is unbounded and the
  capital committed is a margin requirement rather than a known amount of cash. Out of scope.
- **Selling puts continuously and never taking assignment.** Requires closing every tested position
  at whatever it costs, which turns the strategy into a bet on the closing price of options rather
  than on the underlying. Rolling covers the useful part of this without abandoning the cycle.
- **Multi-leg spreads.** They cap the loss and would change the collateral rule completely. Out of
  scope for this version.

## See also

- [Position sizing and compounding](position-sizing-and-compounding.md)
- [Rolling a short option](rolling-a-short-option.md)
- [Assignment and early exercise](assignment-and-early-exercise.md)
- [Option contract conventions](option-contract-conventions.md)
