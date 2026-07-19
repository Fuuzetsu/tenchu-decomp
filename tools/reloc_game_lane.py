#!/usr/bin/env python3
"""Generate the retail-exact, linker-owned game-code scripts.

The ordinary Splat linker script lays every input sequentially, but the
separate symbol script assigns all discovered addresses absolutely.  Those
assignments overwrite the symbols from the relocatable game objects.  This
tool creates an opt-in pair of scripts that instead:

* removes every absolute symbol in the retail game-code range; and
* emits a section-relative public anchor immediately before each artificial
  one-function game input.

The anchors are required even when an object already exports its function:
several original functions were ``static`` and remain local symbols after the
decompilation split placed their callers in separate objects.

This is deliberately only the first relocatable-build gate.  It leaves the
stock SDK byte carves, PS-EXE header, and BSS layout unchanged, so the alternate
link must remain retail-sized until those inputs become relocation-aware.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import tempfile


GAME_TEXT_START = 0x80016134
GAME_TEXT_END = 0x800601D4
EXPECTED_GAME_INPUTS = 555

TEXT_START_MARKER = "main_exe_TEXT_START = .;"
FIRST_SDK_OWNER = "/data/4F9D4.data.s.o(.data);"

ABSOLUTE_SYMBOL_RE = re.compile(
    r"^\s*([A-Za-z_.$][A-Za-z0-9_.$]*)\s*=\s*"
    r"(0[xX][0-9A-Fa-f]+);\s*(?:[#].*)?$"
)
GAME_INPUT_RE = re.compile(
    r"^(?P<indent>\s*)\S*/main\.exe/"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)\.c\.o\(\.text\);\s*$"
)


class LaneError(RuntimeError):
    """A generated-script invariant failed."""


def filter_symbols(
    source: str,
    *,
    start: int = GAME_TEXT_START,
    end: int = GAME_TEXT_END,
) -> tuple[str, dict[str, int]]:
    """Remove absolute assignments in ``[start, end)``.

    Non-absolute expressions and formatting outside the removed lines are
    preserved byte-for-byte.
    """

    removed: dict[str, int] = {}
    output: list[str] = []
    for line in source.splitlines(keepends=True):
        match = ABSOLUTE_SYMBOL_RE.fullmatch(line.rstrip("\r\n"))
        if match is not None:
            name, raw_address = match.groups()
            address = int(raw_address, 16)
            if start <= address < end:
                if name in removed:
                    raise LaneError(f"duplicate removed symbol {name}")
                removed[name] = address
                continue
        output.append(line)

    if not removed:
        raise LaneError(
            f"symbol script contained no assignments in 0x{start:08x}..0x{end:08x}"
        )
    return "".join(output), removed


def rewrite_linker(
    source: str,
    *,
    expected_inputs: int = EXPECTED_GAME_INPUTS,
) -> tuple[str, list[str]]:
    """Insert ``name = .`` before each game-code input in link order."""

    output: list[str] = []
    anchors: list[str] = []
    in_game_text = False
    saw_text_start = False
    saw_sdk_owner = False

    for line in source.splitlines(keepends=True):
        if TEXT_START_MARKER in line:
            if saw_text_start:
                raise LaneError(f"duplicate linker marker: {TEXT_START_MARKER}")
            saw_text_start = True
            in_game_text = True

        if in_game_text and FIRST_SDK_OWNER in line:
            saw_sdk_owner = True
            in_game_text = False

        if in_game_text:
            match = GAME_INPUT_RE.fullmatch(line.rstrip("\r\n"))
            if match is not None:
                name = match.group("name")
                if name in anchors:
                    raise LaneError(f"duplicate game input {name}")
                newline = "\r\n" if line.endswith("\r\n") else "\n"
                output.append(f"{match.group('indent')}{name} = .;{newline}")
                anchors.append(name)

        output.append(line)

    if not saw_text_start:
        raise LaneError(f"missing linker marker: {TEXT_START_MARKER}")
    if not saw_sdk_owner:
        raise LaneError(f"missing first SDK owner: {FIRST_SDK_OWNER}")
    if len(anchors) != expected_inputs:
        raise LaneError(
            f"expected {expected_inputs} game inputs before {FIRST_SDK_OWNER}, "
            f"found {len(anchors)}"
        )
    return "".join(output), anchors


def atomic_write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(descriptor, "w", newline="") as stream:
            stream.write(content)
        os.replace(temporary, path)
    except BaseException:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass
        raise


def generate(
    linker_input: Path,
    symbols_input: Path,
    linker_output: Path,
    symbols_output: Path,
    *,
    expected_inputs: int = EXPECTED_GAME_INPUTS,
) -> tuple[int, int]:
    filtered_symbols, removed = filter_symbols(symbols_input.read_text())
    rewritten_linker, anchors = rewrite_linker(
        linker_input.read_text(), expected_inputs=expected_inputs
    )

    missing = sorted(set(anchors) - set(removed))
    if missing:
        preview = ", ".join(missing[:8])
        suffix = " ..." if len(missing) > 8 else ""
        raise LaneError(
            "game inputs lack retail-range absolute assignments: " + preview + suffix
        )

    atomic_write(linker_output, rewritten_linker)
    atomic_write(symbols_output, filtered_symbols)
    return len(anchors), len(removed)


def arguments(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--linker-in", type=Path, required=True)
    parser.add_argument("--symbols-in", type=Path, required=True)
    parser.add_argument("--linker-out", type=Path, required=True)
    parser.add_argument("--symbols-out", type=Path, required=True)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = arguments(argv)
    try:
        anchors, removed = generate(
            args.linker_in,
            args.symbols_in,
            args.linker_out,
            args.symbols_out,
        )
    except (LaneError, OSError) as error:
        print(f"reloc-game lane: {error}")
        return 1
    print(
        f"reloc-game lane: anchored {anchors} game inputs; "
        f"removed {removed} absolute game-range symbols"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
