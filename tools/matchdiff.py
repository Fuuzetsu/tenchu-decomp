#!/usr/bin/env python3
"""Byte-compare one function between our build and the original main.exe.

The core loop of matching work (docs/matching-cookbook.md): edit the C, run
this, watch the differing-byte count drop, read the per-instruction diff to
see what the compiler did differently. Run inside the nix devShell.

Usage:
  tools/matchdiff.py ProcItemKusuri            # ./Build first, then compare
  tools/matchdiff.py ProcItemKusuri -n         # skip the build (reuse last)
  tools/matchdiff.py ProcItemKusuri --max 60   # show up to 60 differing insns
  tools/matchdiff.py ProcItemKusuri --clusters # per-cluster INSNS *and* BYTES

The function's address comes from config/symbols.main.exe.txt; its size is the
distance to the next symbol (same slot logic as mkmod). Exit status: 0 on a
byte-match, 1 otherwise — usable as a gate in scripts.

This tool is the byte GATE and aligns by ADDRESS. That makes it the authority on
"how many bytes differ" and a poor witness for "what moved": a pure displacement
renders as a clean one-slot shift, and an adjacent swap can sit inside a region
this reports as identical (FUN_8003944c's `sh 24`/`sh 26` swap hid exactly there,
and the lane found it only by dumping the raw target `.s`). For ORDER questions use
`tools/asmdiff.py`, which aligns by CONTENT and flags MOVED instructions.
"""
import argparse, tempfile, os, re, subprocess, sys

from matchlock import MatchToolBusy, matching_tool_lock

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


def source_completion_blockers(path):
    """Asm/guard constructs that make a byte-exact draft incomplete."""
    if not os.path.exists(path):
        return []
    with open(path, errors="replace") as stream:
        text = stream.read()
    code = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    code = re.sub(r"//[^\n]*", "", code)
    blockers = []
    if re.search(r"^\s*#\s*ifndef\s+NON_MATCHING\b", code, re.M):
        blockers.append("NON_MATCHING guard")
    if re.search(r"^\s*INCLUDE_ASM\s*\(", code, re.M):
        blockers.append("INCLUDE_ASM fallback")
    if re.search(r"\b__asm__\b", code):
        blockers.append("inline __asm__")
    return blockers


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


def linked_nonmatching_stub(name):
    """Whether the current map links `<name>.NON_MATCHING` from INCLUDE_ASM.

    Guarded drafts deliberately give their default asm body this label.  It is
    a stronger artifact check than inspecting timestamps or generated source:
    a concurrent/default build can replace the candidate after `-n` was chosen,
    but the final linked map records what actually supplied the bytes.
    """
    if not os.path.exists(MAP):
        return False
    marker = re.compile(r"\b" + re.escape(name) + r"\.NON_MATCHING\b")
    with open(MAP, errors="replace") as stream:
        return any(marker.search(line) for line in stream)


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


def run_build(env=None):
    """Run ./Build, streaming its output to a FILE.

    `env` MUST be passed through: for a `#ifndef NON_MATCHING` file the caller sets
    NON_MATCHING=<name> so ./Build compiles the DRAFT, not the trivially-matching
    INCLUDE_ASM stub. This parameter was missing for a long time, so matchdiff
    silently built the stub and reported a false MATCH for every guarded file.

    Never capture a build log into a pipe or a shell variable: a full build is
    ~780 KB of Verbose command echo, and holding it in memory buys nothing. The
    log stays on disk so a failure can be read afterwards.
    """
    with open(BUILD_LOG, "wb") as log:
        return subprocess.run(["./Build"], stdout=subprocess.DEVNULL, stderr=log,
                              env=env if env is not None else os.environ)


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




def _mnemonic(text):
    """The mnemonic from disasm()'s "<hexword>   <mn>  <operands>" rendering.

    NOT `text.split()[0]` -- that is the 8-hex-digit ENCODING, which differs for
    every instruction in the diff by definition, so classifying on it labels
    everything "opcode". (Caught by fault-checking against a 1-insn/2-byte cluster:
    a 2-byte diff cannot be an opcode change.)
    """
    parts = text.split()
    if parts and len(parts[0]) == 8 and all(c in "0123456789abcdef" for c in parts[0]):
        parts = parts[1:]
    return parts[0] if parts else "?"


def _no_target(text):
    """Strip BOTH the leading hex encoding and the trailing target address.

    The encoding must go too: it differs for every instruction in the diff by
    definition, so comparing with it left in made `branch-retarget` unfirable — the
    identical hex-prefix bug as `_mnemonic`, missed when that one was fixed.
    """
    parts = text.split()
    if parts and len(parts[0]) == 8 and all(c in "0123456789abcdef" for c in parts[0]):
        text = " ".join(parts[1:])
    return re.sub(r"0x[0-9a-f]+\s*$", "", text).strip()


