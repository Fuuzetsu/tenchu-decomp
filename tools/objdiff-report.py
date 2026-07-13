#!/usr/bin/env python3
"""Emit an objdiff v2 progress `Report` (JSON) that decomp.dev ingests.

decomp.dev is *artifact-driven*: it never reads decomp.yaml. Its bot scans a
GitHub repo's Actions runs, downloads an artifact named `<version>_report`
(here `jp_report`), reads `report.json` from it, parses it as an objdiff
`Report` (protobuf schema, JSON-encoded), and stores per-commit progress. See
docs/decomp-dev.md for the full pipeline + self-hosting setup.

This generator is *build-free*: like tools/progress.py it derives everything
from static repo files (the function inventory + the splat/symbol config + the
matched `.c` files), so CI needs only Python — no nix, no cc1, no ELF build.

  tools/objdiff-report.py                      -> .shake/build/tenchu/report.json
  tools/objdiff-report.py --out report.json    (CI: artifact payload)
  tools/objdiff-report.py --include-sdk         include the PsyQ SDK block too
  tools/objdiff-report.py --stdout              print JSON to stdout

"Matched" is defined exactly as in tools/progress.py: a function that is CARVED
(has a `c` subsegment in the splat yaml, so its `.c` is actually linked) AND
whose src/main.exe/<Name>.c has no INCLUDE_ASM. Each function becomes one
objdiff *unit* (per-function granularity); units are tagged into the `game` /
`sdk` progress categories. u64 measure/size/address fields are JSON strings and
u32/float fields are numbers — the encoding objdiff's prost+serde emits and
`Report::parse` expects (verified against a live decomp.dev report).
"""
import argparse
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import function_inventory as FI
import fuzzy_inventory as FZI

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# Kept in sync with tools/progress.py — the game/SDK boundary and text window.
TEXT_START = 0x80011000
TEXT_END = 0x80098000
SDK_START = 0x80060000  # 0x80060xxx+ is the differently-compiled PsyQ SDK block

# The Ghidra export (addr<TAB>size<TAB>name) is gitignored; the committed,
# reviewed snapshot is authoritative for reports.  Do not refresh it as a side
# effect: the snapshot contains corrections for two known bad Ghidra boundaries.
# Pass --functions explicitly when auditing a new live export.
LIVE_TSV = ".shake/ghidra-export/functions.tsv"
SNAPSHOT_TSV = "config/functions.main.exe.tsv"
SYMBOLS = "config/symbols.main.exe.txt"
YAML = "config/splat.main.exe.yaml"
SRC = "src/main.exe"
# Per-function fuzzy match % for NON_MATCHING drafts (tools/fuzz-score.py). Lets
# "mostly matching but not byte-exact" functions show partial progress instead
# of 0%. Optional: absent -> every function is binary 0/100.
FUZZY = "config/fuzzy.main.exe.tsv"
DEFAULT_OUT = ".shake/build/tenchu/report.json"

REPORT_VERSION = 2

CATEGORIES = [
    ("game", "Game code"),
    ("sdk", "PsyQ SDK (LIBCD/LIBSPU/CRT...)"),
]


def resolve_inventory(explicit):
    """Pick a function inventory without mutating the reviewed snapshot."""
    if explicit:
        return explicit
    if os.path.exists(SNAPSHOT_TSV):
        return SNAPSHOT_TSV
    if os.path.exists(LIVE_TSV):
        return LIVE_TSV
    sys.exit(f"error: no function inventory found ({LIVE_TSV} or {SNAPSHOT_TSV})")


def load_functions(tsv):
    """[(addr, size, name)] for real, named functions inside the text window."""
    funcs = []
    rows = FI.load_functions(tsv)
    if os.path.exists(YAML):
        rows, _ = FI.overlay_current_names(rows, YAML)
    for a, s, n in rows:
        if re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", n) and TEXT_START <= a < TEXT_END:
            funcs.append((a, s, n))
    funcs.sort()
    return funcs


