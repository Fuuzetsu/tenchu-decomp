#!/usr/bin/env python3
"""Everything cc1 SAYS about its own decisions — every self-reporting table, one call.

cc1 narrates its work in `;;` commentary across all 15 `-da` dumps. **We were reading
about three of those tables and hand-deriving the rest.** Every time a lane
hand-derived something cc1 prints, it cost a round, and twice it produced a confident
WRONG conclusion:

* `.sched`'s `priority`/`ref_count` table (250 lines): a round read the `ref_count`
  column as priority, concluded an insn "heads a 7-deep chain" and sits at the ceiling
  when it sits at the FLOOR, and INVERTED a correct 6-round-old park. The 7 belonged to
  a different insn two lines away.
* `.sched2`'s ready list: FUN_80057b80's entire 8-byte residual is ONE line
  (`ready list at T-20: 6 (1) 4 (1), now 6 4`). A lane hand-decoded the raw dump and
  called it the single biggest cost of its round.
* `.loop`'s movable log: SetBleedsDir's whole 13-byte residual was one line
  (`moved to 388` vs a matched sibling's `not desirable`). A lane called it "the single
  highest-value artifact here" and found it only by reading the raw dump.

And these were surfaced by NOTHING until this tool — despite rounds spent on each:
* **`.dbr`'s delay-slot histogram** — `12 insns needing delay slots / 4 got 0 delays,
  8 got 1 delays`. FOUR delay-slot rounds ran blind to cc1's own fill count.
* **`.cse`'s block boundaries** — `Processing block from 2 to 76, 29 sets`. We reason
  about "cse's block ends at every CODE_LABEL" constantly and hand-derive where.
* **`.combine`'s success count** — `138 attempts, 101 substitutions, 17 successes`.
  The direct answer to "did my combine fire?".
* **`Registers live:` per block** (`.flow`/`.lreg`/`.combine`/`.jump2`) — the register
  pressure at each point, printed outright.

  tools/cc1says.py <Name>              every table, every pass
  tools/cc1says.py <Name> --pass loop  just one pass
  tools/cc1says.py <Name> --raw        also dump the `;;` lines we do not interpret yet

Compare two functions (e.g. a draft against its matched sibling) by running it on each:
identical movables with opposite verdicts, or differing delay-slot fill counts, are
usually the whole residual.

Run inside the nix devShell.
"""
import argparse
import os
import re
import sys

