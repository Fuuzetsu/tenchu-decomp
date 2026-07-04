#!/usr/bin/env python3
"""Clone a matched function's C onto its byte-identical siblings.

Some functions compile to the EXACT same bytes (template-y disposers, stubs,
paired handlers). Byte-identical code references the same callees/data by the
same addresses, so the matching C is identical too — only the function's own
name changes. Given one matched function, this finds its exact byte-identical
UNMATCHED siblings and (with --write) writes each one's .c as the matched C with
the name substituted, wiring the splat carve / symbol / gp-externs and verifying
`./Build check` stays green.

  tools/clonematch.py <MatchedName>           list byte-identical siblings
  tools/clonematch.py <MatchedName> --write   generate + verify the clones

Only EXACT byte matches are cloned (safe); fuzzy siblings (coddog) need manual
parametrization. Run inside the nix devShell.
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
FILE_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
TSV = ".shake/ghidra-export/functions.tsv"
SRC = "src/main.exe"
GHIDRA_EXPORT = ".shake/ghidra-export"


def fbytes(a, s):
    data = open(ORIG, "rb").read()
    o = FILE_OFF + (a - TEXT_START)
    return data[o:o + s]


def functions():
    out = []
    for line in open(TSV):
        p = line.rstrip("\n").split("\t")
        if len(p) == 3 and re.match(r"^[A-Za-z_]\w*$", p[2]):
            a, s, n = int(p[0], 16), int(p[1]), p[2]
            if TEXT_START <= a < TEXT_END and s >= 8:
                out.append((a, s, n))
    return out


def is_matched(name):
    f = os.path.join(SRC, name + ".c")
    return os.path.exists(f) and not re.search(
        r"^\s*INCLUDE_ASM", open(f).read(), re.M)


def clone_c(src_name, dst_name, dst_addr):
    """The matched C with the function identifier substituted src->dst, and a
    fresh header (substitute FIRST so the header's `clone of <src>` survives)."""
    txt = open(os.path.join(SRC, src_name + ".c")).read()
    txt = re.sub(rf"\b{re.escape(src_name)}\b", dst_name, txt)
    header = (f"/* {dst_name} (0x{dst_addr:08x}) — byte-identical clone of "
              f"{src_name} (tools/clonematch.py). */")
    return re.sub(r"/\*.*?\*/", header, txt, count=1, flags=re.S)


def has_gp(name):
    return bool(re.search(rf'^    syms "{re.escape(name)}" = ',
                          open("shake/src/Build.hs").read(), re.M))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--write", action="store_true",
                    help="generate + verify the clone .c files")
    args = ap.parse_args()
    name = args.name

    if not is_matched(name):
        sys.exit(f"clonematch: {name} isn't matched (no real C in {SRC}) — clone "
                 f"FROM a matched function.")
    funcs = functions()
    me = next((f for f in funcs if f[2] == name), None)
    if not me:
        sys.exit(f"clonematch: {name} not in {TSV}")
    a, s, _ = me
    mybytes = fbytes(a, s)

    sibs = [(fa, fs, fn) for fa, fs, fn in funcs
            if fn != name and fs == s and fbytes(fa, fs) == mybytes]
    todo = [(fa, fs, fn) for fa, fs, fn in sibs if not is_matched(fn)]

    print(f"clonematch: {name} ({s} bytes) has {len(sibs)} exact byte-identical "
          f"sibling(s), {len(todo)} unmatched:")
    for fa, fs, fn in sibs:
        print(f"  {fn:32} {fa:#010x} {'[matched]' if is_matched(fn) else '[clone]'}")
    if not todo:
        print("  (nothing to clone)")
        return
    if not args.write:
        print("  run with --write to generate + verify the clones")
        return

    gp = has_gp(name)
    done = []
    for fa, fs, fn in todo:
        print(f"\nclonematch: cloning -> {fn}")
        # reverse.py adds the splat `c` carve + symbol + a stub; then overwrite
        # the stub with the name-substituted matched C.
        r = subprocess.run(["tools/reverse.py", fn, "--ghidra-export",
                            GHIDRA_EXPORT, "--no-check"],
                           capture_output=True, text=True)
        if r.returncode != 0:
            sys.exit(f"clonematch: reverse.py {fn} failed:\n{r.stdout}{r.stderr}")
        open(os.path.join(SRC, fn + ".c"), "w").write(clone_c(name, fn, fa))
        if gp:
            subprocess.run(["tools/gpsyms.py", fn, "--write"], check=True)
        done.append(fn)

    print("\nclonematch: verifying ./Build check…")
    subprocess.run(["./Build", "clean"], stdout=subprocess.DEVNULL)
    rc = subprocess.run(["./Build", "check"], stdout=subprocess.DEVNULL).returncode
    if rc == 0:
        print(f"clonematch: ✓ GREEN — cloned {len(done)}: {', '.join(done)}")
        print("            review the generated .c headers, then commit.")
    else:
        print(f"clonematch: ✗ ./Build check FAILED after cloning {done}. A sibling"
              f" may not be a true clone (self-call / data quirk); inspect with"
              f" tools/matchdiff.py.", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