def report_clusters(name, bad_insns, diffs, off, addr, o_dis, m_dis, gap):
    """Group the residual into clusters, reporting INSNS *and* BYTES for each.

    Exists because every round byte-accounts by hand and two failure modes recur:
      * **units**: a cluster table was carried through THREE briefs as BYTES when its
        numbers were INSTRUCTIONS (~3x off — a "16-byte" cluster was really 63). Both
        units are printed here, always, with the header saying which is which.
      * **truncation**: `--max 40` silently hid rows and a lane built a byte-account
        from what remained (12 clusters/160B; the truth was 23/239). This path never
        truncates.

    A cluster is a run of differing instructions separated by at most `gap` matching
    ones. That is a HEURISTIC about causes, not a partition of the residual — fixing
    one allocation has collapsed three "separate" clusters at once — so the header
    says so rather than letting the table imply independence.
    """
    if not bad_insns:
        print(f"{name}: no differing instructions")
        return 0
    groups, cur = [], [bad_insns[0]]
    for a in bad_insns[1:]:
        if (a - cur[-1]) // 4 - 1 > gap:
            groups.append(cur)
            cur = [a]
        else:
            cur.append(a)
    groups.append(cur)

    byte_addrs = {addr + (i - off) for i in diffs}
    print(f"{name}: {len(groups)} clusters, {len(bad_insns)} differing INSNS, "
          f"{len(diffs)} differing BYTES")
    print("  (INSNS and BYTES are different units — an insn-vs-byte mix-up survived "
          "three briefs.")
    print("   A register-field-only diff is 1 byte in a 4-byte insn. Clusters are a "
          "HYPOTHESIS about")
    print("   causes, NOT a partition: fixing one allocation has collapsed three "
          "'separate' clusters.)")
    print()
    print(f"  {'#':>3} {'range':>21} {'insns':>6} {'bytes':>6}  kind")
    kinds_seen = []
    for i, g in enumerate(groups):
        lo, hi = g[0], g[-1]
        nb = sum(1 for b in byte_addrs if lo <= b < hi + 4)
        kinds = set()
        for a in g:
            tgt, ours_ = o_dis.get(a, "?"), m_dis.get(a, "?")
            if tgt == "?" or ours_ == "?":
                kinds.add("absent")
                continue
            # disasm() yields "<hexword>   <mnemonic>  <operands>", so the FIRST
            # field is the encoding, not the opcode. Splitting on [0] made every
            # differing insn look like an opcode change -- the hex ALWAYS differs,
            # that is why it is in the diff. Classify on the MNEMONIC.
            tm, om = _mnemonic(tgt), _mnemonic(ours_)
            if tm != om:
                kinds.add("opcode")
            elif _no_target(tgt) == _no_target(ours_):
                kinds.add("branch-retarget")
            else:
                kinds.add("operands")
        kind = "+".join(sorted(kinds))
        kinds_seen.append(kind)
        print(f"  {i:>3} {lo:#010x}..{hi + 4:#010x} {len(g):>6} {nb:>6}  {kind}")
    # Only say it when it is TRUE. This was an unconditional legend, and it made the
    # word `branch-retarget` appear in output that had none -- an orchestrator grep
    # matched the LEGEND and briefed two lanes to "start at the branch-retarget" on a
    # function whose clusters are all operands/opcode. The lane's words: "that
    # paragraph is matchdiff's static footer legend... the brief read the boilerplate
    # as a finding." A legend that reads like a result IS a result to anything that
    # greps.
    if any("branch-retarget" in k for k in kinds_seen):
        print()
        print("  `branch-retarget` above = same opcode, DIFFERENT target — the "
              "signature of a")
        print("  RELOCATED BLOCK. asmdiff HIDES those by default (it normalises the "
              "address);")
        print("  one was 44 of 48 bytes on DrawModelArchive, and its park never saw "
              "it.")
    return 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("-n", "--no-build", action="store_true",
                    help="skip ./Build, compare the existing build")
    ap.add_argument("--clusters", nargs="?", type=int, const=3, default=None,
                    metavar="GAP",
                    help="group the residual into clusters and report INSNS and BYTES "
                         "for each (GAP = max matching insns inside one cluster, "
                         "default 3). Byte-accounting is mandated every round and "
                         "hand-rolled every round; an insn-vs-byte mix-up survived "
                         "THREE briefs (a '16-byte' cluster was really 63).")
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
        r = run_build(env)
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

    srcp = os.path.join("src/main.exe", args.name + ".c")
    guarded = os.path.exists(srcp) and "ifndef NON_MATCHING" in open(srcp).read()
    if guarded and linked_nonmatching_stub(args.name):
        sys.exit(
            f"matchdiff: current image links {args.name}.NON_MATCHING — the "
            "INCLUDE_ASM stub, not the guarded C draft. This byte match would "
            "be trivial/stale. Rebuild the draft without -n (and ensure no "
            "concurrent plain Build replaces it)."
        )

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
        if total != 0:
            print("window matches but the image does not!")
            return 1
        blockers = source_completion_blockers(srcp)
        if blockers:
            print("C DRAFT BYTE-MATCHES, BUT IS NOT COMPLETE: " +
                  ", ".join(blockers) + ".")
            print("  Remove the guard/fallback or inline assembly, rebuild the "
                  "plain C, and verify again before committing it as matched.")
            return 1
        print("MATCH!")
        return 0

    o_dis = disasm(orig, off, size, addr)
    m_dis = disasm(ours, off, size, addr)
    bad_insns = sorted({addr + ((i - off) & ~3) for i in diffs})

    if args.clusters is not None:
        return report_clusters(args.name, bad_insns, diffs, off, addr,
                               o_dis, m_dis, args.clusters)

    print(f"{'addr':10} {'TARGET':48} OURS")
    for i, a in enumerate(bad_insns):
        if i >= args.max:
            # Say the CONSEQUENCE, not just the count. A lane read "+N more" and
            # still built a byte-account from the visible rows -- 12 clusters/160B
            # where the truth was 23/239 -- and only caught it via --max 400.
            hidden = len(bad_insns) - i
            print(f"... (+{hidden} more differing instructions HIDDEN — this view "
                  f"is TRUNCATED at --max {args.max})")
            print(f"    A byte-account or cluster count from this output will be "
                  f"WRONG. Re-run with --max {len(bad_insns)}.")
            break
        print(f"{a:#x} {o_dis.get(a, '?'):48} {m_dis.get(a, '?')}")
    return 1


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("matchdiff", target):
            sys.exit(main())
    except MatchToolBusy as e:
        sys.exit(f"matchdiff: {e}")
