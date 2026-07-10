#!/usr/bin/env python3
"""Parser for Psy-Q ``MND`` debug-symbol files (PSX.SYM).

The Tenchu Japanese demo (PAPX-90029) shipped ``PSX.SYM`` inside its AFS
archive: the linker's debug output for an *earlier, lost* build of the game.
It carries the original source file names, line numbers, function prototypes,
struct/union/enum definitions with field names, and the static-vs-extern
storage class of every symbol.

The build it describes is not any shipping executable, so addresses here are
NOT usable directly against ``main.exe``; names and types are (see
``tools/symlift.py``).

Record format, after an 8-byte ``"MND\\x01" + u32`` header, is a stream of::

    u32 offset ; u8 tag ; <tag-specific payload>

Tag table (all little-endian).  Verified by parsing the whole 1.48 MB file to
EOF with zero unknown tags and reproducing Ghidra's 1097 functions exactly.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field

MAGIC = b"MND\x01"

# ---------------------------------------------------------------- COFF types
# Storage classes (C_*) as used by the Psy-Q/MIPS COFF-derived debug format.
C_AUTO = 1
C_EXT, C_STAT, C_REG, C_LABEL = 2, 3, 4, 6
C_MOS, C_ARG, C_STRTAG, C_MOU, C_UNTAG = 8, 9, 10, 11, 12
C_TPDEF, C_USTATIC, C_ENTAG, C_MOE, C_REGPARM, C_FIELD = 13, 14, 15, 16, 17, 18
C_EOS, C_FILE = 102, 103

CLASS_NAME = {
    1: "AUTO",
    2: "EXT", 3: "STAT", 4: "REG", 6: "LABEL", 8: "MOS", 9: "ARG",
    10: "STRTAG", 11: "MOU", 12: "UNTAG", 13: "TPDEF", 14: "USTATIC",
    15: "ENTAG", 16: "MOE", 17: "REGPARM", 18: "FIELD",
    102: "EOS", 103: "FILE",
}

# Base types live in the low 4 bits.
BASE_TYPE = {
    0: "void", 1: "void", 2: "char", 3: "short", 4: "int", 5: "long",
    6: "float", 7: "double", 8: "struct", 9: "union", 10: "enum",
    11: "enum-member", 12: "unsigned char", 13: "unsigned short",
    14: "unsigned int", 15: "unsigned long",
}
# Derived types: 2 bits each, up to 6 levels, starting at bit 4.
DT_NON, DT_PTR, DT_FCN, DT_ARY = 0, 1, 2, 3


def decode_type(t: int, tagname: str = "", dims: list[int] | None = None) -> str:
    """Render a COFF type word as a C declarator (abstract, no identifier)."""
    base = BASE_TYPE.get(t & 0xF, f"?{t & 0xF}")
    if (t & 0xF) in (8, 9, 10) and tagname:
        base = f"{base} {tagname}"
    derived = [(t >> (4 + 2 * i)) & 3 for i in range(6)]
    dims = list(dims or [])
    # Innermost derivation is the LAST non-zero slot; walk outward-in.
    out = base
    for d in reversed([d for d in derived if d != DT_NON]):
        if d == DT_PTR:
            out += " *"
        elif d == DT_FCN:
            out = f"{out} ()"
        elif d == DT_ARY:
            n = dims.pop(0) if dims else 0
            out = f"{out} [{n}]"
    return out


# ---------------------------------------------------------------- records
@dataclass
class Def:
    offset: int
    cls: int
    type: int
    size: int
    name: str
    dims: list[int] = field(default_factory=list)
    tag: str = ""
    file: str = ""


@dataclass
class Func:
    addr: int
    name: str
    file: str
    line: int
    fp: int = 0
    fsize: int = 0
    retreg: int = 0
    mask: int = 0
    maskoffs: int = 0
    end_line: int = 0
    end_addr: int = 0
    # parameters, in declaration order, collected from the C_ARG/C_REGPARM defs
    # the linker emits between this function's 0x8C and 0x8E records
    args: list = field(default_factory=list)
    # local variables: C_REG (offset = register number) and C_AUTO (offset =
    # signed frame offset).  The originals, names and types included.
    lvars: list = field(default_factory=list)


@dataclass
class Sym:
    addr: int
    name: str
    tag: int


class SymFile:
    def __init__(self, data: bytes):
        if data[:4] != MAGIC:
            raise ValueError(f"not a MND sym file: {data[:4]!r}")
        self.data = data
        self.syms: list[Sym] = []
        self.funcs: list[Func] = []
        self.defs: list[Def] = []
        self.files: list[tuple[int, str]] = []   # (addr, filename) from tag 0x88
        self.unknown: list[tuple[int, int]] = []
        self._parse()

    def _parse(self) -> None:
        d = self.data
        p = 8
        cur_file = ""
        cur_func: Func | None = None
        n = len(d)
        while p < n:
            off = struct.unpack_from("<I", d, p)[0]
            tag = d[p + 4]
            q = p + 5
            if tag < 0x80:                       # plain symbol
                ln = d[q]
                self.syms.append(Sym(off, d[q + 1:q + 1 + ln].decode("ascii", "replace"), tag))
                p = q + 1 + ln
            elif tag == 0x80:                    # line++
                p = q
            elif tag == 0x82:                    # line += u8
                p = q + 1
            elif tag == 0x84:                    # line += u16
                p = q + 2
            elif tag == 0x86:                    # line = u32
                p = q + 4
            elif tag == 0x88:                    # line = u32, set source file
                ln = d[q + 4]
                cur_file = d[q + 5:q + 5 + ln].decode("ascii", "replace")
                self.files.append((off, cur_file))
                p = q + 5 + ln
            elif tag == 0x8A:                    # end of line-number block
                p = q
            elif tag == 0x8C:                    # function start
                # u16 fp, u32 fsize, u16 retreg, u32 mask, i32 maskoffs, u32 line
                fp, fsize, retreg, mask, maskoffs, line = struct.unpack_from("<HIHIiI", d, q)
                r = q + 20
                fl = d[r]; fname = d[r + 1:r + 1 + fl].decode("ascii", "replace"); r += 1 + fl
                nl = d[r]; nm = d[r + 1:r + 1 + nl].decode("ascii", "replace"); r += 1 + nl
                cur_func = Func(off, nm, fname or cur_file, line, fp, fsize, retreg, mask, maskoffs)
                self.funcs.append(cur_func)
                p = r
            elif tag == 0x8E:                    # function end: u32 line
                if cur_func is not None:
                    cur_func.end_line = struct.unpack_from("<I", d, q)[0]
                    cur_func.end_addr = off
                    cur_func = None
                p = q + 4
            elif tag in (0x90, 0x92):             # block start / end: u32 line
                p = q + 4
            elif tag == 0x94:                    # def
                cls, ty = struct.unpack_from("<HH", d, q)
                size = struct.unpack_from("<I", d, q + 4)[0]
                nl = d[q + 8]
                nm = d[q + 9:q + 9 + nl].decode("ascii", "replace")
                dd = Def(off, cls, ty, size, nm, file=cur_file)
                self.defs.append(dd)
                if cur_func is not None and cls in (C_ARG, C_REGPARM):
                    cur_func.args.append(dd)
                elif cur_func is not None and cls in (C_REG, C_AUTO):
                    cur_func.lvars.append(dd)
                p = q + 9 + nl
            elif tag == 0x96:                    # def2 (with array dims + struct tag)
                cls, ty = struct.unpack_from("<HH", d, q)
                size = struct.unpack_from("<I", d, q + 4)[0]
                ndim = struct.unpack_from("<H", d, q + 8)[0]
                r = q + 10
                dims = list(struct.unpack_from(f"<{ndim}I", d, r)) if ndim else []
                r += 4 * ndim
                tl = d[r]; tg = d[r + 1:r + 1 + tl].decode("ascii", "replace"); r += 1 + tl
                nl = d[r]; nm = d[r + 1:r + 1 + nl].decode("ascii", "replace"); r += 1 + nl
                dd = Def(off, cls, ty, size, nm, dims, tg, cur_file)
                self.defs.append(dd)
                if cur_func is not None and cls in (C_ARG, C_REGPARM):
                    cur_func.args.append(dd)
                elif cur_func is not None and cls in (C_REG, C_AUTO):
                    cur_func.lvars.append(dd)
                p = r
            elif tag == 0x98:                    # overlay definition: u32 len, u32 id
                p = q + 8
            elif tag == 0x9A:                    # set overlay: id in `off`
                p = q
            else:
                self.unknown.append((p, tag))
                break
        self.end = p

    # -------------------------------------------------------- convenience
    def struct_defs(self) -> dict[str, list[Def]]:
        """Group C_MOS/C_MOU/C_MOE members by their enclosing tag name."""
        out: dict[str, list[Def]] = {}
        cur: str | None = None
        for d in self.defs:
            if d.cls in (C_STRTAG, C_UNTAG, C_ENTAG):
                cur = d.name
                out.setdefault(cur, [])
            elif d.cls == C_EOS:
                cur = None
            elif cur and d.cls in (C_MOS, C_MOU, C_MOE, C_FIELD):
                out[cur].append(d)
        return out


def load(path: str) -> SymFile:
    with open(path, "rb") as f:
        return SymFile(f.read())


if __name__ == "__main__":
    import sys, collections
    sf = load(sys.argv[1])
    print(f"parsed to 0x{sf.end:x} of 0x{len(sf.data):x} "
          f"({'CLEAN' if sf.end == len(sf.data) else 'TRUNCATED'})")
    print(f"unknown tags: {sf.unknown}")
    print(f"symbols={len(sf.syms)} funcs={len(sf.funcs)} defs={len(sf.defs)} "
          f"file-records={len(sf.files)}")
    srcs = sorted({f.file for f in sf.funcs})
    print(f"distinct source files over functions: {len(srcs)}")
    cls = collections.Counter(CLASS_NAME.get(d.cls, d.cls) for d in sf.defs)
    print("def classes:", dict(cls))
