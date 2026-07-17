#!/usr/bin/env python3
"""Aligned instruction diff of one function: target vs our build.

Complements tools/matchdiff.py for BIG or shifted functions: matchdiff
compares per-address (a one-insn insert makes everything after it "differ")
while this tool difflib-aligns the instruction sequences, so inserts/deletes
show as themselves and pure branch-target drift can be suppressed.  The target
extent comes from the reviewed splat carve; the candidate extent comes from the
link map, so stale Ghidra rows and internal/early ``jr ra`` instructions cannot
truncate the view.

  tools/asmdiff.py <Name>              aligned diff (structural view)
  tools/asmdiff.py <Name> --all        include pure branch-target drift lines
  tools/asmdiff.py <Name> --structural only blocks that change the length;
                                           never a byte-match verdict
  tools/asmdiff.py <Name> -n           skip ./Build

Exit 0 when the sequences are identical. Run inside the nix devShell.
"""
import tempfile, argparse, os, re, subprocess, sys

import matchdiff
from matchlock import MatchToolBusy, matching_tool_lock

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
FILE_TEXT_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
OURS = ".shake/build/tenchu/main.exe"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"


def candidate_artifact_error(name):
    """Explain why the current linked image is not a C-candidate artifact."""
    if matchdiff.linked_nonmatching_stub(name):
        return (
            f"asmdiff: current image links {name}.NON_MATCHING — the "
            "INCLUDE_ASM stub, not the guarded/plain C candidate. A no-build "
            "comparison would be trivial or stale; rebuild the draft without "
            "-n."
        )
    return None


def resolve(name):
    """Return the authoritative current splat carve rather than stale Ghidra rows."""
    return matchdiff.symbol_slot(name)


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


def aligned_opcodes(target, candidate, show_all=False, structural=False):
    """Return raw and displayed aligned-diff statistics.

    The structural view deliberately hides same-length replacements.  Keep its
    displayed count separate from the raw aligned residual so that an empty
    structural view can never be reported as an exact match.
    """
    import difflib

    suppressed_drift = [0]

    def stem(insn):
        return re.sub(r"0x[0-9a-f]+$", "", insn)

    opcodes = difflib.SequenceMatcher(
        None, target, candidate, autojunk=False
    ).get_opcodes()
    raw_lines = raw_blocks = 0
    displayed_lines = displayed_blocks = 0
    displayed = []
    for tag, i1, i2, j1, j2 in opcodes:
        if tag == "equal":
            continue
        width = max(i2 - i1, j2 - j1)
        raw_lines += width
        raw_blocks += 1
        # `stem` strips the trailing hex target, so "branch drift" (an address
        # that moved because code above it shifted) and a genuine RETARGET (the
        # branch now goes somewhere ELSE) are indistinguishable here -- and the
        # default silently drops both. That cost two lanes the diagnosis on
        # DrawModelArchive: `beqz v0,0x8001779c` vs `beqz v0,0x800177c4` was THE
        # informative row -- same instruction, different destination, the
        # signature of a RELOCATED BLOCK -- and it was suppressed. Count what we
        # hide and say what it might mean.
        drift = (tag == "replace" and (i2 - i1) == (j2 - j1)
                 and all(stem(target[i1 + k]) == stem(candidate[j1 + k])
                         for k in range(i2 - i1)))
        if drift and not show_all:
            suppressed_drift[0] += max(i2 - i1, j2 - j1)
            continue
        if structural and (i2 - i1) == (j2 - j1):
            continue
        displayed_lines += width
        displayed_blocks += 1
        displayed.append((tag, i1, i2, j1, j2))
    return {
        "raw_lines": raw_lines,
        "raw_blocks": raw_blocks,
        "displayed_lines": displayed_lines,
        "displayed_blocks": displayed_blocks,
        "displayed": displayed,
        "suppressed_drift": suppressed_drift[0],
        "identical": target == candidate,
    }


