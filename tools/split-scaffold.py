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
  tools/split-scaffold.py <Name> -n   dry run: touch nothing, just print the carve

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
    """(start, end) vram. Resolves renamed-via-sibling functions too.

    A function renamed in config/symbols keeps its `FUN_<addr>` name in the Ghidra
    export, so a name lookup alone misses it (reverse.py has the same problem and
    solves it by address). Fall back to the symbol's address and match the export
    row by address."""
    rows = []
    for line in open(TSV):
        p = line.rstrip("\n").split("\t")
        if len(p) == 3:
            try:
                rows.append((int(p[0], 16), int(p[1]), p[2]))
            except ValueError:
                pass
    for a, s, n in rows:
        if n == name:
            return a, a + s
    m = re.search(rf"^{re.escape(name)}\s*=\s*(0x[0-9A-Fa-f]+)\s*;",
                  open(SYMBOLS).read(), re.M)
    if m:
        addr = int(m.group(1), 16)
        for a, s, n in rows:
            if a == addr:
                return a, a + s
    sys.exit(f"split-scaffold: {name} not in {TSV} (nor resolvable via {SYMBOLS})")


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


def table_pool(name, fstart, fend, sw_addrs, sd):
    """(pool_vram, words) — the ONE contiguous .rodata run holding every jump
    table of this function.

    A compiled object has a SINGLE `.rodata` section, so we can only place it
    once. Emitting one `static const` array per table and one carve per table
    made `ld` drop the whole section at the first carve and nothing at the rest
    (ActCHASE/ActSQUAT/CameraType1 have two tables, ActENGAGE three).

    Reading the pool as one raw run also sidesteps per-table length guessing:
    ActENGAGE's middle table is 34 words, but its 34th is not a pointer into the
    function, so an entry-by-entry walk stopped at 33 and left a 4-byte hole.
    Only the LAST table's length still needs the pointer heuristic — everything
    before it is bounded by the next table's start.
    """
    starts = sorted(sd[sw] for sw in sw_addrs)
    pool_start = starts[0]
    last = starts[-1]
    pool_end = last + 4 * table_len(last, fstart, fend, list(sd.values()))

    # Another function's table inside our pool would mean we are about to swallow
    # its bytes into this object's .rodata. Refuse rather than corrupt the image.
    foreign = [t for t in sd.values()
               if pool_start < t < pool_end and t not in starts]
    if foreign:
        sys.exit(f"split-scaffold: {name}'s table pool "
                 f"[0x{pool_start:08x},0x{pool_end:08x}) contains another "
                 f"function's table at {[hex(t) for t in foreign]} — "
                 f"place these carves by hand.")
    return pool_start, read_words(pool_start, (pool_end - pool_start) // 4)


def build_c(name, addr, pieces, pool_vram, words, starts):
    tbl = ", ".join(f"0x{t:08x}" for t in starts)
    L = ['#include "common.h"', '#include "main.exe.h"', "",
         "/*", f" * {name} (0x{addr:08x}) — TODO one-line description.",
         " *",
         " * STATUS: NON_MATCHING — split (jump-table) function scaffolded by",
         " * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub",
         " * (INCLUDE_ASM pieces + the jump-table pool as one static const array so",
         " * the .rodata carve has bytes); build the draft with `NON_MATCHING=" + name,
         " * ./Build`. On a full match, delete the guards and the _jtbl array.",
         " */", "", "#ifndef NON_MATCHING"]
    L += sum(([p, ""] for p in pieces), [])
    L.append(f"/* jump-table pool @ 0x{pool_vram:08x} ({len(words)} words; tables at"
             f" {tbl}) — stub-only, one array because the object has one .rodata"
             f" section; the draft's compiled switch emits its own. */")
    L.append(f"static const u32 {name}_jtbl[{len(words)}] = {{")
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
    """carves = [(fileoff, size)]; insert the `.rodata` carve (+ trailing data
    split) into the subsegments, splitting whichever `data` entry covers it.

    Entry lines come from reverse.py's `data_sub`/`rodata_sub`, which tag each
    entry with the `linker_section_order` its address needs — splat groups the
    linker script by section, and our data/rodata runs interleave."""
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import reverse
    text = open(YAML).read()
    lines = text.splitlines(keepends=True)
    start = next(i for i, l in enumerate(lines) if l.strip() == "subsegments:")
    end = next(i for i in range(start + 1, len(lines))
               if re.match(r"  - \[0x[0-9A-Fa-f]+\]\s*$", lines[i]))

    def entries():
        out = []
        for i in range(start + 1, end):
            m = reverse.LIST_SUB.match(lines[i]) or reverse.DICT_SUB.match(lines[i])
            if m:
                out.append((i, m.group(1), int(m.group(2), 16), m.group(3)))
        return out

    printed = []
    for off, size in sorted(carves):
        ind = "      "
        printed.append(reverse.rodata_sub(ind, off, name).rstrip("\n"))
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
        block = [reverse.data_sub(ic, o)] if o < off else []
        block.append(reverse.rodata_sub(ic, off, name))
        if nxt is None or off + size < nxt:
            block.append(reverse.data_sub(ic, off + size))
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
                    help="dry run: write neither the .c nor the yaml; print the carve")
    args = ap.parse_args()
    name = args.name

    fstart, fend = func_bounds(name)
    pieces = stub_pieces(name)
    sd = switchdata_addrs()

    sw_addrs = switch_addrs_of(pieces)
    for sw in sw_addrs:
        if sw not in sd:
            sys.exit(f"split-scaffold: no switchdataD symbol for switchD_{sw:x} in "
                     f"{SYMBOLS} — add it (splat should have), then retry.")
    starts = sorted(sd[sw] for sw in sw_addrs)
    pool_vram, words = table_pool(name, fstart, fend, sw_addrs, sd)

    # Never clobber a function that already MATCHES. The stub we would write is
    # byte-identical to the original, so `./Build check` stays green and the
    # regression is invisible — the function just silently stops being matched.
    target = os.path.join(SRC, name + ".c")
    if os.path.exists(target) and not re.search(
            r"^\s*INCLUDE_ASM", open(target).read(), re.M):
        sys.exit(f"split-scaffold: {target} is already MATCHED (no INCLUDE_ASM). "
                 f"Refusing to overwrite it with a stub — there is nothing to "
                 f"scaffold. Delete the .c first if you really mean to restart.")

    # Build the content BEFORE opening for write: draft_seed reads the current
    # file to preserve an existing #else draft, and open(...,"w") truncates.
    content = build_c(name, fstart, pieces, pool_vram, words, starts)
    if args.no_yaml:
        print(f"split-scaffold: --no-yaml — not writing {target} either "
              f"({len(content.splitlines())} lines would be written).")
    else:
        open(target, "w").write(content)

    carves = [(FILE_OFF + (pool_vram - TEXT_START), len(words) * 4)]
    printed = insert_rodata_carves(name, carves, args.no_yaml)

    print(f"split-scaffold: wrote {SRC}/{name}.c "
          f"({len(pieces)} INCLUDE_ASM pieces, {len(starts)} table(s) in one pool)")
    print(f"  pool @ 0x{pool_vram:08x}: {len(words)} words "
          f"(file 0x{FILE_OFF + (pool_vram - TEXT_START):X}, "
          f"size 0x{len(words) * 4:X}); tables at "
          f"{', '.join('0x%08x' % t for t in starts)}")
    print("  .rodata carve" + (" (add to config/splat.main.exe.yaml):"
                               if args.no_yaml else " inserted:"))
    for l in printed:
        print("   " + l.strip())
    print(f"  next: fill in the #else draft, then `NON_MATCHING={name} ./Build` "
          f"+ tools/matchdiff.py {name}.")
    # Actually verify the default (stub) build is byte-identical — don't just
    # claim it. An INCLUDE_ASM piece boundary that lands right after an unfilled
    # load makes gas add a defensive nop under `.set reorder` (a silent 4-byte
    # inflation that shifts every later object), so a scaffold can look done yet
    # break the image.
    print("split-scaffold: building + checking the stub is byte-identical…")
    if (subprocess.run(["./Build"]).returncode == 0
            and subprocess.run(["./Build", "check"]).returncode == 0):
        print("split-scaffold: ✓ ./Build check GREEN — stub byte-identical.")
    else:
        sys.exit(
            "split-scaffold: ✗ stub NOT byte-identical. If a piece boundary fell "
            "right after an unfilled load, INCLUDE_ASM's `.set reorder` added a "
            "defensive nop — merge those two pieces under one "
            '`__asm__(".set noreorder\\n …\\n.set reorder")` block '
            "(see FUN_80027818.c).")


if __name__ == "__main__":
    main()
