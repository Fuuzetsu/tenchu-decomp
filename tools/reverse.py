#!/usr/bin/env python3
"""Start reversing one function: split its ASM out and seed a src/ .c file.

Incremental bridge from the Ghidra project into the decomp (see
tools/ghidra/ExportDecomp.java). For a single function it:

  1. adds a `[offset, c, <name>]` subsegment to config/splat.main.exe.yaml
     (splitting the surrounding `data` block) so splat disassembles it;
  2. writes src/main.exe/<name>.c = the INCLUDE_ASM stub + Ghidra's decompiled C
     in a /* */ comment (if a Ghidra export is given), so you can turn it into
     matching C at your leisure;
  3. rebuilds and asserts `./Build check` is still byte-identical (the split must
     not change the output — it only relabels bytes as code).

Usage (inside the nix devShell):
  tools/reverse.py <name> --ghidra-export <dir>      # addr+size+C from the export
  tools/reverse.py <name> --addr 0x8001b144 --size 0x11c   # manual

The Ghidra export dir is what ExportDecomp.java wrote (functions.tsv + c/*.c).
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

VRAM = 0x80011000       # main.exe load address
FILE_OFF = 0x800        # file offset of vram base (segment start)
YAML = "config/splat.main.exe.yaml"
SYMBOLS = "config/symbols.main.exe.txt"
SRC_DIR = "src/main.exe"


def file_offset(vram):
    return FILE_OFF + (vram - VRAM)


def from_ghidra(export, name):
    """(vram, size, c) for `name` from a Ghidra export dir, or (None,None,None)."""
    tsv = os.path.join(export, "functions.tsv")
    if not os.path.exists(tsv):
        sys.exit(f"reverse: {tsv} not found (run tools/ghidra/ExportDecomp.java)")
    for line in open(tsv):
        parts = line.rstrip("\n").split("\t")
        if len(parts) == 3 and parts[2] == name:
            addr = int(parts[0], 16)
            size = int(parts[1])
            cpath = os.path.join(export, "c", f"{addr:08x}.c")
            c = open(cpath).read() if os.path.exists(cpath) else None
            return addr, size, c
    sys.exit(f"reverse: function {name!r} not in {tsv}")


def parse_subsegments(text):
    """Return (pre, items, post). items mixes parsed entries
    ("e", indent, off, type, name, raw) with verbatim passthrough lines
    ("raw", line) — comments and anything else hand-maintained in the
    subsegment list survive a rewrite (a dropped `.rodata` carve once broke
    the jump-table link)."""
    lines = text.splitlines(keepends=True)
    start = next(i for i, l in enumerate(lines) if l.strip() == "subsegments:")
    # the segment ends at the top-level terminator `  - [0x....]` (2-space indent)
    end = next(i for i in range(start + 1, len(lines))
               if re.match(r"  - \[0x[0-9A-Fa-f]+\]\s*$", lines[i]))
    items = []
    for l in lines[start + 1:end]:
        m = re.match(r"(\s*)- \[\s*(0x[0-9A-Fa-f]+)\s*,\s*([\w.]+)\s*(?:,\s*([\w.]+)\s*)?\]",
                     l)
        if m:
            items.append(("e", m.group(1), int(m.group(2), 16), m.group(3),
                          m.group(4), l))
        else:
            items.append(("raw", l))
    return lines[:start + 1], items, lines[end:]


def split_config(fstart, fend, name):
    text = open(YAML).read()
    pre, items, post = parse_subsegments(text)
    entries = [(i, it) for i, it in enumerate(items) if it[0] == "e"]
    if any(it[2] == fstart and it[3] == "c" for _, it in entries):
        print(f"reverse: 0x{fstart:x} already a `c` subsegment — leaving config")
        return
    # the entry whose range covers fstart
    offsets = [it[2] for _, it in entries] + \
        [int(re.search(r"0x[0-9A-Fa-f]+", post[0]).group(0), 16)]
    idx = max(i for i, o in enumerate(offsets[:-1]) if o <= fstart)
    item_i, (_, ind, off, typ, _nm, _raw) = entries[idx]
    nxt = offsets[idx + 1]
    if typ != "data":
        sys.exit(f"reverse: 0x{fstart:x} falls in a `{typ}` subsegment (0x{off:x}), not data")
    if not (off <= fstart < fend <= nxt):
        sys.exit(f"reverse: [0x{fstart:x},0x{fend:x}) does not fit in data [0x{off:x},0x{nxt:x})")

    new = []
    if off < fstart:
        new.append(f"{ind}- [0x{off:X}, data]\n")   # leading data (keeps original offset)
    new.append(f"{ind}- [0x{fstart:X}, c, {name}]\n")
    if fend < nxt:
        new.append(f"{ind}- [0x{fend:X}, data]\n")
    out = list(pre)
    for i, it in enumerate(items):
        if i == item_i:
            out.extend(new)
        else:
            out.append(it[-1])
    out.extend(post)
    open(YAML, "w").write("".join(out))
    print(f"reverse: split config -> [0x{fstart:X}, c, {name}]")


DEFAULT_NAME = re.compile(r"^(FUN_|func_|sub_|D_|LAB_|loc_|jtbl_)", re.IGNORECASE)


def add_symbol(name, vram):
    """Define <name> at vram in config/symbols so splat names the function it
    generates <name> (otherwise the generated INCLUDE_ASM path won't match).
    If the address already has a *default* placeholder (FUN_…), replace it with
    <name>; if it has a real name, keep that."""
    text = open(SYMBOLS).read()
    m = re.search(rf"(?mi)^([ \t]*)(\w+)([ \t]*=[ \t]*0x0*{vram:x}[ \t]*;.*)$", text)
    if m:
        existing = m.group(2)
        if existing == name:
            return name
        if DEFAULT_NAME.match(existing):
            text = text[:m.start()] + m.group(1) + name + m.group(3) + text[m.end():]
            open(SYMBOLS, "w").write(text)
            print(f"reverse: renamed placeholder {existing} -> {name} at 0x{vram:x}")
            return name
        print(f"reverse: 0x{vram:x} already named `{existing}` in {SYMBOLS} "
              f"(not a placeholder) — keeping it, ignoring `{name}`")
        return existing
    if not text.endswith("\n"):
        text += "\n"
    open(SYMBOLS, "w").write(text + f"{name} = 0x{vram:x};\n")
    print(f"reverse: added symbol {name} = 0x{vram:x}")
    return name


INCLUDE_TMPL = (
    '#include "common.h"\n#include "main.exe.h"\n\n'
    'INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/{name}", {name});\n')


def write_src(name, ghidra_c):
    path = os.path.join(SRC_DIR, f"{name}.c")
    if os.path.exists(path):
        print(f"reverse: {path} exists — not overwriting")
        return
    body = INCLUDE_TMPL.format(name=name)
    if ghidra_c:
        # Line comments (not /* */): Ghidra's C frequently contains `/* */`, which
        # would nest and break a block comment.
        commented = "\n".join(("// " + l).rstrip() for l in ghidra_c.rstrip("\n").splitlines())
        body += ("\n// Ghidra decompilation (reference — turn this into matching C,\n"
                 "// then drop the INCLUDE_ASM above):\n//\n" + commented + "\n")
    open(path, "w").write(body)
    print(f"reverse: wrote {path}" + ("" if ghidra_c else " (no Ghidra C — stub only)"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--ghidra-export", metavar="DIR")
    ap.add_argument("--addr", type=lambda s: int(s, 0))
    ap.add_argument("--size", type=lambda s: int(s, 0))
    ap.add_argument("--no-check", action="store_true", help="skip ./Build check")
    args = ap.parse_args()

    ghidra_c = None
    if args.ghidra_export:
        vram, size, ghidra_c = from_ghidra(args.ghidra_export, args.name)
        if args.addr:
            vram = args.addr
        if args.size:
            size = args.size
    elif args.addr and args.size:
        vram, size = args.addr, args.size
    else:
        sys.exit("reverse: need --ghidra-export DIR, or --addr and --size")

    fstart, fend = file_offset(vram), file_offset(vram) + size
    print(f"reverse: {args.name} @ vram 0x{vram:x} size 0x{size:x} "
          f"-> file [0x{fstart:X}, 0x{fend:X})")
    name = add_symbol(args.name, vram)      # may differ if addr already named
    split_config(fstart, fend, name)
    write_src(name, ghidra_c)

    if args.no_check:
        return
    print("reverse: rebuilding + checking byte-match…")
    # Clean first: adding/renaming a split changes the generated file set, and
    # the splat generator's incremental staleness check can miss that (it
    # compares the *set* of paths). A clean regen is cheap (~seconds) and sound.
    subprocess.run(["./Build", "clean"], stdout=subprocess.DEVNULL)
    r = subprocess.run(["./Build", "check"], stdout=subprocess.DEVNULL)
    if r.returncode == 0:
        print(f"reverse: ✓ ./Build check GREEN — {args.name} split, still byte-identical.")
        return
    # A jump-table function splits into several .s pieces; the single INCLUDE_ASM
    # seeded above only covers the first, and its table needs a .rodata carve —
    # tools/split-scaffold.py does both. Detect that case and point at it rather
    # than reporting a bare failure.
    genstub = os.path.join(".shake", "gen", "main.exe", "src", args.name + ".c")
    pieces = 0
    if os.path.exists(genstub):
        pieces = sum(1 for l in open(genstub) if l.startswith("INCLUDE_ASM("))
    if pieces > 1:
        print(f"reverse: {args.name} is a jump-table function ({pieces} .s pieces).\n"
              f"         The stub needs all pieces + the table's .rodata carve — run\n"
              f"         `tools/split-scaffold.py {args.name}` to finish it (then green).")
        return
    print(f"reverse: ✗ ./Build check FAILED — the split changed the output.\n"
          f"         Likely the [start,size) boundary is off or the region has\n"
          f"         rodata splat needs told about. Revert with `git checkout {YAML} "
          f"{SRC_DIR}/{args.name}.c` and check the function extent.", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
    main()
