#!/usr/bin/env python3
"""Fetch daily closes from Euronext and write them as date,close files.

Euronext's historical-price download returns up to two years of daily closes per listing on
the free tier, unadjusted (the close as it was printed, which is what a strike is set against;
dividends are modelled separately). Each file written here holds one close a trading day, ISO
dates, oldest first, in the shape `source/io/PriceCsvLoader` reads and `theory/calibration.md`
describes. A listing is named by its ISIN and market; when a company's ISIN changes (Unilever's
did in December 2025) the table below has to follow it.

    python3 -I tools/fetch_prices.py [--out data/prices] [--since 2024-10-20] [--until 2026-10-20]
                                     [TICKER ...]

With no ticker every listing in LISTINGS is fetched.
"""

import argparse
import datetime
import pathlib
import ssl
import sys
import urllib.request

LISTINGS = {
    "AALB": ("NL0000852564", "XAMS"),
    "ABN": ("NL0011540547", "XAMS"),
    "AD": ("NL0011794037", "XAMS"),
    "AF": ("FR001400J770", "XPAR"),
    "INGA": ("NL0011821202", "XAMS"),
    "MT": ("LU1598757687", "XAMS"),
    "PHIA": ("NL0000009538", "XAMS"),
    "PRX": ("NL0013654783", "XAMS"),
    "SHELL": ("GB00BP6MXD84", "XAMS"),
    "UNA": ("GB00BVZK7T90", "XAMS"),
}

ENDPOINT = "https://live.euronext.com/en/ajax/AwlHistoricalPrice/getFullDownloadAjax/{isin}-{mic}"
QUERY = "?format=csv&decimal_separator=.&date_form=d/m/Y&op=Go&adjusted=N&base100=&startdate={since}&enddate={until}"
HEADERS = {
    "X-Requested-With": "XMLHttpRequest",
    "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/129.0 Safari/537.36",
}


def download(isin, mic, since, until):
    url = ENDPOINT.format(isin=isin, mic=mic) + QUERY.format(since=since, until=until)
    request = urllib.request.Request(url, headers=HEADERS)
    context = ssl.create_default_context()
    with urllib.request.urlopen(request, context=context, timeout=60) as response:
        return response.read().decode("utf-8-sig")


def closes_in(text):
    """Return (date, close) pairs, oldest first, from Euronext's semicolon separated download."""
    lines = [line.strip() for line in text.splitlines()]
    try:
        header = next(index for index, line in enumerate(lines) if line.startswith("Date;"))
    except StopIteration:
        raise ValueError("the download holds no Date;... header")
    columns = lines[header].split(";")
    close = columns.index("Close")
    pairs = {}
    for line in lines[header + 1 :]:
        if not line:
            continue
        cells = line.split(";")
        if len(cells) <= close or not cells[0] or not cells[close]:
            continue
        day, month, year = cells[0].split("/")
        date = datetime.date(int(year), int(month), int(day))
        pairs[date] = cells[close]
    return sorted(pairs.items())


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    today = datetime.date.today()
    parser.add_argument("--out", default="data/prices", type=pathlib.Path)
    parser.add_argument("--since", default=(today - datetime.timedelta(days=2 * 365 + 1)).isoformat())
    parser.add_argument("--until", default=today.isoformat())
    parser.add_argument("tickers", nargs="*", default=sorted(LISTINGS))
    arguments = parser.parse_args(argv)
    arguments.out.mkdir(parents=True, exist_ok=True)

    failed = 0
    for ticker in arguments.tickers:
        if ticker not in LISTINGS:
            print(f"{ticker}: not a listing this tool knows", file=sys.stderr)
            failed += 1
            continue
        isin, mic = LISTINGS[ticker]
        try:
            pairs = closes_in(download(isin, mic, arguments.since, arguments.until))
        except Exception as error:  # noqa: BLE001 - every failure is reported by name and the tool goes on
            print(f"{ticker}: {error}", file=sys.stderr)
            failed += 1
            continue
        if len(pairs) < 2:
            print(f"{ticker}: the download holds {len(pairs)} closes", file=sys.stderr)
            failed += 1
            continue
        path = arguments.out / f"{ticker}.csv"
        with open(path, "w", encoding="utf-8", newline="\n") as file:
            file.write("date,close\n")
            for date, close in pairs:
                file.write(f"{date.isoformat()},{close}\n")
        print(f"{ticker}: {len(pairs)} closes from {pairs[0][0]} to {pairs[-1][0]} -> {path}")

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
