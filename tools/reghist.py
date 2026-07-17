#!/usr/bin/env python3
"""Register-mention histogram: target vs our build, per register.

The recommended FIRST move on any big Ghidra-derived function. Three lanes
hand-rolled this with ad-hoc greps before it became a tool; it answers two
questions in one second that otherwise cost a round each:

* **Is there a mega-pseudo?** Ghidra reuses one variable for a dozen unrelated
  jobs. Each C local is ONE pseudo and one pseudo gets ONE hard register for ALL
  its fragments, so a conflict anywhere exiles every use. A caller-saved register
  the DRAFT mentions far more than the target is that tell — splitting the
  variable per site was worth 140 bytes on FUN_80057b80 (`iVar3`: 70 mentions vs
  the target's 5).
* **Are the OPCODES even the same?** A register histogram is blind to `lh` vs
  `lhu`: a wrong mnemonic shows an identical register profile and would read as
  "pure renames", when it is a structural defect no allocation steering can fix
  (ControlHumanoid's 17-byte park was exactly that). This tool checks the
  mnemonic histogram first and says so loudly.
* **Is the decomposition already exhausted?** Read the delta SUM. Deltas that sum
  to ZERO and sit only in the argument registers are the signature of pure
  renames of identical instructions: no splitting lever remains, and the residual
  is allocation/scheduling (StageEndScreen: s0-s8 and v0 exact, args +5/+3/+2/-4/-6,
  sum 0). A non-zero sum means real structural divergence — go find it.

Also useful: a register the target mentions N times has N-2 body refs (the
prologue `sw` and epilogue `lw` are the other two), which is the cheapest
possible check on any "the original's X carries K refs" claim.

  tools/reghist.py <Name>          histogram, differences first (builds the
                                   DRAFT itself for a guarded function)
  tools/reghist.py <Name> --all    every register, including the equal ones
  tools/reghist.py <Name> -n       skip ./Build (reads the image on disk)

The candidate extent comes from the link map, so a stale Ghidra size cannot
truncate the view. Run inside the nix devShell.
"""
import argparse
import collections
import os
import re
import subprocess
import sys

import asmdiff
import matchdiff
from matchlock import MatchToolBusy, matching_tool_lock

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# $at/$k0/$k1/$gp/$sp/$zero are not allocated by the register allocator, so a
# difference there is never a decomposition lever; keep them out of the sum.
ALLOCATABLE = (
    ["v0", "v1"]
    + [f"a{i}" for i in range(4)]
    + [f"t{i}" for i in range(10)]
    + [f"s{i}" for i in range(9)]
    + ["ra", "fp"]
)
# objdump prints MIPS registers WITHOUT the `$` (`lw v0,-2536(v0)`), while
# hand-written asm and RTL dumps use it. Accept both, and require word
# boundaries so a hex literal (`0x800a0`) cannot register as an `a0` mention.
REG_RE = re.compile(r"\b\$?(" + "|".join(ALLOCATABLE) + r")\b")


def is_guarded(name):
    """Whether <name>.c hides its C behind the NON_MATCHING guard."""
    path = os.path.join("src", "main.exe", name + ".c")
    if not os.path.exists(path):
        return False
    with open(path, errors="replace") as stream:
        return bool(re.search(r"^\s*#\s*ifndef\s+NON_MATCHING\b",
                              stream.read(), re.M))


def opcodes(disassembly):
    """Mnemonic histogram — the guard against a false "pure renames" verdict.

    A register histogram is blind to the OPCODE: a draft emitting `lhu` where the
    target has `lh` shows an identical register profile and would be reported as
    "pure renames of identical instructions", when it is structurally wrong and no
    allocation steering could ever reach the target (ControlHumanoid's 17-byte
    park was exactly this). Compare the mnemonics before drawing any conclusion.
    """
    counts = collections.Counter()
    for _addr, text in disassembly:
        counts[text.split()[0]] += 1
    return counts


