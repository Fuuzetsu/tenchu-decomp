#!/usr/bin/env python3
"""Rank functions by assembly similarity — pick references and next targets.

Two uses (docs/matching-cookbook.md):

  tools/findsimilar.py <FuncName>     which MATCHED functions look most like
                                      this one — read those first, they're the
                                      worked examples to imitate
  tools/findsimilar.py --targets      which UNMATCHED functions are closest to
                                      the matched corpus — easy next targets
                                      (sorted best-first, smallest-first)

Similarity = Jaccard overlap of mnemonic 4-grams (operands ignored — register
allocation noise would swamp the structure). The function list comes from the
Ghidra export (.shake/ghidra-export/functions.tsv); bytes from the original
image, disassembled in one objdump pass. Run inside the nix devShell.
"""
import argparse, os, re, subprocess, sys

import function_inventory as FI

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
FILE_TEXT_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
TSV = ".shake/ghidra-export/functions.tsv"
SYMBOLS = "config/symbols.main.exe.txt"
SPLAT = "config/splat.main.exe.yaml"
SRC = "src/main.exe"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"
N = 4  # n-gram size


def load_functions(tsv=TSV, symbols=SYMBOLS, splat=SPLAT):
    """[(addr, size, name)] for reviewed functions inside the text segment.

    Prefer the current splat C name, then config/symbols, then Ghidra's stale
    name so an already-matched rename is never offered as a fresh target.
    """
    symname = {}
    with open(symbols) as stream:
        for line in stream:
            m = re.match(r"([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
            if m:
                symname[int(m.group(2), 16)] = m.group(1)
    carved_names = FI.load_splat_c_names(splat)
    out = []
    for addr, size, name in FI.load_functions(tsv):
        if TEXT_START <= addr < TEXT_END and size >= 8:
            name = carved_names.get(addr, symname.get(addr, name))
            out.append((addr, size, name))
    return out


def matched_names():
    """Functions with real C (no INCLUDE_ASM) in src/main.exe/."""
    names = set()
    for f in os.listdir(SRC):
        if f.endswith(".c"):
            if not re.search(r"^\s*INCLUDE_ASM", open(os.path.join(SRC, f)).read(), re.M):
                names.add(f[:-2])
    return names


def parked_names():
    """Functions with a NON_MATCHING draft — real C behind an INCLUDE_ASM guard.

    THESE ARE REFERENCES TOO, and leaving them out was a serious blind spot. A park
    has real C and, usually, a fully root-caused header explaining its structure —
    everything a near-clone needs. But `matched_names` rejects anything containing an
    INCLUDE_ASM line, and a park's guard contains exactly that, so every parked draft
    was invisible as a reference.

    What that cost, measured: FUN_80059008 vs the parked FUN_80058c70 is similarity
    **1.00** — instruction-for-instruction identical modulo three constants, with the
    sibling's structure already solved and documented. `--targets` reported
    FUN_80059008's nearest reference as RotateVectorS at **0.05** and matcher-prompt's
    NEAR-CLONE path (>=0.99, "don't spawn an agent, clone it inline") never fired. A
    full agent round rediscovered the relationship by hand. Two more pairs were hidden
    the same way (FUN_8005961c/FUN_80059b08 and FUN_80059ff4/FUN_8005a3cc, both 1.00).
    """
    names = set()
    for f in os.listdir(SRC):
        if not f.endswith(".c"):
            continue
        body = open(os.path.join(SRC, f)).read()
        if re.search(r"^\s*INCLUDE_ASM", body, re.M) and \
           re.search(r"#\s*ifndef\s+NON_MATCHING", body):
            names.add(f[:-2])
    return names


def all_mnemonics():
    """{addr: mnemonic} for the whole text segment, one objdump pass."""
    # The EXE has a FILE_TEXT_OFF (0x800) header before the text; adjust so file
    # offset 0x800 maps to TEXT_START (else every address is skewed +0x800 and
    # each function's window reads the code 0x800 bytes before it).
    out = subprocess.run(
        [OBJDUMP, "-D", "-b", "binary", "-m", "mips", "-EL",
         "--adjust-vma", hex(TEXT_START - FILE_TEXT_OFF), ORIG],
        capture_output=True, text=True).stdout
    mn = {}
    pat = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]{8} \t(\S+)")
    for line in out.splitlines():
        m = pat.match(line)
        if m:
            a = int(m.group(1), 16)
            if a >= TEXT_START:
                mn[a] = m.group(2)
    return mn


def ngrams(mn, addr, size):
    seq = [mn.get(a, "?") for a in range(addr, addr + size, 4)]
    return {tuple(seq[i:i + N]) for i in range(len(seq) - N + 1)}


def jaccard(a, b):
    if not a or not b:
        return 0.0
    return len(a & b) / len(a | b)



# The PSX.SYM header each `.c` carries records its ORIGINAL translation unit
# ("3DCTRL.C:240"). 394 of 665 functions have one. Same TU + a high similarity score
# is the condition under which a matched sibling is an ANSWER KEY rather than an
# analogy -- and three lanes in a row found the answer in a sibling's C by eye
# because nothing said so. DrawClip's key (DrawSprite, 0.48, same TU) sat unread
# through an entire park; transcribing it matched on the first build.
TU_RE = re.compile(rb"([A-Z0-9_]+\.C):\d+")


