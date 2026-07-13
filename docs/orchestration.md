# Orchestrating matcher agents — the runbook

How the **orchestrator** (the main session) drives byte-matching at scale by
spawning `matcher` subagents, harvesting their work, and compounding the
tooling. If you're resuming this work, read this first, then
[matching-cookbook.md](matching-cookbook.md) for the actual matching idioms.

The split of responsibility:
- **Matcher agent** (`.claude/agents/matcher.md`) — matches ONE function (or a
  bundle) in an isolated git worktree. Never commits. Reports MATCH/CURRENT(N),
  files touched, new rules, and *where it had to reason manually that a tool
  could have done the work* (that last part drives the tooling loop).
- **Orchestrator** (you) — picks targets, generates prompts, launches agents,
  **harvests on green, commits**, folds reported rules into the cookbook, and
  **builds a tool whenever a friction recurs**.

## Resuming the hands-off flywheel (quick-start)

**State (keep this line current):** ~322/555 game functions (~58%), ~30% of
game bytes, whole-image byte-identical (sha256 `0690a5c1…3558`). Live count:
`nix develop --command tools/progress.py`.

**The loop (repeat until the user stops you):**
1. `nix develop --command 'tools/triage.py'` → pick a batch of **5 clean-seam
   functions**. Clean seam = non-jump-table, ≤~500 bytes, non-dispatch, ideally
   with a matched twin (`~0.NN to <Twin>`). triage already hides parked functions
   and de-ranks the GTE/SIGNEXT classes; you still filter OUT `switchD`,
   `Think3*`, `handle_char_state_*`, `0x80060xxx`, and — until the inline-asm
   policy lands — **`DrawTMD`**, whose handler interface uses non-ABI live
   registers (m2c confirms the callees read `input_t0/t3/t5`). Prefer trivial/easy
   tier + a strong twin. **`LoadCard` and `FUN_800593a0` are under-sized in
   `functions.tsv`** — carve them with `--size 0x168` / `--size 0x27C` or their
   `.c` can never match (see `tools/coverage.py`).
   **Select only from the live `triage.py` output on current master.** Old session
   summaries, `reference/psxsym-unnamed.tsv`, and a filename/comment saying
   `NON_MATCHING` are not match-status inventories. Before dispatch, run
   `tools/triage.py <Name>` and check the current source still has an active
   `INCLUDE_ASM`; the worker's first action is an authoritative `matchdiff`/build
   preflight. If the function is already exact pure C, recycle the slot immediately
   instead of creating a duplicate checkpoint. This avoided repeated work on
   `ProcItemFire` and `FUN_8003562c`, both of which old target notes still described
   as unmatched after master already contained exact implementations.
2. Spawn ONE `general-purpose` agent, `model: "sonnet"`, `isolation:
   "worktree"`, `run_in_background: true`, with a prompt naming the 5 functions,
   their matched twins, the proven shared structs, and the early-stop rules
   (copy a recent batch prompt from this session's history — they're near-
   identical; the only variables are the 5 names + twins). Two agents in
   parallel on disjoint families is fine.
3. On the completion notification, **harvest** (procedure below), `./Build
   clean && ./Build check` must be **BYTE-IDENTICAL**, then commit.
4. Fold any genuinely-novel mechanical rule into `matching-cookbook.md`; fix any
   reported tool bug centrally. Be selective — the cookbook is mature.
5. Brief progress line to the user. Repeat.

