#!/usr/bin/env python3
"""Score how close each NON_MATCHING draft is to the original, for decomp.dev.

decomp.dev shows per-function progress via objdiff's `fuzzy_match_percent`. Our
default report is binary (a function is byte-matched = 100%, or an INCLUDE_ASM
stub = 0%). But ~120 functions carry a `#else` NON_MATCHING draft (a C attempt
that is close but not byte-exact -- usually a sub-C register-allocation /
scheduling residual). This tool measures HOW close each draft is, so those
functions show real partial progress instead of 0%.

Method (an approximation of objdiff's fuzzy match, reusing tools/asmdiff.py's
machinery): for each draft, build it (`NON_MATCHING=<name> ./Build`),
disassemble the original function and our compiled draft, normalize away
position-dependent operands (absolute branch/jump targets, trailing immediates
-- the same `stem` asmdiff uses), and take difflib's instruction-sequence
similarity ratio. A draft that is not byte-exact is capped below 100 (100 is
reserved for a real byte-match, which would already be promoted out of a draft).

Output: config/fuzzy.main.exe.tsv (committed), consumed by tools/objdiff-report.py.
Build failures (a draft that does not compile) are recorded and get no score
(they stay 0% in the report). Run INSIDE the nix devShell; it is slow (one
incremental build per draft). Re-run and commit when drafts change.

  tools/fuzz-score.py                    # score every NON_MATCHING draft
  tools/fuzz-score.py --only Foo,Bar     # just these (for iterating)
  tools/fuzz-score.py --keep-going       # don't restore the green build at the end
"""
import argparse
import difflib
import os
import re
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import function_inventory as FI
import fuzzy_inventory as FZI

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
FILE_TEXT_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
OURS = ".shake/build/tenchu/main.exe"
SYMBOLS = "config/symbols.main.exe.txt"
TSV = ".shake/ghidra-export/functions.tsv"
SNAPSHOT_TSV = "config/functions.main.exe.tsv"
SRC = "src/main.exe"
OUT = "config/fuzzy.main.exe.tsv"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"
CAP = 99.9  # a non-byte-exact draft never reads as "complete" (100)


def sizes_by_addr():
    tsv = TSV if os.path.exists(TSV) else SNAPSHOT_TSV
    return {addr: size for addr, size, _ in FI.load_functions(tsv)}


