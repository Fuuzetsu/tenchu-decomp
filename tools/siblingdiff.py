#!/usr/bin/env python3
"""Draft from a matched sibling, or compare retail with the named demo body.

The single most decisive move when drafting a function in a family (the ProcItem*,
Act*, Draw*, Think* twins) is: find the already-matched sibling whose asm is the
same shape, read its C, and change only the instructions that differ. Agents were
doing this by hand — `findsimilar` to pick the sibling, then eyeballing two `.s`
files. This does all of it in one shot:

    tools/siblingdiff.py <Target>            # auto-pick the nearest matched sibling
    tools/siblingdiff.py <Target> <Sibling>  # force a specific sibling
    tools/siblingdiff.py <Target> --demo     # same-named Japanese demo function
    tools/siblingdiff.py <Target> --full     # whole aligned listing, not just diffs
    tools/siblingdiff.py <Target> --no-c     # skip printing the sibling's C

It disassembles BOTH functions straight from the original image (same objdump pass
and function list as findsimilar), normalizes intra-function branch/jump targets to
labels relative to the function start (so the same control-flow shape at two
different addresses ALIGNS instead of reading as all-different), then prints:
  1. the ranked sibling candidates (so you can retry with a different one),
  2. a compact SequenceMatcher diff — `ref:` = sibling, `tgt:` = target — with runs
     of identical instructions collapsed to `… N identical`,
  3. the sibling's matched C (src/main.exe/<Sibling>.c) verbatim, ready to transcribe.

In ordinary sibling mode, inter-function `jal`/`j` targets are left absolute: both
bodies came from retail, so the same callee already aligns. In `--demo` mode those
targets are normalized to function names because the earlier executable was linked
at different addresses. Only a same-named body from the committed demo inventory is
accepted. When a named demo call disappeared in retail, the tool also checks whether
the retail target contains most of that callee's mnemonic bigrams. A strong hit is an
inline-expansion hint: reconstruct a local static-inline call before hand-copying the
callee body. Register allocation and real demo/retail edits still show through — the
demo is a structural source oracle, never the exact-byte gate. Run inside the nix
devShell.
"""
from __future__ import annotations

import argparse
import difflib
import os
import re
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
FILE_TEXT_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
TSV = ".shake/ghidra-export/functions.tsv"
DEMO_ORIG = "disks/demo/PSX.EXE"
DEMO_TSV = "reference/demo-psxexe.functions.tsv"
SYMBOLS = "config/symbols.main.exe.txt"
SRC = "src/main.exe"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"
N = 4  # mnemonic n-gram size, matches findsimilar

BRANCH = re.compile(r"^(b|beq|bne|beqz|bnez|bgez|bgtz|blez|bltz|bgezal|bltzal"
                    r"|bc1t|bc1f|j)\w*$")
TEXTADDR = re.compile(r"\b(?:0x)?(80[0-9a-f]{6})\b")  # objdump prefixes targets 0x


def load_functions():
    """[(addr, size, name)] inside text, real-name-preferred (mirrors findsimilar)."""
    symname = {}
    for line in open(SYMBOLS):
        m = re.match(r"([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            symname[int(m.group(2), 16)] = m.group(1)
    out = []
    for line in open(TSV):
        parts = line.rstrip("\n").split("\t")
        if len(parts) == 3:
            addr, size, name = int(parts[0], 16), int(parts[1]), parts[2]
            if TEXT_START <= addr < TEXT_END and size >= 8:
                out.append((addr, size, symname.get(addr, name)))
    return out


def load_demo_functions(path=DEMO_TSV):
    """[(addr, size, name)] from either supported demo TSV spelling."""
    out = []
    for line in open(path):
        parts = line.rstrip("\n").split("\t")
        if len(parts) >= 4 and parts[0] == "F":
            parts = parts[1:]
        if len(parts) < 3:
            continue
        try:
            addr, size = int(parts[0], 16), int(parts[1])
        except ValueError:
            continue
        if size >= 8 and size % 4 == 0:
            out.append((addr, size, parts[2]))
    return out


def psx_text_start(path):
    """Read the load address from a PS-X EXE header."""
    with open(path, "rb") as fh:
        header = fh.read(FILE_TEXT_OFF)
    if len(header) < 0x20 or header[:8] != b"PS-X EXE":
        raise ValueError(f"{path}: not a PS-X EXE")
    return struct.unpack_from("<I", header, 0x18)[0]


def matched_names():
    """Functions with real C (no INCLUDE_ASM) in src/main.exe/."""
    names = set()
    for f in os.listdir(SRC):
        if f.endswith(".c") and not re.search(
                r"^\s*INCLUDE_ASM", open(os.path.join(SRC, f)).read(), re.M):
            names.add(f[:-2])
    return names


def all_insns(orig=ORIG, text_start=TEXT_START):
    """{addr: (mnemonic, operands)} for the whole text segment, one objdump pass."""
    out = subprocess.run(
        [OBJDUMP, "-D", "-z", "-b", "binary", "-m", "mips", "-EL",
         "--adjust-vma", hex(text_start - FILE_TEXT_OFF), orig],
        capture_output=True, text=True).stdout
    insns = {}
    pat = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]{8} \t(\S+)(?:\s+(.*))?$")
    for line in out.splitlines():
        m = pat.match(line)
        if m:
            a = int(m.group(1), 16)
            if a >= text_start:
                insns[a] = (m.group(2), (m.group(3) or "").strip())
    return insns


