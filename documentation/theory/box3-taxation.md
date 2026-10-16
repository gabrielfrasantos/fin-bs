# Wealth taxation of an investment portfolio

## Summary

The Netherlands does not tax the profit an investor makes; it taxes the wealth the investor holds.
Once a year, on a single reference date, every asset is valued, a *deemed* return is imputed to it
from published percentages, and a fixed rate of tax is charged on the part of that deemed return that
belongs to wealth above a tax-free allowance. Premiums, capital gains, losses and dividends do not
enter the calculation at all.

## Motivation

Almost every published options-income calculator is written for a jurisdiction that taxes realised
profit. Transplanting one to this jurisdiction produces answers that are wrong in both directions and
that are wrong *structurally*, not by a few percent.

- A strategy whose income is large relative to its capital - which is exactly what selling options
  is - is taxed **far less** than a profit tax would suggest, because the tax does not see the income.
- A strategy that loses money is still taxed, because the tax does not see the loss. A portfolio can
  pay tax in a year in which it fell.
- Sitting in cash is **not** free: cash on the reference date is wealth, and is taxed, at a lower
  imputed return than investments but at more than nothing. A wheel strategy holds a great deal of
  cash as collateral, so this is not a rounding error.
- Trading more often changes the tax bill **not at all**, because the tax has no idea how often you
  traded. The only decision that changes it is what you are holding when the clock strikes.

Getting this wrong does not produce a slightly optimistic projection; it produces a projection of a
different strategy in a different country. This is the single most consequential modelling choice in
the project.

## Definitions & notation

| Symbol | Meaning | Unit |
|---|---|---|
| $t^\*$ | the **reference date**: the one instant in the year at which wealth is measured | a date |
| $D$ | **deposits**: money held as a balance at a bank | minor units |
| $I$ | **investments**: shares, funds, options and everything else that is not a deposit | minor units |
| $W = D + I$ | the **return base**: everything owned on the reference date | minor units |
| $A$ | the **allowance**: the amount of wealth that is free of tax | minor units |
| $G = \max(W - A,\ 0)$ | the **taxable base**: wealth above the allowance | minor units |
| $d,\ i$ | the **imputed returns** on deposits and on investments, published per year | basis points |
| $\tau$ | the **tax rate** applied to the taxable return | basis points |
| $w$ | the **withholding rate** on a dividend | basis points |

Every rate is a whole number of basis points, one basis point being one ten-thousandth. See
[decision D18](../design/decisions.md).

## Derivation / Rules

### The annual charge

The charge is built in four steps, and the third is the one everybody skips.

$$Y = \frac{d \cdot D}{10^4} + \frac{i \cdot I}{10^4}$$

is the **deemed return**: what the rules assume the wealth earned, given how it was held. Its
composition matters, because the imputed return on investments is several times the one on deposits.

$$G = \max(W - A,\ 0)$$

is the **taxable base**. The allowance is subtracted from the *wealth*, not from the return.

$$V = Y \cdot \frac{G}{W} \qquad (W > 0)$$

is the **taxable return**. The allowance does not exempt a particular asset; it exempts a *share* of
the whole portfolio, and the deemed return is reduced in that same proportion. Applying the allowance
to the wealth and then imputing a return to what is left would give the same answer only if all
wealth were of one kind; with a mixture it does not, and the difference is not small.

$$T = \frac{\tau \cdot V}{10^4}$$

is the **charge**. It is struck on the reference date and depends on nothing else that happened in
the year.

### Dividends

A dividend is paid with tax already withheld at the source. Withholding at the domestic rate is a
prepayment: it is credited in full against the annual charge. Withholding by a foreign state is
credited only up to the rate that is creditable here; anything withheld above that is **leakage** - a
cost that is neither recovered from the foreign state nor returned by the domestic one.

$$\text{withheld} = \frac{w \cdot \text{gross}}{10^4}, \qquad
\text{creditable} = \frac{\min(w,\ \bar{w}) \cdot \text{gross}}{10^4}, \qquad
\text{leakage} = \text{withheld} - \text{creditable}$$

