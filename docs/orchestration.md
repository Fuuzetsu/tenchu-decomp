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

**State (keep this line current):** ~205/555 game functions (~37%), ~14% of
game bytes, whole-image byte-identical (sha256 `0690a5c1…3558`). Live count:
`nix develop --command tools/progress.py`.

**The loop (repeat until the user stops you):**
1. `nix develop --command 'tools/triage.py'` → pick a batch of **5 clean-seam
   functions**. Clean seam = non-jump-table, ≤~500 bytes, non-dispatch, ideally
   with a matched twin (`~0.NN to <Twin>`). triage already hides parked functions
   and de-ranks the GTE/SIGNEXT classes; you still filter OUT `switchD`,
   `Think3*`, `handle_char_state_*`, `0x80060xxx`, and — until the inline-asm
   policy lands — **`DrawTMD` and `ArrangeLocalMatrix`**, which triage rates
   TRIVIAL/EASY but which pass arguments to the GTE handlers in `$t2..$t6`
   (non-ABI; m2c confirms the callees read `input_t0/t3/t5`). Prefer trivial/easy
   tier + a strong twin. **`LoadCard` and `FUN_800593a0` are under-sized in
   `functions.tsv`** — carve them with `--size 0x168` / `--size 0x27C` or their
   `.c` can never match (see `tools/coverage.py`).
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
| `tools/matcher-prompt.py <Name>` | **generate the agent launch prompt** — never hand-write it. Auto-fills address/size, jump-table detection, nearest matched worked-examples, TU family, and the evolving `GUIDANCE` block. Routes findsimilar-1.00 near-clones to an inline recipe. |
| `tools/reverse.py <Name> --ghidra-export .shake/ghidra-export` | split + seed the `.c` with Ghidra C **and** m2c C **and** a `triage:` line + likely-relevant docs. Adds the splat carve. |
| `tools/split-scaffold.py <Name>` | jump-table function: all INCLUDE_ASM pieces + the jtbl array + the `.rodata` carve, green before drafting. |
| `tools/xref.py <Name>` | callers (pin the prototype) + callees (matched vs needs-extern). |
| `tools/access.py <Name> [--order]` | each pointer offset's width/signedness/direction from the mnemonics; `--order` = the store sequence. Struct layout without hand-tracing `.s`. |
| `tools/gpsyms.py <Name> --write` | derive the gp-extern set from `%gp_rel` and sync Build.hs + permute.py. Shake-oracle-tracked, so it just takes effect. |
| `tools/clonematch.py <Matched>` | write the `.c` for exact byte-identical unmatched siblings (verified). |
| `tools/matchdiff.py <Name>` / `tools/asmdiff.py <Name>` | iterate. matchdiff = whole-image gate; asmdiff = aligned view for big/split functions. |
| `tools/autorules.py <Name>` | once the draft compiles: mechanically sweep the *local* cookbook rules (type-width flips; `&&` split/merge and single-use temp inline via tree-sitter) and greedily keep what shrinks the asmdiff, reporting which edit helped. Deterministic first pass; a "no win" verdict means the residual is structure/regalloc, not a local rule. |
| `tools/permute.py <Name>` | decomp-permuter for pure register-allocation ties (the stochastic search; autorules is its deterministic, explainable complement). |
| `tools/gte2word.py <dir>` | rewrite splat's GTE command mnemonics (`RTPT`…) to `.word` so `as` can assemble them. **Runs automatically from Build.hs after `split.py`** — you never invoke it by hand. Byte-exact (the `.word` is the byte-SWAP of splat's WORD comment). |
| `tools/coverage.py [--all]` | code claimed by NO function — finds under-sized `functions.tsv` entries (a truncated carve still builds green; the tail becomes a `.data` blob that defines the `.L` labels, so nothing complains). `LoadCard` and `FUN_800593a0` are the two in game code. |
| `tools/regalloc.py <Name>` | **diagnose a register tie** — runs `cc1 -dg` and surfaces which values are live across calls (forced callee-saved), the pseudo→hard-reg map, and the copy-chains that bias the coloring. Run this BEFORE blindly permuting a sub-C tie; it tells you which copy-chain to break. |

## Launching an agent

1. `tools/matcher-prompt.py <Name>` (in the nix devShell). Read its top line —
   it annotates JUMP-TABLE and the TU family, and if the target is a
   findsimilar-1.00 **near-clone** it prints an inline recipe instead: do that
   yourself (clone the twin, swap the differing constant, `matchdiff`) —
   ~2 min, no agent.
2. Otherwise paste the generated prompt into the **Agent** tool with
   `isolation: "worktree"` and the model from *Model routing* below. Background
   is fine; you're notified on completion.
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
tools/gpsyms.py <Name> --write            # per function; oracle picks it up
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
  alone.** matchdiff's window can be shorter than the function — `cd_open`
  reported "0 differing bytes" in its 64-byte window while 4 bytes differed
  past it; only the sha caught it. A batch isn't green until `./Build check` is.
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
  switch-vs-ladder, union-offset casts, case ordering) can't be sited blindly —
  they stay cookbook-only until an AST-based transform can place them.
- A recurring *friction* → **build a tool**. That's the whole game. This session:
  agents hand-traced struct widths → `access.py`; hand-traced the store order →
  `access.py --order`; re-derived gp lists → `gpsyms.py`; the gp edit didn't
  invalidate the build → a Shake `GpFlags` oracle (the *proper* fix, not a
  cache-bust); re-scaffolded jump tables → `split-scaffold.py`; the orchestrator
  hand-wrote prompts → `matcher-prompt.py`. **Fix a tool bug centrally the
  moment the first agent hits it** — don't let N parallel agents each re-diagnose
  it (the `reverse.py` carve-drop cost ~every agent tokens before it was fixed).
- Sub-C-level residual (≤10-byte register swap / adjacent reorder surviving a
  bounded permuter run) → it's below C; root-cause it, mark NON_MATCHING, move
  on. Don't sink sessions into it (~1.4M tokens for 0 matches taught this).

## Picking targets

`tools/triage.py` (easiest first, or `--leverage` for high call in-degree, or
`--max-size N`). Prefer functions from a TU you've already touched — the shared
header (e.g. `src/main.exe/item.h`) and gp lists already exist, and the worked
examples make them near-mechanical. `findsimilar.py --targets` is the other
lens.

## Gotchas (all learned the hard way)

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
- **Remaining item family**: the ~16 large `ProcItem*` (784–2468B, Fable tier)
  and the 5272B `ReqItemUse` dispatcher.

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
  instead of blind permuting. (Open v2: diff our greg allocation against the
  target asm's registers to auto-flag the divergent value.)
- **DONE — the GTE→`.word` pass is built** (`tools/gte2word.py`, run from
  Build.hs after `split.py`). All 25 GTE functions are carved and `./Build check`
  is byte-identical. **What remains is the OWNER'S POLICY CALL**: matching them
  needs register-pinned locals / inline asm, because no C construct emits a GTE
  opcode and the region uses a non-ABI calling convention (values live in
  `$t2..$t5`/`$s0` at entry). The same decision unblocks `GetPad`/`GetPadXY`/
  `FUN_8001b174` (the sign-extension trio) and `PClseek`'s `break 0x107`. Until
  it is made, those ~29 functions stay VERY-HARD/parked. m2c can already read the
  region (`--input-regs`), so a decision is all that is missing.
