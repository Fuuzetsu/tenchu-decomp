#!/usr/bin/env python3
"""Generate a matcher-agent launch prompt for a function.

The orchestrator shouldn't hand-write agent instructions from scratch each time.
This emits a complete, ready-to-launch prompt with the per-function facts
auto-derived (address/size, jump-table-or-not, nearest matched worked examples,
TU family, gp reminder) wired into a stable template. The evolving cross-cutting
guidance lives in GUIDANCE below (and, in depth, in docs/matching-cookbook.md +
.claude/agents/matcher.md) — update it here as we learn, so every future launch
inherits the lesson.

  tools/matcher-prompt.py <Name>            print the prompt
  tools/matcher-prompt.py <Name> --model sonnet   (annotate suggested model)

Run inside the nix devShell (uses findsimilar for the nearest examples).
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TSV = ".shake/ghidra-export/functions.tsv"
SYMBOLS = "config/symbols.main.exe.txt"

# ---- evolving guidance: the load-bearing lessons every agent should get at
# launch (deeper detail is in the cookbook, which the agent reads). Edit here. --
GUIDANCE = [
    "Ghidra's SCALAR types are gold, but its inferred STRUCT NAMES are often "
    "wrong (it invents PARAM_ITEM_DROP etc. with fields shifted 4 bytes) and "
    "NEITHER Ghidra nor m2c discloses access WIDTH. For struct layout trust "
    "item.h's proven view + `tools/access.py` (offset -> sw/sh/sb width) over "
    "Ghidra's struct name; reach a divergent-width union field via an offset "
    "cast off the same proven pointer (`*(s32 *)((u8 *)pp + 0xC) = 0;`).",
    "Lean on the tools; don't re-derive their output by hand. gpsyms --write "
    "syncs the gp lists, xref gives the prototype/externs, reverse.py seeds both "
    "a Ghidra AND an m2c reference (m2c = cleaner control flow / register temps, "
    "Ghidra = types/names).",
    "Once your draft COMPILES (set NON_MATCHING for a partial), run "
    "`tools/autorules.py <Name>` BEFORE hand-tuning: it mechanically sweeps the "
    "*local* cookbook rules — every local's integer type (s16<->s32 index/call-"
    "result, sign toggles), splitting/merging `&&` chains, inlining single-use "
    "temps — and greedily keeps what shrinks the asmdiff, reporting which edit "
    "helped (`n: s16->s32: 4->0`). It's the deterministic first pass — if it "
    "reports no win, the residual is NOT one of those mechanical rules (don't "
    "keep guessing them; it's structure/regalloc — read the diff or permute).",
    "For a pure REGISTER tie (right length, right instructions, a value in the "
    "wrong register — and autorules found nothing + a short permuter run won't "
    "beat the base), don't blind-permute: run `tools/regalloc.py <Name>`. It runs "
    "cc1 -dg and shows which values are live across calls (forced callee-saved), "
    "the pseudo->hard-reg map, and the copy-chains that bias the coloring — so you "
    "break the specific copy-chain / shorten the live range it names instead of "
    "guessing. (This is a genuine sub-C tie class; cap effort and NON_MATCHING it "
    "if regalloc.py shows the target's register is unreachable from C.)",
    "N loads adjacent with no use between them are source temps (us/ty), even if "
    "the scheduler later scatters their stores.",
    "When Ghidra's union rendering has MORE assignment lines than access.py's "
    "per-register offset count, check the m2c reference's raw-offset dump "
    "(already in the seed) BEFORE opening the raw .s — it usually disambiguates "
    "which stores are pp-relative vs it->param-relative directly.",
]


def func(name):
    for line in open(TSV):
        p = line.rstrip("\n").split("\t")
        if len(p) == 3 and p[2] == name:
            return int(p[0], 16), int(p[1])
    sys.exit(f"matcher-prompt: {name} not in {TSV}")


def is_jump_table(addr, size):
    pat = re.compile(r"switchD_([0-9a-fA-F]+)__switchdataD_")
    for line in open(SYMBOLS):
        m = pat.search(line)
        if m and addr <= int(m.group(1), 16) < addr + size:
            return True
    return False


def nearest_matched(name, k=3):
    try:
        out = subprocess.run(["tools/findsimilar.py", name],
                             capture_output=True, text=True).stdout
    except Exception:
        return []
    rows = []
    for line in out.splitlines():
        m = re.match(r"\s*([01]\.\d\d)\s+(\w+)\s+\((\d+) bytes\)", line)
        if m and m.group(2) != name and float(m.group(1)) > 0:
            rows.append((float(m.group(1)), m.group(2)))
    return rows[:k]


def family(name):
    if re.match(r"^(ReqItem|ProcItem)", name):
        return ("item TU", "item.h",
                "takes PARAM_ITEM_USE *p (type@0, user@4, start@8, end@0x18) and "
                "gp-addresses COUNTER_FOR_ITEM_ARRAY_")
    return (None, None, None)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--model", default="sonnet",
                    help="suggested model (annotation only)")
    args = ap.parse_args()
    name = args.name
    addr, size = func(name)
    split = is_jump_table(addr, size)
    near = nearest_matched(name)
    fam, header, famnote = family(name)

    # Routing: an IDENTICAL mnemonic sequence (findsimilar 1.00) is a NEAR-CLONE —
    # same code, only immediates/symbols differ. Don't burn a full agent; clone
    # the twin and adjust the few constants matchdiff flags. (Exact byte-identical
    # -> tools/clonematch.py instead.)
    if near and near[0][0] >= 0.99:
        twin = near[0][1]
        print(f"# NEAR-CLONE of {twin} (findsimilar {near[0][0]:.2f}) — DON'T spawn "
              f"a full agent; do this inline (~2 min):")
        print(f"#   1. cp src/main.exe/{twin}.c src/main.exe/{name}.c  (substitute "
              f"the function name; item Req/Proc twins also swap Proc<Twin>-><this>)")
        print(f"#   2. tools/reverse.py {name} --ghidra-export .shake/ghidra-export "
              f"--no-check  (carve); ./Build; tools/gpsyms.py {name} --write")
        print(f"#   3. tools/matchdiff.py {name} — it shows the few differing insns "
              f"(a %lo(symbol) or an li immediate); change the matching source "
              f"constants; a local `j` that only differs in target auto-relocates.")
        print(f"#   4. ./Build check. If the mnemonics turn out to differ (not a "
              f"true clone), fall back to the full agent prompt below.\n#")
        # still print the full prompt below as a fallback

    examples = ", ".join(f"src/main.exe/{n}.c ({s:.2f})" for s, n in near) or \
        "(run tools/findsimilar.py to pick worked examples)"
    ex_names = [n for _, n in near]

    P = []
    P.append(f"You are a matcher agent for the Tenchu PS1 decomp. Your ONE "
             f"function: **{name}** @ {addr:#010x} ({size} bytes).")
    if near:
        P.append(f"Nearest matched (worked examples to imitate): {examples}.")
    if fam:
        P.append(f"It's a {fam} function — {famnote}. Reuse {header} types; "
                 f"don't redefine them.")
    P.append("")
    P.append("FIRST read, in order: .claude/agents/matcher.md (your contract), "
             "docs/matching-cookbook.md (workflow + Iteration protocol + rules)"
             + (", then " + ", ".join(f"src/main.exe/{n}.c" for n in ex_names)
                + (f" and src/main.exe/{header}" if header else "")
                if ex_names else "") + ".")
    P.append("")
    P.append("Worktree setup (link untracked inputs from the main checkout):\n"
             "```\nROOT=/home/shana/programming/tenchu-decomp\n"
             "for e in slps_019.01 system.cnf tenchu; do ln -sfn \"$ROOT/disks/$e\" "
             "\"disks/$e\"; done\nmkdir -p .shake && ln -sfn "
             "\"$ROOT/.shake/ghidra-export\" .shake/ghidra-export\n```\n"
             "Run build/tool commands via `nix develop --command bash -c \"...\"`.")
    P.append("")
    P.append("Tool-driven workflow (lean on these — they do the mechanical work):")
    P.append(f"- `tools/reverse.py {name} --ghidra-export .shake/ghidra-export` — "
             "splits + seeds the file with BOTH a Ghidra and an m2c reference.")
    if split:
        P.append(f"- **{name} is a JUMP-TABLE function** — after reverse.py, run "
                 f"`tools/split-scaffold.py {name}` (writes all INCLUDE_ASM pieces "
                 "+ the jtbl + the .rodata carve, green before you draft).")
    P.append(f"- `tools/xref.py {name}` — callers (pin the prototype) + callees "
             "(matched vs needs-extern).")
    P.append(f"- `tools/access.py {name}` — each pointer offset's WIDTH/"
             "signedness/direction (sw vs sh vs sb, lh vs lhu) straight from the "
             "mnemonics; `--order` shows the store SEQUENCE. Use it for struct "
             "layout instead of hand-tracing .s.")
    P.append(f"- `tools/gpsyms.py {name} --write` — auto-derives + syncs the "
             "gp-externs into Build.hs + permute.py (run after splitting).")
    P.append(f"- `tools/matchdiff.py {name}` / `tools/asmdiff.py {name}` — iterate. "
             f"Done = matchdiff MATCH (0 whole-image diffs) AND `./Build check` "
             "exit 0.")
    P.append("")
    P.append("Guidance (deeper detail in the cookbook):")
    for g in GUIDANCE:
        P.append(f"- {g}")
    P.append("")
    P.append("Follow the contract exactly: never commit, writable-file allowlist "
             "only, restore the stub (NON_MATCHING convention) on failure after "
             "~10 attempts. Report back: MATCH or CURRENT(N); which tools you used; "
             "**where you had to reason manually that a tool could have done "
             "(this run also evaluates the tooling for a smaller model)**; files "
             "touched; any NEW cookbook rule; gp/symbol additions; contract "
             "deviations.")

    print(f"# suggested model: {args.model}"
          + ("  |  JUMP-TABLE function" if split else "")
          + (f"  |  {fam}" if fam else ""))
    print("\n".join(P))


if __name__ == "__main__":
    main()
