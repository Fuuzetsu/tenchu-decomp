#!/usr/bin/env python3
"""Push the repo's verified knowledge INTO the Ghidra program (repo -> Ghidra).

The other direction (Ghidra -> repo) already exists: tools/ghidra/Export*.java +
tools/import_symbols.py + tools/reverse.py. This closes the loop so a type or
signature you *proved* here (./Build check green) flows back and improves Ghidra's
decompilation of every other function.

What it syncs (the "knowledge layer" only — never C bodies, never renames):
  * types       src/main.exe/game_types.h (structs/enums) -> program data types
  * signatures  function prototypes from main.exe.h + matched src/main.exe/*.c
  * global types  `extern T x;` globals from main.exe.h  -> typed data at x

It prepares a flat, preprocessed header + an apply-list, then runs
tools/ghidra/ImportToGhidra.java under analyzeHeadless. DRY RUN by default
(-readOnly, nothing saved) — review the log, then re-run with --commit.

Usage (in the nix devShell):
  tools/sync_to_ghidra.py                 # dry run against the real project
  tools/sync_to_ghidra.py --commit        # actually write to Ghidra
  tools/sync_to_ghidra.py --project DIR --project-name NAME
Close the Ghidra GUI first (headless needs the project lock).
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TYPES_H = "include/types.h"
GAME_TYPES = "src/main.exe/game_types.h"
MAIN_H = "src/main.exe/main.exe.h"
SRC_DIR = "src/main.exe"
SCRIPT_DIR = "tools/ghidra"
SYMBOLS = "config/symbols.main.exe.txt"


def load_symbol_addrs():
    """name -> '0x........' from config/symbols (the address is the sync key)."""
    addrs = {}
    for line in open(SYMBOLS):
        m = re.match(r"^([A-Za-z_]\w*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line.strip())
        if m:
            addrs[m.group(1)] = m.group(2)
    return addrs

# ident, allowing the `*`/`[]` decoration around it
EXTERN_RE = re.compile(r"^extern\s+(.*);\s*$")
FUNC_DEF_RE = re.compile(r"^([A-Za-z_][\w\s\*]*?\b([A-Za-z_]\w*)\s*\([^;{]*\))\s*\{", re.M)


def gather_prototypes_and_globals():
    """Returns (functions, globals).
    functions: list of (name, prototype)  where prototype is the full C signature
    globals:   list of (name, base_type, ptr_depth)
    """
    functions, globals_, seen = [], [], set()

    # --- externs in main.exe.h: classify function vs global ---
    for line in open(MAIN_H):
        m = EXTERN_RE.match(line.strip())
        if not m:
            continue
        decl = m.group(1).strip()
        if "(" in decl:  # function prototype
            name = extract_func_name(decl)
            if name and name not in seen:
                functions.append((name, decl))
                seen.add(name)
        else:  # global variable: `TYPE stars NAME` (skip arrays for now)
            g = parse_global(decl)
            if g:
                globals_.append(g)
            else:
                sys.stderr.write(f"  (skipping complex global: {decl})\n")

    # --- matched C definitions: pull their signatures ---
    for fn in sorted(os.listdir(SRC_DIR)):
        if not fn.endswith(".c"):
            continue
        text = open(os.path.join(SRC_DIR, fn)).read()
        if "INCLUDE_ASM" in text:
            continue  # not fully matched -> its signature may still be a guess
        for m in FUNC_DEF_RE.finditer(text):
            sig, name = re.sub(r"\s+", " ", m.group(1).strip()), m.group(2)
            if name not in seen:
                functions.append((name, sig))
                seen.add(name)

    return functions, globals_


def extract_func_name(decl):
    # `ret stars NAME(params)` -> NAME
    head = decl.split("(", 1)[0]
    ids = re.findall(r"[A-Za-z_]\w*", head)
    return ids[-1] if ids else None


def parse_global(decl):
    # `character_state *Me_THINK_C` -> ("Me_THINK_C", "character_state", 1)
    if "[" in decl or "(" in decl:
        return None
    ptrs = decl.count("*")
    toks = re.findall(r"[A-Za-z_]\w*", decl.replace("*", " "))
    if len(toks) < 2:
        return None
    name = toks[-1]
    base = " ".join(toks[:-1])
    return (name, base, ptrs)


# Only the sN/uN typedefs Ghidra's generic C types LACK. We deliberately do NOT
# redefine u_long/u_char/u_short/byte/uint — Ghidra already has those, and
# redefining them makes our copy clobber Ghidra's (which its SDK types reference),
# breaking type resolution. Game types resolve those against Ghidra's existing set.
BASE_PREAMBLE = """
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;
"""


def normalize_proto(proto):
    # FunctionSignatureParser reads the token before `(` as the name; for a
    # pointer return `void *Foo(...)` that token is `*Foo`. Bind the star to the
    # return type instead: `void* Foo(...)`. (Only the name's star, before `(`.)
    return re.sub(r"([A-Za-z_0-9])\s+\*([A-Za-z_]\w*)\s*\(", r"\1* \2(", proto)


def build_import_header(functions, prep):
    """Concatenate base types + game types + prototypes, then cpp -P to flatten.
    The prototypes let CParser see every referenced type; signatures themselves
    are (re)parsed in Ghidra by FunctionSignatureParser against the result."""
    combined = os.path.join(prep, "combined.h")
    with open(combined, "w") as w:
        w.write(BASE_PREAMBLE)
        w.write("\n")
        w.write(open(GAME_TYPES).read())
        w.write("\n/* --- prototypes (pull in all referenced types) --- */\n")
        for _name, proto in functions:
            w.write(f"extern {normalize_proto(proto)};\n")

    flat = os.path.join(prep, "import.h")
    cpp = os.environ.get("CPP", "mipsel-unknown-linux-gnu-cpp")
    # -P: no line markers; -nostdinc/-undef: don't pull the host's headers/macros.
    # SDK type names (GsIMAGE, VECTOR, ...) pass through as identifiers and are
    # resolved by CParser against the program's existing data types.
    subprocess.run([cpp, "-P", "-nostdinc", "-undef", combined, "-o", flat], check=True)
    return flat


def write_apply_tsv(functions, globals_, addrs, prep):
    """Emit directives keyed by address (from config/symbols). Names with no
    known address are dropped with a warning — we can't target them in Ghidra."""
    missing = []
    with open(os.path.join(prep, "apply.tsv"), "w") as w:
        for name, proto in functions:
            if name in addrs:
                w.write(f"FUNC\t{name}\t{addrs[name]}\t{normalize_proto(proto)}\n")
            else:
                missing.append(name)
        for name, base, ptrs in globals_:
            if name in addrs:
                w.write(f"GLOBAL\t{name}\t{base}\t{ptrs}\t{addrs[name]}\n")
            else:
                missing.append(name)
    if missing:
        sys.stderr.write("  (no address in config/symbols, skipped: "
                         + ", ".join(missing) + ")\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--project", default=os.path.expanduser("~/programming/ghidra-projects"),
                    help="Ghidra project directory")
    ap.add_argument("--project-name", default="tenchu-decompile")
    ap.add_argument("--program", default="main.exe")
    ap.add_argument("--commit", action="store_true",
                    help="actually write to Ghidra (default is a read-only dry run)")
    ap.add_argument("--headless", default="ghidra-analyzeHeadless")
    ap.add_argument("--keep-prep", action="store_true", help="don't delete the prep dir")
    args = ap.parse_args()

    functions, globals_ = gather_prototypes_and_globals()
    addrs = load_symbol_addrs()
    print(f"Gathered {len(functions)} function signatures, {len(globals_)} globals.")

    prep = tempfile.mkdtemp(prefix="tenchu-ghidra-sync-")
    try:
        build_import_header(functions, prep)
        write_apply_tsv(functions, globals_, addrs, prep)

        cmd = [args.headless, args.project, args.project_name,
               "-process", args.program,
               "-scriptPath", SCRIPT_DIR,
               "-postScript", "ImportToGhidra.java", prep,
               "-noanalysis"]
        if not args.commit:
            cmd.insert(5, "-readOnly")

        print(("COMMIT" if args.commit else "DRY RUN") + ": " + " ".join(cmd))
        r = subprocess.run(cmd)
        if r.returncode != 0:
            sys.stderr.write(
                "\nheadless exited non-zero. Common cause: the project is open in the "
                "Ghidra GUI (headless needs the lock) — close it and retry.\n")
        sys.exit(r.returncode)
    finally:
        if args.keep_prep:
            print(f"prep dir kept at {prep}")
        else:
            shutil.rmtree(prep, ignore_errors=True)


if __name__ == "__main__":
    main()