def load_matched():
    """Return (matched_addrs, addr_to_cfile) using progress.py's exact rule.

    A `.c` counts only if it is carved (linked) and contains no INCLUDE_ASM;
    its address comes from the symbol table (Ghidra's inventory name for the
    same function may differ from the `.c` filename, e.g. FUN_... vs a real
    name), so matching is done by address, consistent with tools/progress.py.
    """
    sym_addr = {}
    for line in open(SYMBOLS):
        m = re.match(r"([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            sym_addr[m.group(1)] = int(m.group(2), 16)
    carved = set(re.findall(r"^\s+- \[0x[0-9A-Fa-f]+,\s*c,\s*(\S+)\]",
                            open(YAML).read(), re.M))
    matched_addrs = set()
    addr_to_cfile = {}
    for f in sorted(os.listdir(SRC)):
        if not f.endswith(".c"):
            continue
        name = f[:-2]
        if name not in carved:
            continue
        if re.search(r"^\s*INCLUDE_ASM", open(os.path.join(SRC, f)).read(), re.M):
            continue
        if name in sym_addr:
            a = sym_addr[name]
            matched_addrs.add(a)
            addr_to_cfile[a] = f"{SRC}/{f}"
    return matched_addrs, addr_to_cfile, sym_addr


def load_fuzzy(sym_addr):
    """addr -> (fuzzy%, cfile) for NON_MATCHING drafts scored by fuzz-score.py.
    Keyed by address via the symbol table (as with matched functions). Missing
    file just means no partials -> every function stays binary 0/100."""
    fuzzy_by_addr, src_by_addr = {}, {}
    if not os.path.exists(FUZZY):
        return fuzzy_by_addr, src_by_addr
    errors = FZI.validate(SRC, FUZZY)
    if errors:
        detail = "\n  - ".join(errors)
        sys.exit("error: fuzzy progress is stale:\n  - " + detail +
                 "\nrun tools/fuzz-score.py --only <changed names> (or a full run)")
    for line in open(FUZZY):
        if not line.strip() or line.startswith("#"):
            continue
        p = line.rstrip("\n").split("\t")
        if len(p) < 2 or not p[1] or p[0] not in sym_addr:
            continue          # no score (e.g. a build failure) -> leave at 0%
        a = sym_addr[p[0]]
        fuzzy_by_addr[a] = float(p[1])
        src_by_addr[a] = f"{SRC}/{p[0]}.c"
    return fuzzy_by_addr, src_by_addr


def measures(total_code, matched_code, fuzzy_weight, complete_code, total_funcs,
             matched_funcs, total_units, complete_units):
    """An objdiff `Measures`, JSON-encoded like prost+serde: u64 -> string,
    u32 -> number, float -> number; default (0) fields omitted.

    Following objdiff's own semantics: `matched_code` is the fuzzy-weighted
    matched bytes (partial credit included), `complete_code` counts only fully
    (byte-exact) functions. `matched_code` and `fuzzy_weight` (Σ fuzzy_i·size_i)
    are pre-summed by the caller so aggregates equal the exact sum of their units
    — the same AddAssign objdiff/decomp.dev use (no rounding drift)."""
    def pct(num, den):
        return round(100.0 * num / den, 6) if den else 0.0

    m = {}
    if total_code:
        m["fuzzy_match_percent"] = pct(fuzzy_weight, total_code * 100)
        m["total_code"] = str(total_code)
    if matched_code:
        m["matched_code"] = str(matched_code)
        m["matched_code_percent"] = pct(matched_code, total_code)
    if complete_code:
        m["complete_code"] = str(complete_code)
        m["complete_code_percent"] = pct(complete_code, total_code)
    if total_funcs:
        m["total_functions"] = total_funcs
    if matched_funcs:
        m["matched_functions"] = matched_funcs
        m["matched_functions_percent"] = pct(matched_funcs, total_funcs)
    if total_units:
        m["total_units"] = total_units
    if complete_units:
        m["complete_units"] = complete_units
    return m


def build_report(funcs, matched_addrs, addr_to_cfile, fuzzy_by_addr, include_sdk):
    units = []
    # accumulator: [total_code, matched_code, fuzzy_weight, complete_code,
    #               total_funcs, matched_funcs, total_units, complete_units]
    # matched_code sums the per-unit ROUNDED bytes, and fuzzy_weight sums
    # fuzzy·size, so each aggregate is the exact sum of its units.
    acc = {cid: [0, 0, 0.0, 0, 0, 0, 0, 0] for cid, _ in CATEGORIES}
    tot = [0, 0, 0.0, 0, 0, 0, 0, 0]

    for addr, size, name in funcs:
        is_sdk = addr >= SDK_START
        if is_sdk and not include_sdk:
            continue
        cat = "sdk" if is_sdk else "game"
        if addr in matched_addrs:
            fuzzy = 100.0
        else:
            fuzzy = fuzzy_by_addr.get(addr, 0.0)
        complete = 1 if fuzzy >= 100.0 else 0
        matched_code = int(round(size * fuzzy / 100.0))
        fuzzy_weight = fuzzy * size
        cc = size if complete else 0

        for bucket in (acc[cat], tot):
            bucket[0] += size
            bucket[1] += matched_code
            bucket[2] += fuzzy_weight
            bucket[3] += cc
            bucket[4] += 1
            bucket[5] += complete
            bucket[6] += 1
            bucket[7] += complete

        func_item = {
            "name": name,
            "size": str(size),
            "fuzzy_match_percent": round(fuzzy, 2),
            "address": str(addr),
        }
        meta = {
            "complete": bool(complete),
            "progress_categories": [cat],
            "auto_generated": False,
        }
        if addr in addr_to_cfile:            # matched or draft source
            meta["source_path"] = addr_to_cfile[addr]
        units.append({
            "name": name,
            "measures": measures(size, matched_code, fuzzy_weight, cc,
                                 1, complete, 1, complete),
            "sections": [],
            "functions": [func_item],
            "metadata": meta,
        })

    categories = []
    for cid, cname in CATEGORIES:
        a = acc[cid]
        if a[4] == 0:  # no units (total_functions) in this category (e.g. sdk omitted)
            continue
        categories.append({"id": cid, "name": cname, "measures": measures(*a)})

    return {
        "measures": measures(*tot),
        "units": units,
        "version": REPORT_VERSION,
        "categories": categories,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default=DEFAULT_OUT,
                    help=f"output path (default {DEFAULT_OUT})")
    ap.add_argument("--stdout", action="store_true", help="write JSON to stdout")
    ap.add_argument("--include-sdk", action="store_true",
                    help="also include the PsyQ SDK block (top-level = whole image)")
    ap.add_argument("--functions", help="function-inventory TSV (overrides autodetect)")
    ap.add_argument("--no-fuzzy", action="store_true",
                    help="ignore config/fuzzy.main.exe.tsv (binary 0/100 report)")
    args = ap.parse_args()

    tsv = resolve_inventory(args.functions)
    funcs = load_functions(tsv)
    matched_addrs, addr_to_cfile, sym_addr = load_matched()
    fuzzy_by_addr, fuzzy_src = ({}, {}) if args.no_fuzzy else load_fuzzy(sym_addr)
    # A matched (byte-exact) .c wins over a draft .c at the same address.
    source_by_addr = {**fuzzy_src, **addr_to_cfile}
    report = build_report(funcs, matched_addrs, source_by_addr, fuzzy_by_addr,
                          args.include_sdk)

    text = json.dumps(report, indent=1)
    if args.stdout:
        print(text)
    else:
        os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
        with open(args.out, "w") as fh:
            fh.write(text)

    tm = report["measures"]
    tc = int(tm.get("total_code", 0))
    mc = int(tm.get("matched_code", 0))          # fuzzy-weighted
    cc = int(tm.get("complete_code", 0))          # byte-exact
    fpct = 100.0 * mc / tc if tc else 0.0
    cpct = 100.0 * cc / tc if tc else 0.0
    scope = "game+sdk" if args.include_sdk else "game"
    npart = sum(1 for u in report["units"]
                if 0.0 < u["functions"][0]["fuzzy_match_percent"] < 100.0)
    print(f"report ({scope}): {len(report['units'])} units, "
          f"complete {tm.get('matched_functions', 0)}/{tm.get('total_functions', 0)} funcs "
          f"({cc}/{tc}B = {cpct:.2f}%), fuzzy {fpct:.2f}% "
          f"(+{npart} partial) -> {'stdout' if args.stdout else args.out}",
          file=sys.stderr)


if __name__ == "__main__":
    main()
