# Price histories

One file per underlying, named by its ticker as the scenario states it (`SHELL.csv`, `INGA.csv`),
in the shape `source/io/PriceCsvLoader` reads: a header `date,close`, then one close per trading
day, ISO dates, oldest first, no adjustments. The ten BENELUX files were fetched on 2026-10-20 with
`tools/fetch_prices.py` from Euronext's historical-price download, which returns the last two years
per listing on the free tier (2024-10-21 to 2026-10-19 here). `example.csv` is the small fixture the
tests use.

| Ticker | Listing | ISIN | Market |
|---|---|---|---|
| AALB | Aalberts | NL0000852564 | XAMS |
| ABN | ABN AMRO | NL0011540547 | XAMS |
| AD | Ahold Delhaize | NL0011794037 | XAMS |
| AF | Air France-KLM | FR001400J770 | XPAR (the Amsterdam line holds no history) |
| INGA | ING | NL0011821202 | XAMS |
| MT | ArcelorMittal | LU1598757687 | XAMS |
| PHIA | Philips | NL0000009538 | XAMS |
| PRX | Prosus | NL0013654783 | XAMS |
| SHELL | Shell | GB00BP6MXD84 | XAMS |
| UNA | Unilever | GB00BVZK7T90 (new ISIN since December 2025) | XAMS |

To refresh or extend: `python3 -I tools/fetch_prices.py --out data/prices` fetches every listing in
the tool's table for the last two years; name tickers to fetch a few, `--since` and `--until` to
bound the dates. A longer history from another source can replace a file as long as it keeps the
shape; the loader refuses a duplicate or reversed date, a non-positive close, or a decimal comma,
and names the line. A scenario finds the folder through its `calibration.price_histories` key,
relative to the configuration directory (`../data/prices` from `config/`).