def mentions(disassembly):
    """Count register mentions across an instruction listing."""
    counts = collections.Counter()
    for _addr, text in disassembly:
        # Strip the objdump comment tail so a symbolic branch target's name
        # cannot be miscounted as a register mention.
        body = text.split(";")[0].split("<")[0]
        for reg in REG_RE.findall(body):
            counts[reg] += 1
    return counts


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("-n", "--no-build", action="store_true")
    ap.add_argument("--all", action="store_true",
                    help="show every register, not just the differing ones")
    args = ap.parse_args()
    name = args.name

    if not args.no_build:
        # Build the DRAFT, not the stub. A parked function's plain `./Build`
        # links its INCLUDE_ASM stub, so the histogram would compare the target
        # against its own bytes and report "every register matches exactly" --
        # a vacuous truth, and the most dangerous possible false negative, since
        # it tells an agent no lever remains. Set NON_MATCHING ourselves when the
        # source is guarded rather than relying on the caller to remember.
        env = dict(os.environ)
        if is_guarded(name):
            env["NON_MATCHING"] = name
        build = subprocess.run(["./Build"], env=env, capture_output=True,
                               text=True)
        if build.returncode:
            # Only show the log when it explains a failure — otherwise Shake's
            # per-file chatter buries the histogram this tool exists to print.
            sys.exit((build.stdout or "") + (build.stderr or "")
                     + "\nreghist: build failed")
    # Backstop: whatever the caller did, refuse to measure a stub artifact.
    blocker = asmdiff.candidate_artifact_error(name)
    if blocker:
        sys.exit(blocker.replace("asmdiff:", "reghist:", 1))

    addr, size = matchdiff.symbol_slot(name)
    target = asmdiff.dis(asmdiff.ORIG, addr, size)
    candidate = asmdiff.candidate_disassembly(name, addr, size)

    t_counts, o_counts = mentions(target), mentions(candidate)
    rows = []
    for reg in ALLOCATABLE:
        t, o = t_counts.get(reg, 0), o_counts.get(reg, 0)
        if t or o:
            rows.append((reg, t, o, o - t))

    shown = [r for r in rows if r[3]] or []
    total = sum(r[3] for r in rows)
    print(f"{name} @ {addr:#x}: register mentions "
          f"(target {len(target)} insns, ours {len(candidate)})")
    print(f"{'reg':>5} {'target':>7} {'ours':>7} {'delta':>7}")
    for reg, t, o, d in (rows if args.all else (shown or rows)):
        flag = "" if not d else ("  <-- draft-heavy" if d > 0 else "  <-- target-heavy")
        print(f"{reg:>5} {t:>7} {o:>7} {d:>+7}{flag}")

    # A register verdict is only meaningful once the OPCODES agree: a wrong
    # mnemonic (lh vs lhu) is invisible to a register histogram but is a
    # structural defect no allocation steering can fix.
    t_ops, o_ops = opcodes(target), opcodes(candidate)
    op_delta = sorted((op, o_ops.get(op, 0) - t_ops.get(op, 0))
                      for op in set(t_ops) | set(o_ops)
                      if o_ops.get(op, 0) != t_ops.get(op, 0))

    print()
    if op_delta:
        print("reghist: *** OPCODES DIFFER — the register verdict below is not "
              "the whole story ***")
        for op, d in op_delta:
            print(f"         {op:>8} {d:+d}")
        print("         A wrong mnemonic is a STRUCTURAL defect (a type/width or "
              "expression-shape bug),")
        print("         not an allocation tie — no register steering can reach "
              "the target from here.")
        print("         Fix the opcodes first, then re-read the register "
              "histogram. (See docs/matching-cookbook.md.)")
        print()

    # A `move` in either listing is exactly where equal counts hide UNEQUAL
    # IDENTITIES: `move v0,v1` (k = key) and a pointer round-trip
    # (`move v0,a0` / `move a0,v0`) have the same opcode and register counts and
    # a completely different decomposition. This tool told a lane "the
    # decomposition already matches the target; do not hunt a mega-pseudo" on
    # SetupSpline, and that steer was wrong and cost most of a round. Hedge
    # whenever copies are present rather than stating a verdict.
    copies = sum(1 for _a, t in target if t.split()[0] == "move")
    copies += sum(1 for _a, t in candidate if t.split()[0] == "move")

    def rename_caveat():
        if not copies:
            return
        print("         *** CAVEAT: the listings contain `move` copies. A "
              "zero-sum histogram")
        print("         CANNOT distinguish a rename from a same-count / "
              "different-IDENTITY copy")
        print("         structure. Check which C value each register actually "
              "carries before")
        print("         accepting the verdict above — and read `tools/siblingdiff.py "
              "--demo`,")
        print("         which answers 'is this copy real codegen or our "
              "scaffolding?' in one call.")

    if not shown:
        print("reghist: every allocatable register matches the target exactly.")
        if not op_delta:
            print("         Probably no mega-pseudo and no splitting lever — the "
                  "residual is likely")
            print("         allocation/scheduling rather than decomposition.")
            rename_caveat()
        return 0
    print(f"reghist: {len(shown)} register(s) differ; delta sum {total:+d}")
    if total == 0 and not op_delta:
        print("         Sum ZERO is CONSISTENT WITH pure renames of identical "
              "instructions,")
        print("         which would mean the variable decomposition already "
              "matches the target.")
        rename_caveat()
    elif total == 0:
        print("         Sum ZERO, but the OPCODES differ (above) — this is NOT "
              "'pure renames'.")
    else:
        print("         Non-zero sum = real structural divergence (instructions "
              "our draft has and the target does not, or vice versa).")
    worst = max(shown, key=lambda r: r[3])
    if worst[3] >= 10:
        print(f"         ${worst[0]} is draft-heavy by {worst[3]} — suspect a "
              f"MEGA-PSEUDO (a Ghidra variable doing several unrelated jobs);")
        print("         split it per site. See docs/matching-cookbook.md.")
    return 0


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("reghist", target):
            sys.exit(main())
    except MatchToolBusy as e:
        sys.exit(f"reghist: {e}")
