#!/usr/bin/env python3
"""Load reviewed retail function boundaries with the repository's current names.

Ghidra's ``functions.tsv`` is the baseline boundary/size inventory, but it is a
snapshot: reviewed carve corrections and adopted names do not rewrite that
export.  Apply the small set of proven boundary corrections here, then overlay
the named ``c`` subsegments from splat's current config so every consumer sees
the repository's real source/object names and extents.
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
    0x8005FD3C: 0xFC,   # AdtVsprintf (Ghidra omitted its delay-slot restore)
}

# Ghidra split AdtVsprintf's final delay-slot instruction into a phantom
# function at 0x8005fe34.  The real following function begins one word later.
# Keep this as a full-row rewrite rather than a size override: consumers must
# not offer FUN_8005fe34 as a target or attribute its leading instruction to
# the wrong body.
ROW_OVERRIDES_BY_ADDR = {
    0x8005FE34: (0x8005FE38, 0x50, "FUN_8005fe38"),
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
            size = int(p[1])
            name = p[2]
            addr, size, name = ROW_OVERRIDES_BY_ADDR.get(
                addr, (addr, size, name)
            )
            size = SIZE_OVERRIDES_BY_ADDR.get(addr, size)
            out.append((addr, size, name))
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
