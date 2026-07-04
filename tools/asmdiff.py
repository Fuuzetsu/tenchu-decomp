#!/usr/bin/env python3
"""Aligned instruction diff of one function: target vs our build.

Complements tools/matchdiff.py for BIG or shifted functions: matchdiff
compares per-address (a one-insn insert makes everything after it "differ")
and sizes the window by the next config/symbols entry (mid-function labels
cap it). This tool takes the REAL size from the Ghidra export, disassembles
both images, and difflib-aligns the instruction sequences, so inserts/deletes
show as themselves and pure branch-target drift can be suppressed.

  tools/asmdiff.py <Name>              aligned diff (structural view)
  tools/asmdiff.py <Name> --all        include pure branch-target drift lines
  tools/asmdiff.py <Name> --structural only blocks that change the length
  tools/asmdiff.py <Name> -n           skip ./Build

Exit 0 when the sequences are identical. Run inside the nix devShell.
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
FILE_TEXT_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
OURS = ".shake/build/tenchu/main.exe"
SYMBOLS = "config/symbols.main.exe.txt"
TSV = ".shake/ghidra-export/functions.tsv"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"


def resolve(name):
    """(addr, size) — address from config/symbols, size from the Ghidra export."""
    addr = None
    for line in open(SYMBOLS):
        m = re.match(rf"{re.escape(name)}\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            addr = int(m.group(1), 16)
            break
    if addr is None:
        sys.exit(f"asmdiff: {name} not in {SYMBOLS}")
    size = None
    for line in open(TSV):
        p = line.rstrip("\n").split("\t")
        if len(p) == 3 and int(p[0], 16) == addr:
            size = int(p[1])
            break
    if size is None:
        sys.exit(f"asmdiff: no size for {name} @ {addr:#x} in {TSV}")
    return addr, size


def dis(path, addr, size):
    data = open(path, "rb").read()
    off = FILE_TEXT_OFF + (addr - TEXT_START)
    tmp = "/tmp/asmdiff.bin"
    with open(tmp, "wb") as f:
        f.write(data[off:off + size])
    out = subprocess.run(
        [OBJDUMP, "-D", "-b", "binary", "-m", "mips", "-EL",
         "--adjust-vma", hex(addr), tmp],
        capture_output=True, text=True).stdout
    r = []
    for line in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\t[0-9a-f]{8} \t(.*)", line)
        if m:
            r.append((int(m.group(1), 16), m.group(2).replace("\t", " ")))
    return r


def main():
    import difflib
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("-n", "--no-build", action="store_true")
    ap.add_argument("--all", action="store_true",
                    help="also show pure branch-target drift")
    ap.add_argument("--structural", action="store_true",
                    help="only show blocks that change the instruction count")
    args = ap.parse_args()

    if not args.no_build:
        # Build a NON_MATCHING partial's draft (not its trivially-matching stub).
        env = dict(os.environ)
        srcp = os.path.join("src/main.exe", args.name + ".c")
        if os.path.exists(srcp) and "ifndef NON_MATCHING" in open(srcp).read():
            env["NON_MATCHING"] = args.name
        r = subprocess.run(["./Build"], stdout=subprocess.DEVNULL,
                           stderr=subprocess.STDOUT, env=env)
        if r.returncode != 0:
            sys.exit("asmdiff: ./Build FAILED (run it directly for the error)")

    addr, size = resolve(args.name)
    tgt = dis(ORIG, addr, size)
    # our side may be longer/shorter: disassemble with slack, cut at last jr ra
    ours = dis(OURS, addr, size + 0x100)
    jrs = [i for i, (_, x) in enumerate(ours) if x.startswith("jr ra")
           and i + 1 >= (size // 4) - 0x40]
    oe = jrs[0] + 2 if jrs else len(ours)
    ours = ours[:oe]

    t = [x[1] for x in tgt]
    o = [x[1] for x in ours]

    def stem(x):
        return re.sub(r"0x[0-9a-f]+$", "", x)

    sm = difflib.SequenceMatcher(None, t, o, autojunk=False)
    nd = nb = 0
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            continue
        drift = (tag == "replace" and (i2 - i1) == (j2 - j1)
                 and all(stem(t[i1 + k]) == stem(o[j1 + k])
                         for k in range(i2 - i1)))
        if drift and not args.all:
            continue
        nd += max(i2 - i1, j2 - j1)
        nb += 1
        if args.structural and (i2 - i1) == (j2 - j1):
            continue
        print(f"--- {tag}")
        for k in range(i1, i2):
            print(f"  T {tgt[k][0]:#x}  {tgt[k][1]}")
        for k in range(j1, j2):
            print(f"  O {ours[k][0]:#x}  {ours[k][1]}")
    print(f"[{args.name}: {nd} differing lines in {nb} blocks; "
          f"length ours {len(o)} vs target {len(t)}]")
    return 0 if (nd == 0 and len(o) == len(t)) else 1


if __name__ == "__main__":
    sys.exit(main())
