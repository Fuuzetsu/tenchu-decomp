#!/usr/bin/env python3
"""Per-function objdiff: build both objects, hand off to objdiff-cli's TUI.

The readable viewer. tools/asmdiff.py and tools/matchdiff.py stay the
machine-checkable gates (they diff the LINKED image by address and print a
residual count); this one gives objdiff's interactive side-by-side on the same
function, which is what you want while actually editing the C.

  tools/objdiff.py <Name>              build both objects, open the TUI
  tools/objdiff.py <Name> -n           skip ./Build, use what is already built
  tools/objdiff.py <Name> --dump       one-shot JSON to stdout (no TUI); the
                                       per-section match_percent lives in there

Unlike asmdiff/matchdiff this compares OBJECT FILES, not the linked exe:

  target -> assembled from the reviewed splat carve
            .shake/gen/main.exe/asm/nonmatchings/<Name>/<Name>.s
  base   -> our build's object, .shake/build/main.exe/<Name>.c.o

Consequence worth knowing: an object diff is pre-link, so it shows relocations
as relocations rather than resolved addresses. That is usually a FEATURE (a
`%hi`/`%lo` pair against the right symbol reads as equal even when the final
addresses would differ), but it means a green objdiff is NOT a byte-match
verdict. `tools/matchdiff.py <Name>` remains the gate, and `./Build check` the
ground truth.

Run inside the nix devShell (needs objdiff-cli + the cross `as`).
"""
import argparse
import os
import subprocess
import sys
import tempfile

import asmdiff
from matchlock import MatchToolBusy, matching_tool_lock

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

AS = "mipsel-unknown-linux-gnu-as"
# Mirrors Build.hs's `asFlags`. The target object must be assembled exactly as
# the real build assembles an INCLUDE_ASM'd carve, or the diff shows toolchain
# noise instead of source differences.
AS_FLAGS = ["-EL", "-Iinclude", "-march=r3000", "-mtune=r3000",
            "-no-pad-sections", "-O1", "-G0"]
# include_asm.h picks labels.inc (INCLUDE_ASM_USE_MACRO_INC is never defined);
# it supplies glabel/nonmatching and pulls in gte_macros.inc.
LABELS_INC = "include/labels.inc"


def carved_asm(name):
    return f".shake/gen/main.exe/asm/nonmatchings/{name}/{name}.s"


def our_object(name):
    return f".shake/build/main.exe/{name}.c.o"


def assemble_target(name, out):
    """Assemble the splat carve into a target object, the way the build does."""
    src = carved_asm(name)
    if not os.path.exists(src):
        sys.exit(f"objdiff: no carved asm at {src}\n"
                 f"  {name} may not be carved yet — see tools/reverse.py.")
    wrapper = None
    try:
        with tempfile.NamedTemporaryFile("w", suffix=".s", delete=False) as f:
            wrapper = f.name
            f.write(f'.include "{LABELS_INC}"\n')
            f.write(".section .text\n")
            f.write(f'.include "{src}"\n')
        try:
            r = subprocess.run([AS] + AS_FLAGS + ["-o", out, wrapper],
                               capture_output=True, text=True)
        except FileNotFoundError:
            sys.exit(f"objdiff: {AS} not found — run inside `nix develop`.")
    finally:
        if wrapper:
            os.unlink(wrapper)
    if r.returncode != 0:
        sys.exit(f"objdiff: assembling the target carve failed:\n{r.stderr}")


def build(name):
    """Build our side. Mirrors asmdiff.py: a guarded partial needs its draft."""
    env = dict(os.environ)
    srcp = os.path.join("src/main.exe", name + ".c")
    guarded = os.path.exists(srcp) and "ifndef NON_MATCHING" in open(srcp).read()
    if guarded:
        env["NON_MATCHING"] = name
    obj = our_object(name)
    before = os.path.getmtime(obj) if os.path.exists(obj) else None
    log = os.path.join(tempfile.gettempdir(), "tenchu-objdiff-build.log")
    with open(log, "wb") as lf:
        r = subprocess.run(["./Build", obj],
                           stdout=subprocess.DEVNULL, stderr=lf, env=env)
    if r.returncode != 0:
        tail = "\n".join(open(log, errors="replace").read().splitlines()[-15:])
        sys.exit(f"objdiff: ./Build FAILED (rc={r.returncode}), log: {log}\n{tail}")
    # Say whether the object actually changed. Without this you cannot tell a
    # recompile from a cache hit, and a silently-stale object reads as a real
    # diff result (see the ActSQUAT stale-artifact note in the cookbook).
    after = os.path.getmtime(obj)
    state = "rebuilt" if before is None or after != before else "up to date"
    print(f"objdiff: {obj} {state}"
          f"{' (NON_MATCHING draft)' if guarded else ''}", file=sys.stderr)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("-n", "--no-build", action="store_true")
    ap.add_argument("--dump", action="store_true",
                    help="one-shot diff to stdout instead of the TUI")
    args = ap.parse_args()

    if not args.no_build:
        build(args.name)
    else:
        # -n can leave us pointed at a stale default build whose object is the
        # INCLUDE_ASM stub -- that diffs identical against the target and means
        # nothing. asmdiff already knows how to spot it from the link map.
        stale = asmdiff.candidate_artifact_error(args.name)
        if stale:
            sys.exit(stale.replace("asmdiff:", "objdiff:"))

    ours = our_object(args.name)
    if not os.path.exists(ours):
        sys.exit(f"objdiff: {ours} does not exist — drop -n to build it.")

    with tempfile.TemporaryDirectory() as td:
        tgt = os.path.join(td, f"{args.name}.target.o")
        assemble_target(args.name, tgt)
        cmd = ["objdiff-cli", "diff", "-1", tgt, "-2", ours]
        if args.dump:
            cmd += ["-o", "-", "--format", "json-pretty"]
        cmd.append(args.name)
        try:
            return subprocess.run(cmd).returncode
        except FileNotFoundError:
            sys.exit("objdiff: objdiff-cli not found — run inside `nix develop`.")


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("objdiff", target):
            sys.exit(main())
    except MatchToolBusy as e:
        sys.exit(f"objdiff: {e}")
