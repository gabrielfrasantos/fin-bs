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
