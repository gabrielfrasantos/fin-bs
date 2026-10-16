# Rates and compounding

## Summary

An interest rate is a number *plus a compounding convention*; the two are inseparable. This project
holds every rate **continuously compounded**, because that is the convention Black-Scholes is
derived in, and converts to and from the **effective annual** rate at the boundary where humans and
configuration files live.

## Motivation

"4%" is not a rate. 4% compounded annually grows 1 EUR to 1.0400 after a year; 4% compounded
continuously grows it to 1.0408. The gap is small at 4% over one year, and it is not small at 20%
over thirty years:

$$
(1.20)^{30} = 237.4 \qquad \text{versus} \qquad e^{0.20 \times 30} = 403.4
$$

a factor of 1.7. A compounding tool that loses track of the convention does not produce a slightly
wrong answer; it produces a confidently wrong one. Making the convention part of what a rate *is*
means the mistake cannot survive being written down.

Black-Scholes uses the continuous convention throughout - $e^{-r\tau}$ for discounting, $e^{-q\tau}$
for the dividend yield - because the derivation lives in continuous time. Broker statements, tax
tables and the Dutch *forfaitair rendement* all use effective annual percentages. The rate concept
is the translator between the two worlds.

## Definitions & notation

| Symbol | Meaning |
|---|---|
| $r_c$ | continuously compounded annual rate |
| $r_a$ | effective annual rate (annual compounding) |
| $r_m$ | nominal annual rate compounded $m$ times per year |
| $\tau$ | year fraction (see [day-count conventions](day-count-conventions.md)) |

All rates are **per annum**. There is no monthly rate in this model; a period rate is obtained by
applying an annual rate over a year fraction.

## Derivation / Rules

### Growth and discount factors

$$
\mathrm{Growth}(\tau) = e^{r_c \tau}, \qquad
\mathrm{Discount}(\tau) = e^{-r_c \tau}
$$

These are exact inverses for every $\tau$, which is what makes the continuous convention pleasant:

$$
\mathrm{Growth}(\tau_1)\,\mathrm{Growth}(\tau_2) = \mathrm{Growth}(\tau_1 + \tau_2)
$$

with no re-derivation, so a projection can advance in arbitrary, irregular steps and still compose
exactly.

### Conversions

Equating the one-year growth factors,

$$
e^{r_c} = 1 + r_a
\implies
\boxed{r_c = \ln(1 + r_a)} \qquad \boxed{r_a = e^{r_c} - 1}
$$

and for a nominal rate compounded $m$ times per year,

$$
\left(1 + \frac{r_m}{m}\right)^{m} = e^{r_c}
\implies
r_c = m \ln\!\left(1 + \frac{r_m}{m}\right),
\qquad
\lim_{m \to \infty} \left(1 + \frac{r_m}{m}\right)^{m} = e^{r_m}
$$

The limit is the definition of continuous compounding: it is the $m \to \infty$ end of the same
family, not a different kind of object.

### Ordering of the conventions

For $r > 0$, $\ln(1+r) < r < e^{r} - 1$, so

$$
r_c < r_a \quad \text{whenever both describe the same growth}
$$

The continuously compounded number is always the *smaller* of the two for the same actual return. A
quick sanity check when reading a configuration file: if the continuous figure is the larger one,
the conversion has been run the wrong way.

The expansion $r_c = r_a - \tfrac{1}{2}r_a^2 + O(r_a^3)$ shows the gap is quadratic in the rate -
negligible at 2%, close to 1.8 percentage points at 20%.

## Worked example

| $r_a$ | $r_c = \ln(1+r_a)$ | 1-year factor | 30-year factor |
|---|---|---|---|
| 2% | 0.019803 | 1.0200 | 1.8114 |
| 4% | 0.039221 | 1.0400 | 3.2434 |
| 20% | 0.182322 | 1.2000 | 237.38 |

Both factor columns are computed from $r_c$: by construction $e^{30 r_c} = (1 + r_a)^{30}$. The
point of the table is that the *factor* is convention-free - only the rate that labels it is not.

Discounting a EUR 100 strike due in 45 days at $r_a = 4\%$ under ACT/365F:

$$
\tau = 45/365 = 0.123288, \quad r_c = 0.039221, \quad
100\,e^{-r_c \tau} = 100 \times 0.995176 = 99.5176
$$

## Edge cases & pitfalls

- **$r_a \le -100\%$** is meaningless: $1 + r_a \le 0$ and the logarithm is undefined. It is
  rejected. Negative rates above that bound are legitimate and fully supported - the euro area had
  them for years, and a negative rate is exactly the case where a tool that quietly clamps to zero
  misprices every put.
- **A zero rate is the identity**: growth and discount factors are both 1, with no special case
  anywhere.
- **A rate is not a return.** A rate describes a risk-free discounting convention. The drift
  estimated from a price series is a different quantity with a different meaning, even though both
  are "per annum" numbers; they are kept as separate concepts so they cannot be swapped.
- **Do not annualise by multiplying.** A 1% monthly rate is $e^{12 \ln 1.01} - 1 = 12.68\%$ per year
  effective, not 12%.
- **Match the day count.** $e^{-r\tau}$ is only meaningful if $\tau$ uses the convention the rate was
  quoted in. ACT/365F is the default pairing here.

## Alternatives considered

| Option | Verdict |
|---|---|
| Hold the effective annual rate instead | Rejected: every pricing step would pay a logarithm to convert, and the exact composition over irregular steps is lost |
| A bare decimal for rates | Rejected: nothing then prevents an effective annual figure being used where a continuous one is meant |
| Carry a label saying which convention a rate is in | Rejected: the label would have to be inspected at every use; one canonical form with named conversions gives the same safety for free |

## Verification

Checked: the round trip $r_a \to r_c \to r_a$; the inverse relationship between growth and discount
factors; the composition property $\mathrm{Growth}(\tau_1 + \tau_2) =
\mathrm{Growth}(\tau_1)\mathrm{Growth}(\tau_2)$; the ordering $r_c < r_a$ for positive rates; the
table values above; and the refusal of an effective annual rate at or below $-100\%$.

## References

- J. Hull, *Options, Futures, and Other Derivatives*, chapter 4 - interest rates and compounding
  frequencies.
- F. Black and M. Scholes, *The Pricing of Options and Corporate Liabilities*,
  Journal of Political Economy 81(3), 1973.
