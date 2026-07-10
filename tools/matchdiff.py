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
import argparse, tempfile, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
FILE_TEXT_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
OURS = ".shake/build/tenchu/main.exe"
MAP = ".shake/build/tenchu/main.exe.map"
SYMBOLS = "config/symbols.main.exe.txt"
YAML = "config/splat.main.exe.yaml"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"


def linked_text_size(name):
    """Bytes the linker actually placed for <name>.c.o's .text, from the map.

    Independent of the byte comparison: if this disagrees with the carve extent,
    the DRAFT is the wrong length, full stop -- and everything after it in the
    image has shifted. Catches the case where a NON_MATCHING shadow build reports
    a spurious whole-image MATCH on a split/override function (matchdiff once
    printed `MATCH! 0 differing bytes` for a 456-byte draft in a 460-byte slot).
    Returns None if the map has no such line (e.g. a pure-asm function).
    """
    if not os.path.exists(MAP):
        return None
    obj = re.compile(
        r"^\s*\.text\s+0x[0-9a-fA-F]+\s+0x([0-9a-fA-F]+)\s+.*/" + re.escape(name) + r"\.c\.o\b")
    total = None
    for line in open(MAP, errors="replace"):
        m = obj.match(line)
        if m:
            total = (total or 0) + int(m.group(1), 16)
    return total


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


DIAGNOSIS = ("an env var exceeds MAX_ARG_STRLEN (128 KiB). Find it with:\n  env | awk -F= 'length($0) > 131072 {print $1, length($0)}'\nThen unset it. Root cause + fix: docs/build-system.md")

BUILD_LOG = os.path.join(tempfile.gettempdir(), "tenchu-matchdiff-build.log")


def run_build():
    """Run ./Build, streaming its output to a FILE.

    Never capture a build log into a pipe or a shell variable: a full build is
    ~780 KB of Verbose command echo, and holding it in memory buys nothing. The
    log stays on disk so a failure can be read afterwards.
    """
    with open(BUILD_LOG, "wb") as log:
        return subprocess.run(["./Build"], stdout=subprocess.DEVNULL, stderr=log)


def build_log_tail(n=15):
    try:
        with open(BUILD_LOG, "r", errors="replace") as f:
            return "\n".join(f.read().splitlines()[-n:])
    except OSError:
        return "(no build log)"


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
        r = run_build()
        if r.returncode != 0:
            tail = build_log_tail()
            if r.returncode in (126, 127) and "Argument list too long" in tail:
                # Not a build failure. execve() returned E2BIG because ONE env
                # string exceeds MAX_ARG_STRLEN (128 KiB) -- classically a
                # `out=$(./Build ...)` in a shell where nix exported `out`, so
                # the assignment rewrote the environment. The devShell now unsets
                # those names; if this fires, something re-introduced one.
                sys.exit("matchdiff: could not EXEC ./Build -- environment "
                         "failure, not a build failure.\n" + DIAGNOSIS)
            # Surface the real error. Swallowing this behind "run it directly"
            # once cost a long, wrong diagnosis: an exec failure and a compile
            # error looked identical from here.
            sys.exit(f"matchdiff: ./Build FAILED (rc={r.returncode}), "
                     f"log: {BUILD_LOG}\n{tail}")

    addr, size = symbol_slot(args.name)

    # Length cross-check, independent of the byte comparison. A draft that links
    # SHORTER than its carve slot shifts everything after it; on a split/override
    # function the windowed byte diff can still read as MATCH. The map does not lie.
    linked = linked_text_size(args.name)
    if linked is not None and linked != size:
        print(f"{args.name}: LENGTH MISMATCH — carve extent {size} bytes, but the "
              f"linker placed {linked} bytes of .text for {args.name}.c.o.")
        print("  The draft is the wrong length; everything after it has shifted.")
        print("  This is NOT a match, whatever the byte window below says.")
        return 1

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
