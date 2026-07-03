#!/usr/bin/env python3
"""Run decomp-permuter on a split function to find the C that byte-matches.

For a function whose src/main.exe/<name>.c is close-but-not-matching (correct
arithmetic, wrong register allocation / schedule — see get_held_buttons/GetRealPad),
this sets up a permuter working dir and runs it:

  * compile.sh   = this repo's pipeline (cc1 -G8 | maspsx | as) on preprocessed C
  * base.c       = the preprocessed src/main.exe/<name>.c (what gets perturbed)
  * target.o     = the original function assembled from its nonmatching .s

On score 0 the winning source is written under the work dir; drop it into
src/main.exe/<name>.c and confirm with `./Build check`.

Usage (in the nix devShell):  tools/permute.py <name> [-- <permuter args>]
e.g.  tools/permute.py GetRealPad --stop-on-zero -j4
"""
import argparse, glob, os, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# Kept in sync with shake/src/Build.hs.
CPP = ("mipsel-unknown-linux-gnu-cpp -Iinclude -undef -Wall -lang-c -fno-builtin "
       "-gstabs -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips -Dpsx "
       "-D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C "
       "-DLANGUAGE_C -DHACKS -I src/main.exe").split()
CC_FLAGS = ("-mcpu=3000 -quiet -G8 -w -O2 -funsigned-char -fpeephole -ffunction-cse "
            "-fpcc-struct-return -fcommon -fverbose-asm -fgnu-linker -mgas "
            "-msoft-float").split()
AS_FLAGS = ("-EL -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections -O1 -G0").split()

# Per-function `--gp-extern SYM` maspsx flags — MUST mirror maspsxGpExterns in
# shake/src/Build.hs (ASPSX gp-addresses only TU-local definitions; these are the
# small globals the function's ORIGINAL translation unit defined).
GP_EXTERNS = {
    "Think1sleep": ["Me_THINK_C", "SR", "Attrib", "FRAMES_UNTIL_END_OF_ALERT"],
    "ReqItemDrop": ["COUNTER_FOR_ITEM_ARRAY_"],
}

COMPILE_SH = r"""#!/usr/bin/env bash
# permuter compile: preprocessed C -> object, via this repo's cc1|maspsx|as.
set -e
IN=$(realpath "$1"); OUT=$(realpath -m "$3")
cd {root}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cc1-281 {ccflags} < "$IN" > "$TMP/a.s"
maspsx --aspsx-version=2.77 -G8{gpexterns} < "$TMP/a.s" > "$TMP/b.s"
mipsel-unknown-linux-gnu-as {asflags} -o "$OUT" "$TMP/b.s"
"""


def find_nonmatching_s(name):
    hits = glob.glob(f".shake/gen/main.exe/asm/nonmatchings/*/{name}.s")
    if not hits:
        sys.exit(f"permute: {name}.s not found under .shake/gen — is {name} split "
                 f"and built? run `./Build` (or split it with tools/reverse.py).")
    return hits[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("rest", nargs=argparse.REMAINDER,
                    help="args after `--` are passed to permuter.py")
    args = ap.parse_args()
    name = args.name
    src = f"src/main.exe/{name}.c"
    if not os.path.exists(src):
        sys.exit(f"permute: {src} not found")

    subprocess.run(["./Build"], stdout=subprocess.DEVNULL, check=False)
    target_s = find_nonmatching_s(name)

    work = os.path.join(".shake", "permuter", name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)

    # compile.sh
    csh = os.path.join(work, "compile.sh")
    gpx = "".join(f" --gp-extern {s}" for s in GP_EXTERNS.get(name, []))
    open(csh, "w").write(COMPILE_SH.format(
        root=ROOT, ccflags=" ".join(CC_FLAGS), asflags=" ".join(AS_FLAGS),
        gpexterns=gpx))
    os.chmod(csh, 0o755)

    # base.c = preprocessed src (what the permuter perturbs)
    base = os.path.join(work, "base.c")
    subprocess.run(CPP + ["-DPERMUTER", src, base], check=True)

    # target.o = the original function, assembled from its nonmatching .s
    tgt_s = os.path.join(work, "target.s")
    open(tgt_s, "w").write('.include "macro.inc"\n' + open(target_s).read())
    subprocess.run(["mipsel-unknown-linux-gnu-as", *AS_FLAGS,
                    "-o", os.path.join(work, "target.o"), tgt_s], check=True)

    # sanity: base.c must compile
    subprocess.run([csh, base, "-o", os.path.join(work, "base.o")], check=True)

    print(f"permute: set up {work} — running permuter…")
    rest = args.rest[1:] if args.rest and args.rest[0] == "--" else args.rest
    if not rest:
        rest = ["--stop-on-zero", "-j4"]
    rc = subprocess.run(["permuter.py", work, *rest]).returncode

    wins = sorted(glob.glob(os.path.join(work, "output-0-*", "source.c")))
    if wins:
        print(f"\npermute: ★ byte-match found → {wins[0]}\n"
              f"         clean it up into {src}, then `./Build check`.")
    sys.exit(rc)


if __name__ == "__main__":
    main()
