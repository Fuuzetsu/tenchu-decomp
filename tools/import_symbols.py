#!/usr/bin/env python3
"""Adopt Ghidra's symbol names across the decomp (functions + data globals).

Renames symbols to the names from the Ghidra export (tools/ghidra/
ExportSymbolsTypes.java -> symbols.tsv), everywhere they appear:
config/symbols.main.exe.txt, config/splat.main.exe.yaml, src/main.exe/*.c
(contents + filenames), and src/main.exe/main.exe.h. Since only *names* change
(addresses are identical), `./Build check` must stay byte-identical.

Only renames game-RAM symbols (>= 0x80010000) — leaves PSX hardware/BIOS names
alone. Skips renames that would collide with an existing name, and de-dupes
Ghidra names that map to multiple addresses.

Usage (in the nix devShell):  tools/import_symbols.py [--ghidra-export DIR]
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)
SYMBOLS = "config/symbols.main.exe.txt"
YAML = "config/splat.main.exe.yaml"
SRC_DIR = "src/main.exe"
# gp-extern lists reference symbols as quoted strings; a gp-small rename must
# update them too or maspsx stops gp-addressing the symbol (byte-break).
GPFILES = ["shake/src/Build.hs", "tools/permute.py"]
GAME_LO = 0x80010000

IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
PLACEHOLDER = re.compile(r"^(FUN_|LAB_|sub_|loc_|SUB_|D_|DAT_|switchD_|caseD_|jtbl_|s_|u_stack)",
                         re.IGNORECASE)


def sanitize(name):
    n = re.sub(r"[^A-Za-z0-9_]", "_", name)
    if n and n[0].isdigit():
        n = "_" + n
    return n


def load_ghidra(export):
    """{addr: name}; FUNCTION wins over DATA; de-dupe names across addresses."""
    tsv = os.path.join(export, "symbols.tsv")
    if not os.path.exists(tsv):
        sys.exit(f"import_symbols: {tsv} not found (run ExportSymbolsTypes.java)")
    by_addr, prio = {}, {}
    for line in open(tsv):
        p = line.rstrip("\n").split("\t")
        if len(p) != 4:
            continue
        addr, kind, _size, name = int(p[0], 16), p[1], p[2], sanitize(p[3])
        if addr < GAME_LO or not IDENT.match(name) or PLACEHOLDER.match(name):
            continue
        rank = 1 if kind == "FUNCTION" else 0
        if addr not in by_addr or rank > prio[addr]:
            by_addr[addr], prio[addr] = name, rank
    # de-dupe: a name may only belong to one address
    seen = {}
    for addr in sorted(by_addr):
        nm = by_addr[addr]
        if nm in seen:
            by_addr[addr] = f"{nm}_{addr:08x}"
        seen[by_addr[addr]] = addr
    return by_addr


def load_renames_tsv(path):
    """{addr: name} from `addr<TAB>oldname<TAB>newname<TAB>...` (tools/symmatch.py,
    tools/xbuildnames.py). Only the first and third columns are read."""
    by_addr = {}
    for line in open(path):
        p = line.rstrip("\n").split("\t")
        if len(p) < 3 or line.startswith("#"):
            continue
        addr, name = int(p[0], 16), sanitize(p[2])
        if addr < GAME_LO or not IDENT.match(name) or PLACEHOLDER.match(name):
            continue
        by_addr[addr] = name
    return by_addr


def load_current():
    """[(name, addr)] and the set of all current names, from config/symbols."""
    cur, names = [], set()
    for line in open(SYMBOLS):
        m = re.match(r"\s*([A-Za-z_]\w*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            cur.append((m.group(1), int(m.group(2), 16)))
            names.add(m.group(1))
    return cur, names


def build_additions(gh, cur, names):
    """Addresses the rename table names but config/symbols has never heard of.

    Data globals recovered by tools/datamatch.py are usually like this: splat
    auto-labels them `D_800BC108` wherever asm references them, and that name is
    all `src/` ever sees.  Naming one means inserting the definition *and*
    rewriting the auto-label everywhere, so return it as a rename of the splat
    placeholder plus a new `NAME = 0xADDR;` line.
    """
    have = {addr for _, addr in cur}
    adds, renames = [], {}
    for addr, new in sorted(gh.items()):
        if addr in have or new in names:
            continue
        adds.append((new, addr))
        renames[f"D_{addr:08X}"] = new
    return adds, renames


def insert_symbols(adds):
    """Append `NAME = 0xADDR;` lines. config/symbols has NO comment syntax."""
    if not adds:
        return
    with open(SYMBOLS, "a") as f:
        for name, addr in adds:
            f.write(f"{name} = 0x{addr:08x};\n")
    print(f"import_symbols: defined {len(adds)} new symbols in {SYMBOLS}.")


def build_renames(gh, cur, names):
    renames = {}
    targets = set()
    for name, addr in cur:
        new = gh.get(addr)
        if not new or new == name:
            continue
        if PLACEHOLDER.match(new):
            continue          # never downgrade a real repo name to a Ghidra placeholder
        if new in names or new in targets:
            print(f"  skip {name} -> {new} (name already in use)")
            continue
        renames[name] = new
        targets.add(new)
    return renames


def apply_renames(renames):
    if not renames:
        print("import_symbols: nothing to rename.")
        return
    alt = "|".join(re.escape(k) for k in sorted(renames, key=len, reverse=True))
    rep = lambda m, g: renames[m.group(g)]

    # config/symbols: only the LHS of `NAME = 0x...;` (a bare token could be a
    # common word like `start` that clashes elsewhere).
    sym = re.compile(rf"(?m)^([ \t]*)({alt})([ \t]*=)")
    s = open(SYMBOLS).read()
    open(SYMBOLS, "w").write(sym.sub(lambda m: m.group(1) + rep(m, 2) + m.group(3), s))

    # splat yaml, two subsegment spellings:
    #   list form `[off, c|asm|hasm, NAME]`, and
    #   dict form `{ start: off, type: .rodata, name: NAME, ... }` -- used by the
    #   per-function .rodata carves.  A function's .rodata subsegment must keep
    #   the function's name or splat emits an object the linker never sees.
    y = open(YAML).read()
    yml = re.compile(rf"(\[[^,\]]+,[ \t]*(?:c|asm|hasm)[ \t]*,[ \t]*)({alt})([ \t]*\])")
    y = yml.sub(lambda m: m.group(1) + rep(m, 2) + m.group(3), y)
    ydict = re.compile(rf"(\{{[^}}]*?\bname:[ \t]*)({alt})\b")
    y = ydict.sub(lambda m: m.group(1) + rep(m, 2), y)
    open(YAML, "w").write(y)

    # C sources/headers: whole-word (identifier) references.
    ident = re.compile(rf"\b({alt})\b")
    srcs = [os.path.join(SRC_DIR, f) for f in os.listdir(SRC_DIR)
            if f.endswith((".c", ".h"))]
    for f in srcs:
        c = open(f).read()
        open(f, "w").write(ident.sub(lambda m: rep(m, 1), c))

    # gp-extern lists (Build.hs `syms`, permute.py GP_EXTERNS) name symbols as
    # quoted strings — rewrite the quoted form only, so a renamed gp-small keeps
    # its --gp-extern flag and maspsx stays byte-identical.
    quoted = re.compile(rf'"({alt})"')
    for f in GPFILES:
        if os.path.exists(f):
            t = open(f).read()
            open(f, "w").write(quoted.sub(lambda m: '"' + rep(m, 1) + '"', t))

    # rename src/*.c files whose basename is a renamed function
    nfiles = 0
    for f in os.listdir(SRC_DIR):
        base, ext = os.path.splitext(f)
        if ext == ".c" and base in renames:
            os.rename(os.path.join(SRC_DIR, f),
                      os.path.join(SRC_DIR, renames[base] + ".c"))
            nfiles += 1
    print(f"import_symbols: renamed {len(renames)} symbols "
          f"(symbols/yaml/{len(srcs)} src files, {nfiles} file renames).")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ghidra-export", default=".shake/ghidra-export")
    ap.add_argument("--renames", metavar="TSV",
                    help="adopt names from a symmatch/xbuildnames TSV instead of Ghidra")
    ap.add_argument("--no-check", action="store_true")
    args = ap.parse_args()

    gh = load_renames_tsv(args.renames) if args.renames else load_ghidra(args.ghidra_export)
    cur, names = load_current()
    renames = build_renames(gh, cur, names)
    adds = []
    if args.renames:
        adds, extra = build_additions(gh, cur, names)
        renames.update(extra)
    print(f"import_symbols: {len(renames)} symbols to adopt Ghidra names for "
          f"({sum(1 for k in renames if k.startswith('FUN_'))} were FUN_ placeholders"
          + (f", {len(adds)} newly defined" if adds else "") + ").")
    apply_renames(renames)
    insert_symbols(adds)

    if args.no_check:
        return
    print("import_symbols: rebuilding + checking byte-match…")
    subprocess.run(["./Build", "clean"], stdout=subprocess.DEVNULL)
    if subprocess.run(["./Build", "check"], stdout=subprocess.DEVNULL).returncode == 0:
        print("import_symbols: ✓ ./Build check GREEN — names adopted, byte-identical.")
    else:
        print("import_symbols: ✗ ./Build check FAILED — revert with "
              "`git checkout config src` (and undo any src file renames).", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
