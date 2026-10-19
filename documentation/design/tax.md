# Tax contracts

## Summary

The tax layer answers one question - what does holding this portfolio cost in tax this year - and it
answers it from parameters that live outside the model, versioned by the year they apply to. It
computes an annual charge on wealth measured on a single reference date, and it computes what a
dividend leaves behind after withholding and after the credit that withholding earns. It knows
nothing about premiums, gains or losses, because the tax it models does not.

## Motivation

Two properties dominate the design, and both are about being wrong in a way that is visible rather
than plausible.

1. **No rate is written into the model.** The rules of this jurisdiction are being actively reformed;
   the imputed returns are re-set every year and some are provisional when the year starts. A set of
   rules is therefore a *document*, named for the year it governs, and the model is the same code for
   every year. A projection that cites no year is not a projection.
2. **The charge is a function of holdings, never of income.** The type the model is given describes
   what is *held* and in what form. There is no way to hand it a premium, a gain or a loss, so the
   most damaging mistake available in this project - charging a wealth tax as though it were a tax on
   profit - cannot be made by writing the calling code wrongly. It could only be made by changing what
   the layer accepts. See [decision D26](decisions.md).

## Definitions & notation

| Quantity | Unit | Meaning |
|---|---|---|
| Tax year | a year | the year whose published rules a parameter set states |
| Allowance | minor units | wealth that is free of tax |
| Imputed return on deposits | basis points per year | what the rules assume a bank balance earned |
| Imputed return on investments | basis points per year | what the rules assume everything else earned |
| Tax rate | basis points | charged on the taxable return |
| Withholding rate | basis points | deducted from a dividend at the source |

**Wealth** is stated as two amounts - deposits and investments - because the rules impute a different
return to each and nothing else about an asset matters. Both are non-negative and in one currency.

A **dividend** is a gross amount together with the rate that was actually withheld from it. The rate
travels with the payment rather than with the rules, because a foreign payer withholds at its own
rate, and the difference between that rate and the creditable one is the whole point.

## Derivation / Rules

The arithmetic is in [wealth taxation of an investment portfolio](../theory/box3-taxation.md).

### What the caller must supply

| Requirement | Why it is a requirement |
|---|---|
| A tax year, and rates that are fractions of the whole amount | a set of rules that names no year cannot be checked against a source |
| An allowance that is not negative | a negative allowance is not an exemption |
| Wealth that is not negative, in the currency of the rules | rules published in one currency cannot charge wealth held in another |
| A gross dividend that is not negative | a negative dividend is a defect in the caller |
| A non-negative charge to credit withholding against | the credit is capped at the charge, and a negative cap is meaningless |

As everywhere else in this project, a parameter set **read from a document** is external input: a
missing key, a value of the wrong kind, a negative allowance or a rate beyond the whole amount is
*reported*, naming the key that was refused, and no parameter set comes into existence. A parameter
set **constructed in code** that is malformed is a programming error, and the model refuses to be
built from it. See [decision D17](decisions.md).

### What the layer guarantees

- **The charge depends only on what is held.** The same wealth is charged the same, whatever produced
  it. There is no input by which income could influence the answer.
- **Composition is visible.** The deemed return of a mixture is exactly the sum of the deemed returns
  of its parts, so a report can attribute the charge to the collateral and to the shares separately.
- **The charge is monotone in wealth.** More wealth is never charged less tax.
- **The allowance is a hard floor.** Wealth at or below the allowance is charged nothing, exactly, and
  the charge grows continuously from zero above it.
- **Withholding, credit and leakage add up.** What was withheld is what was credited plus what leaked;
  what was withheld plus what was received is the gross dividend.
- **A credit is never a repayment.** The credit is capped at the annual charge, so a portfolio that
  owes no tax recovers nothing.
- **Every amount is a whole number of minor units**, computed from whole minor units and whole basis
  points, with one exception stated below.

### The one derived proportion

