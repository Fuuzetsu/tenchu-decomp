#!/usr/bin/env python3
"""Show me the COMPILER's source — the authority, in one grep.

**cc1's own source settles in seconds what costs rounds to model.** It is nix-pinned
and on disk, but nothing surfaced it, so lanes reasoned about cc1's behaviour from
recollection and from each other's prose — and this project's record on that is bad:

* A round proposed "make the dependence's cost != 1" as a lever to promote an insn.
  `rank_for_schedule` says an insn that is INDEPENDENT of `last_scheduled_insn` is
  already class 3, the maximum, and the sort is descending — so the proposal was
  exactly BACKWARDS. One look at the comparator, and the whole "open lever" evaporates.
* `tools/schedtrace.py` shipped "priority is DEPTH-FROM-TOP" and "nothing can lift a
  `LOG_LINKS (nil)` insn off priority 1". Both false; `priority()`'s own comment on its
  `- 1` term explains that a unit-latency chain floors at 1 REGARDLESS of depth.
* Two lanes independently asked for this tool. One said the pinned sources "settled in
  one grep what cost prior rounds whole rounds to model".

**And the layout itself is a trap**: this tree is FLAT. `sched.c`, `loop.c`, `global.c`,
`cse.c`, `combine.c`, `reload1.c` are at the ROOT, not under `gcc/`. Every instinct says
`gcc/sched.c`; that path does not exist and the grep silently finds nothing. (The
orchestrator hit this on its first try, with full context.) Only the target files live
in a subdirectory, `config/mips/`.

  tools/ccsrc.py priority            the definition of a function or macro
  tools/ccsrc.py rank_for_schedule   K&R decls are handled (name at column 0)
  tools/ccsrc.py LAUNCH_PRIORITY     macros too
  tools/ccsrc.py --grep ADJUST_COST  grep the whole tree, with file:line
  tools/ccsrc.py --grep 'REG_N_SETS' --context 3
  tools/ccsrc.py --file sched.c      just resolve a path
  tools/ccsrc.py --list              the passes that matter here, and where they are

This is gcc 2.8.1 — the SAME compiler the build uses (canonical decompals
`gcc-2.8.1-psx`, verified equivalent to Sony's psyq4.3). What it says is what our bytes
do. Set `CC1_SOURCE` to override the pinned tree.
"""
import argparse
import glob
import os
import re
import subprocess
import sys

# The nix-pinned unpacked source. Kept here rather than in prose so a doc restructure
# cannot lose it -- it previously existed only as one line buried at cookbook:7038.
PINNED = "/nix/store/117i80brbgcdmcl46gmpzwizikbjyx5m-gcc-2.8.1.tar.gz"

# The passes a matching lane actually reasons about, and the fact that makes each one
# worth opening. Not a file listing -- a router.
KEY_FILES = [
    ("sched.c", "sched1/sched2: priority(), adjust_priority(), rank_for_schedule(), "
                "birthing_insn_p(). The ready-list dump lines are printed here."),
    ("global.c", "global_alloc: allocno_compare's priority formula; find_reg walks "
                 "hard regs NUMERICALLY (mips.h defines no REG_ALLOC_ORDER)."),
    ("local-alloc.c", "local_alloc colours quantities BEFORE global_alloc; its choices "
                      "are not reachable by global weighting levers."),
    ("reload1.c", "reload: alter_reg assigns spill slots (AFTER assign_parms and "
                  "expand_decl -- so a declared local always sits below a spill)."),
    ("loop.c", "the hoist gate: threshold*savings*lifetime >= insn_count."),
    ("cse.c", "cse1/cse2: block boundaries die at every CODE_LABEL; find_best_addr."),
    ("combine.c", "combine: what folded and what did not."),
    ("toplev.c", "the PASS ORDER itself -- e.g. reload_cse_regs runs after .greg and "
                 "before sched2, and emits no dump."),
    ("stmt.c", "expand_decl / how C statements become RTL."),
    ("function.c", "assign_parms, frame layout."),
    ("config/mips/mips.h", "the TARGET macros. Absence is load-bearing: no "
                           "REG_ALLOC_ORDER, no PROMOTE_MODE, no ADJUST_PRIORITY."),
    ("config/mips/mips.md", "the machine description: which RTL becomes which insn."),
]


def tree():
    """The pinned source root, or a loud failure."""
    root = os.environ.get("CC1_SOURCE")
    if root:
        if not os.path.isdir(root):
            sys.exit(f"ccsrc: CC1_SOURCE={root} is not a directory")
        return root
    if os.path.isdir(PINNED):
        return PINNED
    # The store path is a pin, not a guess -- but if it ever changes, say so loudly
    # and try to find the replacement rather than silently searching nothing.
    found = sorted(glob.glob("/nix/store/*-gcc-2.8.1.tar.gz"))
    found = [p for p in found if os.path.isdir(p)]
    if found:
        print(f"ccsrc: WARNING — the pinned path {PINNED} is gone; using {found[0]}.\n"
              f"       Update PINNED in {__file__}.", file=sys.stderr)
        return found[0]
    sys.exit(
        f"ccsrc: the pinned gcc-2.8.1 source is not on disk.\n"
        f"       Expected an unpacked tree at {PINNED}\n"
        f"       Set CC1_SOURCE=<dir>, or realise the nix store path.\n"
        f"       Do NOT proceed by recalling what gcc does — that is what this tool "
        f"exists to prevent.")


def sources(root):
    """Every C/H/MD file in the tree, root-relative."""
    out = []
    for dirpath, _dirs, files in os.walk(root):
        for name in files:
            if name.endswith((".c", ".h", ".md")):
                full = os.path.join(dirpath, name)
                out.append(full)
    return sorted(out)


