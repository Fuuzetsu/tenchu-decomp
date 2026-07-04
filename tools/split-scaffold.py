#!/usr/bin/env python3
"""Scaffold a jump-table (split) function's NON_MATCHING stub.

A function with a table-`switch` is split by splat into several .s pieces, and
its jump table lives in .rodata routed through the C object (see
docs/matching-cookbook.md "Split functions"). Turning that into a buildable
stub is mechanical but fiddly — every agent that hit it re-derived the same
steps by hand. This does them:

  * pull ALL the INCLUDE_ASM pieces from splat's generated stub,
  * find each jump table (via the `switchD_<addr>__switchdataD_<table>` symbol),
    read its words from the ORIGINAL image, and emit a `static const u32
    <sym>_jtbl[]` that supplies the table bytes in the stub build,
  * wrap it all as `#ifndef NON_MATCHING` (stub + jtbl) `#else` (a Ghidra-
    seeded draft) `#endif` — the XOR the convention needs,
  * insert the `.rodata` carve(s) into config/splat.main.exe.yaml.

Prerequisite: run `tools/reverse.py <Name>` first (adds the function's `c`
carve and lets splat generate the pieces). Then:

  tools/split-scaffold.py <Name>      scaffold + carve, leave ./Build green
  tools/split-scaffold.py <Name> -n   don't touch the yaml, just print the carve

Run inside the nix devShell. If <Name> turned out to have no jump table (a
single .s piece), it says so and changes nothing — reverse.py's seed is fine.
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
FILE_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
TSV = ".shake/ghidra-export/functions.tsv"
SYMBOLS = "config/symbols.main.exe.txt"
YAML = "config/splat.main.exe.yaml"
SRC = "src/main.exe"
GEN_STUB = ".shake/gen/main.exe/src/{name}.c"
GHIDRA_C = ".shake/ghidra-export/c/{addr:08x}.c"


def func_bounds(name):
    for line in open(TSV):
        p = line.rstrip("\n").split("\t")
        if len(p) == 3 and p[2] == name:
            a, s = int(p[0], 16), int(p[1])
            return a, a + s
    sys.exit(f"split-scaffold: {name} not in {TSV}")


def switchdata_addrs():
    """{switch_addr: table_vram} from the switchD_<a>__switchdataD_<t> symbols."""
    out = {}
    pat = re.compile(r"switchD_([0-9a-fA-F]+)__switchdataD_([0-9a-fA-F]+)\s*=")
    for line in open(SYMBOLS):
        m = pat.search(line)
        if m:
            out[int(m.group(1), 16)] = int(m.group(2), 16)
    return out


def read_words(vram, n):
    data = open(ORIG, "rb").read()
    off = FILE_OFF + (vram - TEXT_START)
    return [int.from_bytes(data[off + 4 * i:off + 4 * i + 4], "little")
            for i in range(n)]


def table_len(table_vram, fstart, fend, table_starts):
    """How many entries: word-aligned code pointers inside [fstart,fend), stopping
    at the next switch table's start (tables can be back-to-back in one pool)."""
    nexts = [t for t in table_starts if t > table_vram]
    hard_end = min(nexts) if nexts else TEXT_END
    n = 0
    for i in range((hard_end - table_vram) // 4):
        w = read_words(table_vram + 4 * i, 1)[0]
        if not (fstart <= w < fend):
            break
        n += 1
    return n


def stub_pieces(name):
    """The INCLUDE_ASM lines splat generated (all switch pieces), verbatim."""
    path = GEN_STUB.format(name=name)
    if not os.path.exists(path):
        sys.exit(f"split-scaffold: {path} missing — run `tools/reverse.py {name}` "
                 f"then `./Build` so splat generates the pieces, and retry.")
    lines = [l.rstrip("\n") for l in open(path) if l.startswith("INCLUDE_ASM(")]
    if len(lines) <= 1:
        sys.exit(f"split-scaffold: {name} has {len(lines)} INCLUDE_ASM piece(s) — "
                 f"not a jump-table function; reverse.py's stub is already correct.")
    return lines


def switch_addrs_of(pieces):
    """switch dispatch addresses referenced by the pieces (switchD_<addr>__…)."""
    seen = []
    for l in pieces:
        for m in re.finditer(r"switchD_([0-9a-fA-F]+)__", l):
            a = int(m.group(1), 16)
            if a not in seen:
                seen.append(a)
    return seen


def draft_seed(name, addr):
    """Preserve an existing #else draft if present; else seed from Ghidra's C."""
    cur = os.path.join(SRC, name + ".c")
    if os.path.exists(cur):
        txt = open(cur).read()
        m = re.search(r"#else[^\n]*\n(.*)#endif", txt, re.S)
        if m and m.group(1).strip():
            return m.group(1).rstrip() + "\n"
    gc = GHIDRA_C.format(addr=addr)
    body = open(gc).read().rstrip() if os.path.exists(gc) else \
        f"/* TODO: draft {name} here (Ghidra C at {gc}). */"
    return ("/* Draft — turn this into matching C, then delete the #ifndef/#else/\n"
            "   #endif guards and the _jtbl array(s) above.  Reference: */\n"
            + "\n".join("// " + l for l in body.splitlines()) + "\n")


def build_c(name, addr, pieces, tables):
    L = ['#include "common.h"', '#include "main.exe.h"', "",
         "/*", f" * {name} (0x{addr:08x}) — TODO one-line description.",
         " *",
         " * STATUS: NON_MATCHING — split (jump-table) function scaffolded by",
         " * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub",
         " * (INCLUDE_ASM pieces + the jump table(s) as static const arrays so the",
         " * .rodata carve has bytes); build the draft with `NON_MATCHING=" + name,
         " * ./Build`. On a full match, delete the guards and the _jtbl array(s).",
         " */", "", "#ifndef NON_MATCHING"]
    L += sum(([p, ""] for p in pieces), [])
    for sw, (tv, words) in tables.items():
        L.append(f"/* jump table @ 0x{tv:08x} ({len(words)} entries) — stub-only;"
                 f" the draft's compiled switch emits its own. */")
        L.append(f"static const u32 switchD_{sw:08x}_jtbl[{len(words)}] = {{")
        for i in range(0, len(words), 4):
            L.append("    " + " ".join(f"0x{w:08X}," for w in words[i:i + 4]))
        L.append("};")
        L.append("")
    L.append("#else /* NON_MATCHING */")
    L.append(draft_seed(name, addr))
    L.append("#endif /* NON_MATCHING */")
    return "\n".join(L) + "\n"


# ---- yaml carve insertion (line-preserving, mirrors reverse.py) ----------
def insert_rodata_carves(name, carves, dry):
    """carves = [(fileoff, size)]; insert `- [off, .rodata, name]` (+ trailing
    data split) into the subsegments, splitting whichever `data` entry covers it."""
    text = open(YAML).read()
    lines = text.splitlines(keepends=True)
    start = next(i for i, l in enumerate(lines) if l.strip() == "subsegments:")
    end = next(i for i in range(start + 1, len(lines))
               if re.match(r"  - \[0x[0-9A-Fa-f]+\]\s*$", lines[i]))
    row = re.compile(r"(\s*)- \[\s*(0x[0-9A-Fa-f]+)\s*,\s*([\w.]+)\s*(?:,\s*[\w.]+\s*)?\]")

    def entries():
        out = []
        for i in range(start + 1, end):
            m = row.match(lines[i])
            if m:
                out.append((i, m.group(1), int(m.group(2), 16), m.group(3)))
        return out

    printed = []
    for off, size in sorted(carves):
        ind = "      "
        printed.append(f"{ind}- [0x{off:X}, .rodata, {name}]")
        es = entries()
        # the covering `data` entry (largest start <= off that is data)
        cover = None
        for (i, ic, o, t) in es:
            nxt = next((oo for (_, _, oo, _) in es if oo > o), None)
            if o <= off and (nxt is None or off < nxt) and t == "data":
                cover = (i, ic, o, nxt)
        if cover is None:
            printed.append(f"{ind}  # (adjacent to existing rodata — place by hand)")
            continue
        i, ic, o, nxt = cover
        block = [f"{ic}- [0x{o:X}, data]\n"] if o < off else []
        block.append(f"{ic}- [0x{off:X}, .rodata, {name}]\n")
        if nxt is None or off + size < nxt:
            block.append(f"{ic}- [0x{off + size:X}, data]\n")
        if not dry:
            lines[i:i + 1] = block
            open(YAML, "w").write("".join(lines))
            # re-read since indices shifted
            text = open(YAML).read(); lines = text.splitlines(keepends=True)
            start = next(k for k, l in enumerate(lines) if l.strip() == "subsegments:")
            end = next(k for k in range(start + 1, len(lines))
                       if re.match(r"  - \[0x[0-9A-Fa-f]+\]\s*$", lines[k]))
    return printed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("-n", "--no-yaml", action="store_true",
                    help="don't edit the yaml, just print the .rodata carve")
    args = ap.parse_args()
    name = args.name

    fstart, fend = func_bounds(name)
    pieces = stub_pieces(name)
    sd = switchdata_addrs()
    table_starts = list(sd.values())

    tables = {}
    for sw in switch_addrs_of(pieces):
        if sw not in sd:
            sys.exit(f"split-scaffold: no switchdataD symbol for switchD_{sw:x} in "
                     f"{SYMBOLS} — add it (splat should have), then retry.")
        tv = sd[sw]
        n = table_len(tv, fstart, fend, table_starts)
        tables[sw] = (tv, read_words(tv, n))

    # Build the content BEFORE opening for write: draft_seed reads the current
    # file to preserve an existing #else draft, and open(...,"w") truncates.
    content = build_c(name, fstart, pieces, tables)
    open(os.path.join(SRC, name + ".c"), "w").write(content)

    carves = [(FILE_OFF + (tv - TEXT_START), len(w) * 4) for tv, w in tables.values()]
    printed = insert_rodata_carves(name, carves, args.no_yaml)

    print(f"split-scaffold: wrote {SRC}/{name}.c "
          f"({len(pieces)} INCLUDE_ASM pieces, {len(tables)} table(s))")
    for sw, (tv, w) in tables.items():
        print(f"  table switchD_{sw:08x} @ 0x{tv:08x}: {len(w)} entries "
              f"(file 0x{FILE_OFF + (tv - TEXT_START):X}, size 0x{len(w) * 4:X})")
    print("  .rodata carve" + (" (add to config/splat.main.exe.yaml):"
                               if args.no_yaml else " inserted:"))
    for l in printed:
        print("   " + l.strip())
    print(f"  next: fill in the #else draft, then `NON_MATCHING={name} ./Build` "
          f"+ tools/matchdiff.py {name}; default `./Build check` stays green now.")


if __name__ == "__main__":
    main()
