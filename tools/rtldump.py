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
    tools/rtldump.py <Name> --draft         # REQUIRED for a guarded C draft
    tools/rtldump.py <Name> --lines         # + source-line-mapped object/disassembly

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
import hashlib
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

# Key the dump directory to THIS worktree. The old default was a bare shared
# `<tmp>/tenchu-rtldump`, so every concurrent agent wrote RTL dumps for the same
# function name to the same path — and a lane was nearly misled by a sibling's
# stale `.greg` before it noticed and overrode CLAUDE_SCRATCH by hand. Dumps are
# the ground truth these tools reason from; a torn read there produces confident
# nonsense, which is the same failure the private-scratch rule exists to prevent.
SCRATCH = os.environ.get("CLAUDE_SCRATCH") or os.path.join(
    tempfile.gettempdir(),
    "tenchu-rtldump-" + hashlib.sha1(ROOT.encode()).hexdigest()[:10],
)


def which(prog):
    p = shutil.which(prog)
    if not p:
        sys.exit(f"rtldump: {prog} not on PATH — run inside `nix develop`")
    return p


def _load_maspsx_config():
    """Return the build-equivalent maspsx/as configuration.

    permute.py already mirrors Build.hs's per-function gp externs and div
    expansion flags.  Import it rather than growing a third hand-maintained
    table here.  The import is deliberately lazy: ordinary RTL dumps do not
    need maspsx or the assembler at all.
    """
    import permute
    return permute.GP_EXTERNS, permute.MASPSX_EXTRA, permute.AS_FLAGS


def cc_flags_for(name):
    """Return the build-equivalent cc1 flags for one translation unit."""
    import permute
    return CC_FLAGS + permute.CC_EXTRA_FLAGS.get(name, [])


def compile_rtl(name, passes=None, draft=False, src=None, debug_lines=False,
                assemble=False):
    """Compile one source file and return paths to its asm/RTL artifacts.

    This is the reusable backend for rtldump.py, regalloc/guide-style tools,
    and future mechanical searches.  `debug_lines` adds cc1's `-g`: it does not
    participate in the real build, but causes source NOTE objects to survive in
    every RTL pass.  `assemble` additionally runs the exact maspsx/as pipeline
    and writes an objdump whose instructions retain their originating C line.

    Returns a dict with outdir, preprocessed, asm, dumps, stderr and, when
    requested, processed_asm/object/objdump.
    """
    which(CC)
    cpp = shutil.which(CPP) or which("cpp")
    src = src or os.path.join("src", "main.exe", name + ".c")
    if not os.path.exists(src):
        raise FileNotFoundError(src)
    with open(src, errors="replace") as stream:
        guarded_stub = "ifndef NON_MATCHING" in stream.read()

    want = list(passes or DEFAULT_PASSES)
    bad = [p for p in want if p not in PASS_FLAG]
    if bad:
        raise ValueError(f"unknown pass(es) {bad}; choose from {sorted(PASS_FLAG)}")

    outdir = os.path.join(SCRATCH, "rtl", name)
    os.makedirs(outdir, exist_ok=True)
    stem = os.path.join(outdir, name)
    ic, sfile = stem + ".i", stem + ".s"

    # A previous `--pass all` otherwise contaminates a later narrow run's file
    # listing and, worse, lets consumers read a stale pass.  Keep only artifacts
    # produced by this invocation.
    prefix = os.path.basename(ic) + "."
    for old in os.listdir(outdir):
        if old.startswith(prefix) or old in {
                os.path.basename(sfile), name + ".maspsx.s", name + ".o",
                name + ".objdump"}:
            try:
                os.unlink(os.path.join(outdir, old))
            except FileNotFoundError:
                pass

    cpp_cmd = [cpp, *CPP_DEFS, "-I", "include", "-I", os.path.join("src", "main.exe")]
    if draft:
        cpp_cmd += ["-DNON_MATCHING"]
    cpp_cmd += [src]
    with open(ic, "w") as f:
        r = subprocess.run(cpp_cmd, stdout=f, stderr=subprocess.PIPE, text=True)
    if r.returncode:
        raise RuntimeError("cpp failed:\n" + r.stderr)

    dflags = sorted({PASS_FLAG[p] for p in want})
    debug = ["-g"] if debug_lines else []
    r = subprocess.run([CC, *cc_flags_for(name), *debug, *dflags, ic, "-o", sfile],
                       stderr=subprocess.PIPE, text=True, cwd=outdir)
    err = (r.stderr or "").strip()
    if not os.path.exists(sfile):
        raise RuntimeError(f"cc1 produced no .s (rc={r.returncode}):\n{err}")

    dumps = sorted(os.path.join(outdir, f) for f in os.listdir(outdir)
                   if f.startswith(prefix))
    result = dict(outdir=outdir, preprocessed=ic, asm=sfile, dumps=dumps,
                  stderr=err, guarded_stub=guarded_stub and not draft)

    if assemble:
        if not debug_lines:
            raise ValueError("assemble=True requires debug_lines=True")
        which("maspsx")
        assembler = which("mipsel-unknown-linux-gnu-as")
        objdump = which("mipsel-unknown-linux-gnu-objdump")
        gp_externs, extra, as_flags = _load_maspsx_config()
        processed = stem + ".maspsx.s"
        obj = stem + ".o"
        listing = stem + ".objdump"
        maspsx_cmd = ["maspsx", "--aspsx-version=2.77", "-G8",
                      *extra.get(name, [])]
        for sym in gp_externs.get(name, []):
            maspsx_cmd += ["--gp-extern", sym]
        with open(sfile) as inp, open(processed, "w") as out:
            r = subprocess.run(maspsx_cmd, stdin=inp, stdout=out,
                               stderr=subprocess.PIPE, text=True)
        if r.returncode:
            raise RuntimeError("maspsx failed:\n" + r.stderr)
        r = subprocess.run([assembler, *as_flags, "-I", ROOT, "-o", obj, processed],
                           capture_output=True, text=True, cwd=ROOT)
        if r.returncode:
            raise RuntimeError("as failed:\n" + r.stderr)
        with open(listing, "w") as out:
            r = subprocess.run([objdump, "-drSl", obj], stdout=out,
                               stderr=subprocess.PIPE, text=True)
        if r.returncode:
            raise RuntimeError("objdump failed:\n" + r.stderr)
        result.update(processed_asm=processed, object=obj, objdump=listing)
    return result


