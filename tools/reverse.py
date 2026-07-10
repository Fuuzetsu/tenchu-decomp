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


def addr_from_symbols(name):
    """vram of `name` from config/symbols.main.exe.txt, or None."""
    if not os.path.exists(SYMBOLS):
        return None
    for line in open(SYMBOLS):
        m = re.match(rf"\s*{re.escape(name)}\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            return int(m.group(1), 16)
    return None


def from_ghidra(export, name):
    """(vram, size, c) for `name` from a Ghidra export dir.

    Resolves by the functions.tsv name first; if `name` isn't there — the very
    common case where the export still calls it `FUN_<addr>` but config/symbols
    has since given it a project name — falls back to config/symbols for the
    address, then reads size + Ghidra C by address. Removes the manual
    `--addr/--size` dance for any function already named in config/symbols."""
    tsv = os.path.join(export, "functions.tsv")
    if not os.path.exists(tsv):
        sys.exit(f"reverse: {tsv} not found (run tools/ghidra/ExportDecomp.java)")
    rows = []
    for line in open(tsv):
        parts = line.rstrip("\n").split("\t")
        if len(parts) == 3:
            rows.append((int(parts[0], 16), int(parts[1]), parts[2]))

    def by_addr(addr):
        for a, size, _ in rows:
            if a == addr:
                cpath = os.path.join(export, "c", f"{addr:08x}.c")
                c = open(cpath).read() if os.path.exists(cpath) else None
                return addr, size, c
        return None

    for addr, size, nm in rows:            # 1) the export's own name
        if nm == name:
            cpath = os.path.join(export, "c", f"{addr:08x}.c")
            c = open(cpath).read() if os.path.exists(cpath) else None
            return addr, size, c
    addr = addr_from_symbols(name)         # 2) config/symbols fallback
    if addr is not None:
        hit = by_addr(addr)
        if hit:
            return hit
        sys.exit(f"reverse: {name!r} = {addr:#x} (config/symbols) not in {tsv} "
                 f"— pass --addr/--size")
    sys.exit(f"reverse: function {name!r} not in {tsv} or config/symbols "
             f"— pass --addr/--size")


# File offset of the first function (vram 0x80016134). Everything below this is
# the leading .data run; everything above is the text run, whose `data`
# subsegments are really UN-CARVED CODE. splat's grouped linker-script generation
# needs each entry tagged with the ORDER GROUP its address belongs to — see the
# section_order comment in config/splat.main.exe.yaml.
TEXT_FOFF = 0x5934

# `- [0x123, data]` / `- [0x123, c, Name]`
LIST_SUB = re.compile(
    r"(\s*)- \[\s*(0x[0-9A-Fa-f]+)\s*,\s*([\w.]+)\s*(?:,\s*([\w.]+)\s*)?\]")
# `- { start: 0x123, type: data, linker_section_order: .text }`
DICT_SUB = re.compile(
    r"(\s*)- \{\s*start:\s*(0x[0-9A-Fa-f]+)\s*,\s*type:\s*([\w.]+)\s*"
    r"(?:,\s*name:\s*([\w.]+)\s*)?(?:,\s*linker_section_order:\s*[\w.]+\s*)?\}")


def data_sub(indent, off):
    """A `data` subsegment line, tagged into the right linker order group."""
    if off >= TEXT_FOFF:
        return (f"{indent}- {{ start: 0x{off:X}, type: data, "
                f"linker_section_order: .text }}\n")
    return f"{indent}- [0x{off:X}, data]\n"


def rodata_sub(indent, off, name):
    """A `.rodata` carve line, tagged into the right linker order group.

    A jump table belonging to an un-carved function lives inside the TEXT run, so
    such a carve must join the `.text` order group — untagged it would land in the
    (trailing) `.rodata` group and silently move. Today's six carves are all in
    the leading run; this keeps the next one honest.
    """
    group = ".data" if off < TEXT_FOFF else ".text"
    return (f"{indent}- {{ start: 0x{off:X}, type: .rodata, name: {name}, "
            f"linker_section_order: {group} }}\n")


def parse_subsegments(text):
    """Return (pre, items, post). items mixes parsed entries
    ("e", indent, off, type, name, raw) with verbatim passthrough lines
    ("raw", line) — comments and anything else hand-maintained in the
    subsegment list survive a rewrite (a dropped `.rodata` carve once broke
    the jump-table link). Both the plain list form and the dict form (which
    carries `linker_section_order`) are recognised."""
    lines = text.splitlines(keepends=True)
    start = next(i for i, l in enumerate(lines) if l.strip() == "subsegments:")
    # the segment ends at the top-level terminator `  - [0x....]` (2-space indent)
    end = next(i for i in range(start + 1, len(lines))
               if re.match(r"  - \[0x[0-9A-Fa-f]+\]\s*$", lines[i]))
    items = []
    for l in lines[start + 1:end]:
        m = LIST_SUB.match(l) or DICT_SUB.match(l)
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
        new.append(data_sub(ind, off))   # leading data (keeps original offset)
    new.append(f"{ind}- [0x{fstart:X}, c, {name}]\n")
    if fend < nxt:
        new.append(data_sub(ind, fend))
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

INCLUDE_RE = re.compile(r'^INCLUDE_ASM\("([^"]+)",\s*([^)]+)\);')


def piece_symbols(genstub):
    """The symbol of each .s piece splat generated for this function, in order."""
    out = []
    for line in open(genstub):
        m = INCLUDE_RE.match(line.strip())
        if m:
            out.append((m.group(1), m.group(2).strip()))
    return out


# A jump-table split names its extra pieces switchD_<addr>__… (switchD/caseD).
# Anything else that splits a function is just an interior SYMBOL: a Ghidra
# `__override__prt_` call-site marker, or a plain label/data symbol inside the
# body (e.g. INIT_STAGE_STATS inside StartStageSequence). Those only need every
# piece seeded; a real jump table also needs its .rodata carve.
JT_PIECE = re.compile(r"switchD_[0-9a-fA-F]+__")


def is_jump_table(pieces):
    return any(JT_PIECE.search(sym) for _, sym in pieces[1:])


def expand_stub(name, pieces):
    """Rewrite <name>.c's single INCLUDE_ASM with one line per generated piece.

    splat starts a new .s piece at every symbol inside the function's range. Any
    interior symbol therefore silently splits a perfectly ordinary function — a
    Ghidra `__override__prt_` call-SITE prototype override, or a plain label/data
    symbol in the body — and a stub carrying only the first piece fails to link:
    a branch whose target lives in piece 2 goes undefined. Emitting every piece
    restores the green stub. (A jump table also splits, but additionally needs a
    .rodata carve — that's split-scaffold.py's job, not this.)
    """
    path = os.path.join(SRC_DIR, f"{name}.c")
    src = open(path).read()
    lines = "".join(f'INCLUDE_ASM("{d}", {s});\n' for d, s in pieces)
    old = f'INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/{name}", {name});\n'
    if old not in src:
        return False
    open(path, "w").write(src.replace(old, lines, 1))
    return True


def write_src(name, ghidra_c):
    path = os.path.join(SRC_DIR, f"{name}.c")
    if os.path.exists(path):
        print(f"reverse: {path} exists — not overwriting")
        return
    body = INCLUDE_TMPL.format(name=name)
    # Difficulty + likely-relevant cookbook sections (from the function's asm
    # features) so the drafter knows what they're in for and which rules apply.
    try:
        import triage
        hit = next((x for x in triage.load()["funcs"] if x[2] == name), None)
        if hit:
            f = triage.features(hit[0], hit[1])
            sim, twin = triage.nearest_matched(hit[0], hit[1], name)
            docs = triage.docs_for(f)
            body += (f"\n// triage: {triage.bucket(triage.score(f, sim)).upper()} — "
                     f"{triage.why(f, sim, twin)}\n")
            if docs:
                body += "// likely-relevant cookbook sections:\n"
                body += "".join(f"//   - {sec}: {w}\n" for sec, w in docs)
    except Exception:
        pass
    if ghidra_c:
        # Line comments (not /* */): Ghidra's C frequently contains `/* */`, which
        # would nest and break a block comment.
        commented = "\n".join(("// " + l).rstrip() for l in ghidra_c.rstrip("\n").splitlines())
        body += ("\n// Ghidra decompilation (reference — turn this into matching C,\n"
                 "// then drop the INCLUDE_ASM above):\n//\n" + commented + "\n")
    open(path, "w").write(body)
    print(f"reverse: wrote {path}" + ("" if ghidra_c else " (no Ghidra C — stub only)"))


def m2c_ref(name):
    """m2c's C for the function (mipsel-gcc-c target), or None. m2c reconstructs
    the real control flow / register temps straight from the asm — a good
    STRUCTURAL second opinion next to Ghidra's typed-but-normalized C."""
    import glob
    pieces = sorted(glob.glob(
        f".shake/gen/main.exe/asm/nonmatchings/{name}/*.s")) or glob.glob(
        f".shake/gen/main.exe/asm/nonmatchings/*/{name}.s")
    if not pieces:
        return None
    try:
        r = subprocess.run(["m2c.py", "-t", "mipsel-gcc-c", *pieces],
                           capture_output=True, text=True)
    except FileNotFoundError:
        return None
    return r.stdout if r.returncode == 0 and r.stdout.strip() else None


def append_m2c(name):
    """Append m2c's C as a second reference comment (after Ghidra's), once."""
    path = os.path.join(SRC_DIR, f"{name}.c")
    if not os.path.exists(path):
        return
    txt = open(path).read()
    if "m2c (" in txt or "INCLUDE_ASM" not in txt:
        return
    c = m2c_ref(name)
    if not c:
        return
    commented = "\n".join(("// " + l).rstrip() for l in c.rstrip("\n").splitlines())
    open(path, "a").write(
        "\n// m2c (mipsel-gcc-c reference — cleaner control flow + register\n"
        "// temps straight from the asm; Ghidra above has the real types):\n//\n"
        + commented + "\n")
    print(f"reverse: appended m2c reference to {path}")


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
        append_m2c(name)      # .s now exists; add the m2c second-opinion comment
        return
    # A function can split into several .s pieces for two very different reasons.
    # Use the RESOLVED name (`name`), not args.name: a renamed-via-sibling target
    # is generated under its real name.
    genstub = os.path.join(".shake", "gen", "main.exe", "src", name + ".c")
    pieces = piece_symbols(genstub) if os.path.exists(genstub) else []
    # (a) An interior symbol (override marker, label, data) sitting inside the
    #     body. Not a jump table, no .rodata: seed every piece and we're green.
    if len(pieces) > 1 and not is_jump_table(pieces):
        if expand_stub(name, pieces):
            r = subprocess.run(["./Build", "check"], stdout=subprocess.DEVNULL)
            if r.returncode == 0:
                print(f"reverse: ✓ ./Build check GREEN — {name} split into "
                      f"{len(pieces)} pieces at an interior symbol "
                      f"(not a jump table); seeded all pieces.")
                append_m2c(name)
                return
    # (b) A real jump table: needs all pieces AND the table's .rodata carve —
    #     tools/split-scaffold.py does both.
    if len(pieces) > 1:
        print(f"reverse: {name} is a jump-table function ({len(pieces)} .s pieces).\n"
              f"         The stub needs all pieces + the table's .rodata carve — run\n"
              f"         `tools/split-scaffold.py {name}` to finish it (then green).")
        return
    print(f"reverse: ✗ ./Build check FAILED — the split changed the output.\n"
          f"         Likely the [start,size) boundary is off or the region has\n"
          f"         rodata splat needs told about. Revert with `git checkout {YAML} "
          f"{SRC_DIR}/{name}.c` and check the function extent.", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
    main()
