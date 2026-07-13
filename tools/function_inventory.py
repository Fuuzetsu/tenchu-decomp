#!/usr/bin/env python3
"""Load retail function boundaries with the repository's current names.

Ghidra's ``functions.tsv`` remains the authoritative boundary/size inventory,
but it is a snapshot: adopting a name in the decomp does not rewrite that
export.  The named ``c`` subsegments in splat's current config are the
authoritative source/object names.  Overlay those names by address so symbol
recovery does not treat already-renamed functions as placeholders or lose
their names when resolving callees.
"""
from __future__ import annotations

import re


VB = 0x80011000
FO = 0x800

# Ghidra stops these functions at internal labels even though the splat carve
# and control flow prove that their bodies continue.  Keep the corrections in
# the shared loader so every consumer sees the real function extent even when
# it reads the live (stale) export.
SIZE_OVERRIDES_BY_ADDR = {
    0x80056F50: 0x168,  # LoadCard
    0x800593A0: 0x27C,  # FUN_800593a0
}

_C_SUBSEGMENT = re.compile(
    r"^\s*-\s*\[\s*(0x[0-9A-Fa-f]+)\s*,\s*c\s*,\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*\]"
)


def load_functions(path: str) -> list[tuple[int, int, str]]:
    """Read ``addr<TAB>size<TAB>name`` rows, ignoring comments/metadata."""
    out = []
    with open(path) as fh:
        for line in fh:
            p = line.rstrip("\n").split("\t")
            if len(p) < 3 or line.startswith("#"):
                continue
            addr = int(p[0], 16)
            size = SIZE_OVERRIDES_BY_ADDR.get(addr, int(p[1]))
            out.append((addr, size, p[2]))
    return out


def load_splat_c_names(path: str, vb: int = VB, fo: int = FO) -> dict[int, str]:
    """Return ``{vram: current_name}`` for named C subsegments.

    Restricting this to ``c`` entries is deliberate.  Named data/rodata
    entries and same-address linker aliases are not necessarily functions.
    """
    out = {}
    with open(path) as fh:
        for line in fh:
            m = _C_SUBSEGMENT.match(line)
            if not m:
                continue
            off = int(m.group(1), 16)
            if off >= fo:
                out[vb + off - fo] = m.group(2)
    return out


def overlay_current_names(
    funcs: list[tuple[int, int, str]], splat_path: str
) -> tuple[list[tuple[int, int, str]], int]:
    """Replace stale inventory names where splat has a C entry at that address.

    Boundaries and sizes always remain those from ``funcs``.  The returned
    count is the number of names that actually changed.
    """
    current = load_splat_c_names(splat_path)
    changed = 0
    out = []
    for addr, size, old in funcs:
        new = current.get(addr, old)
        changed += new != old
        out.append((addr, size, new))
    return out, changed