def find_definition(root, symbol):
    """Locate `symbol` as a C definition or a macro. Returns [(path, line, kind)]."""
    # K&R style puts the name at column 0 followed by ` (` or `(`:
    #     static int
    #     priority (insn)
    #          rtx insn;
    #     {
    func = re.compile(r"^" + re.escape(symbol) + r"\s*\(")
    macro = re.compile(r"^#\s*define\s+" + re.escape(symbol) + r"\b")
    hits = []
    for path in sources(root):
        try:
            with open(path, errors="replace") as stream:
                for number, line in enumerate(stream, 1):
                    if func.match(line):
                        hits.append((path, number, "function"))
                    elif macro.match(line):
                        hits.append((path, number, "macro"))
        except OSError:
            continue
    return hits


def print_function(path, line):
    """Print a K&R definition: its return-type lines, the body, to the closing brace."""
    with open(path, errors="replace") as stream:
        body = stream.readlines()
    start = line - 1
    # Walk back over the return type / storage class lines that precede the name.
    while start > 0:
        prev = body[start - 1].rstrip()
        if not prev or prev.endswith((";", "}", "*/")) or prev.startswith(("/*", " *")):
            break
        start -= 1
    end = line - 1
    depth, seen = 0, False
    while end < len(body):
        depth += body[end].count("{") - body[end].count("}")
        if "{" in body[end]:
            seen = True
        if seen and depth <= 0:
            break
        end += 1
    for offset in range(start, min(end + 1, len(body))):
        print(f"{offset + 1:>6}  {body[offset].rstrip()}")


def print_macro(path, line):
    """Print a #define plus its backslash continuations."""
    with open(path, errors="replace") as stream:
        body = stream.readlines()
    index = line - 1
    while index < len(body):
        print(f"{index + 1:>6}  {body[index].rstrip()}")
        if not body[index].rstrip().endswith("\\"):
            break
        index += 1


def main(argv=None):
    """argv is injectable so the tests can drive the CLI paths directly."""
    ap = argparse.ArgumentParser(
        description="Read the pinned gcc-2.8.1 source — the authority on what our "
                    "bytes do.")
    ap.add_argument("symbol", nargs="?", help="function or macro to show")
    ap.add_argument("--grep", help="regex to search the whole tree for")
    ap.add_argument("--context", type=int, default=0, help="lines of context for --grep")
    ap.add_argument("--file", help="resolve a filename to its path (the tree is FLAT)")
    ap.add_argument("--list", action="store_true",
                    help="the passes that matter for matching, and where they live")
    args = ap.parse_args(argv)
    root = tree()

    if args.list:
        print(f"gcc 2.8.1 — the compiler our build actually uses — at {root}")
        print("\n*** The tree is FLAT: sched.c/loop.c/global.c are at the ROOT, NOT "
              "under gcc/. ***")
        print("    Only the target files live in a subdirectory (config/mips/).\n")
        for name, why in KEY_FILES:
            mark = "" if os.path.exists(os.path.join(root, name)) else "   [ABSENT]"
            print(f"  {name:<22}{mark}")
            for chunk in _wrap(why, 72):
                print(f"      {chunk}")
        return 0

    if args.file:
        matches = [p for p in sources(root)
                   if os.path.basename(p) == os.path.basename(args.file)]
        if not matches:
            sys.exit(f"ccsrc: no file named {args.file!r} in the tree. "
                     f"Try tools/ccsrc.py --list")
        for path in matches:
            print(path)
        return 0

    if args.grep:
        # ripgrep-free: keep the dependency surface at zero, the tree is only 812 files.
        pattern = re.compile(args.grep)
        total = 0
        for path in sources(root):
            try:
                with open(path, errors="replace") as stream:
                    body = stream.readlines()
            except OSError:
                continue
            for number, line in enumerate(body, 1):
                if not pattern.search(line):
                    continue
                total += 1
                rel = os.path.relpath(path, root)
                if args.context:
                    lo = max(0, number - 1 - args.context)
                    hi = min(len(body), number + args.context)
                    print(f"--- {rel}:{number}")
                    for offset in range(lo, hi):
                        mark = ">" if offset == number - 1 else " "
                        print(f"{mark} {offset + 1:>6}  {body[offset].rstrip()}")
                else:
                    print(f"{rel}:{number}: {line.rstrip()}")
        if not total:
            sys.exit(f"ccsrc: no match for {args.grep!r} in {root}.\n"
                     f"       The tree is FLAT — if you were expecting gcc/sched.c, "
                     f"it is sched.c.")
        print(f"\nccsrc: {total} match(es) in {root}")
        return 0

    if not args.symbol:
        sys.exit("ccsrc: give a symbol, --grep, --file or --list "
                 "(tools/ccsrc.py --help)")

    hits = find_definition(root, args.symbol)
    if not hits:
        sys.exit(f"ccsrc: no definition of {args.symbol!r} found.\n"
                 f"       It may be a variable or a use rather than a definition — "
                 f"try:\n"
                 f"         tools/ccsrc.py --grep '{args.symbol}'")
    for path, line, kind in hits:
        rel = os.path.relpath(path, root)
        print(f"=== {rel}:{line}  ({kind} {args.symbol})")
        if kind == "macro":
            print_macro(path, line)
        else:
            print_function(path, line)
        print()
    if len(hits) > 1:
        print(f"ccsrc: {len(hits)} definitions — check which one your target builds "
              f"(config/mips/ is ours).")
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