def original_tu(name):
    """The function's original TU per its PSX.SYM header, or None."""
    path = os.path.join("src", "main.exe", name + ".c")
    if not os.path.exists(path):
        return None
    with open(path, "rb") as stream:
        m = TU_RE.search(stream.read(4000))
    return m.group(1).decode() if m else None


def verdict(name, rows, byname):
    """Say TRANSCRIBE when a matched sibling is an answer key, or why not.

    Never fires on similarity alone: signature outranks shape (the symmatch
    frame-shape lesson). The TU is the evidence that two functions came from one
    author's one file, which is what makes a body transcribable.
    """
    mine = original_tu(name)
    best = next(((j, m) for j, m in rows if m != name), None)
    print()
    if best is None:
        print("verdict: no matched sibling to compare against.")
        return
    j, m = best
    theirs = original_tu(m)
    same_tu = mine and theirs and mine == theirs
    if j >= 0.35 and same_tu:
        print(f"verdict: *** TRANSCRIBE {m}.c — it is an ANSWER KEY, not an analogy "
              f"***")
        print(f"  score {j:.2f} AND same original TU ({mine}). Read its C and copy the "
              "body shape;")
        print("  change only what the target's own bytes force. Three lanes found the "
              "answer in a")
        print("  sibling's C by eye because nothing said this — DrawClip's key sat "
              "unread through a")
        print("  whole park, then matched on the FIRST BUILD once transcribed.")
        print("  Look especially at: goto/label PLACEMENT, guard shape, and where "
              "shared tails live —")
        print("  those do not show up in an instruction diff.")
    else:
        why = []
        if j < 0.35:
            why.append(f"top score {j:.2f} < 0.35")
        if not same_tu:
            why.append(f"different TU ({mine or '?'} vs {theirs or '?'})"
                       if mine or theirs else "no TU recorded for either")
        print(f"verdict: no transcription candidate — {'; '.join(why)}.")
        print("  Similarity alone is NOT enough (signature outranks shape), so work "
              "from the dumps.")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name", nargs="?")
    ap.add_argument("--targets", action="store_true",
                    help="rank unmatched functions by closeness to the matched corpus")
    ap.add_argument("--top", type=int, default=15)
    ap.add_argument("--max-size", type=int, default=0x800,
                    help="--targets: ignore functions bigger than this (bytes)")
    ap.add_argument("--by-value", action="store_true",
                    help="--targets: rank by FUNCTION SIZE (what a match is worth) "
                         "instead of by similarity (what looks easy). The byte counter "
                         "is all-or-nothing per function, so a 97%%-exact park on a "
                         "6084-byte function is worth 6084 bytes and a matched 48-byte "
                         "function is worth 48. Similarity ranking buries the prize.")
    ap.add_argument("--scope", choices=["game", "sdk", "all"], default="game",
                    help="--targets: which population to rank. DEFAULT game — the "
                         "SDK (>=0x80060000) is stock Sony library code linked from "
                         "prebuilt .LIBs, not part of Tenchu's source, so its asm is "
                         "the faithful form and decompiling it is a different project. "
                         "This default exists because the RANKING IS BY SIMILARITY TO "
                         "MATCHED CODE, i.e. by EASINESS, and small simple SDK leaves "
                         "dominate it: the top 50 was 41 SDK / 9 game, while the "
                         "remaining UNDRAFTED game functions sat at rank 367-382 "
                         "(similarity 0.05, because they are large and unique). The "
                         "orchestrator read the top of that list and spent a session "
                         "on libgs.")
    args = ap.parse_args()

    funcs = load_functions()
    byname = {n: (a, s) for a, s, n in funcs}
    matched = matched_names() & set(byname)
    if not matched:
        sys.exit("findsimilar: no matched functions found in src/main.exe")
    # Parked drafts are references too — they have real C and a root-caused header.
    # Excluding them hid three 1.00 near-clone pairs; see parked_names().
    parked = (parked_names() & set(byname)) - matched
    refs = matched | parked
    mn = all_mnemonics()
    mgrams = {m: ngrams(mn, *byname[m]) for m in refs}

    if args.targets:
        rows = []
        omitted_scope = omitted_size = omitted_hw = 0
        # The owner-decided handwritten-asm originals are NOT matching targets --
        # their asm IS the faithful source. triage knows; this picker did not, and
        # kept listing the whole draw* family as candidates. matcher-prompt refuses
        # to brief them, but by then a human or agent has already spent thought on
        # one (drawF3 got a full round that way).
        try:
            import triage as _triage
            handwritten = set(_triage.HANDWRITTEN)
        except Exception:
            handwritten = set()
        for a, s, n in funcs:
            if n in matched:
                continue
            if n in handwritten:
                omitted_hw += 1
                continue
            in_sdk = a >= 0x80060000
            if args.scope == "game" and in_sdk:
                omitted_scope += 1
                continue
            if args.scope == "sdk" and not in_sdk:
                omitted_scope += 1
                continue
            if s > args.max_size:
                omitted_size += 1
                continue
            g = ngrams(mn, a, s)
            best, bestm = 0.0, ""
            for m, gm in mgrams.items():
                if m == n:
                    continue
                j = jaccard(g, gm)
                if j > best:
                    best, bestm = j, m
            if bestm in parked:
                bestm += "  [PARKED — clone its C, read its header]"
            rows.append((best, s, n, bestm))
        if args.by_value:
            # Rank by what a match is WORTH. The byte counter is ALL-OR-NOTHING per
            # function -- a park at 97% exact contributes ZERO matched bytes, because
            # the default build still links its INCLUDE_ASM stub. So the payoff for
            # cracking a function is its FULL SIZE, and similarity ranking (= ease)
            # systematically buries the prize: the remaining board's top 4 functions
            # are 42% of all unmatched game bytes while the bottom 10 are ~2%, and a
            # session went into 4- and 9-byte parks on 500-byte functions.
            rows.sort(key=lambda r: (-r[1], -r[0]))
            order = "SIZE — i.e. by what a match is WORTH (all-or-nothing per function)"
        else:
            rows.sort(key=lambda r: (-r[0], r[1]))
            order = ("SIMILARITY TO MATCHED CODE — i.e. by how EASY they look, NOT by "
                     "what matters. Try --by-value")
        print(f"scope={args.scope}: {len(rows)} candidates ranked BY {order}.")
        # A truncation must state its consequence. This list silently showed its top
        # 15 (easiest) while the remaining undrafted GAME functions sat at rank
        # 367-382 -- large, unique, similarity 0.05 -- and a whole session went into
        # small SDK leaves that float to the top instead.
        if omitted_scope:
            other = "SDK" if args.scope == "game" else "game"
            print(f"  ({omitted_scope} {other} function(s) omitted by --scope; "
                  f"--scope all to include them)")
        if omitted_size:
            print(f"  ({omitted_size} omitted as bigger than --max-size "
                  f"{args.max_size}; raise it to see them)")
        if omitted_hw:
            print(f"  ({omitted_hw} handwritten-asm original(s) omitted — owner "
                  f"decision, config/handwritten-asm.txt; never targets)")
        # CLONE GROUPS AMONG THE TARGETS THEMSELVES. Two undrafted functions cannot
        # be each other's "reference" (neither has C), so the ref scan above misses
        # them entirely -- yet FUN_8005961c/FUN_80059b08 and FUN_80059ff4/FUN_8005a3cc
        # are each 1.00 mnemonic-identical pairs. That is a 2-for-1: solve one and the
        # twin is a constants edit. Worth stating outright, because the orchestrator
        # hand-diffed their disassembly (addresses included), concluded "same shape,
        # not clones", and briefed two lanes on that.
        tgrams = {n: ngrams(mn, *byname[n]) for _b, _s, n, _m in rows}
        names = [r[2] for r in rows]
        groups, seen_in_group = [], set()
        for i, a_name in enumerate(names):
            if a_name in seen_in_group:
                continue
            twins = [b_name for b_name in names[i + 1:]
                     if b_name not in seen_in_group
                     and jaccard(tgrams[a_name], tgrams[b_name]) >= 0.99]
            if twins:
                groups.append([a_name] + twins)
                seen_in_group.update([a_name] + twins)
        if groups:
            print()
            print("  CLONE GROUPS among these targets (>=0.99 — identical mnemonics, "
                  "differing constants):")
            for grp in groups:
                print(f"    {' == '.join(grp)}")
            print("    Solve ONE and the rest are a constants edit "
                  "(tools/matcher-prompt.py prints the inline clone recipe).")
        shown = rows[:args.top]
        print()
        print(f"{'sim':>5} {'size':>6}  {'function':32} nearest reference")
        for best, s, n, bestm in shown:
            print(f"{best:5.2f} {s:6}  {n:32} {bestm}")
        if len(rows) > len(shown):
            print(f"\n  ... {len(rows) - len(shown)} more NOT shown (--top {args.top}). "
                  f"LOW SIMILARITY IS NOT LOW VALUE:")
            print(f"  a big unique function scores ~0.05 and sinks; that is a "
                  f"statement about resemblance,")
            print(f"  not about whether it is worth matching. Check the tail before "
                  f"concluding you are done.")
        return

    if not args.name:
        sys.exit("findsimilar: give a function name or --targets")
    if args.name not in byname:
        sys.exit(f"findsimilar: {args.name} not in {TSV}")
    g = ngrams(mn, *byname[args.name])
    rows = sorted(((jaccard(g, gm), m) for m, gm in mgrams.items()), reverse=True)
    print(f"{args.name} ({byname[args.name][1]} bytes) vs matched corpus:")
    for j, m in rows[:args.top]:
        tu = original_tu(m)
        tag = f"  [{tu}]" if tu else ""
        print(f"{j:5.2f}  {m} ({byname[m][1]} bytes) — src/main.exe/{m}.c{tag}")
    verdict(args.name, rows, byname)


if __name__ == "__main__":
    main()
