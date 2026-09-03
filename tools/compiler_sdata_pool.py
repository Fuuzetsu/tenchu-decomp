#!/usr/bin/env python3
"""Restore an anonymous compiler constant shared by a split translation unit.

The retail program was compiled a translation unit at a time, so GCC emitted one
``$LC`` aggregate template when several functions used the same initializer.
The decompilation build compiles each recovered function separately; writing the
natural initializer in every C file would therefore emit one copy per object.

This filter runs after maspsx for the few evidenced shared templates.  The owner
exports its compiler-generated label under a build-only name, while later users
refer to that name and drop their duplicate bytes.  It changes no instructions
and keeps the pool relocatable; its strict shape checks make compiler drift fail
at the generating function instead of silently changing the image.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys


class PoolError(RuntimeError):
    """The compiler output no longer has the expected initializer shape."""


@dataclass(frozen=True)
class Initializer:
    align_index: int
    end_index: int
    label: str


LABEL_RE = re.compile(r"^\s*(\$LC\d+):\s*$")
ALIGN_RE = re.compile(r"^\s*\.align\s+2\s*$")
HALF_RE = re.compile(r"^\s*\.half\s+([^\s#]+)\s*$")
SPACE_RE = re.compile(r"^\s*\.space\s+2\s*$")


def parse_int(token: str) -> int:
    try:
        return int(token, 0)
    except ValueError as error:
        raise PoolError(f"invalid .half value {token!r}") from error


def find_initializer(lines: list[str], halfwords: tuple[int, ...]) -> Initializer:
    matches: list[Initializer] = []
    for label_index, line in enumerate(lines):
        label_match = LABEL_RE.match(line)
        if label_match is None or label_index == 0:
            continue
        if not ALIGN_RE.match(lines[label_index - 1]):
            continue

        value_lines = lines[label_index + 1:label_index + 1 + len(halfwords)]
        if len(value_lines) != len(halfwords):
            continue
        value_matches = [HALF_RE.match(value_line) for value_line in value_lines]
        if any(value_match is None for value_match in value_matches):
            continue
        values = tuple(parse_int(value_match.group(1)) for value_match in value_matches)
        space_index = label_index + 1 + len(halfwords)
        if values != halfwords or space_index >= len(lines):
            continue
        if not SPACE_RE.match(lines[space_index]):
            continue
        matches.append(
            Initializer(label_index - 1, space_index + 1, label_match.group(1))
        )

    if len(matches) != 1:
        rendered = ", ".join(str(value) for value in halfwords)
        raise PoolError(
            f"expected one aligned {{{rendered}}} initializer with two pad bytes; "
            f"found {len(matches)}"
        )
    return matches[0]


def replace_label(text: str, label: str, symbol: str) -> tuple[str, int]:
    pattern = re.compile(rf"(?<![A-Za-z0-9_.$]){re.escape(label)}(?![A-Za-z0-9_.$])")
    return pattern.subn(symbol, text)


def transform(
    source: str,
    *,
    symbol: str,
    halfwords: tuple[int, ...],
    owner: bool,
) -> str:
    lines = source.splitlines(keepends=True)
    initializer = find_initializer(lines, halfwords)

    definition = "".join(lines[initializer.align_index:initializer.end_index])
    reference_count = definition.count(initializer.label)
    total_count = source.count(initializer.label)
    if reference_count != 1 or total_count <= reference_count:
        raise PoolError(
            f"{initializer.label} has {total_count - reference_count} uses outside its "
            "definition; expected at least one"
        )

    if owner:
        label_index = initializer.align_index + 1
        newline = "\r\n" if lines[label_index].endswith("\r\n") else "\n"
        lines[label_index] = (
            f".globl\t{symbol}{newline}"
            f".hidden\t{symbol}{newline}"
            f"{symbol}:{newline}"
        )
    else:
        del lines[initializer.align_index:initializer.end_index]

    output, replacements = replace_label("".join(lines), initializer.label, symbol)
    expected_replacements = total_count - 1
    if replacements != expected_replacements:
        raise PoolError(
            f"replaced {replacements} uses of {initializer.label}; "
            f"expected {expected_replacements}"
        )
    return output


def halfword_list(value: str) -> tuple[int, ...]:
    try:
        values = tuple(int(token.strip(), 0) for token in value.split(","))
    except ValueError as error:
        raise argparse.ArgumentTypeError("halfwords must be comma-separated integers") from error
    if not values:
        raise argparse.ArgumentTypeError("at least one halfword is required")
    return values


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--symbol", required=True)
    parser.add_argument("--halfwords", required=True, type=halfword_list)
    role = parser.add_mutually_exclusive_group(required=True)
    role.add_argument("--owner", action="store_true")
    role.add_argument("--user", action="store_true")
    args = parser.parse_args()

    try:
        output = transform(
            args.input.read_text(),
            symbol=args.symbol,
            halfwords=args.halfwords,
            owner=args.owner,
        )
    except PoolError as error:
        print(f"compiler_sdata_pool: {args.input}: {error}", file=sys.stderr)
        return 1
    args.output.write_text(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