def is_guarded(name, src=None):
    """Whether <name>.c hides its C behind the NON_MATCHING guard."""
    src = src or os.path.join("src", "main.exe", name + ".c")
    if not os.path.exists(src):
        return False
    with open(src, errors="replace") as stream:
        return "ifndef NON_MATCHING" in stream.read()


# Pass order as cc1 runs them, so --trace lines the dumps up chronologically
# rather than alphabetically. A rewrite with no dump of its own (reload_cse_regs)
# shows up as a delta between the two passes that bracket it.
# cc1 2.8.1's real rest_of_compilation order (toplev.c). Ordering these
# ALPHABETICALLY, or omitting a pass so it sorts to the end, would put cse2/bp
# after dbr and invent a transformation that never happened -- the whole point of
# --trace is that ADJACENCY carries the meaning.
# Verified against the pinned toplev.c's own dump sequence, not recalled.
PASS_ORDER = ["rtl", "jump", "addressof", "cse", "loop", "cse2", "bp", "flow",
              "combine", "sched", "lreg", "greg", "sched2", "jump2", "dbr"]


def trace_insn(result, uid, want):
    """Print one insn's RTL across every dumped pass, in cc1's own pass order.

    Three lanes asked for this and each hand-rolled it with `grep -A3 "insn 27 "`
    across the dump files. It is what makes an invisible pass visible: GetNearestHumanoid's
    `(const_int 0)` survives to `.greg` and is a register copy by `.sched2`, which
    localises the rewrite to `reload_cse_regs` (toplev.c:3501) — a pass that emits
    no dump at all and is only legible as a delta between its neighbours.
    """
    import re as _re
    head = _re.compile(r"^\((?:insn|call_insn|jump_insn)(?:/\w+)? %d " % uid)
    order = {p: i for i, p in enumerate(PASS_ORDER)}
    unknown = [p.rsplit(".", 1)[-1] for p in result["dumps"]
               if p.rsplit(".", 1)[-1] not in order]
    if unknown:
        # Never silently sort an unknown pass to the end: --trace's meaning is
        # positional, so a mis-placed pass fabricates a delta.
        print(f"  rtldump: WARNING: pass(es) {sorted(set(unknown))} are not in "
              "PASS_ORDER; their position below is NOT cc1's order.")
    dumps = sorted(result["dumps"],
                   key=lambda p: order.get(p.rsplit(".", 1)[-1], 99))
    print(f"\n  --- insn {uid} across {len(dumps)} pass(es), in cc1's pass order ---")
    prev = None
    for path in dumps:
        name = path.rsplit(".", 1)[-1]
        body, keep, buf = open(path, errors="replace").read(), False, []
        for line in body.splitlines():
            if head.match(line):
                keep, buf = True, [line]
            elif keep:
                if line.startswith("(") or not line.strip():
                    break
                buf.append(line)
        text = " ".join(" ".join(buf).split())[:150] if buf else "(absent)"
        mark = "  <-- CHANGED" if prev is not None and text != prev else ""
        print(f"  {name:>8}: {text}{mark}")
        prev = text
    print("  A change between two passes with no dump between them is that pass's "
          "doing —")
    print("  e.g. .greg -> .sched2 brackets reload_cse_regs (toplev.c:3501), which "
          "has no dump.")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--pass", dest="passes", default=",".join(DEFAULT_PASSES),
                    help="comma list from " + ",".join(sorted(PASS_FLAG)) + " (or 'all')")
    ap.add_argument("--draft", action="store_true",
                    help="compile the #else NON_MATCHING draft (the DEFAULT for a "
                         "guarded function; this flag is only needed to be explicit)")
    ap.add_argument("--stub", action="store_true",
                    help="compile the INCLUDE_ASM stub of a guarded function. Almost "
                         "never what you want: the stub's RTL describes the ASM "
                         "TRAMPOLINE, not your C.")
    ap.add_argument("--src", help="override the .c path")
    ap.add_argument("--lines", action="store_true",
                    help="preserve C-line notes and emit a line-mapped .o/.objdump")
    ap.add_argument("--trace", metavar="UID", type=int,
                    help="print insn UID across EVERY pass, in pass order — the "
                         "single highest-value ask from three lanes, each of which "
                         "hand-rolled it with grep. A transformation is often only "
                         "visible as a delta BETWEEN two dumps (reload_cse_regs runs "
                         "after .greg and before .sched2 and has no dump of its own), "
                         "and that delta is invisible unless you line the passes up.")
    args = ap.parse_args()
    want = [p.strip() for p in args.passes.split(",") if p.strip()]

    # Default to the DRAFT whenever the source is guarded. Dumping the stub is
    # never useful -- its RTL is the INCLUDE_ASM trampoline, so every pass reads
    # empty and any conclusion drawn from it is fiction. rtldump is the
    # escalation tool of last resort and a guarded function is precisely when
    # you reach for it, so the old stub-by-default behaviour meant the tool was
    # at its most misleading exactly when it mattered most. A warning was not
    # enough: a lane confirmed it would be easy to read the stub's RTL and draw
    # confident nonsense from it (and reghist had the identical bug -- see its
    # is_guarded() note). --stub still opts in explicitly.
    draft = args.draft
    if not draft and not args.stub and is_guarded(args.name, args.src):
        draft = True
        print(f"rtldump: {args.name} is guarded — dumping its C DRAFT "
              "(pass --stub for the INCLUDE_ASM stub's RTL).", file=sys.stderr)
    try:
        result = compile_rtl(args.name, want, draft, args.src,
                             debug_lines=args.lines, assemble=args.lines)
    except (FileNotFoundError, ValueError, RuntimeError) as e:
        sys.exit(f"rtldump: {e}")
    outdir, sfile, err = result["outdir"], result["asm"], result["stderr"]
    dumps = [os.path.basename(p) for p in result["dumps"]]
    if result["guarded_stub"]:
        print(f"rtldump: WARNING: {args.name} is guarded and you passed --stub, so "
              "this run compiled the INCLUDE_ASM trampoline, NOT your C draft. "
              "Every pass below describes the stub. Drop --stub.", file=sys.stderr)
    print(f"rtldump: {args.name}{' (draft)' if draft else ''} -> {outdir}")
    print(f"  asm:   {sfile}")
    print(f"  dumps: {', '.join(dumps) if dumps else '(none — check the pass names)'}")
    if args.lines:
        print(f"  object: {result['object']}")
        print(f"  lines:  {result['objdump']}")
    if err:
        print(f"  cc1 stderr: {err.splitlines()[-1]}")
    if args.trace is not None:
        trace_insn(result, args.trace, want)
    print("\n  reg legend: 2 v0  3 v1  4-7 a0-a3  8-15 t0-t7  16-23 s0-s7  30 fp  31 ra")
    print("  read .greg for `N in H` (pseudo N in hard reg H), `N preferences: H`,")
    print("  `N conflicts: …`; .loop for hoist decisions; see docs/matching-cookbook.md")
    print("  section \"Reading cc1's RTL dumps (the escalation method)\".")


if __name__ == "__main__":
    main()