import rtldump

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# Each entry: pass -> (heading, [(regex, note)]). The note says what the table MEANS,
# because every one of these has been misread at least once.
TABLES = {
    "combine": ("did combine fire?", [
        (r"^;; Combiner statistics:.*", None),
        (r"^;; \d+ successes\.", "attempts vs SUCCESSES — a combine you expected that "
                                "is not in the successes did not happen"),
    ]),
    "cse": ("cse's OWN block boundaries", [
        (r"^;; Processing block from (\d+) to (\d+), (\d+) sets\.",
         "cse's table dies at every CODE_LABEL — these ARE the blocks, do not derive "
         "them. An equivalence cannot cross a boundary printed here."),
    ]),
    "cse2": ("cse2's block boundaries (WIDER — after_loop=1)", [
        (r"^;; Processing block from (\d+) to (\d+), (\d+) sets\.",
         "cse2's extended block is wider than cse1's, which is why a copy cse1 kept "
         "can get folded back onto its SOURCE here"),
    ]),
    "loop": ("movable HOIST decisions", [
        (r"^Loop from \d+ to \d+: \d+ real insns\.",
         "the loop's REAL INSN COUNT is the gate's right-hand side"),
        (r"^Insn \d+: regno \d+ .*(moved to \d+|not desirable|not worth moving).*",
         "gate: threshold*savings*lifetime >= insn_count (loop.c:1640); threshold = "
         "(loop_has_call?1:2)*(1+n_non_fixed_regs) = 29 here. A high/lo_sum address "
         "pair scores 116 -> hoists out of any loop of <= 116 real insns."),
    ]),
    "lreg": ("local-alloc dispositions", [
        (r"^;; Register (\d+) in (\d+)\.",
         "local_alloc coloured these BEFORE global_alloc ran; weighting/splitting "
         "levers do not apply to them"),
    ]),
    "greg": ("global allocation", [
        (r"^;; Hard regs used:.*", None),
        (r"^;; \d+ regs to allocate:.*",
         "THIS is the allocation ORDER — it, not the priority alone, decides who wins"),
        (r"^;; \d+ conflicts:.*",
         "a hard-reg conflict is categorical: no weighting can make a pseudo legal in "
         "a register listed here"),
        (r"^;; \d+ preferences:.*", None),
        (r"^;; Register dispositions:.*", None),
        (r"^;; Need \d+ regs? of class \w+ \(for insn \d+\)\.",
         "reload's OWN spill demands, per insn and per class. A HI/LO/MD_REGS line is "
         "a mult/div; an ALL_REGS line is real pressure at that insn."),
    ]),
    "sched": ("sched1 — priority table, ready lists, and cc1's OWN explanations", [
        (r"^;; insn\[\s*\d+\]: priority = \s*\d+, ref_count = \s*\d+",
         "priority = DEPTH-FROM-TOP (walks LOG_LINKS = PRODUCERS); ref_count = "
         "CONSUMERS. They are DIFFERENT metrics and reading one as the other has "
         "inverted a park."),
        (r"^;;.*ready list at T-\d+:.*", None),
        (r"^;;.*insn \d+ has a greater potential hazard.*",
         "cc1 NAMES the hazard swap. We have a whole rule about this swap being a "
         "SYMPTOM of an equal-priority group — and the insn is printed right here, "
         "no derivation needed."),
        (r"^;;.*blocking insn \d+ for \d+ cycles.*",
         "why an insn was held back, in cycles — a real latency, not a tie"),
        (r"^;;.*launching \d+ before \d+ with no stalls.*", None),
        (r"^;;.*register \d+ (life (extended|shortened) from \d+ to \d+|no longer "
         r"crosses calls).*",
         "SCHED CHANGES LIVE RANGES, and that feeds allocation. `no longer crosses "
         "calls` means the value stopped needing a callee-saved register — often the "
         "real cause of a register 'tie' downstream."),
    ]),
    "sched2": ("sched2 (POST-RELOAD — the only pass that sees the prologue)", [
        (r"^;; insn\[\s*\d+\]: priority = \s*\d+, ref_count = \s*\d+", None),
        (r"^;;.*ready list at T-\d+:.*",
         "sched is BACKWARD: T-1 is the LAST slot, filled FIRST, so the insn PICKED "
         "EARLIER LANDS LATER. Parm-copy order lives ONLY here."),
        (r"^;;.*insn \d+ has a greater potential hazard.*", None),
        (r"^;;.*blocking insn \d+ for \d+ cycles.*", None),
    ]),
    "dbr": ("delay-slot fill report", [
        (r"^;; Reorg pass #\d+:", None),
        (r"^;; \d+ insns needing delay slots", None),
        (r"^;; \d+ got \d+ delays.*",
         "cc1's OWN fill count. `N got 0 delays` = N slots reorg could not fill. "
         "Compare against the target's nop count before theorising about WHY."),
    ]),
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--pass", dest="only", help="just this pass")
    ap.add_argument("--raw", action="store_true",
                    help="also list the `;;` lines this tool does not interpret yet")
    args = ap.parse_args()

    path = os.path.join("src", "main.exe", args.name + ".c")
    if not os.path.exists(path):
        sys.exit(f"cc1says: {path} not found")
    with open(path, errors="replace") as stream:
        draft = "ifndef NON_MATCHING" in stream.read()

    want = [args.only] if args.only else ["all"]
    try:
        result = rtldump.compile_rtl(args.name, want, draft=draft)
    except (FileNotFoundError, ValueError, RuntimeError) as e:
        sys.exit(f"cc1says: {e}")

    by_pass = {p.rsplit(".", 1)[-1]: p for p in result["dumps"]}
    print(f"{args.name}{' (draft)' if draft else ''} — what cc1 says about its own "
          f"decisions")

    order = ["rtl", "jump", "cse", "loop", "cse2", "flow", "combine", "sched",
             "lreg", "greg", "sched2", "jump2", "dbr"]
    shown_any = False
    for name in order:
        if name not in by_pass or name not in TABLES:
            continue
        if args.only and name != args.only:
            continue
        heading, pats = TABLES[name]
        body = open(by_pass[name], errors="replace").read().splitlines()
        compiled = [(re.compile(rx), note) for rx, note in pats]
        # DOCUMENT order, not pattern order: `Reorg pass #1` must be followed by ITS
        # own counts. Grouping by pattern lumps every pass's numbers together and
        # destroys the only reading that matters.
        # Document order, but cap PER PATTERN. A flat cap lets the biggest table
        # (sched's 250-row priority list) drown the rarest and most valuable lines
        # (`insn N has a greater potential hazard` — the exact thing a cookbook rule
        # is about). Bulk tables get a pointer to the tool that reads them properly;
        # rare explanations get printed in full.
        BULK = 6
        counts = {}
        rows, notes = [], []
        for ln in body:
            for i, (c, note) in enumerate(compiled):
                if not c.match(ln):
                    continue
                counts[i] = counts.get(i, 0) + 1
                rows.append((i, ln.rstrip()))
                if note and note not in notes:
                    notes.append(note)
                break
        if not rows:
            continue
        shown_any = True
        print(f"\n=== .{name} — {heading}")
        printed = {}
        for i, ln in rows:
            total = counts[i]
            seen = printed.get(i, 0)
            if total > BULK * 2 and seen >= BULK:
                if seen == BULK:
                    tool = ("tools/schedtrace.py" if name.startswith("sched")
                            else "tools/regalloc.py" if name == "greg" else None)
                    hint = f" — read them with {tool}" if tool else ""
                    print(f"    … ({total} rows of this shape{hint})")
                    printed[i] = seen + 1
                continue
            print(f"    {ln}")
            printed[i] = seen + 1
        for note in notes:
            for chunk in _wrap(note, 74):
                print(f"      -> {chunk}")

    if args.raw:
        print("\n=== `;;` lines this tool does NOT interpret (candidates for the next "
              "table)")
        seen = set()
        for name, p in by_pass.items():
            known = [re.compile(rx) for rx, _ in TABLES.get(name, ("", []))[1]]
            for ln in open(p, errors="replace").read().splitlines():
                if not ln.startswith(";;") or any(k.match(ln) for k in known):
                    continue
                shape = re.sub(r"\d+", "N", ln.rstrip())
                if shape not in seen:
                    seen.add(shape)
                    print(f"    .{name:8} {shape[:82]}")

    if not shown_any:
        print("\ncc1says: no known tables in these dumps — try --raw, or a wider --pass")
    return 0


def _wrap(text, width):
    out, line = [], ""
    for word in text.split():
        if len(line) + len(word) + 1 > width:
            out.append(line)
            line = word
        else:
            line = f"{line} {word}".strip()
    if line:
        out.append(line)
    return out


if __name__ == "__main__":
    sys.exit(main())
