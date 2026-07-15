#!/usr/bin/env python3
"""Aligned instruction diff of one function: target vs our build.

Complements tools/matchdiff.py for BIG or shifted functions: matchdiff
compares per-address (a one-insn insert makes everything after it "differ")
while this tool difflib-aligns the instruction sequences, so inserts/deletes
show as themselves and pure branch-target drift can be suppressed.  The target
extent comes from the reviewed function inventory; the candidate extent comes
from the link map, so an internal/early ``jr ra`` cannot truncate the view.

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
        drift = (tag == "replace" and (i2 - i1) == (j2 - j1)
                 and all(stem(target[i1 + k]) == stem(candidate[j1 + k])
                         for k in range(i2 - i1)))
        if drift and not show_all:
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
    for tag, i1, i2, j1, j2 in stats["displayed"]:
        print(f"--- {tag}")
        for k in range(i1, i2):
            print(f"  T {tgt[k][0]:#x}  {tgt[k][1]}")
        for k in range(j1, j2):
            print(f"  O {ours[k][0]:#x}  {ours[k][1]}")
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
    return 0 if stats["identical"] else 1


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("asmdiff", target):
            sys.exit(main())
    except MatchToolBusy as e:
        sys.exit(f"asmdiff: {e}")