def load_symbols():
    addr = {}
    for line in open(SYMBOLS):
        m = re.match(r"([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            addr[m.group(1)] = int(m.group(2), 16)
    return addr


def dis(path, addr, size):
    """Disassembled instruction text for [addr, addr+size) of a raw PSX-EXE."""
    data = open(path, "rb").read()
    off = FILE_TEXT_OFF + (addr - TEXT_START)
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        f.write(data[off:off + size])
        tmp = f.name
    try:
        out = subprocess.run(
            [OBJDUMP, "-D", "-b", "binary", "-m", "mips", "-EL",
             "--adjust-vma", hex(addr), tmp],
            capture_output=True, text=True).stdout
    finally:
        os.unlink(tmp)
    insns = []
    for line in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\t[0-9a-f]{8} \t(.*)", line)
        if m:
            insns.append(m.group(2).replace("\t", " ").strip())
    return insns


def raw(path, addr, size):
    data = open(path, "rb").read()
    off = FILE_TEXT_OFF + (addr - TEXT_START)
    return data[off:off + size]


def stem(insn):
    # Drop position-dependent absolute operands (branch/jump targets, trailing
    # immediates) so pure address drift from a size shift isn't counted as a
    # difference -- register allocation and instruction order are preserved.
    return re.sub(r"0x[0-9a-f]+$", "", insn)


def our_insns(addr, size):
    """Our build's instructions for the function, tolerant of a size shift: read
    with slack and cut at the trailing `jr ra` (asmdiff's heuristic)."""
    ours = dis(OURS, addr, size + 0x100)
    jrs = [i for i, x in enumerate(ours)
           if x.startswith("jr ra") and i + 1 >= (size // 4) - 0x40]
    return ours[:jrs[0] + 2] if jrs else ours[:max(size // 4, 1)]


def build(name):
    env = dict(os.environ)
    env["NON_MATCHING"] = name
    log = os.path.join(tempfile.gettempdir(), "tenchu-fuzz-build.log")
    with open(log, "wb") as lf:
        r = subprocess.run(["./Build"], stdout=subprocess.DEVNULL, stderr=lf, env=env)
    return r.returncode == 0, log


def score_one(name, addr, size):
    ok, log = build(name)
    if not ok:
        return None, "buildfail"
    tgt = dis(ORIG, addr, size)
    ours = our_insns(addr, size)
    if not tgt:
        return None, "no-target"
    # Raw byte equality in the function's slot is the authoritative match test.
    # Do NOT gate it on the instruction-count heuristic: our_insns() reads a
    # slack window and can over-read, which made a genuine byte-match (e.g.
    # ReqItemGun, later promoted to a plain matched .c) score <100 as a partial.
    if raw(ORIG, addr, size) == raw(OURS, addr, size):
        return 100.0, "exact"
    t = [stem(x) for x in tgt]
    o = [stem(x) for x in ours]
    ratio = difflib.SequenceMatcher(None, t, o, autojunk=False).ratio()
    return min(round(ratio * 100.0, 2), CAP), "ok"


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--only", help="comma-separated subset to score")
    ap.add_argument("--keep-going", action="store_true",
                    help="don't restore the green (no-NON_MATCHING) build at the end")
    ap.add_argument("--check", action="store_true",
                    help="fail if a guarded draft is missing, orphaned, or source-stale")
    args = ap.parse_args()

    if args.check:
        errors = FZI.validate(SRC, OUT)
        if errors:
            print("fuzz-score: stale fuzzy inventory:", file=sys.stderr)
            for error in errors:
                print(f"  - {error}", file=sys.stderr)
            print("run tools/fuzz-score.py --only <changed names> (or a full run)",
                  file=sys.stderr)
            return 1
        print("fuzz-score: OK — every guarded draft has a current source fingerprint")
        return 0

    sym = load_symbols()
    sz = sizes_by_addr()
    sources = FZI.guarded_sources(SRC)
    requested = ([name.strip() for name in args.only.split(",") if name.strip()]
                 if args.only else sorted(sources))
    names = [name for name in requested if name in sources]
    dropped = sorted(set(requested) - set(sources))

    # Preserve existing scores for anything we're not re-scoring (subset runs).
    prev = {}
    if os.path.exists(OUT):
        for line in open(OUT):
            if line.strip() and not line.startswith("#"):
                p = line.rstrip("\n").split("\t")
                if len(p) >= 3:
                    prev[p[0]] = line.rstrip("\n")
    # A full run owns the whole file: drop stale entries for functions that are
    # no longer NON_MATCHING drafts (e.g. promoted to a plain matched .c).
    if not args.only:
        keep = set(sources)
        prev = {k: v for k, v in prev.items() if k in keep}
    else:
        for name in dropped:
            if name in prev:
                del prev[name]
            print(f"{name}: removed stale row (function is no longer guarded)",
                  file=sys.stderr)

    results = {}

    def flush():
        merged = dict((l.split("\t")[0], l) for l in prev.values())
        merged.update(results)
        with open(OUT, "w") as fh:
            fh.write("# name\tfuzzy_match_percent\tstatus\tsource_sha256  "
                     "(generated by tools/fuzz-score.py; empty = no score)\n")
            for name in sorted(merged):
                fh.write(merged[name] + "\n")
        return merged

    merged = {}
    for i, name in enumerate(names, 1):
        if name not in sym or sym[name] not in sz:
            print(f"[{i}/{len(names)}] {name}: SKIP (no addr/size)", file=sys.stderr)
            continue
        addr, size = sym[name], sz[sym[name]]
        fuzzy, status = score_one(name, addr, size)
        val = "" if fuzzy is None else f"{fuzzy:.2f}"
        results[name] = (f"{name}\t{val}\t{status}\t"
                         f"{FZI.source_hash(sources[name])}")
        print(f"[{i}/{len(names)}] {name}: {status} {val}", file=sys.stderr)
        merged = flush()  # incremental: crash-safe + monitorable during long runs

    if dropped and not names:
        merged = flush()
    scored = [r for r in results.values() if r.split("\t")[1]]
    print(f"\nwrote {OUT}: {len(scored)}/{len(results)} scored this run "
          f"({len(merged)} total)", file=sys.stderr)

    if not args.keep_going:
        subprocess.run(["./Build"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return 0


if __name__ == "__main__":
    sys.exit(main())
