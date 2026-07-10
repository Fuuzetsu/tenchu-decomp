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
import argparse, bisect, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TSV = ".shake/ghidra-export/functions.tsv"
SYMBOLS = "config/symbols.main.exe.txt"

# ---- evolving guidance: the load-bearing lessons every agent should get at
# launch (deeper detail is in the cookbook, which the agent reads). Edit here. --
GUIDANCE = [
    "`./Build check` is the ONLY gate. matchdiff's window can be shorter than "
    "the function (and for a split function covers only piece 1 — use asmdiff). "
    "Removing the INCLUDE_ASM is not evidence of a match; never report MATCH "
    "without a green `./Build clean && ./Build check`.",

    "The permuter is worth ONE bounded run whenever autorules finds nothing and "
    "the residual is the SAME LENGTH as the target — it has cracked 5-byte "
    "register ties, a 61-byte schedule/colour miss, and a pure statement-order "
    "fix (one field store one line earlier than its offset-order position). Only "
    "two residual classes are known permuter-immune, and you must NOT permute "
    "them: the `la`/address-materialization reload tie (`%hi` in a temp vs the "
    "target register), and the guard delay-slot fill tie (branch's own `move "
    "$v0,zero` vs the fallthrough's data-independent `lui`). Park those at once.",

    "A bare `lui $rN,0xHHHH` with NO `addiu`, reused as a base across several "
    "accesses, means the source held the address in a LOCAL via a literal "
    "pointer cast — `(PersistentState *)0x80010000`, not `extern u8 D_80010047`. "
    "The extern form lets cc1 fold the address into each memory operand, so `as` "
    "re-materialises `%hi` per use through `$at`: extra `lui`s and WRONG LENGTH.",

    "If the link fails with `undefined reference to D_XXXXXXXX`, splat only "
    "auto-labels data that still-carved *asm* references — matching the last "
    "such function drops the symbol. Add a plain `D_XXXXXXXX = 0xXXXXXXXX;` to "
    "config/symbols.main.exe.txt (that file has NO comment syntax).",

    "A 3-instruction `sll 16 / sra k / sra 16-k` (k!=16) is the blocked GetPad "
    "sign-extension class — park it. The ordinary 2-instruction `sll 16 / sra K` "
    "is just a short-indexed array's fused extend+scale and matches fine.",

    "Before trusting the Ghidra export's SIZE, remember two entries are "
    "under-sized (LoadCard, FUN_800593a0) and a truncated carve still builds "
    "GREEN — run `tools/coverage.py` if a function's body seems to run past its "
    "carve, and re-carve with `reverse.py <Name> --size 0x<true>`.",

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
    "Before inventing a `D_XXXXXXXX` name or an anonymous struct layout for an "
    "unknown global, look it up in the Ghidra export: `.shake/ghidra-export/"
    "types.h` (by struct tag) and `symbols.tsv`/`functions.tsv` (by address). "
    "Ghidra often has the fully-named ORIGINAL struct/symbol (e.g. PadArrange, "
    "TCdaStatus) even when the calling function's own decompilation showed only "
    "offsets — use the real name/layout and add a plain `NAME = 0xADDR;` to "
    "config/symbols if it isn't auto-named.",
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


def globals_touched(name):
    """Globals this function references, typed from PSX.SYM.

    Saves the agent from inventing a layout for `D_800BC108` when the original
    source called it `struct ConflictObjectType ConflictObject[64]`.
    """
    hdr = "reference/psxsym-globals.h"
    if not os.path.exists(hdr):
        return []
    sys.path.insert(0, "tools")
    try:
        import datamatch as D
    except Exception:
        return []
    try:
        addr, size = func(name)
    except SystemExit:
        return []
    decl = {}
    for line in open(hdr):
        m = re.match(r"extern (.*);\s+/\* (0x[0-9a-f]+)", line)
        if m:
            decl[int(m.group(2), 16)] = m.group(1)
    if not decl:
        return []
    exe = open("disks/tenchu/main.exe", "rb").read()
    refs, _ = D.data_refs(exe[0x800:], 0x80011000, addr, size, D.RETAIL_GP, *D.RETAIL_TEXT)
    starts = sorted(decl)
    hits = []
    for _, y in refs:
        i = bisect.bisect_right(starts, y) - 1
        if i >= 0 and y - starts[i] < 0x2000 and starts[i] not in hits:
            hits.append(starts[i])
    if not hits:
        return []
    out = ["- Globals it touches, with the original declaration (reference/"
           "psxsym-globals.h) — use these instead of inventing a `D_` layout:"]
    for a in hits[:12]:
        out.append(f"    `extern {decl[a]};`  /* {a:#010x} */")
    return out


def psxsym_facts(name):
    """Original prototype / TU / storage class for `name`, from the demo's PSX.SYM
    (see docs/psx-sym.md).  These are the real names the authors used; a matching
    signature usually falls straight out of them."""
    out = []
    proto = f"reference/psxsym-protos.h"
    if os.path.exists(proto):
        pat = re.compile(rf"^(.*\b{re.escape(name)}\s*\(.*)$", re.M)
        m = pat.search(open(proto).read())
        if m:
            out.append(f"- **Original prototype** (PSX.SYM, parameter names are the "
                       f"authors' own): `{m.group(1).strip()}`")
    tumap = "reference/psxsym-tu-map.tsv"
    if os.path.exists(tumap):
        for line in open(tumap):
            p = line.rstrip("\n").split("\t")
            if len(p) == 8 and p[7] == name:
                out.append(f"- PSX.SYM says it lived in **{p[2]}:{p[3]}**, `{p[6]}`, "
                           f"frame {p[4]} bytes, saved-reg mask {p[5]}. Functions from "
                           f"one source file are contiguous — its neighbours in "
                           f"reference/psxsym-tu-map.tsv are its TU-mates, which pins "
                           f"rodata pooling and %gp choices.")
                break
    for line in globals_touched(name):
        out.append(line)
    cand = "reference/psxsym-candidates.tsv"
    if name.startswith("FUN_") and os.path.exists(cand):
        for line in open(cand):
            if line.startswith("#"):
                continue
            p = line.rstrip("\n").split("\t")
            if len(p) >= 6 and p[1] == name:
                out.append(f"- PSX.SYM suggests this may be **{p[2]}** ({p[4]} confidence, "
                           f"{p[5]}). NOT adopted — corroborate with "
                           f"`tools/callmatch.py --verify` before renaming anything.")
    if out:
        out.insert(0, "Original-source facts recovered from the demo's PSX.SYM:")
    return out


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
    for line in psxsym_facts(name):
        P.append(line)
    P.append("")
    P.append("FIRST read, in order: .claude/agents/matcher.md (your contract), "
             "docs/matching-cookbook.md (workflow + Iteration protocol + rules)"
             + (", then " + ", ".join(f"src/main.exe/{n}.c" for n in ex_names)
                + (f" and src/main.exe/{header}" if header else "")
                if ex_names else "") + ".")
    P.append("")
    P.append("Worktree setup (link untracked inputs from the main checkout):\n"
             "```\nROOT=/home/shana/programming/tenchu-decomp\n"
             "for e in slps_019.01 system.cnf tenchu demo; do ln -sfn \"$ROOT/disks/$e\" "
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
