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
import argparse
import bisect, os, re, subprocess, sys

import function_inventory as FI
import triage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TSV = ".shake/ghidra-export/functions.tsv"
SYMBOLS = "config/symbols.main.exe.txt"
SPLAT = "config/splat.main.exe.yaml"
DEMO_TSV = "reference/demo-psxexe.functions.tsv"

# ---- evolving guidance: the load-bearing lessons every agent should get at
# launch (deeper detail is in the cookbook, which the agent reads). Edit here. --
GUIDANCE = [
    "THE ORIGINAL WAS C A HUMAN WROTE — use that as a lever, not just a "
    "sentiment. A parked draft that byte-chased its way to a low residual is "
    "often a LOCAL OPTIMUM AWAY FROM THE SOURCE: full of fences, split "
    "variables, dead-carrier reuse and seed temps no person types. Those props "
    "can be exactly what pins you above 0. So when a residual looks stuck, try "
    "writing the function the way a human would — from PSX.SYM's own declarations "
    "if it has a `BEGIN PSX.SYM` block (its parameter names, local names/types, "
    "and nested-block scopes ARE the structure), and from the matched sibling's "
    "own MACROS for any do{}while(0) cluster (a statement-wrapped function-like "
    "macro is a legitimate HUMAN fence — transcribe the sibling's macro before "
    "deleting fences one by one). EXPECT WORSE BYTES AT FIRST and refine from "
    "there; a cleaner human base out-refines a byte-chased dead end, and the "
    "owner has adopted the human structure over a lower byte count more than once "
    "(SetWire 70->80, mission_score_screen 145->187). The target itself is a "
    "source oracle: a const/copy def next to its uses means the original set that "
    "variable ONCE (birthing bump); a load above a byte store means the human "
    "wrote the load first (QImode alias pin). Everything is banked in git, so "
    "this costs nothing to try.",
    "GTE/COP2 operations are matchable ONLY for functions whitelisted in "
    "config/gte-allowlist.txt, via the shared src/main.exe/gte.h macro layer "
    "(PsyQ INLINE_N.H names; GTE commands as .word) plus pinned-register "
    "locals for non-ABI entries — see docs/gte-policy.md. __asm__ anywhere "
    "else is an automatic matchdiff blocker; never use it to force bytes.",
    "If this function still has a FUN_ or hand-written descriptive name and "
    "the original demo name becomes genuinely converged, report/adopt it "
    "instead of leaving the custom label: first prove the demo name is unused "
    "in the current retail inventory, callmatch --verify is non-ambiguous (or "
    "a byte-matched callback setter supplies identity), and prototype, constants, "
    "behavior, and surrounding source allocation agree. Keep the exact-C match "
    "and the rename as separate commits. A candidates.tsv row alone is never "
    "enough.",

    "`./Build check` is the ONLY gate. matchdiff's window can be shorter than "
    "the function (and for a split function covers only piece 1 — use asmdiff). "
    "Removing the INCLUDE_ASM is not evidence of a match; never report MATCH "
    "without a green `./Build clean && ./Build check`.",

    "The permuter is a NON-LEVER for any function that `#include`s gte.h "
    "(gte-allowlist targets): `tools/permute.py` REFUSES inline-asm functions at "
    "the gte.h gate (permute.py:985) BEFORE it compiles — its C parser can't read "
    "the macro layer, so it never searches the function at all and the -fno-builtin "
    "CC_FLAGS fix changes nothing for this class. For a gte.h target, skip the "
    "permuter and escalate STRAIGHT to RTL regardless of how small the residual is "
    "(FUN_80057b80's 8-byte sched2 tie, FUN_80058c70's prologue tie). Otherwise: "
    "the permuter is worth ONE bounded FOREGROUND run (`timeout 300 "
    "tools/permute.py <Name> -- --stop-on-zero -j4`; never background it, never "
    "Monitor-wait it) whenever autorules finds nothing and the residual is the "
    "SAME LENGTH. But do NOT park a same-length residual just because the permuter "
    "fails — that is the signal to READ THE RTL. Run `tools/rtldump.py <Name>` and "
    "follow the cookbook's \"Reading cc1's RTL dumps (the escalation method)\": "
    "nine \"permuter-immune\" ties fell this way (register-coalescing, guard "
    "delay-slot, %hi-reload, loop.c hoist — all previously listed as un-permutable) "
    "and every one was reachable source structure named in the dumps. Only a GTE "
    "COMMAND opcode (mvmva/rtps in the .s) is truly un-matchable in this cc1 — park "
    "that on sight.",

    "A bare `lui $rN,0xHHHH` with NO `addiu`, reused as a base across several "
    "accesses, means the source held the address in a LOCAL via a literal "
    "pointer cast — `(PersistentState *)0x80010000`, not `extern u8 D_80010047`. "
    "The extern form lets cc1 fold the address into each memory operand, so `as` "
    "re-materialises `%hi` per use through `$at`: extra `lui`s and WRONG LENGTH.",

    "For a stock PsyQ library leaf, do not assume the game's cc1 address-splitting "
    "default. If the original SDK object proves one unsplit `la` feeding multiple "
    "offset-0 accesses, use a narrowly scoped Build.hs `ccExtraFlags` entry such as "
    "`-mno-split-addresses`, and mirror it in permute.py's `CC_EXTRA_FLAGS`. Confirm "
    "the provenance and run check-all before accepting a per-TU flag (MemCardCallback).",

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
    "Lean on the tools; don't re-derive their output by hand. maspsxflags --write "
    "syncs gp externs plus guarded-variable-div flags, xref gives the prototype/"
    "externs, reverse.py seeds both "
    "a Ghidra AND an m2c reference (m2c = cleaner control flow / register temps, "
    "Ghidra = types/names).",
    "Once your draft COMPILES (set NON_MATCHING for a partial), run "
    "`tools/autorules.py <Name>` BEFORE hand-tuning: it mechanically sweeps the "
    "*local* cookbook rules — every local's integer type (s16<->s32 index/call-"
    "result, sign toggles), splitting/merging `&&` chains, inlining single-use "
    "temps — and greedily keeps what shrinks the asmdiff, reporting which edit "
    "helped (`n: s16->s32: 4->0`). It's the deterministic first pass — if it "
    "reports no win, the residual is NOT one of those mechanical rules (don't "
    "keep guessing them; it's structure/regalloc — read the diff or permute). "
    "Type-width deliberately refuses signed->unsigned candidates when the local "
    "is explicitly sign-tested or arithmetically right-shifted: a better partial "
    "score cannot justify deleting a negative correction or changing sra to srl.",
    "If a returning guard's branch delay slot contains the fallthrough's first "
    "arithmetic producer, keep that producer AFTER the returning `if`; hoisting "
    "it before the guard lengthens its lifetime and can make reorg steal the "
    "wrong later instruction instead (death_camera_something_).",
    "When an inlined endian/byte-pack helper's target loads use two already-live "
    "cursor registers, pass both cursor identities to the helper and load each "
    "byte through its target base. This can preserve both inline pseudos and an "
    "address-taken output store (AfsGetEntry); require the raw asm load bases.",
    "If the target freshly reloads an extern pointer-array slot before a field "
    "read even though cc1 can CSE the earlier pointer, use a site-local volatile "
    "view of the SLOT: `(*(T *volatile *)&Array[i])->field`. Do not make `T` or "
    "the shared extern globally volatile; require the target's reload+nop evidence "
    "(UpdateEvent). If an index-first integer pointer sum still interleaves its "
    "narrow sll/sra with `%hi/%lo`, name the scaled byte offset in an s32 statement "
    "before the sum.",
    "If a single-use stack parameter is load-sunk but the target reads it once at "
    "entry and keeps the value across calls, an ordinary local copy may collapse. "
    "Qualify the parameter object and copy it once instead: `volatile int mode`, "
    "then `saved = mode` and use only `saved`. Top-level parameter qualifiers are "
    "ABI-compatible; require the target's one early load (CdaPlayXA).",
    "For a pure REGISTER tie (right length, right instructions, a value in the "
    "wrong register — and autorules found nothing + a short permuter run won't "
    "beat the base), don't blind-permute: run `tools/regalloc.py <Name>`. It runs "
    "cc1 -dg and shows which values are live across calls (forced callee-saved), "
    "the pseudo->hard-reg map, and the copy-chains that bias the coloring — so you "
    "break the specific copy-chain / shorten the live range it names instead of "
    "guessing. (This is a genuine sub-C tie class; cap effort and NON_MATCHING it "
    "if regalloc.py shows the target's register is unreachable from C.)",
    "If an indexed table loop and a pointer cursor have the same final induction "
    "instructions but the preheader addresses are scheduled in the wrong order, "
    "try `T[index]` with an `s32` index. loop.c can eliminate that BIV and create "
    "the strength-reduced pointer GIV later, changing its RTL UID/order without "
    "changing the loop body. Require `.loop` BIV/GIV evidence and `.sched2` "
    "confirmation (check_for_known_button_combination).",
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
    funcs = FI.load_functions(TSV)
    funcs, _changed = FI.overlay_current_names(funcs, SPLAT)
    for addr, size, current_name in funcs:
        if current_name == name:
            return addr, size
    sys.exit(f"matcher-prompt: {name} not in the current function inventory")


def has_demo_body(name):
    """Whether the committed demo inventory has one same-named function body."""
    found = 0
    for line in open(DEMO_TSV):
        parts = line.rstrip("\n").split("\t")
        if len(parts) >= 4 and parts[0] == "F":
            parts = parts[1:]
        if len(parts) >= 3 and parts[2] == name:
            found += 1
    return found == 1


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

    Delegates to tools/symnote.py so there is exactly ONE implementation of the
    ownership rule (resolve against the dense symbol table, not the sparse typed
    set -- see the note there)."""
    try:
        sys.path.insert(0, "tools")
        import symnote
        decls = symnote.globals_of(name)
    except Exception:
        return []
    if not decls:
        return []
    out = ["- Globals it touches, with the original declaration (reference/"
           "psxsym-globals.h) — use these instead of inventing a `D_` layout:"]
    out += [f"    `{d}`" for d in decls]
    return out


def repeated_local_scope_hints(rows):
    """Turn repeated PSX.SYM local names into an explicit scope warning.

    The linker debug stream records one row per source declaration, including
    same-named declarations in nested blocks.  Collapsing those rows into one
    function-scope C variable also collapses their RTL pseudos, so surface the
    declaration identity mechanically instead of relying on a worker to notice
    duplicate names in a long locals list.
    """
    grouped = {}
    order = []
    for row in rows:
        if len(row) != 6:
            continue
        _, _, kind, where, ty, local_name = row
        if local_name not in grouped:
            grouped[local_name] = []
            order.append(local_name)
        grouped[local_name].append((kind, where, ty))

    repeated = [(name, grouped[name]) for name in order
                if len(grouped[name]) > 1]
    if not repeated:
        return []

    out = [
        "- **Scope identity hint:** PSX.SYM records these as separate "
        "same-named declarations. Keep them block-scoped initially; one "
        "function-scope variable creates one broad RTL pseudo/hard-register "
        "identity. Demo register homes are hints, not retail requirements:"
    ]
    for local_name, declarations in repeated:
        sites = ", ".join(f"{kind} {where}" for kind, where, _ in declarations)
        out.append(f"    `{local_name}` x{len(declarations)} ({sites})")
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
    loc = "reference/psxsym-locals.tsv"
    if os.path.exists(loc):
        rows = [l.rstrip("\n").split("\t") for l in open(loc) if not l.startswith("#")]
        mine = [r for r in rows if len(r) == 6 and r[0] == name]
        if mine:
            out.append(f"- **The original locals** ({len(mine)}), from PSX.SYM. Their "
                       f"NUMBER and TYPES are high-value codegen evidence, not a retail "
                       f"spec: an earlier-build helper/API change can replace either. "
                       f"Retail access widths and callee ABI win. A repeated name is a "
                       f"nested-block scope, not a duplicate:")
            for _, _, kind, where, ty, vn in mine[:16]:
                out.append(f"    {kind:5s} {where:8s} {ty} {vn}")
            out.extend(repeated_local_scope_hints(mine))
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
    ap.add_argument("--force", action="store_true",
                    help="brief a target triage EXCLUDES (handwritten-asm canonical). "
                         "Only if the owner decision in config/handwritten-asm.txt "
                         "changed — not to 'try anyway'.")
    args = ap.parse_args()
    name = args.name

    # REFUSE an excluded target before doing anything else. The orchestrator picked
    # drawF3 by ranking parked drafts on raw residual byte count -- bypassing
    # findsimilar/triage, which rank it 0.008 (dead LAST of ~1010) and hide it by
    # design -- and briefed a lane onto a HANDWRITTEN-ASM original whose asm IS the
    # canonical source. The lane spent a full round re-deriving an owner-closed
    # investigation. Worse, the brief pointed at `li r,0` vs `move r,zero` as the
    # opportunity: that spelling is the very cc1-invariant TELL the exclusion is
    # based on, and the cookbook has a standing park-on-sight rule for it.
    # "Is this a legitimate target?" is a decision over data we already have --
    # so it is a program, not something a brief-writer should remember.
    reason = triage.parked(name)
    if reason and "handwritten-asm CANONICAL" in reason and not args.force:
        sys.exit(
            f"matcher-prompt: REFUSING to brief {name} — it is NOT a matching "
            f"target.\n"
            f"  {reason}\n"
            f"  It is listed in config/handwritten-asm.txt (owner decision, "
            f"docs/gte-policy.md);\n"
            f"  progress.py counts it as done-by-asm and triage.py hides it from "
            f"targets.\n"
            f"  A residual byte count for this function is MEANINGLESS: there is no C "
            f"that\n"
            f"  reaches it, so 'only N bytes left' is not evidence of closeness.\n"
            f"  Pick targets with `tools/findsimilar.py --targets` or "
            f"`tools/triage.py`, which\n"
            f"  exclude this class already. Override with --force only if the owner "
            f"decision changed.")

    addr, size = func(name)
    split = is_jump_table(addr, size)

    # PRINT THE EXISTING PARK STATUS. triage.parked() already returns the file's own
    # STATUS line, and the brief-writer threw it away: I hand-wrote "ActITEM has no C
    # at all yet -- you are drafting it from scratch" for a function that had carried
    # a full NON_MATCHING draft at 26/572 for two days. The lane caught it from the
    # file and ignored my brief, which is the only reason the round was not wasted.
    # "Does a draft already exist, and what does its header claim?" is a lookup, not
    # something a brief-writer should reconstruct by eye.
    status = reason if reason and "handwritten-asm CANONICAL" not in reason else None
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
              f"  (carve — let it run its check); ./Build; tools/maspsxflags.py {name} "
              f"--write")
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
    if status:
        P.append(f"\n**THIS FUNCTION IS ALREADY PARKED — a draft exists. Read "
                 f"src/main.exe/{name}.c BEFORE anything else, and pick up from its "
                 f"banked checkpoint; do NOT re-derive it.** Its own STATUS line says:\n"
                 f"    {status}\n"
                 f"Treat that as a HYPOTHESIS, not a fact — park prose has been wrong "
                 f"in every direction here (a 'failed attempt' that was a sibling's "
                 f"winning technique; a negative measured with another variable pinned; "
                 f"a permuter-immune claim whose search had crashed). But it is the "
                 f"starting state, and any task text that contradicts it is stale: "
                 f"trust the FILE over the brief.")
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
             "```\nROOT=" + ROOT + "\n"
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
    if near:
        P.append(f"- `tools/siblingdiff.py {name}` — **START DRAFTING HERE.** Prints "
                 "the nearest MATCHED sibling's C plus a normalized asm diff "
                 "(intra-function branches aligned to labels; `ref:`=sibling, "
                 "`tgt:`=target, runs of identical insns collapsed). Transcribe the "
                 "sibling's C, then change ONLY the flagged instructions. A hunk "
                 "that differs in just the REGISTERS (same mnemonics/operands "
                 "shape) is an allocation tie for rtldump/permute, not a source "
                 "edit. Pass a different candidate as the 2nd arg to compare vs "
                 "another sibling.")
    if has_demo_body(name):
        P.append(f"- `tools/siblingdiff.py {name} --demo` — compare retail with the "
                 "same-named earlier demo body. It relativizes local branches and "
                 "normalizes cross-build call targets to shared names, exposing "
                 "preserved statement/control-flow shape without confusing relinked "
                 "addresses. Treat it as a source-structure oracle; retail bytes "
                 "remain authoritative.")
    P.append(f"- `tools/xref.py {name}` — callers (pin the prototype) + callees "
             "(matched vs needs-extern).")
    P.append(f"- `tools/access.py {name}` — each pointer offset's WIDTH/"
             "signedness/direction (sw vs sh vs sb, lh vs lhu) straight from the "
             "mnemonics; `--order` shows the store SEQUENCE. Use it for struct "
             "layout instead of hand-tracing .s.")
    P.append(f"- `tools/stackplan.py {name}` — target/candidate frame, outgoing-"
             "argument boundary, first saved-register spill, and every working "
             "sp+offset access. Run it when aggregate locals inflate the frame; "
             "its working-window size is the mechanical scratch-union extent.")
    P.append(f"- `tools/maspsxflags.py {name} --write` — auto-derives + syncs "
             "the gp externs and guarded-variable-div `--expand-div` setting "
             "into Build.hs + permute.py (run after splitting).")
    P.append(f"- `tools/matchdiff.py {name}` / `tools/asmdiff.py {name}` — iterate. "
             f"Done = matchdiff MATCH (0 whole-image diffs) AND `./Build check` "
             "exit 0.")
    P.append("")
    P.append("Guidance (deeper detail in the cookbook):")
    for g in GUIDANCE:
        P.append(f"- {g}")
    P.append("")
    P.append("Follow the contract exactly: commit the final exact result or "
             "honestly documented checkpoint to your isolated worker branch, "
             "never push/merge/rebase, writable-file allowlist only, restore "
             "the stub (NON_MATCHING convention) on failure after ~10 attempts. "
             "Report back: MATCH or CURRENT(N), worker commit hash; which tools you used; "
             "**where you had to reason manually that a tool could have done "
             "(this run also evaluates the tooling for a smaller model)**; files "
             "touched; any NEW cookbook rule; gp/symbol additions; contract "
             "deviations.")

    print(f"# suggested model: {args.model}"
          + ("  |  JUMP-TABLE function" if split else "")
          + (f"  |  {fam}" if fam else ""))
    # Stamp master's tip into the prompt. wt-init.sh's staleness warning has a
    # bootstrap hole -- a worktree branched BEFORE that check landed runs the old
    # script and is never warned (one lane was 42 commits stale that way). This
    # runs in the PRIMARY worktree, so it is always current, and the expected SHA
    # travels inside the brief itself.
    head = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True,
                          text=True).stdout.strip()
    subject = subprocess.run(["git", "log", "-1", "--format=%s"],
                             capture_output=True, text=True).stdout.strip()
    if head:
        print(f"\nBASE CHECK (do this before ANY edit): master was at {head[:12]}"
              f' ("{subject}") when this task was written.\n'
              f"  git merge-base --is-ancestor {head[:12]} HEAD || git merge --ff-only master\n"
              "Worktrees are routinely branched from a stale commit -- lanes have "
              "started 26-42 commits behind, one missing the very checkpoint its\n"
              "task described, another missing a tool its task told it to run first. "
              "Re-measure your baseline after fast-forwarding.")
    print("\n".join(P))


if __name__ == "__main__":
    main()