**Economics that make it hands-off:** the clean seam matches **4–5/5 per batch
at ~150–260k agent tokens**; the tie-prone bucket (big `handle_char_state_*`/
`Think3*` dispatchers, 1400–1700 B) costs 2–3× for 2/5 and its residuals are
sub-C reload/schedule ties — **DEFER them** (don't burn Sonnet on them). If a
batch's residual is one of the two NAMED permuter-immune classes — the
`la`/address-materialization reload tie, or the guard delay-slot fill tie — it is
**NON_MATCHING immediately**, never permute. Otherwise, **a right-LENGTH residual
is worth one bounded permuter run**: it has cracked 5-byte register ties, a
61-byte schedule/colour miss, and a pure statement-order fix (see the cookbook's
early-stop section — its pessimistic tone predates those).

A **MATCHED** jump-table function harvests fine: `reverse.py <Name>` then
`./Build` then `split-scaffold.py <Name>` reproduces both the `c` carve and the
`.rodata` carve (done for `EquipWeapon`). Two-piece `__override__prt_` splits are
automatic now. What is still fiddly is a jump-table **NON_MATCHING** stub —
**defer it** (leave in the blob, save the draft to `scratch/`), like
`cd_open`/`handle_char_state_using_item_`/`FUN_80027818`.

**Biggest lever — family-first:** before a new subsystem, map its shared struct
ONCE (`item.h`, `game_types.h`) and point every agent at it; sequence small
leaves first so the struct gets byte-proven early, then the family is cheap.
`character_state` (AI) is now proven; `item.h`'s `Humanoid`/`ModelType`/
`MotionManager`/`MotionRegistType`/`MotionDataType` are proven.

## The tool map

Everything the pipeline needs, in the order you touch it:

| Tool | Does |
|---|---|
| `tools/triage.py` | rate every function EASY..VERY-HARD + why + relevant cookbook sections; `--leverage` = call in-degree; `<Name>` = detail; `--parked` = the shelved ones + why. Pick targets. It now **hides** functions already parked NON_MATCHING and flags the two un-matchable idiom classes (`GTE CMD — UN-SPLITTABLE`, `SIGNEXT-SPLIT`), so the default list is all live targets. |
| `tools/findsimilar.py [--targets]` | mnemonic-similarity ranking (nearest matched / easiest unmatched). |
| `tools/siblingdiff.py <Name> [<Sibling>] [--demo]` | **the drafting kickstart for related functions** — ordinary mode auto-picks the nearest MATCHED retail sibling, prints its C plus a normalized asm diff. `--demo` instead aligns the same-named earlier demo function, relativizing local branches and replacing cross-build call/jump addresses with shared function names. It also flags a demo-only named call when most of that retail helper's mnemonic bigrams occur inside the target—the mechanical tell for a likely local inline expansion. Identical runs collapse in both modes. The demo exposes preserved source/control shape, but retail bytes remain the exact-match authority. |
| `tools/matcher-prompt.py <Name>` | **generate the agent launch prompt** — never hand-write it. Auto-fills address/size, jump-table detection, nearest matched worked-examples, TU family, and the evolving `GUIDANCE` block. Routes findsimilar-1.00 near-clones to an inline recipe. |
| `tools/reverse.py <Name> --ghidra-export .shake/ghidra-export` | split + seed the `.c` with Ghidra C **and** m2c C **and** a `triage:` line + likely-relevant docs. Adds the splat carve. |
| `tools/split-scaffold.py <Name>` | jump-table function: all INCLUDE_ASM pieces + the jtbl array + the `.rodata` carve, green before drafting. |
| `tools/xref.py <Name>` | callers (pin the prototype) + callees (matched vs needs-extern). |
| `tools/symnear.py <Name-or-address> [...]` | list fixed symbols immediately before scalar addresses and compute candidate containing-field offsets; multiple queries also report common candidate bases. Use when `rtlguide` reports `enclosing-global-field-load`; confirm the proposed struct fields independently. |
| `tools/access.py <Name> [--order]` | each pointer offset's width/signedness/direction from the mnemonics; `--order` = the store sequence. Struct layout without hand-tracing `.s`. |
| `tools/stackplan.py <Name> [--emit-overlay]` | recover target/candidate frame size, outgoing arguments, saved-register boundary, and every stack access. It flags the address-taken xyz / second xyz save-reload signature of a likely `VECTOR[2]` workspace. `--emit-overlay` recognizes address-taken aggregates and prints a padded C workspace scaffold; use `--args N` to override an ambiguous inferred o32 boundary. Its prerequisite build is quiet on success; failures report the tail and path of `/tmp/tenchu-stackplan-build.log`, so fresh-worktree compiler warnings cannot bury the actual stack report. |
| `tools/gpsyms.py <Name> --write` | derive the gp-extern set from `%gp_rel` and sync Build.hs + permute.py. Shake-oracle-tracked, so it just takes effect. |
| `tools/maspsxflags.py <Name> --write` | preferred combined setup pass: sync gp externs and detect/sync ASPSX guarded variable division's `--expand-div` flag from the target asm. |
| `tools/merge_metadata_conflicts.py` | during a cherry-pick, conservatively resolve the recurring additive conflicts in Build.hs/permute.py gp and `--expand-div` tables. It unions only recognized metadata entries, rejects same-key disagreements or ordinary code, validates both files before writing, and leaves staging/`cherry-pick --continue` to the orchestrator. |
| `tools/clonematch.py <Matched>` | write the `.c` for exact byte-identical unmatched siblings (verified). |
| `tools/matchdiff.py <Name>` / `tools/asmdiff.py <Name>` | iterate. matchdiff = authoritative whole-image byte gate; asmdiff = aligned diagnostic view for big/split functions. `asmdiff --structural` deliberately hides every same-length replacement, so even zero displayed structural lines is not a match claim; its summary retains the raw residual and exact-sequence verdict. For guarded drafts, matchdiff rejects a linked `<Name>.NON_MATCHING` stub, so `-n` cannot bless a stale trivial match; an exact C draft also remains a failing/incomplete result until its `NON_MATCHING`/`INCLUDE_ASM` guard and inline `__asm__` are gone. |
| `tools/autorules.py <Name>` | once the draft compiles: mechanically sweep the *local* cookbook rules and greedily keep what shrinks the authoritative byte diff. It recognizes target-gp anti-sites, aggregate VECTOR copies, explicit builtin abs, pointer-index integer sums in declarations or later assignments, subscript-postincrement working copies, paired same-call argument producers, same-literal field assignment chains, and literal equality swaps; `--guided` can invert a compound if/else to score the opposite physical body layout, turn a three-literal equality ladder into all six sparse-switch body orders, split a constant right shift into every two-stage form, toggle divisible affine multiply spellings to block unwanted cross-jumps, name an extern array base before indexing, swap adjacent literal stores to distinct fields for late scheduling ties, toggle a local integer pointer's volatile pointee, fence one statement or a safe 2–4-statement range, add two or three nested weights atomically, split adjacent statements behind two one-shot fences, dead-evict a switch index from CSE, shift a statement across an existing LOOP_END, duplicate one statement into zero-code identical arms, duplicate one adjacent shared assignment into both compound if/else arms, try real case labels, and enumerate the six three-term addition trees while preserving nonconstant/call order. Output is line-buffered for monitoring and flags a byte win that worsens the equal-length aligned instruction shape as `LOCAL-SHAPE REGRESSION`. Candidates compile from a private staged source; only the selected result is atomically published. Owned build process groups, Linux parent-death handling, and the per-worktree matching lock prevent interrupted/orphaned work from mutating or racing the live source. It never emits inline asm. |
| `tools/permute.py <Name>` | late-stage decomp-permuter for pure register-allocation ties, after deterministic structure/RTL work. A preflight refuses drafts over one instruction from target length or broader than 128 aligned lines / 32 blocks; `--force-early` requires an explicit reviewed allocation-cascade justification. Every retained output is full-link rescored and must be bisected rather than pasted wholesale. It shares the per-worktree matching lock, so an accidental launch during autorules/diff/rtlguide fails immediately with the owning PID/target instead of producing a torn build. |
| `tools/dedupe-symbols.py [--check]` | one name per address in `config/symbols.main.exe.txt`. splat >= 0.4x refuses duplicates, and our file cannot disambiguate them (it doubles as an ld script, no comment syntax). Re-run after any Ghidra symbol import. |
| `tools/coverage.py [--all]` | code claimed by NO function — finds under-sized `functions.tsv` entries (a truncated carve still builds green; the tail becomes a `.data` blob that defines the `.L` labels, so nothing complains). `LoadCard` and `FUN_800593a0` are the two in game code. |
| `tools/rtldump.py <Name> [--pass …] [--draft]` | **the escalation tool** — standalone cc1-281 RTL dumps (`.greg`/`.lreg`/`.loop`/`.combine`/`.jump2`/`.sched2`/`.dbr`), race-free in the scratchpad, ~1 s. When a same-length residual beats respelling + the permuter, dump the pass that owns the diverging decision and read it (cookbook: "Reading cc1's RTL dumps"). Cracked 9 "permuter-immune" ties this session. |
| `tools/rtlguide.py <Name>` | **mechanical RTL escalation** — aligns target asm with our candidate, classifies each hunk by owning pass, recompiles with debug RTL notes, maps residual instructions back to C lines, names locals in the divergent hard registers, and emits the exact guided autorules command. It also audits target/candidate physical branch/jump/call/return counts (warning when a score win invents a conditional branch), detects target-only physical calls, distinguishes `jal abs` from target inline abs, recognizes postincrement working-copy, cross-call argument-pipeline, and terminal commutative-equality residuals, summarizes CALL_INSN fingerprints through jump2, names proven residual signatures, reports source lines whose first RTL instruction is fenced by LOOP_END in sched/sched2, and flags allocnos whose target hard register is categorically forbidden by `.greg` (so loop weighting cannot help). The target has no RTL; target asm is the specification and our RTL is the causal trace. `--json` is stable; direct runs share the matching lock. |
| `tools/regalloc.py <Name>` | **diagnose a register tie** — reads both `-dl` and `-dg`, filters to real global allocnos, shows refs/live-length/computed priority, pseudo→hard-reg dispositions, hard-register conflict sets, and copy chains. `--prefer a0` focuses the allocnos carrying one hard-register preference so distant call-argument donors become visible. `--compare P Q [--enclosed-refs N]` quantifies how many weighted refs/loop depths P needs to outrank Q; `--between SUBJECT LOWER UPPER` finds a bounded three-allocno priority window (equal-mode comparisons). Run this BEFORE blindly permuting a sub-C tie. |
| `tools/extract-demo.py`, `tools/psxsym.py`, `tools/symdump.py` | carve/parse/dump the demo disc's `PSX.SYM` — original prototypes, locals, structs, TU map. See [psx-sym.md](psx-sym.md). `matcher-prompt.py` injects the per-function facts automatically. |
| `tools/symmatch.py`, `tools/xbuildnames.py`, `tools/callmatch.py`, `tools/datamatch.py` | recover original **names** (functions, then globals) from `PSX.SYM` + the demo `PSX.EXE`. All four keep Ghidra's boundary/size inventory but mechanically overlay current splat `c` names, so adopted names, named callees, and data-reference pairs cannot go stale. |
| `tools/symnote.py --write --all` | stamp every `src/main.exe/*.c` with a `BEGIN PSX.SYM` block: the original prototype, locals (name/type/register-or-stack), touched globals, and any recorded candidate name. Idempotent; `--check` gates staleness; `--rename-params` adopts the authors' parameter names. Comments only — `./Build check` must stay byte-identical. |
| `tools/import_symbols.py --renames <tsv>` | adopt a rename table: renames existing symbols **and defines new ones** (rewriting splat's `D_…` auto-label across `src/`, the gp-extern lists, the yaml), then gates on a byte-identical `./Build check`. |

## Name recovery: never trust one matcher

Four independent matchers, each measuring its own precision against a control set of
things already named on both sides. Full detail in [psx-sym.md](psx-sym.md); the
operating rules:

- **Frame size + saved-register mask agreement is not evidence.** `symmatch` proposed
  `SetPadState` for `0x80032610` with *both* matching. The callees said
  `UpdateTexScroll`, and they were right. Call signature outranks frame shape.
- **Always `tools/callmatch.py --verify <table>` before `import_symbols.py`, then
  verify parameters/constants/semantics.** It checks that the demo function's named
  callees are *contained in* the retail function's (containment, not equality — retail
  functions grow). It rejects many positional false positives, but similar callbacks
  can collide: the historical `AttackFire` adoption passed containment even though a
  later demo-assembly audit identified retail `FUN_80027730` as the frame-range napalm
  function and the currently named function as a one-frame lightning callback.
  `callmatch --verify` now reports `AMBIGUOUS` and fails the gate when another
  full-containment candidate has no more extra named calls and no worse size distance.
- **Ambiguous ⇒ keep the placeholder, record the candidate.** `reference/psxsym-candidates.tsv`
  and `reference/psxsym-data-candidates.tsv` hold every suggestion we did not adopt;
  `matcher-prompt.py` surfaces them to whoever touches the function next.
- **Names and data feed each other.** `datamatch.py` can only see a global through a
  function named on *both* sides, so every batch of function renames unlocks more data
  symbols. **Re-run `datamatch.py` after any function renames.** It currently proposes
  zero — it has harvested everything the present 997 shared names can reach.

## Launching an agent

1. `tools/matcher-prompt.py <Name>` (in the nix devShell). Read its top line —
   it annotates JUMP-TABLE and the TU family, and if the target is a
   findsimilar-1.00 **near-clone** it prints an inline recipe instead: do that
   yourself (clone the twin, swap the differing constant, `matchdiff`) —
   ~2 min, no agent.
2. Otherwise paste the generated prompt into the **Agent** tool with
   `isolation: "worktree"` and the model from *Model routing* below. Background
   is fine; you're notified on completion.
   A command frontend reporting **`Script running with session ID ...` is not a
   completion notification**. It means that exact process is still live: poll
   that session until it returns an exit status. Never launch autorules,
   permute, matchdiff, asmdiff, or rtlguide in the same worktree while such a
   session remains active.
3. On the completion notification: **harvest** (below), commit on green, fold
   rules, remove the worktree.

The generated prompt already carries the contract pointer, the tool workflow,
the worked examples (which self-update as more functions match), and the
cross-cutting `GUIDANCE`. **Update `GUIDANCE` in `matcher-prompt.py` (and the
cookbook) as lessons accrue** — every future launch then inherits them.

## Model routing

- **Sonnet** — sub-~800B, non-jump-table, structural-twin-of-something-matched.
  It went 4/4 on the easy tier at ~65–110k tokens each; the mid Proc* at ~96k.
- **Fable** — jump-table functions, >~1000B, heavy register-allocation surgery,
  or anything with a residual that needs reading gcc-2.8.1 sources. The
  `matcher-prompt.py` top line flags jump-table; size and float/GTE (from
  `triage.py`) flag the rest.

## Bundling: one agent for many functions

The decisive lever. Bundling amortizes fixed setup (contract/cookbook/worked-
examples/headers, paid once) **plus** whatever *investigation transfers*
between the functions. Measured over four experiments:

| What's shared between the bundled functions | Cost/fn | vs separate agents |
|---|---|---|
| Whole function — exact/near clone (findsimilar 1.00, ≤3-word diff) | ~28k | **4–5× cheaper** |
| Skeleton — structural twins (findsimilar ~0.5–0.85) | ~84k | ~30% |
| Family conventions — low-sim but same TU family (shared idioms/dispose path) | ~96k | ~15–25% |
| Nothing — independent functions | ~116k | ~none |

**Bundle by FAMILY, not just by similarity.** The mid Proc* were barely
similar to each other yet beat the independent Req stragglers, because the
ProcItem *conventions* transferred within the session. Rules of thumb:
- Tight clone cluster → **one agent, anchor-then-clone** (or do it inline
  yourself). Find them: `clonematch` (exact) or `findsimilar.py <Name>` == 1.00.
- Structural twins / same-family → **bundle ~4–5 in one agent**, one at a time,
  self-contained, tell it to **re-affirm the contract before each function**
  (multi-function sessions drift into editing the cookbook — matcher.md warns).
- Independent functions → **parallel single-function agents** (setup-only
  savings don't justify serial wall-clock + bloat risk).
- Context bloat did **not** degrade quality across 4–5 real matches in one
  session in any experiment — so ~5 is a safe bundle size; be cautious past that
  or with multiple 1000B+ functions.

Tell an agent bundling several to **stub-and-move-on** (NON_MATCHING) if one
resists, so a single hard function can't sink the session.

## Harvesting (deterministic — avoid cross-agent conflicts)

Agents branch from a point-in-time worktree; by the time they finish, `main`
has moved (other harvests). **Do not merge their config edits.** Re-derive them
with the tools instead. Per matched function, in the main tree:

**NEVER copy a SHARED file (`item.h`, `game_types.h`, `effect.h`, `Build.hs`,
`permute.py`, `config/symbols.*`) wholesale from a worktree — apply only the agent's
own hunk.** The worktree may be dozens of commits behind, so `cp`-ing its `item.h`
over yours silently reverts every intervening change (I clobbered SoundEx's
`int→short`, UpdateMotion, wepid, TraceLine this way; a `conflicting types` compile
error caught it, but a size-preserving struct edit would have linked wrong silently).
Extract the agent's added struct fields / prototype with a targeted
`old→new` string replace against your CURRENT copy, then rebuild. Only per-function
`.c` files are safe to copy whole.

```
cp <worktree>/src/main.exe/<Name>.c src/main.exe/          # the matched C
# apply any config/symbols.main.exe.txt ADDITIONS the agent reported (it has no
# comment syntax — plain `NAME = 0xADDR;` lines only)
tools/reverse.py <Name> --ghidra-export .shake/ghidra-export --no-check   # carve — NEVER hand-edit the yaml, offsets are easy to get wrong
```
If the target was **renamed via an adjacent sibling** (config/symbols has a real
name but the Ghidra export still lists `FUN_<addr>` at that address), invoke
`reverse.py FUN_<addr>` — it detects the address already carries a non-placeholder
name and writes the correctly-named `.c` (e.g. `reverse.py FUN_8001ab64` →
`handle_balmer_acm_.c`). Passing the real name fails the export lookup.
then once all are copied+carved:
```
./Build                                   # generates the split .s
tools/maspsxflags.py <Name> --write       # gp + guarded-div flags; oracle picks it up
./Build
tools/matchdiff.py <Name>                 # per function → MATCH
./Build check                             # whole image byte-identical
```
Commit the `.c` + `config/splat.main.exe.yaml` + `shake/src/Build.hs` +
`tools/permute.py` (+ symbols if added) together, then
`git worktree remove <path> --force && git branch -D <branch>`.

### Harvest gotchas (hit in the AI + CD batches)
- **An agent's `INCLUDE_ASM`-free `.c` is NOT proof of a match.** An agent that
  runs out of turn (e.g. parked on a background permuter and never reported) can
  leave a *draft* with the stub already removed. Re-verify every file yourself:
  the AFS batch's `AfsGetHeader` came back as pure C but was 61 bytes off, and
  its own header comment described a fix it had never applied. Tell: per-function
  `matchdiff` on the *other* functions says `window matches but the image does
  not` — the image diff is coming from the unverified sibling. Always finish with
  `./Build clean && ./Build check`, and read the header comments of anything an
  interrupted agent touched.
- **Trust the whole-image `./Build check` (sha), never per-function matchdiff
  alone.** matchdiff's window used to be the gap to the next *symbol*, so a `D_`
  label or a jump table inside a function truncated it — `cd_open` reported "0
  differing bytes" in its 64-byte window while 4 bytes differed past it. **Fixed:
  now that every function is carved, matchdiff sizes the window from the splat
  carve extent** and prints `window N -> M bytes` when the old symbol gap would
  have been short. Eleven matched functions had been verified only over part of
  their body (EquipWeapon, PathFileRead, BriefingAndInventorySelectionScreen,
  SetupStageSequence, …); all 224 re-verified MATCH over their true extents. The
  whole-image sha is still the gate — a batch isn't green until `./Build check` is.
- **A phantom `nonmatchings/<name>.s` "can't open for reading" error after
  incremental carves is stale Shake state, not a real break** (the INCLUDE_ASM
  dep-tracking latent issue). `./Build clean` (~7s) fixes it; harvests in a
  churny region (e.g. the 0x8005fxxx CD block) may as well `./Build clean` up
  front.
- **Write the commit message to a file and use `git commit -F`.** These messages
  are full of backticks (`` `as` ``, `` `c2` ``, `` `--parked` ``); inside a
  double-quoted `-m "…"` bash runs them as command substitution. `` `as` ``
  invokes the assembler, which blocks reading stdin — the commit just hangs until
  the tool times out (and leaves an `a.out` behind).
- **A function splat splits into TWO non-contiguous pieces can't be
  NON_MATCHING-stubbed** — the INCLUDE_ASM covers only the first piece, so a
  cross-piece `.L<addr>` branch target goes undefined at link. Such a function
  must be fully MATCHED or left in the original blob (un-carved via
  `git checkout config/splat…` + re-carve the rest); it can't be parked as a
  stub+draft (`cd_open` — draft saved to `scratch/` instead).

### Strategy: family-first for a new tier (biggest token lever)
Before launching a tier that shares a struct (AI → `character_state`; items →
`PARAM_ITEM_USE`/`Humanoid`; CD/AFS → `FILE`/`TAFS`), map that struct in
`game_types.h`/`item.h` ONCE, point every agent at it, and sequence
leaves/small functions first so it gets byte-PROVEN early — then the big
handlers reuse it for free instead of each re-deriving it. `character_state`
(Ghidra-complete but with a few wrong field-type guesses) went unproven →
proven+corrected on the first small batch, de-risking the whole
`handle_char_state_*`/`think_*` family. Also cross-check
`reference/ghidra_types.h` (Ghidra's own fuller structs) before inventing field
names past a struct's known extent.

## The reflection loop (how the tooling compounds)

After each agent, read its report's **"where I reasoned manually"** and **new
rules**:
- A new *idiom* → fold into `matching-cookbook.md` (quote the agent verbatim;
  agents don't edit it themselves). Anchor on an existing nearby rule; **verify
  the anchor string matches** (wrapped lines / backticks bite — check after).
  If the idiom is *mechanical and local* (a token/expression rewrite at an
  enumerable site — a type width, a sign toggle, a condition normalization),
  ALSO add it as a rule in `tools/autorules.py` so it's machine-applied and
  never hand-guessed again. Recognition/structural idioms (loop shape,
  switch-vs-ladder, union-offset casts, case ordering) stay cookbook-only until
  an AST transform can place them safely; guided source-line filtering and
  exact-byte scoring now make some structural transforms mechanical too.
- A recurring *friction* → **build a tool**. That's the whole game. This session:
  agents hand-traced struct widths → `access.py`; hand-traced the store order →
  `access.py --order`; re-derived gp/div flags → `maspsxflags.py`; the gp edit didn't
  invalidate the build → a Shake `GpFlags` oracle (the *proper* fix, not a
  cache-bust); re-scaffolded jump tables → `split-scaffold.py`; the orchestrator
  hand-wrote prompts → `matcher-prompt.py`. **Fix a tool bug centrally the
  moment the first agent hits it** — don't let N parallel agents each re-diagnose
  it (the `reverse.py` carve-drop cost ~every agent tokens before it was fixed).
  Parallel function commits repeatedly colliding at the additive metadata
  insertion point is the same class of friction: run
  `merge_metadata_conflicts.py`, then review/stage, rather than manually
  rebuilding both mirrored tables on every harvest.
  Treat process lifecycle as part of tool correctness: a backup/restore wrapper
  is insufficient when its invoking shell can die while the Python driver or a
  build descendant survives. `autorules` now compiles candidates through
  per-function staged-source Shake overrides, owns/reaps build process groups,
  and arms a Linux parent-death signal. New mutating search tools should use the
  same staged-then-atomic-publish boundary.
- A transform that sometimes helps but is categorically wrong at a target site
  needs an **asm/RTL anti-oracle**, not a wider blind sweep. The extern-array
  rule now parses target `%gp_rel` operands and excludes those symbols. Prefer
  this pattern—enumerate source candidates, eliminate impossible sites from
  target evidence, then byte-score—over adding more unconstrained permutations.
- A worktree report must include both successful rules and **flat experiments**.
  AttackLong's commutative PLUS, AttackShort's jump2 threading, and
  DefaultActionHumanoid's adjacent-load sched2 tie became named early-stop
  signatures; AttackIndirect's three-statement fence, by contrast, became a
  mechanical `loop-range` rule. Reflection is what decides which side a lesson
  belongs on.
- Sub-C-level residual (≤10-byte register swap / adjacent reorder surviving a
  bounded permuter run) → it's below C; root-cause it, mark NON_MATCHING, move
  on. Don't sink sessions into it (~1.4M tokens for 0 matches taught this).

## Picking targets

`tools/triage.py` (easiest first, or `--leverage` for high call in-degree, or
`--max-size N`). Prefer functions from a TU you've already touched — the shared
header (e.g. `src/main.exe/item.h`) and gp lists already exist, and the worked
examples make them near-mechanical. `findsimilar.py --targets` is the other
lens.

Before assigning a name from a saved report, agent message, or old shortlist,
preflight the live worktree with `tools/matchdiff.py <Name>` and
`tools/progress.py`. Target inventories become stale as parallel branches land;
this cheap check prevents spending an agent turn rediscovering that the chosen
function already byte-matches on current `master`.

## Gotchas (all learned the hard way)

- **Wrap a maybe-not-closing draft in the `#ifndef NON_MATCHING` guard IMMEDIATELY, not
  only at the end.** `matchdiff`/`./Build` under `NON_MATCHING=1` happily succeeds while
  the DEFAULT `./Build check` silently breaks if the guard is missing — an agent leaked
  SetBleedsDir's 88 stray bytes into the default image this way, caught only by an
  explicit default `./Build check`.

- **Never read `matchdiff`/`asmdiff` output from a build whose `./Build check` did not
  return 0.** A failed or torn build leaves stale/inconsistent artifacts, and the diff
  against them is fiction. I "diagnosed" a phantom 2-byte gp-offset bug in a
  correct EndDrawing draft this way, after my own item.h clobber had failed the build.
  Confirm the build is green FIRST, then diff.

- **An agent can confidently report a MATCH that is incomplete — verify length, not
  just the agent's word.** One agent this session claimed SetSmoke "done" (pure C,
  no guard) when its draft linked 748 bytes into a 788-byte carve — 40 bytes of
  missing logic, not a register tie. `matchdiff.py` now cross-checks the linked
  `.text` size against the carve extent and `symcheck.py` flags the resulting
  region drift; together they catch it in one second. This is WHY every claimed
  match is re-verified at harvest with matchdiff + symcheck + a green ./Build check,
  never trusted from the report. (Same agent also hallucinated a non-existent
  sub-agent and blocked on it; a matcher works alone and cannot spawn matchers.)

- **A whole-park autorules re-sweep found 0 free matches (106 parks, all 8 rules).**
  The mechanical rules (abs-ge, or-inplace, ptr-index-sum, min-ternary, cmp-swap,
  type-width, &&-nest, temp-inline) earn their keep on NEW drafts -- autorules runs
  first when an agent picks a function up -- not retroactively on the existing park
  backlog, which is genuine sub-C ties / structural residuals. Don't re-run the mass
  sweep; PlaySE was a one-off that closed before the sweep existed. New mechanical
  rule => still add it to autorules (it helps the next fresh draft), just don't expect
  it to unlock parks.

- **Run `tools/symcheck.py` during every harvest.** It asserts that a symbol named
  `D_<HEX>` resolves to `0x<HEX>`. A missing `--gp-extern` entry silently relocates a
  whole data region -- the link succeeds and the image is just wrong. Measured: one
  missing entry moved 388 symbols by +16 bytes.

- **A fresh worktree needs `tools/wt-init.sh` before anything works.** `disks/` (the
  game images) and `.shake/` (which holds the Ghidra export) are both gitignored, so a
  new worktree has neither: `./Build` cannot find the target image, and
  matcher-prompt.py / coverage.py / triage.py / findsimilar.py all die with
  FileNotFoundError on `.shake/ghidra-export/functions.tsv`. wt-init.sh symlinks both
  from the primary worktree. Tell every worktree-isolated agent to run it first.

- **Escalating a same-length register tie to an RTL-capable model works.** vrealloc
  sat parked through ~3200 permuter iterations and a full C-respelling sweep. Told to
  ignore the C and read `-dg`/`-dl` dumps instead, a stronger model matched it in one
  pass by finding a call-argument register preference travelling through a merged
  variable. Route same-length register rotations there directly; do not spend another
  agent on respelling.

- **Spawn matcher agents with `isolation: "worktree"`.** `.claude/agents/matcher.md`
  does NOT declare isolation, so an agent launched without it edits the MAIN working
  tree and shares one `.shake/` database with every sibling agent and with you. Six
  agents launched this way fought over the build lock, corrupted each other's builds,
  and silently invalidated experiments running in the main tree. There is no warning.

- **The linker map's `LOAD .text ADDR SIZE` line is not authoritative for final
  placement.** When one function in a batch has the wrong length, that line can make
  an *earlier, already-matched* function look broken. Only the map's symbol table
  (`ADDR NAME`) and the real bytes of the built image settle it. Cost an agent
  significant time chasing a phantom upstream bug when the real culprit was a later
  function's own 8-byte shortfall.

- **A build killed mid-run poisons exactly one subsequent `./Build`.** `./Build clean`
  removes `.shake/{gen,build,processed}` but NOT Shake's database (`.shake/`
  itself is `shakeFiles`). A torn database from a SIGKILLed build can make the next
  invocation exit non-zero while its log shows a completed, byte-identical build.
  Re-run once before believing a failure whose log says success.

- **Never run `tools/permute.py` and `tools/matchdiff.py` concurrently in one
  worktree.** Both drive `./Build` against the same `.shake/` database, and the
  torn read produces a wildly garbled diff — one that typically appears as a length
  mismatch in some *downstream* function, so you go hunting in the wrong place. Two
  agents in two worktrees are fine (separate `.shake/`); one agent doing both at once
  is not. If a diff suddenly stops making sense, check for a live permuter first.
  This is now mechanically enforced for autorules, permute, matchdiff, asmdiff,
  and rtlguide by `.shake/matching-tool.lock`: a second tool exits with the
  owning PID/tool/target. The lock is per worktree, inherited by driver helpers
  such as autorules→matchdiff `-n`, and kernel-released on exit. It prevents new
  collisions; it does not turn a yielded session into a completed command—keep
  polling the original session.
  `autorules` additionally keeps speculative C under `.shake` and publishes a
  winner with one atomic rename, so even SIGKILL cannot leave a candidate in
  `src/`. Its normal TERM/HUP path also kills and reaps the active build session.

- **Never hand-edit `config/splat.main.exe.yaml` carves** — use `reverse.py`;
  the data-offset math is easy to get wrong (bit me twice). It preserves
  hand-added `.rodata` carves now (it used to drop them).
- **`config/symbols.main.exe.txt` has no comment syntax** — `//`/`/* */` is a
  hard splat parse error. Plain `NAME = 0xADDR;` only.
- **A whole-image `./Build check` failure while a function is mid-match is
  EXPECTED** (wrong length shifts every later object — even matched siblings
  show huge matchdiff diffs). Finish the function; don't bisect.
- **Objdump the whole EXE with `--adjust-vma (TEXT_START - 0x800)`**, not
  `TEXT_START` — the 0x800 header skews every address (this bug was silently
  wrong in `findsimilar` for a while; now fixed and it's the norm in the tools).
- Agents sometimes `cd` into the main checkout instead of their worktree
  (their bash cwd resets to the worktree each call, but an explicit `cd`
  overrides). Read-only tools are harmless there; writes aren't — the prompt
  and matcher.md both stress the worktree symlink setup.

## High-leverage backlog (do these for outsized payoff)

- **Flesh out the `Humanoid` struct** in `item.h` (character/actor state, ~724
  accesses at its 4 signature offsets alone; Ghidra `types.h` has a fuller
  candidate). It's `CamState.Owner` and every enemy/player — unlocks a large
  slice of remaining game code. **It is 0xCE bytes, NOT ~0x145** (Ghidra's own
  struct ends near 0xCD, `character_state`'s independently-proven twin near
  0xD0) — an earlier version of this line said 0x145 and a matcher agent caught
  it. Progress: `attrib`(s16)@0x28 and `weapon_kind`(u16)@0x8E are now
  byte-proven (GetHumanoid/DisposeWeapon/EquipWeapon/StickonCheck batch).
- **Easy + high-in-degree functions** (from `triage.py --leverage`): matching
  them confirms a signature for many callers. DONE so far: `UpdateCoordinate`,
  `DeleteConflict`, `SetNowMotion`, `GetAbsolutePosition`, `MoveHumanoid`;
  `Sound`/`InsertConflict` are parked NON_MATCHING (sub-C regalloc ties). Next
  high-lev: `AdtMessageBox` (87), `GetConflictResult` (conflict trio), `SoundEx`.
- **Remaining item family**: the ~16 large `ProcItem*` (784–2468B, Fable tier).
  The 5272B `ReqItemUse` dispatcher is now MATCHING; RTL-guided, byte-neutral
  cross-case variable reuse resolved its last register-allocation permutation.

## The m2c auto-draft sweep (measured; worth productizing)

An experiment worth knowing the numbers for before repeating it. **Do not port
`autorules.py`'s rules into decomp-permuter — upstream already has them**:
`type-width` = `perm_randomize_internal_type`, `and-nest` = `perm_condition`,
`temp-inline` = `perm_expand_expr` (see the permuter's `default_weights.toml`,
whose weights are overridable per-function via a `settings.toml` in the
non-matching dir — no fork needed). `autorules`' value is being deterministic
and *explainable*, not having rules the permuter lacks. What the permuter really
lacks are our *structural* rules (literal pointer cast, `T[i]` vs `e++`,
whole-struct assignment) — and even some of those it reaches indirectly via
`perm_reorder_stmts`/`perm_temp_for_expr` (that is how it cracked AfsGetHeader).

**"Permute every unmatched function briefly" does not work as stated**: only
carved functions have a per-function `.s`, so there is no `base.c` to perturb and
no `target.o` to score against. What *does* work, and is cheap:

1. In a throwaway worktree, carve every unmatched non-GTE function (import
   `reverse.py`'s `add_symbol`/`split_config`, loop, then run `split.py` once).
2. `m2c.py -t mipsel-gcc-c --context <cpp'd headers> --valid-syntax <pieces>`.
   Strip the top-level `__asm__()` out of the cpp'd context first — pycparser
   chokes on it. Define `M2C_UNK`, `M2C_FIELD`, `M2C_ERROR` and the draft mostly
   compiles.
3. Compile with our `cc1|maspsx|as` pipeline and score against `target.o` using
   decomp-permuter's own `Scorer` (`algorithm="difflib"` — `levenshtein` needs a
   module we don't ship; `objdump_command=None`).

Measured over all **341** unmatched non-GTE functions: m2c produced usable output
for 286, **146 compiled**, and **3 scored 0** — i.e. m2c's raw output already
byte-matched (`GetVectorRotation`, `ChkCard`, `FormatCard`, all since committed).
Only 3 more scored ≤200. Median score was 2520, so **a brief permuter run over
everything would be wasted** — from a score-1165 base the permuter did not
converge in 45 s, while the score-0 ones need no permuter at all.

**Upgrading m2c did NOT move this needle.** We replaced the 3-year-stale
`m2c-overlay` pin with upstream master + our PS1 patch series (`nix/m2c/*.patch`).
On the identical 331-function set the scored count was **138 before and 138
after**: the stack-passed-argument and bitfield fixes change the drafts but not
whether `cc1` accepts them. The m2c upgrade's payoff is GTE/COP2 readability, not
draft throughput. The remaining compile failures are `sp32 undeclared` stack
temps, `void value not ignored`, and arg-count mismatches against context
prototypes — that is still where more zeros hide.

Takeaways: the sweep's value is (a) free matches and (b) a *ranked worklist* —
score-vs-target is a far better "what's easy next" signal than triage's
size/similarity heuristic. The 195 that never scored are blocked on draft quality
(`sp32 undeclared` stack temps, `void value not ignored`, arg-count mismatches
against context prototypes), so a small m2c fix-up layer is where more zeros hide.

## Tooling backlog (recurring friction → build these)

- **DONE — every game function is carved.** All 555 game functions (plus the two
  SDK ones we had) now have a `c` subsegment, their own
  `asm/nonmatchings/<Name>/<Name>.s`, and a `.c` (a stub where unmatched).
  `./Build check` is byte-identical; a clean build is ~25s (was ~13s).
  So `permute.py`, m2c and the auto-draft sweep can target ANY function without
  carving first — `tools/sweep.py`, when built, no longer needs the throwaway
  worktree. Carving never affected analysis: `triage.py`/`access.py`/
  `coverage.py`/`findsimilar.py` and the Ghidra export all read the ORIGINAL
  binary via objdump. What it changes is the build — splat disassembles the range
  into its own `.s` instead of leaving it inside a raw `data` blob.
  Three things it took to get there, all now in the tools:
  * `reverse.py`'s `expand_stub` seeds every piece when splat splits a function at
    an interior SYMBOL (25 of them: Ghidra `__override__prt_` call-site markers,
    plus plain labels like `INIT_STAGE_STATS` inside `StartStageSequence`).
    `is_jump_table()` is the predicate — a `switchD_` piece, not the marker name.
  * `split-scaffold.py` reads a function's whole jump-table POOL as one raw run
    and emits one array + one carve, because the compiled object has a single
    `.rodata` section (28 scaffolded; `ActENGAGE` has three tables).
  * A full carve is a stronger consistency check than `coverage.py`: it surfaced
    five "matched" functions that had never been carved, so their `.c` was never
    linked and `matchdiff` had been reporting a false MATCH. Four were drafts.

- **Productize the sweep as `tools/sweep.py`** (see the section above). Needs:
  resolve the permuter's source + python env from the `permuter.py` wrapper
  rather than hardcoding `/nix/store` paths, generate the cpp'd context itself,
  manage its own throwaway worktree, and emit a score-ranked worklist. Then add a
  fix-up layer for the 3 recurring m2c compile failures to lift the 146/341
  scoring rate.
- **Drop the 79 `__override__prt_` non-symbols from `config/symbols.main.exe.txt`.**
  They are Ghidra call-SITE prototype overrides, not functions, but splat consumes
  the file as `symbol_addrs` and starts a new `.s` piece at each one — silently
  splitting 34 ordinary functions (30 still unmatched) in two. `reverse.py` now
  works around it (seeds every piece; see the cookbook), but the root fix is to
  stop emitting them. Blocked on `FileOption.c`, whose parked 18-piece stub
  `INCLUDE_ASM`s an override piece *by name*; fix that stub first, then also
  filter them in `tools/import_symbols.py` so they don't come back.

- **A register-preference/tie diagnoser** — BUILT: `tools/regalloc.py <Name>`.
  Parses cc1 `-dg` into the pseudo→hard-reg map, the values live across calls
  (forced callee-saved — the pressure), and the copy-chains that bias coloring.
  The tell that you're in tie territory (when to reach for it): `autorules`
  reports no win AND a bounded permuter run never beats the base score. Run
  `regalloc.py` then break the copy-chain / shorten the live range it points at,
  instead of blind permuting. **v2 is built as `tools/rtlguide.py`:** it infers
  candidate→target register goals from aligned target asm, attaches `.greg`
  dispositions/preferences and cc1 `-g` locals, and gives `autorules --guided`
  the implicated source lines and mutation families.
- **GTE splitting is DONE, upstream.** splat >= 0.4x generates
  `include/gte_macros.inc`, so `as` assembles the GTE command opcodes; all 25 GTE
  functions are carved and `./Build check` is byte-identical. (Our stopgap
  `tools/gte2word.py` is deleted.) **What remains is the OWNER'S POLICY CALL**:
  matching them needs register-pinned locals / inline asm, because no C construct
  emits a GTE opcode and the region uses a non-ABI calling convention (values in
  `$t2..$t5`/`$s0` at entry). The same decision unblocks `GetPad`/`GetPadXY`/
  `FUN_8001b174` (the sign-extension trio), `DrawTMD`, and `PClseek`'s
  `break 0x107`. Until it is made, those functions stay VERY-HARD/parked. m2c
  can already read the region (`--input-regs`). ArrangeLocalMatrix was an old
  false positive in this list: its `$t2..$t6` values are internal loop
  temporaries and its calls use the normal ABI; it now matches in pure C.
