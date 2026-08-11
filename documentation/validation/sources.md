# External parameter sources

One row per parameter that comes from outside this project. Append rows; do not delete them - the
history of what a rate used to be is what makes an old projection reproducible.

| Parameter | Value | Applies to | Authoritative source | Checked on | Checked by |
|---|---|---|---|---|---|
| _(none yet)_ | | | | | |

## How to add a row

1. Find the parameter on the issuing body's own site - Belastingdienst for tax, DEGIRO for fees. A
   blog post or a forum answer is not an authoritative source.
2. Record the exact value as published, including its unit and the period it applies to.
3. Hold the value as configuration data, never inside the model.
4. Add the row here with today's date.

## Conventions that are *not* external parameters

Day-count denominators (365, 252), the standard option multiplier (100) and the relationship between
continuous and effective annual rates are market conventions, not published rates. They are fixed
and are explained in [documentation/theory](../theory/). They are still verified.

## Reference valuations used as fixtures

Published option values that the pricing model is checked against. These are not parameters - they
do not change and they are not configuration - but they come from outside the project, so their
provenance is recorded here rather than assumed.

| Inputs | Published value | Source | Checked on |
|---|---|---|---|
| $S=42$, $K=40$, $r=10\%$, $q=0$, $\sigma=20\%$, $\tau=0.5$ | call 4.76, put 0.81 | Hull, *Options, Futures, and Other Derivatives*, worked example in the Black-Scholes-Merton chapter | 2026-08-11 |
| $S=100$, $K=95$, $r=10\%$, $q=5\%$, $\sigma=20\%$, $\tau=0.5$ | put 2.4648 | Haug, *The Complete Guide to Option Pricing Formulas*, Merton (1973) example | 2026-08-11 |
| $S=75$, $K=70$, $r=10\%$, cost of carry $5\%$, $\sigma=35\%$, $\tau=0.5$ | put 4.0870 | Haug, *The Complete Guide to Option Pricing Formulas*, generalised Black-Scholes-Merton example | 2026-08-11 |

Two further checks need no source at all, which is why they are the most valuable ones: put-call
parity holds exactly for every input, and an at-the-money option with no interest rate and no
dividend is worth $S\left(2\Phi(\sigma\sqrt{\tau}/2)-1\right)$, which is
$7.9655674554$ for $S=100$, $\sigma=20\%$, $\tau=1$. A fixture that agrees with a book but violates
parity is a coincidence; a model that satisfies parity and the bounds is constrained everywhere, not
just at three points.

The printed page numbers for the two textbook examples above have not been re-checked against a
physical edition; the values are corroborated by the closed-form identity and by parity. Confirm
them before citing this table anywhere outside the project.