def candidate_disassembly(name, addr, target_size):
    """Disassemble exactly the candidate text the linker placed.

    Old versions guessed the end by taking the first ``jr ra`` near the target
    size.  A function with an early return therefore lost its real tail (the
    72-instruction check_for_known_button_combination was displayed as 65).
    The map's per-object .text extent is authoritative for a compiled draft.
    Keep the old return heuristic only as a fallback for artifacts whose map
    has no C-object entry.
    """
    linked = matchdiff.linked_text_size(name)
    if linked is not None:
        return dis(OURS, addr, max(linked, 4))

    ours = dis(OURS, addr, target_size + 0x100)
    jrs = [i for i, (_, insn) in enumerate(ours)
           if insn.startswith("jr ra")
           and i + 1 >= (target_size // 4) - 0x40]
    end = jrs[0] + 2 if jrs else len(ours)
    return ours[:end]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("-n", "--no-build", action="store_true")
    ap.add_argument("--all", action="store_true",
                    help="also show pure branch-target drift")
    ap.add_argument("--structural", action="store_true",
                    help="only show blocks that change the instruction count")
    ap.add_argument("--context", type=int, default=0, metavar="N",
                    help="show N matching instructions either side of each hunk. "
                         "Asked for three times: this tool prints only DIFFERING "
                         "lines, so an insn that is present and CORRECT is invisible "
                         "-- and that absence has twice been the fact that refuted a "
                         "park (FadeOutDirect's surviving `sb v0,0xf7(sp)` disproved "
                         "its dead-store story).")
    args = ap.parse_args()

    if not args.no_build:
        # Build a NON_MATCHING partial's draft (not its trivially-matching stub).
        env = dict(os.environ)
        srcp = os.path.join("src/main.exe", args.name + ".c")
        if os.path.exists(srcp) and "ifndef NON_MATCHING" in open(srcp).read():
            env["NON_MATCHING"] = args.name
        log = os.path.join(tempfile.gettempdir(), "tenchu-asmdiff-build.log")
        with open(log, "wb") as lf:   # a file, never a pipe: a build log is ~780 KB
            r = subprocess.run(["./Build"], stdout=subprocess.DEVNULL,
                               stderr=lf, env=env)
        if r.returncode != 0:
            tail = "\n".join(open(log, errors="replace").read().splitlines()[-15:])
            if r.returncode in (126, 127) and "Argument list too long" in tail:
                # execve E2BIG: one env string > MAX_ARG_STRLEN (128 KiB).
                sys.exit("asmdiff: could not EXEC ./Build -- environment failure, "
                         "not a build failure. An env var exceeds 128 KiB; find it "
                         "with: env | awk -F= 'length($0) > 131072 {print $1}'. "
                         "See docs/build-system.md")
            sys.exit(f"asmdiff: ./Build FAILED (rc={r.returncode}), log: {log}\n{tail}")

    # Check the final link map even when the source guard has just been removed:
    # `-n` may still point at the previous default/stub build. Source text and
    # timestamps cannot prove which artifact supplied the bytes; the map can.
    artifact_error = candidate_artifact_error(args.name)
    if artifact_error:
        sys.exit(artifact_error)

    addr, size = resolve(args.name)
    tgt = dis(ORIG, addr, size)
    ours = candidate_disassembly(args.name, addr, size)

    t = [x[1] for x in tgt]
    o = [x[1] for x in ours]

    stats = aligned_opcodes(t, o, args.all, args.structural)
    if args.structural:
        print("[STRUCTURAL FILTER ONLY — empty output is not an exact-match "
              "claim; run tools/matchdiff.py for the byte gate]")
    # An instruction that MOVED (notably into a branch delay slot) is reported as a
    # delete/replace on one side and an insert/replace on the other, which reads as
    # "an instruction vanished" — a lane hand-mapped two whole streams to discover an
    # `addiu` had merely moved into a delay slot. Pair identical texts that appear on
    # BOTH sides of the displayed residual (across any hunk kinds) and say so. Compare
    # by COUNT so a text genuinely added N extra times is not mislabelled a move.
    import collections as _c
    t_side, o_side = _c.Counter(), _c.Counter()
    t_at, o_at = {}, {}
    for tag, i1, i2, j1, j2 in stats["displayed"]:
        for k in range(i1, i2):
            t_side[tgt[k][1]] += 1
            t_at.setdefault(tgt[k][1], tgt[k][0])
        for k in range(j1, j2):
            o_side[ours[k][1]] += 1
            o_at.setdefault(ours[k][1], ours[k][0])
    moved = sorted(x for x in set(t_side) & set(o_side) if t_side[x] == o_side[x])

    def context_rows(rows, lo, hi):
        """The N matching rows either side of a hunk, clipped to the listing."""
        n = args.context
        before = rows[max(0, lo - n):lo]
        after = rows[hi:hi + n]
        return before, after

    for tag, i1, i2, j1, j2 in stats["displayed"]:
        width = max(i2 - i1, j2 - j1)
        print(f"--- {tag} [T{i2 - i1}/O{j2 - j1} insns, edit-weight {width * 4}B]")
        if args.context:
            before, _ = context_rows(tgt, i1, i2)
            for a, txt in before:
                print(f"    {a:#x}  {txt}")
        for k in range(i1, i2):
            note = "   <- MOVED, not missing" if tgt[k][1] in moved else ""
            print(f"  T {tgt[k][0]:#x}  {tgt[k][1]}{note}")
        for k in range(j1, j2):
            note = "   <- MOVED, not new" if ours[k][1] in moved else ""
            print(f"  O {ours[k][0]:#x}  {ours[k][1]}{note}")
        if args.context:
            _, after = context_rows(tgt, i1, i2)
            for a, txt in after:
                print(f"    {a:#x}  {txt}")
    if moved:
        print()
        print("asmdiff: these instructions MOVED — they exist on BOTH sides at "
              "different positions:")
        for text_ in moved:
            print(f"    {text_:<26} target {t_at[text_]:#x} -> ours {o_at[text_]:#x}")
        print("  A move is a SCHEDULING / delay-slot question, not a missing or "
              "surplus instruction.")
        print("  Read the moved insn's LOG_LINKS before calling it a tie (a real "
              "REG_DEP_ANTI")
        print("  looks identical from here). MIPS loads are INELIGIBLE for a delay "
              "slot. See")
        print("  docs/matching-cookbook.md, \"An empty `do{}while(0)` at the END of "
              "an `if` body")
        print("  flips reorg's branch prediction\".")
    if args.structural:
        print(f"[{args.name}: {stats['displayed_lines']} length-changing lines "
              f"in {stats['displayed_blocks']} displayed blocks; raw aligned "
              f"residual {stats['raw_lines']} lines in {stats['raw_blocks']} "
              f"blocks; length ours {len(o)} vs target {len(t)}; "
              f"exact instruction sequence: "
              f"{'YES' if stats['identical'] else 'NO'}]")
    else:
        print(f"[{args.name}: {stats['displayed_lines']} displayed differing "
              f"lines in {stats['displayed_blocks']} blocks; raw aligned "
              f"residual {stats['raw_lines']} lines in {stats['raw_blocks']} "
              f"blocks; length ours {len(o)} vs target {len(t)}; "
              f"exact instruction sequence: "
              f"{'YES' if stats['identical'] else 'NO'}]")
    if stats.get("suppressed_drift") and not args.all:
        print()
        print(f"asmdiff: *** {stats['suppressed_drift']} branch line(s) HIDDEN as "
              "'target drift' — re-run with --all ***")
        print("  A hidden line is `same opcode, DIFFERENT target`. That is EITHER a "
              "harmless address")
        print("  shift OR a genuine RETARGET — the signature of a RELOCATED BLOCK — "
              "and this tool")
        print("  cannot tell them apart, because it strips the trailing address to "
              "align. On")
        print("  DrawModelArchive the hidden row WAS the whole diagnosis "
              "(`beqz v0,0x8001779c` vs")
        print("  `beqz v0,0x800177c4` = 44 of 48 bytes). Cross-check with "
              f"`tools/matchdiff.py {args.name} --max 400`,")
        print("  whose raw byte view does not normalise.")

    if not stats["identical"]:
        # The per-hunk `edit-weight` is an ALIGNMENT metric (differing slots x 4),
        # NOT the byte count -- the two disagree in both directions, since a moved
        # instruction perturbs bytes at both positions. A brief this session read
        # "weight 4 bytes" x2 as "8 bytes" and told a lane the residual was 8 when
        # matchdiff (and the file's own STATUS) said 11.
        print("  NB edit-weight above is an alignment metric, not a byte count. "
              "For bytes, use")
        print(f"     tools/matchdiff.py {args.name}.")
    return 0 if stats["identical"] else 1


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("asmdiff", target):
            sys.exit(main())
    except MatchToolBusy as e:
        sys.exit(f"asmdiff: {e}")