def ngrams(insns, addr, size):
    seq = [insns.get(a, ("?", ""))[0] for a in range(addr, addr + size, 4)]
    return {tuple(seq[i:i + N]) for i in range(len(seq) - N + 1)}


def jaccard(a, b):
    return len(a & b) / len(a | b) if a and b else 0.0


def norm_line(insns, a, start, end, target_names=None):
    """One normalized instruction string; intra-window targets -> L<off>."""
    mnem, ops = insns.get(a, ("?", ""))

    def repl(m):
        t = int(m.group(1), 16)
        if BRANCH.match(mnem) and start <= t < end:
            return f"L{t - start:x}"          # relative label, aligns across addrs
        if BRANCH.match(mnem) and target_names and t in target_names:
            return target_names[t]            # cross-build call/jump -> shared name
        return m.group(0)                      # inter-function target: keep absolute

    return f"{mnem} {TEXTADDR.sub(repl, ops)}".rstrip()


def disasm(insns, addr, size, target_names=None):
    end = addr + size
    return [norm_line(insns, a, addr, end, target_names)
            for a in range(addr, end, 4)]


def call_counts(seq):
    """Named direct calls in a normalized disassembly sequence."""
    out = {}
    for line in seq:
        m = re.match(r"^jal ([A-Za-z_$][\w$]*)$", line)
        if m:
            name = m.group(1)
            out[name] = out.get(name, 0) + 1
    return out


def mnemonic_bigrams(seq):
    """Multiset of adjacent mnemonic pairs, retaining repeated helper loops."""
    mnems = [line.split(None, 1)[0] for line in seq]
    out = {}
    for i in range(len(mnems) - 1):
        pair = (mnems[i], mnems[i + 1])
        out[pair] = out.get(pair, 0) + 1
    return out