The share of the taxable base in the return base is not a published rate and cannot be a whole number
of basis points: it is a ratio of two amounts and it is different for every portfolio. It is therefore
carried as a decimal, and the taxable return is rounded to minor units once, where the proportion is
applied. This is the single place in the tax layer where an amount passes through a decimal, and it is
[decision D27](decisions.md).

### What cannot be expressed

- **A tax on income.** There is no input for one.
- **A debt as its own class.** The strategy this project describes never borrows on purpose, and the
  published treatment of borrowings is not modelled ([decision D29](decisions.md)). A balance the
  wealth tax itself drove below zero ([decision D50](decisions.md)) is netted against the investment
  class by the caller before the layer is asked; wealth is non-negative by construction.
- **Two tax years at once.** A model applies one document. Projecting across a change of rules means
  applying two models, each to its own reference date.
- **A charge in a currency the rules were not written in.** Refused at the point of use.

### Where the charge is struck in a projection

A projection advances by cycles, not by calendar periods, so an annual charge cannot fall on a row
boundary ([decision D24](decisions.md)). A reference date is therefore part of the script
([decision D28](decisions.md)): the period containing it says when it falls and what the underlying was
worth then. The projection values the portfolio at that instant - including any position still open -
charges the tax, pays it out of uncommitted cash and records it on that date. A reference date that a
projection reaches with no position open is still struck; a projection that contains no reference date
is charged nothing.

## Worked example

The worked example of the charge is in the
[theory note](../theory/box3-taxation.md); it is the example the tests assert.

A dividend example, with a creditable rate of 15%: a domestic payment of 1 000 has 150 withheld, 850
is received, 150 is credited and nothing leaks. A foreign payment of 1 000 withheld at 26.38% has
263.80 withheld, 736.20 is received, 150 is credited and 113.80 leaks. Both payments credit the same
amount against the annual charge; the foreign one is 113.80 worse off, and no year in which it is
held will ever recover it.

## Edge cases & pitfalls

- **Rounding is per category, then summed.** The deemed return of deposits and of investments are each
  rounded to minor units before being added, so the total is reproducible and attributable. Rounding
  the sum instead would differ by at most one minor unit and could not be attributed.
- **A charge just above the allowance is nil.** Wealth one minor unit above the allowance produces a
  taxable return that rounds to nothing. This is correct, and it is the boundary the tests pin.
- **The charge is paid from uncommitted cash, or borrowed.** A projection whose free cash cannot
  meet the charge pays it anyway and carries the shortfall as a balance below zero, repaid from the
  next receipts at the broker's debit rate ([decision D50](decisions.md)). The cash buffer in the
  reinvestment policy is the lever that keeps that rare.
- **A reference date must lie inside the cycle that carries it.** It is refused otherwise, because a
  date outside the cycle would be valued against the wrong holdings.
- **Wealth is split by where it sits, not by what it is for.** Cash held as collateral against a
  written put is still a bank balance and is imputed the deposit return. An investor whose broker does
  not hold cash at a bank should state it as an investment instead.

## Alternatives considered

**Compute the charge inside the strategy engine.** Rejected: the engine would then have to know a tax
year, and the tax would be untestable except through a projection. Keeping the charge a pure function
of wealth lets it be checked against a worked example on its own, and lets the same engine be run
under a different jurisdiction by supplying a different model.

**Let the model classify a portfolio into deposits and investments itself.** Rejected: the
classification depends on the broker's legal structure, not on the strategy, and it is the one part of
the calculation the investor may need to override. The split is an input.

## See also

- [Wealth taxation of an investment portfolio](../theory/box3-taxation.md) - the arithmetic and why
  it is on assets.
- [External parameter sources](../validation/sources.md) - every published figure, with its authority
  and the date it was checked.
- [Strategy contracts](strategy.md) - where the charge is struck in a projection.
- [Decision log](decisions.md) - D26 to D29.
