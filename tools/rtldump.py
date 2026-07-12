#!/usr/bin/env python3
"""Dump cc1-281's RTL passes for ONE function, standalone and race-free.

This is the escalation tool for a same-length register/coloring/scheduling residual
that no C respelling and no permuter run has closed. Nine such "permuter-immune,
below-the-C-level" parks fell this session by READING these dumps instead of
guessing — every one turned out to be reachable source structure. See
docs/matching-cookbook.md, "Reading cc1's RTL dumps (the escalation method)".

It compiles `src/main.exe/<Name>.c` exactly the way `./Build` does (same cpp defines,
same cc1-281 flags), but standalone in the scratchpad — no Shake, no `.shake/`
database, so it never races a concurrent build. It requests the RTL dump passes and
leaves them next to the `.s` for you to read.

    tools/rtldump.py <Name>                 # default passes: greg, lreg, jump, combine
    tools/rtldump.py <Name> --pass all      # -da: every pass (loop, cse, flow, sched, reorg…)
    tools/rtldump.py <Name> --pass loop     # just loop.c (invariant hoisting: the .loop dump)
    tools/rtldump.py <Name> --pass greg,lreg,reorg,sched
    tools/rtldump.py <Name> --draft         # compile the #else NON_MATCHING draft

The dumps land in <scratchpad>/rtl/<Name>/ ; the tool prints the paths and a legend.

Reading them (short version — the doc has the worked mechanics):
  .greg  (-dg, global-alloc)  allocno priorities, `N conflicts: …`, `N preferences: H`,
                              and `Register dispositions: N in H` (pseudo N -> hard H).
                              Two roles in one hard reg = one variable; one value in
                              two hard regs = two variables (2.8.1 never splits a range).
  .lreg  (-dl, local-alloc)   local (per-block) allocation before global.
  .loop  (-dL, loop opt)      invariant hoisting decisions as a threshold economy
                              (`Insn N: regno R (life L), savings S moved / not desirable`).
  .jump2 (-dj, jump opt)      cross-jump / return-insn / delay-branch decisions.
  .combine (-dc)              instruction combining (where an addu/subu operand order
                              or a fold reassociation gets decided).
  .sched2/.dbr (-dR/-dd)      scheduling + delay-slot (reorg) fills.

Hard register numbers: 0 zero, 1 at, 2 v0, 3 v1, 4-7 a0-a3, 8-15 t0-t7, 16-23 s0-s7,
24-25 t8-t9, 28 gp, 29 sp, 30 fp/s8, 31 ra.
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# Kept byte-identical to shake/src/Build.hs's ccFlags / the cpp defines. If Build.hs
# changes these, change them here too, or the dumps stop reflecting the real build.
CC = "cc1-281"
CC_FLAGS = [
    "-mcpu=3000", "-quiet", "-fno-builtin", "-G8", "-w", "-O2", "-funsigned-char",
    "-fpeephole", "-ffunction-cse", "-fpcc-struct-return", "-fcommon",
    "-fverbose-asm", "-fgnu-linker", "-mgas", "-msoft-float",
]
CPP = "mipsel-unknown-linux-gnu-cpp"
CPP_DEFS = [
    "-undef", "-lang-c", "-Dmips", "-D__GNUC__=2", "-D__OPTIMIZE__", "-D__mips__",
    "-D__mips", "-Dpsx", "-D__psx__", "-D__psx", "-D_PSYQ", "-D__EXTENSIONS__",
    "-D_MIPSEL", "-D_LANGUAGE_C", "-DLANGUAGE_C", "-DHACKS",
]

# pass name -> cc1 -d letter. -da = all passes.
PASS_FLAG = {
    "rtl": "-dr", "jump": "-dj", "jump2": "-dj", "cse": "-ds", "loop": "-dL",
    "combine": "-dc", "flow": "-df", "lreg": "-dl", "greg": "-dg", "sched": "-dS",
    "reorg": "-dR", "dbr": "-dd", "all": "-da",
}
DEFAULT_PASSES = ["greg", "lreg", "jump", "combine"]

SCRATCH = os.environ.get(
    "CLAUDE_SCRATCH",
    os.path.join(tempfile.gettempdir(), "tenchu-rtldump"),
)


def which(prog):
    p = shutil.which(prog)
    if not p:
        sys.exit(f"rtldump: {prog} not on PATH — run inside `nix develop`")
    return p


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--pass", dest="passes", default=",".join(DEFAULT_PASSES),
                    help="comma list from " + ",".join(sorted(PASS_FLAG)) + " (or 'all')")
    ap.add_argument("--draft", action="store_true",
                    help="compile the #else NON_MATCHING draft (define NON_MATCHING)")
    ap.add_argument("--src", help="override the .c path")
    args = ap.parse_args()

    which(CC)
    cpp = shutil.which(CPP) or which("cpp")

    src = args.src or os.path.join("src", "main.exe", args.name + ".c")
    if not os.path.exists(src):
        sys.exit(f"rtldump: no {src}")

    outdir = os.path.join(SCRATCH, "rtl", args.name)
    os.makedirs(outdir, exist_ok=True)
    stem = os.path.join(outdir, args.name)
    ic, sfile = stem + ".i", stem + ".s"

    # 1. preprocess exactly as ./Build does (add the draft define if asked)
    cpp_cmd = [cpp, *CPP_DEFS, "-I", "include"]
    if args.draft:
        cpp_cmd += ["-DNON_MATCHING"]
    cpp_cmd += [src]
    with open(ic, "w") as f:
        r = subprocess.run(cpp_cmd, stdout=f, stderr=subprocess.PIPE, text=True)
    if r.returncode:
        sys.exit(f"rtldump: cpp failed:\n{r.stderr}")

    # 2. cc1 with the real flags + the requested dump letters
    want = [p.strip() for p in args.passes.split(",") if p.strip()]
    bad = [p for p in want if p not in PASS_FLAG]
    if bad:
        sys.exit(f"rtldump: unknown pass(es) {bad}; choose from {sorted(PASS_FLAG)}")
    dflags = sorted({PASS_FLAG[p] for p in want})
    # cc1 writes each dump next to the INPUT as <input>.<passname> (e.g. foo.i.greg)
    r = subprocess.run([CC, *CC_FLAGS, *dflags, ic, "-o", sfile],
                       stderr=subprocess.PIPE, text=True, cwd=outdir)
    err = (r.stderr or "").strip()
    if not os.path.exists(sfile):
        sys.exit(f"rtldump: cc1 produced no .s (rc={r.returncode}):\n{err}")

    dumps = sorted(f for f in os.listdir(outdir)
                   if f.startswith(os.path.basename(ic) + ".") and not f.endswith(".s"))
    print(f"rtldump: {args.name}{' (draft)' if args.draft else ''} -> {outdir}")
    print(f"  asm:   {sfile}")
    print(f"  dumps: {', '.join(dumps) if dumps else '(none — check the pass names)'}")
    if err:
        print(f"  cc1 stderr: {err.splitlines()[-1]}")
    print("\n  reg legend: 2 v0  3 v1  4-7 a0-a3  8-15 t0-t7  16-23 s0-s7  30 fp  31 ra")
    print("  read .greg for `N in H` (pseudo N in hard reg H), `N preferences: H`,")
    print("  `N conflicts: …`; .loop for hoist decisions; see docs/matching-cookbook.md")
    print("  section \"Reading cc1's RTL dumps (the escalation method)\".")


if __name__ == "__main__":
    main()