def inline_hints(ref, tgt, helpers, min_hits=6, min_coverage=0.60):
    """Find demo-call deficits whose retail helper body occurs inside target.

    ``helpers`` maps a retail callee name to its normalized disassembly.  The
    comparison deliberately uses mnemonic bigrams: inline expansion changes hard
    registers, stack offsets, and local labels, while retaining enough adjacent
    operation shape to distinguish a real expanded body from an ordinary missing
    call.  The result is a drafting hint, never a match claim.
    """
    ref_calls = call_counts(ref)
    tgt_calls = call_counts(tgt)
    rows = []
    for name, ref_count in sorted(ref_calls.items()):
        deficit = ref_count - tgt_calls.get(name, 0)
        helper = helpers.get(name)
        if deficit <= 0 or not helper:
            continue
        helper_pairs = mnemonic_bigrams(helper)
        total = sum(helper_pairs.values())
        width = min(len(helper), len(tgt))
        best = (0, 0)
        for start in range(max(1, len(tgt) - width + 1)):
            target_pairs = mnemonic_bigrams(tgt[start:start + width])
            hits = sum(min(n, target_pairs.get(pair, 0))
                       for pair, n in helper_pairs.items())
            best = max(best, (hits, -start))
        hits, neg_start = best
        start = -neg_start
        coverage = hits / total if total else 0.0
        if hits >= min_hits and coverage >= min_coverage:
            rows.append((name, deficit, hits, total, coverage, start))
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("target")
    ap.add_argument("sibling", nargs="?", help="force this matched sibling")
    ap.add_argument("--demo", action="store_true",
                    help="compare with the same-named function in demo PSX.EXE")
    ap.add_argument("--top", type=int, default=5, help="sibling candidates to list")
    ap.add_argument("--full", action="store_true", help="whole aligned listing")
    ap.add_argument("--context", type=int, default=2, help="context insns per hunk")
    ap.add_argument("--no-c", action="store_true", help="don't print the sibling C")
    args = ap.parse_args()

    funcs = load_functions()
    byname = {n: (a, s) for a, s, n in funcs}
    if args.target not in byname:
        sys.exit(f"siblingdiff: {args.target} not in {TSV}")
    ta, tsz = byname[args.target]
    insns = all_insns()

    if args.demo:
        if args.sibling:
            sys.exit("siblingdiff: --demo does not take a sibling argument")
        demo_funcs = load_demo_functions()
        demos = [(a, s) for a, s, n in demo_funcs if n == args.target]
        if len(demos) != 1:
            sys.exit(f"siblingdiff: expected one demo {args.target}, found {len(demos)}")
        sa, ssz = demos[0]
        demo_start = psx_text_start(DEMO_ORIG)
        ref_insns = all_insns(DEMO_ORIG, demo_start)
        retail_names = {a: n for a, _, n in funcs}
        demo_names = {a: n for a, _, n in demo_funcs}
        ref = disasm(ref_insns, sa, ssz, demo_names)
        tgt = disasm(insns, ta, tsz, retail_names)
        print(f"target {args.target}  retail {ta:08x} ({tsz} bytes)")
        print(f"demo   {args.target}  demo   {sa:08x} ({ssz} bytes)")
        print("source: disks/demo/PSX.EXE + reference/demo-psxexe.functions.tsv\n")
        demo_calls = call_counts(ref)
        helper_seqs = {
            name: disasm(insns, *byname[name], retail_names)
            for name in demo_calls if name in byname
        }
        hints = inline_hints(ref, tgt, helper_seqs)
        if hints:
            print("inline-expansion hints (demo calls missing from retail):")
            for name, deficit, hits, total, coverage, start in hints:
                call_word = "call" if deficit == 1 else "calls"
                print(f"  {name}: {deficit} missing demo {call_word}; "
                      f"retail+0x{start * 4:x} window contains {hits}/{total} "
                      f"helper mnemonic bigrams ({coverage:.0%})")
            print("  hint only: confirm the target islands, then try a local "
                  "static-inline helper\n")
        ref_kind = "demo"
        sib = None
    else:
        matched = matched_names() & set(byname)
        matched.discard(args.target)
        if not matched:
            sys.exit("siblingdiff: no matched functions to compare against")
        tg = ngrams(insns, ta, tsz)
        ranked = sorted(
            ((jaccard(tg, ngrams(insns, *byname[m])), m) for m in matched),
            reverse=True)
        if args.sibling:
            if args.sibling not in matched:
                sys.exit(f"siblingdiff: {args.sibling} is not a matched sibling")
            sib = args.sibling
        else:
            sib = ranked[0][1]
        sa, ssz = byname[sib]
        print(f"target  {args.target}  ({tsz} bytes)")
        print(f"sibling {sib}  ({ssz} bytes)  "
              f"sim {dict((m, j) for j, m in ranked)[sib]:.2f}"
              f"  -> src/main.exe/{sib}.c")
        print("candidates: " + ", ".join(
            f"{m}({j:.2f})" for j, m in ranked[:args.top]))
        print()
        ref = disasm(insns, sa, ssz)
        tgt = disasm(insns, ta, tsz)
        ref_kind = "sibling"
    sm = difflib.SequenceMatcher(a=ref, b=tgt, autojunk=False)
    same = sum(b - a for tag, a, b, _, _ in sm.get_opcodes() if tag == "equal")
    print(f"asm: {same}/{max(len(ref), len(tgt))} instructions identical after "
          f"normalization  (ratio {sm.ratio():.2f})")
    print(f"  ref: = {ref_kind}   tgt: = retail target   "
          "Lxx = intra-function label\n")

    if args.full:
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == "equal":
                for k in range(i1, i2):
                    print(f"    {ref[k]}")
            else:
                for k in range(i1, i2):
                    print(f"  ref: {ref[k]}")
                for k in range(j1, j2):
                    print(f"  tgt: {tgt[k]}")
    else:
        c = args.context
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == "equal":
                n = i2 - i1
                if n > 2 * c + 1:
                    for k in range(i1, i1 + c):
                        print(f"    {ref[k]}")
                    print(f"    … {n - 2 * c} identical")
                    for k in range(i2 - c, i2):
                        print(f"    {ref[k]}")
                else:
                    for k in range(i1, i2):
                        print(f"    {ref[k]}")
            else:
                for k in range(i1, i2):
                    print(f"  ref: {ref[k]}")
                for k in range(j1, j2):
                    print(f"  tgt: {tgt[k]}")

    if not args.demo and not args.no_c:
        p = os.path.join(SRC, sib + ".c")
        if os.path.exists(p):
            print(f"\n===== src/main.exe/{sib}.c (transcribe, then edit the diffs) =====")
            sys.stdout.write(open(p).read())


if __name__ == "__main__":
    main()
