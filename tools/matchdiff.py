#!/usr/bin/env python3
"""Byte-compare one function between our build and the original main.exe.

The core loop of matching work (docs/matching-cookbook.md): edit the C, run
this, watch the differing-byte count drop, read the per-instruction diff to
see what the compiler did differently. Run inside the nix devShell.

Usage:
  tools/matchdiff.py ProcItemKusuri            # ./Build first, then compare
  tools/matchdiff.py ProcItemKusuri -n         # skip the build (reuse last)
  tools/matchdiff.py ProcItemKusuri --max 60   # show up to 60 differing insns

The function's address comes from config/symbols.main.exe.txt; its size is the
distance to the next symbol (same slot logic as mkmod). Exit status: 0 on a
byte-match, 1 otherwise — usable as a gate in scripts.
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
FILE_TEXT_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
OURS = ".shake/build/tenchu/main.exe"
SYMBOLS = "config/symbols.main.exe.txt"
YAML = "config/splat.main.exe.yaml"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"


def is_carved(name):
    """True iff a `c` subsegment exists for `name`.

    An UN-carved function's bytes come from a raw `data` blob, so the built image
    matches the original at that address no matter what its .c says — this tool
    would happily print MATCH for a .c that is never even linked. Five functions
    sat in the tree as bogus "matches" that way.
    """
    pat = re.compile(rf"^\s+- \[0x[0-9A-Fa-f]+,\s*c,\s*{re.escape(name)}\]", re.M)
    return bool(pat.search(open(YAML).read()))


SUBSEG = re.compile(r"^\s+- (?:\[\s*0x([0-9A-Fa-f]+)\s*,\s*[\w.]+\s*(?:,\s*[\w.]+\s*)?\]"
                    r"|\{\s*start:\s*0x([0-9A-Fa-f]+)\b)", re.M)
TEXT_FOFF, TEXT_VADDR = 0x800, 0x80011000


def carve_extent(name):
    """(addr, size) from the splat carve, or None.

    Every game function has a `c` subsegment, so the distance to the NEXT
    subsegment is the function's true extent. The symbol gap is not: a `D_` label
    or a jump table sitting inside a function truncates it, and a short window is
    how `cd_open` once reported "0 differing bytes" while 4 bytes differed past
    its end. Two Ghidra `functions.tsv` sizes are also wrong (LoadCard,
    FUN_800593a0) -- the carve is right where the export lies.
    """
    y = open(YAML).read()
    offs = sorted(int(m.group(1) or m.group(2), 16) for m in SUBSEG.finditer(y))
    pat = re.compile(rf"^\s+- \[0x([0-9A-Fa-f]+),\s*c,\s*{re.escape(name)}\]", re.M)
    m = pat.search(y)
    if not m:
        return None
    off = int(m.group(1), 16)
    nxt = next((o for o in offs if o > off), None)
    if nxt is None:
        return None
    return off - TEXT_FOFF + TEXT_VADDR, nxt - off


def build_once():
    return subprocess.run(["./Build"], stdout=subprocess.DEVNULL,
                          stderr=subprocess.PIPE)


def symbol_slot(name):
    """(addr, size) for `name`. Prefer the splat carve; fall back to the symbol gap."""
    syms = {}
    for line in open(SYMBOLS):
        m = re.match(r"([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            syms[m.group(1)] = int(m.group(2), 16)
    if name not in syms:
        sys.exit(f"matchdiff: {name} not in {SYMBOLS}")
    addr = syms[name]
    carve = carve_extent(name)
    higher = [a for a in set(syms.values()) if a > addr]
    gap = (min(higher) - addr) if higher else None
    if carve and carve[0] == addr:
        if gap is not None and carve[1] > gap:
            print(f"matchdiff: window {gap} -> {carve[1]} bytes (a symbol inside "
                  f"{name} would have truncated it)", file=sys.stderr)
        return carve
    if gap is None:
        sys.exit(f"matchdiff: no symbol after {name} @ {addr:#x}; can't size it")
    return addr, gap


def disasm(data, off, size, base):
    """{vaddr: 'bytes  mnemonic operands'} for the window."""
    tmp = "/tmp/matchdiff.bin"
    with open(tmp, "wb") as f:
        f.write(data[off:off + size])
    out = subprocess.run(
        [OBJDUMP, "-D", "-b", "binary", "-m", "mips", "-EL",
         "--adjust-vma", hex(base), tmp],
        capture_output=True, text=True).stdout
    r = {}
    for line in out.splitlines():
        line = line.strip()
        if ":\t" in line:
            a, rest = line.split(":\t", 1)
            r[int(a, 16)] = rest.replace("\t", "  ")
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("-n", "--no-build", action="store_true",
                    help="skip ./Build, compare the existing build")
    ap.add_argument("--max", type=int, default=40,
                    help="max differing instructions to print")
    args = ap.parse_args()

    if not is_carved(args.name):
        sys.exit(f"matchdiff: {args.name} has no `c` subsegment in {YAML} — it is "
                 f"NOT carved, so its .c is never linked and this comparison would "
                 f"trivially 'MATCH' (the bytes come from the raw data blob). Run "
                 f"`tools/reverse.py {args.name}` first.")

    if not args.no_build:
        # A NON_MATCHING partial (INCLUDE_ASM stub XOR draft behind #ifndef
        # NON_MATCHING) trivially matches in the default build (the stub IS the
        # original bytes), so build ITS draft to compare what we're iterating on.
        env = dict(os.environ)
        srcp = os.path.join("src/main.exe", args.name + ".c")
        if os.path.exists(srcp) and "ifndef NON_MATCHING" in open(srcp).read():
            env["NON_MATCHING"] = args.name
        r = build_once()
        if r.returncode in (126, 127) and b"Argument list too long" in r.stderr:
            # Not a build failure. execve() can return E2BIG when the kernel
            # fails to populate the new process's argument stack -- verified with
            # strace: 2 argv entries, 153 env vars, ~24 KB total. It is flaky and
            # follows the shell having just materialised a large buffer (e.g. a
            # captured build log). Retry once rather than report a phantom break.
            print("matchdiff: spurious exec failure (E2BIG); retrying once",
                  file=sys.stderr)
            r = build_once()
        if r.returncode != 0:
            err = r.stderr.decode(errors="replace").strip()
            # Surface it. Swallowing this cost a long, wrong "it's just load"
            # diagnosis once: an exec failure and a real compile error looked the
            # same from here.
            sys.exit(f"matchdiff: ./Build FAILED (rc={r.returncode})\n"
                     + "\n".join(err.splitlines()[-15:]))

    addr, size = symbol_slot(args.name)
    off = FILE_TEXT_OFF + (addr - TEXT_START)
    orig = open(ORIG, "rb").read()
    ours = open(OURS, "rb").read()

    diffs = [i for i in range(off, off + size) if orig[i] != ours[i]]
    total = sum(1 for a, b in zip(orig, ours) if a != b) + abs(len(orig) - len(ours))
    print(f"{args.name} @ {addr:#x} ({size} bytes): "
          f"{len(diffs)} differing bytes ({total} in the whole image)")
    if total > len(diffs):
        print("NOTE: diffs beyond the function window — if your function is a"
              " different LENGTH, everything after it shifts (symbols drift too).")
    if not diffs:
        print("MATCH!" if total == 0 else "window matches but the image does not!")
        return 0 if total == 0 else 1

    o_dis = disasm(orig, off, size, addr)
    m_dis = disasm(ours, off, size, addr)
    bad_insns = sorted({addr + ((i - off) & ~3) for i in diffs})
    print(f"{'addr':10} {'TARGET':48} OURS")
    for i, a in enumerate(bad_insns):
        if i >= args.max:
            print(f"... (+{len(bad_insns) - i} more)")
            break
        print(f"{a:#x} {o_dis.get(a, '?'):48} {m_dis.get(a, '?')}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
