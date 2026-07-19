#!/usr/bin/env python3
"""Did my edit change the CODEGEN at all, or did cc1 fold it away?

The question that separates "my edit did nothing" from "my edit did something and
the something was wrong" — and it has cost four separate lanes real time:

* One spent a detour hand-rolling `sha256sum` + revert + rebuild to discover that
  `s32 xPos; xPos = 0x66; ->x = xPos;` at four sites produced a BYTE-IDENTICAL
  object: cse1 folds the constant into the store and the seed dies before loop.c
  ever sees it. The edit was a literal no-op, and the round's model was built on it
  doing something.
* Three mis-diagnosed a fast `./Build` as a stale/skipped build. It is not: the
  build-profile cc1 compiles a TU in ~0.1 s, and Shake keys on CONTENT, so a codegen-neutral
  edit legitimately produces an identical `.o` and skips the relink. **Fast != skipped.
  The reliable tell is the object, not the wall clock**, and this tool is that tell.

  tools/nullcheck.py <Name>              working tree vs HEAD
  tools/nullcheck.py <Name> --against <ref>    vs any git ref (e.g. master, a SHA)

Exit 0 when the codegen CHANGED, 1 when your edit was a no-op — so it reads
naturally in a loop: `nullcheck.py X && matchdiff.py X`.

It compares cc1's `.s` output, which IS the codegen: maspsx and the assembler are
deterministic functions of it. Both variants compile through the identical path, so
nothing filename-derived can leak into the comparison. Run inside the nix devShell.
"""
import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

import rtldump

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)


def digest(path):
    with open(path, "rb") as stream:
        return hashlib.sha256(stream.read()).hexdigest()


def codegen_digest(name, src, draft, workdir):
    """sha256 of cc1's .s for `src`, compiled exactly as the build would.

    Compiles via rtldump's backend with NO dump passes and no -g: the plain
    assembly is what maspsx/as turn into the object, so identical .s means an
    identical .o.
    """
    staged = os.path.join(workdir, name + ".c")
    shutil.copyfile(src, staged)
    result = rtldump.compile_rtl(name, passes=[], draft=draft, src=staged)
    return digest(result["asm"]), result["asm"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--against", default="HEAD",
                    help="git ref to compare against (default: HEAD)")
    args = ap.parse_args()
    name = args.name

    path = os.path.join("src", "main.exe", name + ".c")
    if not os.path.exists(path):
        sys.exit(f"nullcheck: {path} not found")
    with open(path, errors="replace") as stream:
        draft = "ifndef NON_MATCHING" in stream.read()

    show = subprocess.run(["git", "show", f"{args.against}:{path}"],
                          capture_output=True, text=True)
    if show.returncode:
        sys.exit(f"nullcheck: cannot read {path} at {args.against}:\n{show.stderr}")

    with tempfile.TemporaryDirectory() as work:
        baseline_src = os.path.join(work, "baseline.c")
        with open(baseline_src, "w") as stream:
            stream.write(show.stdout)
        # Same name, same flags, same path shape for both -- only the text differs.
        mine, mine_asm = codegen_digest(name, path, draft, work)
        theirs, _ = codegen_digest(name, baseline_src, draft, work)

    kind = "guarded draft" if draft else "plain C"
    print(f"nullcheck: {name} ({kind}) — working tree vs {args.against}")
    print(f"  {args.against:>12}: {theirs}")
    print(f"  working tree: {mine}")
    if mine == theirs:
        print()
        print("nullcheck: *** NO-OP — your edit produced IDENTICAL codegen. ***")
        print("  cc1 folded it away before it could matter (cse1 folds a constant "
              "into its")
        print("  store and the seed dies before loop.c; a dead probe never reaches "
              "loop.c at")
        print("  all — `delete_trivially_dead_insns` iterates). Do not build a model "
              "on this")
        print("  edit doing something. See docs/matching-cookbook.md.")
        return 1
    print()
    print("nullcheck: codegen CHANGED — the edit is real. Measure it with "
          f"tools/matchdiff.py {name}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
