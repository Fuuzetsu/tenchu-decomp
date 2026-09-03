#!/usr/bin/env python3
"""Emit a gdb script that loads per-function source line info for a layout.

GNU ld collapses N_FUN records when it merges hundreds of `.stab` sections,
so a single merged debug ELF does not give usable source lines. But each
per-object debug object (matched C recompiled `-gstabs`, filtered by
tools/stabs_lines.py) is self-consistent and gdb reads it perfectly. So
instead of merging, this emits a gdb script that `add-symbol-file`s each
debug object at the address its function occupies in the *launched* layout
— read from that layout's ELF symbol table. VSCode sources the script and
source breakpoints bind for every matched function; INCLUDE_ASM stubs stay
instruction-level.

Addresses come from the layout ELF, so the same debug objects serve the
exact, mod, and relink images — only the resolved addresses differ.

    tools/gen_debug_gdb.py --elf main.exe.elf --debug-obj-dir .shake/main.exe-dbg \
        --out main.exe.debug.gdb
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys

try:
    from tools import source_units as SU
except ModuleNotFoundError:
    import source_units as SU  # type: ignore[no-redef]

ROOT = Path(__file__).resolve().parent.parent
MAIN_RAM_LO = 0x80010000
MAIN_RAM_HI = 0x80200000
# nm one-letter classes whose symbols carry an address we can place text at.
ADDR_CLASSES = set("AaTtDdRrBb")


def parse_nm(text: str) -> dict[str, int]:
    """name -> 32-bit address from `nm` output (addresses are sign-extended)."""
    out: dict[str, int] = {}
    for line in text.splitlines():
        parts = line.split()
        if len(parts) != 3:
            continue
        value, cls, name = parts
        if cls not in ADDR_CLASSES:
            continue
        try:
            out.setdefault(name, int(value, 16) & 0xFFFFFFFF)
        except ValueError:
            continue
    return out


def objects_for_source_roots(
    debug_obj_dir: Path, source_roots: list[Path]
) -> list[Path]:
    """Map the current C-source union to debug objects, excluding stale output."""

    relative_sources: set[Path] = set()
    for source_root in source_roots:
        if not source_root.is_dir():
            continue
        for source in source_root.rglob("*"):
            if not source.is_file() or source.suffix.lower() != ".c":
                continue
            relative = source.relative_to(source_root).with_suffix(".c")
            unit = SU.explicit_unit_for_function(relative.stem)
            if unit is not None and unit.combined and unit.stem != relative.stem:
                continue
            relative_sources.add(relative)
    return sorted(debug_obj_dir / f"{source}.o" for source in relative_sources)


def build_script(symbols: dict[str, int], debug_objs: list[Path],
                 obj_dir: str, source_root: str) -> tuple[str, int]:
    # Absolute paths: VSCode's cppdbg runs gdb with an unspecified cwd, so
    # relative object paths (and the relative source paths recorded in the
    # stabs, e.g. "src/main.exe/X.c") would not resolve. `directory` gives
    # gdb the tree to find those .c files under.
    obj_dir = str(Path(obj_dir).resolve())
    lines = [
        "set architecture mips:3000",
        f"directory {source_root}",
    ]
    count = 0
    for obj in sorted(debug_objs):
        object_stem = obj.name[:-4] if obj.name.endswith(".c.o") else obj.stem
        unit = SU.explicit_unit_for_stem(object_stem)
        function = unit.functions[0] if unit else object_stem
        address = symbols.get(function)
        if address is None or not MAIN_RAM_LO <= address < MAIN_RAM_HI:
            continue
        lines.append(f"add-symbol-file {obj_dir}/{obj.name} -s .text 0x{address:08x}")
        count += 1
    return "\n".join(lines) + "\n", count


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--debug-obj-dir", type=Path, required=True)
    parser.add_argument(
        "--debug-object",
        action="append",
        default=[],
        type=Path,
        help="current debug object to load (repeatable; otherwise scan the directory)",
    )
    parser.add_argument(
        "--source-root",
        action="append",
        default=[],
        type=Path,
        help="current C source tree used to exclude stale debug objects",
    )
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--nm", default="mipsel-unknown-linux-gnu-nm")
    args = parser.parse_args(argv)
    try:
        nm = subprocess.run(
            [args.nm, str(args.elf)], check=True, text=True,
            stdout=subprocess.PIPE,
        ).stdout
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"gen-debug-gdb: {error}", file=sys.stderr)
        return 1
    symbols = parse_nm(nm)
    if args.debug_object:
        debug_objs = sorted(args.debug_object)
    elif args.source_root:
        debug_objs = objects_for_source_roots(args.debug_obj_dir, args.source_root)
    else:
        debug_objs = sorted(args.debug_obj_dir.glob("*.c.o"))
    script, count = build_script(
        symbols, debug_objs, str(args.debug_obj_dir), str(ROOT)
    )
    args.out.write_text(script)
    print(f"gen-debug-gdb: {count} source objects -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