The credit is capped at the annual charge. A credit larger than the charge is not repaid, so a
portfolio below the allowance recovers nothing at all of what was withheld from it.

## Worked example

Wealth on the reference date: 20 000 held as a deposit and 80 000 held as investments. The published
rules for the year impute 1.37% to deposits and 5.88% to investments, allow 57 684 free of tax, and
charge 36%.

| Step | Working | Result |
|---|---|---|
| Return base | $20\,000 + 80\,000$ | 100 000.00 |
| Deemed return on deposits | $20\,000 \times 0.0137$ | 274.00 |
| Deemed return on investments | $80\,000 \times 0.0588$ | 4 704.00 |
| Deemed return | $274 + 4\,704$ | 4 978.00 |
| Taxable base | $100\,000 - 57\,684$ | 42 316.00 |
| Share of the base in the return base | $42\,316 / 100\,000$ | 0.423 16 |
| Taxable return | $4\,978 \times 0.423\,16$ | 2 106.49 |
| Charge | $2\,106.49 \times 0.36$ | 758.34 |

The same 100 000 held entirely as investments is charged 895.74; held entirely as a deposit, 208.70.
The composition of the wealth changes the bill by a factor of four, and no trade during the year
changes it at all.

### What a profit tax would have said

Suppose the portfolio above collected 5 000 of option premium during the year and ended where it
started. A 30% tax on realised profit would charge 1 500. The wealth tax charges 758.34, and would
have charged exactly the same had the premium been zero, or ten times larger, provided the wealth on
the reference date was the same. Conversely, a portfolio that lost 20 000 over the year pays a profit
tax of nothing and a wealth tax of several hundred.

## Edge cases & pitfalls

- **Wealth exactly at the allowance is charged nothing**, and the charge grows continuously from
  there: one minor unit above the allowance produces a taxable return that rounds to nothing.
- **Wealth of nothing is charged nothing**, and the share of the base in the return base is not
  evaluated at all, so there is no division by zero to guard against downstream.
- **The reference date is not a period boundary.** An options strategy advances by cycles, and a
  cycle straddles the turn of the year. The charge has to be struck on its own date, inside whatever
  cycle contains it, and on the portfolio as it stands at that instant - including any position that
  is open. See [decision D24](../design/decisions.md).
- **An open short option is a liability on the reference date.** A portfolio whose written puts have
  moved against it is worth less than its cash balance, and is taxed on the lower figure.
- **The allowance belongs to the person, not to the portfolio.** Charging the whole allowance against
  one portfolio assumes that portfolio is all the wealth the investor has. An investor with other
  assets is charged more than the projection says.
- **The charge has to be paid out of something.** Cash that is committed as collateral against a
  written option cannot also settle a tax bill; a strategy that commits every last unit has nothing
  left to pay with.
- **Only wealth is modelled, never debt.** The published rules also impute a negative return to
  borrowings above a threshold. The strategy described by this project never borrows, so that term is
  out of scope; a projection for an investor with debts would understate their position.
- **The percentages are set per year and change every year.** Some of them are provisional when the
  year begins and are fixed only afterwards. A projection over a horizon of years is therefore using
  the current year's rules as a forecast of future ones, which is an assumption, not a fact.

## Alternatives considered

**Charge the tax as a flat percentage of wealth.** The product of the tax rate and the imputed return
on investments is about 2.1% of wealth above the allowance, and a single number would be far easier to
explain. It was rejected because it silently assumes the whole portfolio is invested, which is exactly
false for a cash-secured strategy: it would overstate the tax on the collateral by a factor of four
and hide the one lever - what the wealth is held as - that the investor actually controls.

**Charge the tax on income anyway, as a conservative estimate.** Rejected because it is not
conservative: for a high-income, low-capital strategy it charges far too much, and for a losing year
it charges nothing when something is due. Being wrong in both directions is not prudence.

## References

The authority, the value and the date each published figure was checked are recorded in
[external parameter sources](../validation/sources.md). No figure appears in this document that is
not in that table.
