#!/usr/bin/env python3
"""Show fixed retail symbols that could contain a queried data address.

Matching a scalar extern sometimes collapses ``lui`` and ``lw`` into one hard
register, while the target retains a separate base register because the source
accessed a nonzero field of an enclosing global struct. This command turns the
otherwise manual symbol-file scan into a bounded query::

    tools/symnear.py CHOSEN_CHARACTER CHOSEN_LANGUAGE
    tools/symnear.py 0x80089f04 --before 0x40 --after 0x10

The offsets are candidates, not proof of containment: confirm the field layout
from known types, PSX.SYM, and the target access width.
"""
from __future__ import annotations

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_SYMBOLS = os.path.join(ROOT, "config/symbols.main.exe.txt")
SYMBOL = re.compile(
    r"^\s*([A-Za-z_.$][A-Za-z0-9_.$]*)\s*=\s*(0x[0-9a-fA-F]+|[0-9]+)\s*;"
)


def load_symbols(path: str) -> list[tuple[int, str]]:
    rows = []
    with open(path) as stream:
        for line in stream:
            match = SYMBOL.match(line)
            if match:
                rows.append((int(match.group(2), 0), match.group(1)))
    return sorted(rows)


def resolve_query(query: str, symbols: list[tuple[int, str]]) -> int:
    try:
        return int(query, 0)
    except ValueError:
        matches = [address for address, name in symbols if name == query]
        if not matches:
            raise ValueError(f"unknown symbol: {query}")
        if len(set(matches)) != 1:
            raise ValueError(f"ambiguous symbol: {query}")
        return matches[0]


def nearby(symbols: list[tuple[int, str]], address: int,
           before: int = 0x80, after: int = 0) -> list[tuple[int, str, int]]:
    return [
        (candidate, name, address - candidate)
        for candidate, name in symbols
        if address - before <= candidate <= address + after
    ]


def common_bases(symbols: list[tuple[int, str]], addresses: list[int],
                 before: int = 0x80) -> list[tuple[int, str, list[int]]]:
    """Symbols lying before every query, with each query's field offset."""
    if not addresses:
        return []
    candidates = None
    for address in addresses:
        current = {
            (candidate, name)
            for candidate, name in symbols
            if address - before <= candidate <= address
        }
        candidates = current if candidates is None else candidates & current
    return [
        (candidate, name, [address - candidate for address in addresses])
        for candidate, name in sorted(candidates or set())
    ]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("query", nargs="+", help="one or more symbol names/addresses")
    parser.add_argument("--before", type=lambda value: int(value, 0), default=0x80)
    parser.add_argument("--after", type=lambda value: int(value, 0), default=0)
    parser.add_argument("--symbols", default=DEFAULT_SYMBOLS)
    args = parser.parse_args()

    symbols = load_symbols(args.symbols)
    try:
        addresses = [resolve_query(query, symbols) for query in args.query]
    except ValueError as error:
        sys.exit(f"symnear: {error}")
    for index, (query, address) in enumerate(zip(args.query, addresses)):
        if index:
            print()
        print(f"query {address:#010x} ({query})")
        for candidate, name, offset in nearby(
                symbols, address, args.before, args.after):
            relation = f"+{offset:#x}" if offset >= 0 else f"-{abs(offset):#x}"
            print(f"  {candidate:#010x}  {name:48} query = symbol {relation}")
    if len(addresses) > 1:
        print("\ncommon candidate bases:")
        rows = common_bases(symbols, addresses, args.before)
        if not rows:
            print("  (none in window)")
        for candidate, name, offsets in rows:
            joined = ", ".join(f"+{offset:#x}" for offset in offsets)
            print(f"  {candidate:#010x}  {name:48} offsets {joined}")


if __name__ == "__main__":
    main()
