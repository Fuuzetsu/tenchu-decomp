# Orchestrating matcher agents — the runbook

How the **orchestrator** (the main session) drives byte-matching at scale by
spawning `matcher` subagents, harvesting their work, and compounding the
tooling. If you're resuming this work, read this first, then
[matching-cookbook.md](matching-cookbook.md) for the actual matching idioms.

The split of responsibility:
- **Matcher agent** (`.claude/agents/matcher.md`) — matches ONE function (or a
  bundle) in an isolated git worktree. Commits an exact result or an honestly
  documented checkpoint to its worker branch. Reports MATCH/CURRENT(N), files
  touched, new rules, and *where it had to reason manually that a tool could
  have done the work* (that last part drives the tooling loop).
- **Orchestrator** (you) — picks targets, generates prompts, launches agents,
  **reviews and cherry-picks on green**, folds reported rules into the
  cookbook, and **builds a tool whenever a friction recurs**.

## Resuming the hands-off flywheel (quick-start)

**State is deliberately not cached here.** Parallel branches make a handwritten
count stale almost immediately.  Before reporting progress or choosing a target,
run `tools/progress.py`; the committed whole-image build must remain
byte-identical (sha256 `0690a5c1…3558`).

The latest clean shutdown snapshot is
[`flywheel-handoff.md`](flywheel-handoff.md). It records the last anchor,
exhausted near-matches, and a revalidation-first candidate slate. Treat it as a
dated launch pad; the commands above remain authoritative.

### Hard / large-function phase

When the clean seams are exhausted, or the user explicitly asks for difficult
gameplay functions, keep the same flywheel but assign one large function (or one
tightly coupled family) per isolated worktree:

1. Record authoritative target/candidate length, instruction count, CFG counts,
   differing bytes, and fuzzy percentage before editing. Read any parked
   `STATUS` note before repeating old experiments.
2. Recover structure with target asm, demo symbols, siblings, xrefs, CFG and RTL.
   Keep the draft pure C; inline-asm matching patches are not acceptable.
3. Refresh `fuzz-score.py --only <Name>` after every meaningful guarded-draft
   change. Run autorules only against a compiling draft. Do not use the permuter
   as a decompiler: reserve one bounded run for a localized, near-final
   allocation/scheduling residual after deterministic source and RTL work.
4. Commit an exact result or an honestly measured improved checkpoint in the
   worker branch. Root reviews and cherry-picks it, re-runs the global gates,
   then performs the reflection/tooling pass as a separate commit.
5. Recycle the slot immediately onto the next live hard target. A partial result
   is useful only when its `STATUS`, fuzzy row, rejected experiments, and blocker
   are current enough that the next pass starts ahead of it.

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
   clean && ./Build check` must be **BYTE-IDENTICAL**. Before committing any
   changed guarded draft, run `tools/fuzz-score.py --only <Name>`; run the same
   command after an exact promotion to remove its obsolete row. Gate the batch
   with `tools/fuzz-score.py --check`, then commit.
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
| `tools/siblingdiff.py <Name> [<Sibling>] [--demo\|--compose]` | **the drafting kickstart for related functions** — ordinary mode auto-picks the nearest MATCHED retail sibling, prints its C plus a normalized asm diff. `--demo` instead aligns the same-named earlier demo function, relativizing local branches and replacing cross-build call/jump addresses with shared function names. It also flags a demo-only named call when most of that retail helper's mnemonic bigrams occur inside the target—the mechanical tell for a likely local inline expansion. `--compose` compares the target simultaneously with its same-named demo body and the top matched siblings (or `--sources A,B`): the demo claims its exact normalized runs first, ranked siblings fill the gaps, and the report includes per-source potential coverage, a compressed provenance map, and unmapped spans. Identical runs collapse in pairwise modes. These are source-structure hints; retail bytes remain the exact-match authority. |
| `tools/matcher-prompt.py <Name>` | **generate the agent launch prompt** — never hand-write it. Auto-fills address/size, jump-table detection, nearest matched worked-examples, TU family, and the evolving `GUIDANCE` block. Routes findsimilar-1.00 near-clones to an inline recipe. |
| `tools/reverse.py <Name> --ghidra-export .shake/ghidra-export` | split + seed the `.c` with Ghidra C **and** m2c C **and** a `triage:` line + likely-relevant docs. Adds the splat carve. |
| `tools/split-scaffold.py <Name>` | jump-table function: all INCLUDE_ASM pieces + the jtbl array + the `.rodata` carve, green before drafting. |
| `tools/xref.py <Name>` | callers (pin the prototype) + callees (matched vs needs-extern). |
| `tools/symnear.py <Name-or-address> [...]` | list fixed symbols immediately before scalar addresses and compute candidate containing-field offsets; multiple queries also report common candidate bases. Use when `rtlguide` reports `enclosing-global-field-load`; confirm the proposed struct fields independently. |
| `tools/access.py <Name> [--order]` | each pointer offset's width/signedness/direction from the mnemonics; `--order` = the store sequence. Struct layout without hand-tracing `.s`. |
| `tools/stackplan.py <Name> [--emit-overlay]` | recover target/candidate frame size, outgoing arguments, saved-register boundary, and every stack access. It flags the address-taken xyz / second xyz save-reload signature of a likely `VECTOR[2]` workspace, and reconstructs contiguous word-copy edges/chains between same-sized stack aggregates (for example, three distinct `VECTOR` locals linked by assignments). A recovered workspace prints the exact cached `-n --emit-overlay` follow-up command; that mode recognizes address-taken aggregates and prints a padded C workspace scaffold. Use `--args N` to override an ambiguous inferred o32 boundary. Its prerequisite build is quiet on success; failures report the tail and path of `/tmp/tenchu-stackplan-build.log`, so fresh-worktree compiler warnings cannot bury the actual stack report. |
| `tools/gpsyms.py <Name> --write` | derive the gp-extern set from `%gp_rel` and sync Build.hs + permute.py. Shake-oracle-tracked, so it just takes effect. |
| `tools/maspsxflags.py <Name> --write` / `--check` | preferred combined setup pass: sync gp externs and detect/sync ASPSX guarded variable division's `--expand-div` flag from the target asm. `--check` audits every guarded draft and both mirrored tables, catching a missed setup pass before its length/fuzzy diagnostics mislead the matcher. |
| `tools/merge_metadata_conflicts.py` | during a cherry-pick, conservatively resolve the recurring additive conflicts in Build.hs/permute.py gp and `--expand-div` tables. It unions only recognized metadata entries, rejects same-key disagreements or ordinary code, validates both files before writing, and leaves staging/`cherry-pick --continue` to the orchestrator. |
| `tools/clonematch.py <Matched>` | write the `.c` for exact byte-identical unmatched siblings (verified). |
| `tools/matchdiff.py <Name>` / `tools/asmdiff.py <Name>` | iterate. matchdiff = authoritative whole-image byte gate; asmdiff = aligned diagnostic view for big/split functions. `asmdiff --structural` deliberately hides every same-length replacement, so even zero displayed structural lines is not a match claim; its summary retains the raw residual and exact-sequence verdict. Both tools inspect the final link map and reject a linked `<Name>.NON_MATCHING` stub, including `asmdiff -n` immediately after a source guard was removed, so a stale/default artifact cannot masquerade as C progress. An exact C draft also remains a failing/incomplete result until its `NON_MATCHING`/`INCLUDE_ASM` guard and inline `__asm__` are gone. |
| `tools/fuzz-score.py --only <Name>` / `--check` | refresh decomp.dev's fuzzy percentage after **every** guarded-draft change (or remove the row after exact promotion), then verify that no row is missing, orphaned, or stale against its source SHA-256. `objdiff-report.py` enforces the same gate. The percentage is a presentation heuristic, not an acceptance metric: restoring Think3callaid's exact extent cut the authoritative whole-image residual from 1004 to 50 bytes while fuzzy temporarily fell 78.82→76.47. Record it faithfully even when non-monotonic; rank changes by exact extent/CFG and `matchdiff` bytes. |
| `tools/autorules.py <Name>` | once the draft compiles: mechanically sweep the *local* cookbook rules and greedily keep what shrinks the authoritative byte diff. It recognizes target-gp anti-sites, aggregate VECTOR copies, explicit builtin abs, pointer-index integer sums in declarations or later assignments, subscript-postincrement working copies, centered-modulo typed-temp splits, paired same-call argument producers, same-literal field assignment chains, 0/1 default-and-override returns, literal equality swaps, and the atomic byte-literal/direct-indirect-field pair; `--guided` can invert a compound if/else or swap adjacent compound terminal arms to score the opposite physical body layout, atomically move an arithmetic working-copy identity from seed to update/writeback, defer a scalar-global capture behind a read-only decision tree (including the later s16-capture widening needed to preserve exact length), inline a multi-edge shared field writeback into direct compound updates plus the useful fallthrough inversion, split a casted dereference into distinct address/value identities, turn a commutative extern-global sum into both initializer/compound-accumulator orders, turn a three-literal equality ladder into all six sparse-switch body orders, split a constant right shift into every two-stage form, toggle divisible affine multiply spellings to block unwanted cross-jumps, name an extern array base before indexing, selectively rematerialize matching-type local-array member lvalues, swap adjacent literal stores to distinct fields for late scheduling ties, toggle a local integer pointer's volatile pointee, insert or remove a weight-free empty LOOP_END boundary, fence one statement or a safe 2–4-statement range, add two or three nested weights atomically, split adjacent statements behind two one-shot fences, dead-evict a switch index from CSE, shift a statement across an existing LOOP_END, duplicate one statement into zero-code identical arms, permute an existing identical-arm fence's pure discriminator, duplicate one adjacent shared assignment into both compound if/else arms, try real case labels, and enumerate the six three-term addition trees while preserving nonconstant/call order. AST rules mask the inactive side of guarded INCLUDE_ASM drafts when tree-sitter wraps the whole preprocessor region in an error node. Output is line-buffered for monitoring and flags a byte win that worsens the equal-length aligned instruction shape as `LOCAL-SHAPE REGRESSION`. Candidates compile from a private staged source; only the selected result is atomically published. Owned build process groups, Linux parent-death handling, and the per-worktree matching lock prevent interrupted/orphaned work from mutating or racing the live source. It never emits inline asm. |
| `tools/permute.py <Name>` | late-stage decomp-permuter for pure register-allocation ties, after deterministic structure/RTL work. A preflight refuses drafts over one instruction from target length or broader than 128 aligned lines / 32 blocks; `--force-early` requires an explicit reviewed allocation-cascade justification. Every retained output is full-link rescored and must be bisected rather than pasted wholesale. It shares the per-worktree matching lock, so an accidental launch during autorules/diff/rtlguide fails immediately with the owning PID/target instead of producing a torn build. |
| `tools/dedupe-symbols.py [--check]` | one name per address in `config/symbols.main.exe.txt`. splat >= 0.4x refuses duplicates, and our file cannot disambiguate them (it doubles as an ld script, no comment syntax). Re-run after any Ghidra symbol import. |
| `tools/coverage.py [--all]` | code claimed by NO function — finds under-sized `functions.tsv` entries (a truncated carve still builds green; the tail becomes a `.data` blob that defines the `.L` labels, so nothing complains). `LoadCard` and `FUN_800593a0` are the two in game code. |
| `tools/rtldump.py <Name> [--pass …] [--draft]` | **the escalation tool** — standalone cc1-281 RTL dumps (`.greg`/`.lreg`/`.loop`/`.combine`/`.jump2`/`.sched2`/`.dbr`), race-free in the scratchpad, ~1 s. When a same-length residual beats respelling + the permuter, dump the pass that owns the diverging decision and read it (cookbook: "Reading cc1's RTL dumps"). Cracked 9 "permuter-immune" ties this session. |
| `tools/rtlguide.py <Name>` | **mechanical RTL escalation** — aligns target asm with our candidate, classifies each hunk by owning pass, recompiles with debug RTL notes, maps residual instructions back to C lines, names locals in the divergent hard registers, and emits the exact guided autorules command. It also audits target/candidate physical branch/jump/call/return counts (warning when a score win invents a conditional branch), detects target-only physical calls, distinguishes `jal abs` from target inline abs, recognizes postincrement and arithmetic working-copy, cross-call argument-pipeline, terminal commutative-equality, branch-phi register ties, and target stack-address rematerialization residuals, prioritizing the corresponding arm/writeback, working-copy, or alias-rematerialization rules for those traces, summarizes CALL_INSN fingerprints through jump2, names proven residual signatures, reports source lines whose first RTL instruction is fenced by LOOP_END in sched/sched2, and flags allocnos whose target hard register is categorically forbidden by `.greg` (so loop weighting cannot help). The target has no RTL; target asm is the specification and our RTL is the causal trace. `--json` is stable; direct runs share the matching lock. `--no-build` validates the processed source's line-marker provenance and rejects both private staged candidates and a guarded function's default INCLUDE_ASM artifact, so cached bytes cannot masquerade as current C progress. |
| `tools/reghist.py <Name>` | **the first move on a big Ghidra function** — histograms register mentions target-vs-draft. A draft-heavy caller-saved register is a MEGA-PSEUDO (one Ghidra variable doing many jobs; one pseudo gets one hard reg for all fragments, so a conflict anywhere exiles every use) — split it per site (worth 140 bytes on FUN_80057b80). The delta SUM is also an exhaustion proof: zero-sum deltas in the arg registers = pure renames, decomposition already matches, residual is allocation/scheduling. |
| `tools/regalloc.py <Name>` | **diagnose a register tie** — reads both `-dl` and `-dg`, filters to real global allocnos, shows refs/live-length/computed priority, pseudo→hard-reg dispositions, hard-register conflict sets, and copy chains. `--prefer a0` focuses the allocnos carrying one hard-register preference so distant call-argument donors become visible. `--compare P Q [--enclosed-refs N]` quantifies how many weighted refs/loop depths P needs to outrank Q; `--between SUBJECT LOWER UPPER` finds a bounded three-allocno priority window (equal-mode comparisons). Run this BEFORE blindly permuting a sub-C tie. |
| `tools/extract-demo.py`, `tools/psxsym.py`, `tools/symdump.py` | carve/parse/dump the demo disc's `PSX.SYM` — original prototypes, locals, structs, TU map. See [psx-sym.md](psx-sym.md). `matcher-prompt.py` injects the per-function facts automatically. |
| `tools/symmatch.py`, `tools/xbuildnames.py`, `tools/callmatch.py`, `tools/datamatch.py` | recover original **names** (functions, then globals) from `PSX.SYM` + the demo `PSX.EXE`. All four keep Ghidra's boundary/size inventory but mechanically overlay current splat `c` names, so adopted names, named callees, and data-reference pairs cannot go stale. `datamatch` retains label aliases and infers the PSX.SYM-data→demo-data relocation from unique anchors (minimum 64 at 99% dominance) to reconstruct labels omitted by the Ghidra export; duplicate statics require translation-unit agreement, reverse uniqueness covers raw conflicting votes, and `--apply` hard-fails below 100% control precision. |
| `tools/symnote.py --write --all` | stamp every `src/main.exe/*.c` with a `BEGIN PSX.SYM` block: the original prototype, locals (name/type/register-or-stack), touched globals, and any recorded candidate name. Touched-global ownership uses the original object's recorded byte extent (bare pointers are four bytes), never nearest-name guessing; generated `D_` labels may be interior aliases, while meaningful retail symbols split an extent. Idempotent; `--check` gates staleness; `--rename-params` adopts the authors' parameter names. Comments only — `./Build check` must stay byte-identical. |
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
  later demo-assembly audit identified retail `0x80027730` as the frame-range napalm
  function and `0x8001f6b8` as a one-frame lightning callback. That allocation is now
  corrected (`AttackFire` and `handle_char_state_attacking_SEVEN_`, respectively).
  `callmatch --verify` now reports `AMBIGUOUS` and fails the gate when another
  full-containment candidate has no more extra named calls and no worse size distance.
- **Ambiguous ⇒ keep the placeholder, record the candidate.** `reference/psxsym-candidates.tsv`
  and `reference/psxsym-data-candidates.tsv` hold every suggestion we did not adopt;
  `matcher-prompt.py` surfaces them to whoever touches the function next.
- **Converged and unused ⇒ adopt the original name.** Do not leave a `FUN_…` or
  hand-written descriptive name in place once independent evidence agrees: the
  demo name is not already assigned in retail, `callmatch --verify` is
  non-ambiguous (or a byte-matched callback setter supplies the cross-build
  identity), and the prototype, constants, behavior, and surrounding source
  allocation fit. Check both the current splat inventory and symbols table for
  collisions, then use `import_symbols.py --renames`; keep the function match
  and rename in separate commits so the naming proof remains reviewable.
- **A cross-build callback assignment can be decisive naming evidence.** If a
  byte-matched producer installs retail function X in the same field/lifecycle
  where its demo counterpart installs original name Y, X's behavior matches that
  role, and Y is unassigned in retail, adopt Y even when the callback body was
  heavily rewritten and mnemonic similarity is low. Keep the function match and
  rename as separate commits so codegen and naming provenance remain reviewable.
- **Names and data feed each other.** `datamatch.py` can only see a global through a
  function named on *both* sides, so every batch of function renames unlocks more data
  symbols. **Re-run `datamatch.py` after any function renames.** After the calibrated
  relocation batch adopted 71 names, it currently proposes zero multi-witness names —
  it has harvested everything the present 999 shared names can safely reach. The 48
  single-witness rows remain parked for semantic corroboration.

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

## Byte-account before you theorise (orchestrator discipline)

**A single-cause story that explains a third of the residual is not a theory of
it.** Before writing a brief that names THE cause, split the diff into clusters
and attribute bytes to each; five minutes of arithmetic beats a confident
narrative. Measured cost of skipping it: an AddEnemy brief asserted "72 is ONE
cause — fix the loop-2 preheader and all 72 follow", but 40 of those 72 sat in
loop 1 and the prologue with IDENTICAL registers on both sides, differing only in
order — they could not be caused by loop 2 at all. The lane found this in five
minutes and worked the cluster the brief never mentioned (72 -> 52).

The same discipline applies to every "impossible" verdict a brief inherits: the
verdict describes a decomposition, and the numbers behind it are often about a
fraction of the bytes. FIVE briefs in this rollout carried premises that their
lanes disproved with arithmetic (the allocno tie-break, the "8 weighted refs"
debt, the AddEnemy single-cause story, the "local-alloc is a different lever"
claim, and the AddEnemy "find retail's two extra long-lived values" question).
Write briefs that hand over EVIDENCE and open questions, not conclusions.

**The orchestrator is the repeat offender here, so make this mechanical.** A
lane's closing diagnosis is a HYPOTHESIS, not a finding: do not forward it into
the next brief as fact without byte-accounting it yourself, or it becomes a
premise the next lane must spend a round refuting. AddEnemy's "retail keeps
eleven long-lived values to our nine — find the two extra" survived into two
briefs before a lane checked and found our draft ALREADY keeps eleven; the
deficit belonged to a hypothetical edit, not to the draft. Forward the numbers
and the disagreement; let the lane conclude.

## Depth over breadth (owner directive, 2026-07-17)

**Prioritise driving a function to a FULL exact match over widening the
search to shave bytes off many parked drafts.** The success metric is
functions CLOSED, not total residual reduced. Concretely: when a lane returns
an improved-but-not-exact checkpoint, the default next action is to RELAUNCH
THE SAME FUNCTION (continuation brief carrying the new state + dead ends) —
try Opus (or Sonnet when the remaining residual looks mechanical/guided)
before spending Fable, and escalate per the routing below when they stall.
Rotate to a different target only when the function reaches an
evidence-complete park (pass-mechanics proofs on every remaining cluster,
like StageEndScreen's autopsy) or the owner redirects.

## Model routing

Owner directive (2026-07-16): **do not route subagents to Fable unless it is
really necessary** — the tooling + cookbook are mature enough that Sonnet, and
Opus for the hard tier, should carry the flywheel.

- **Sonnet** — the default: clean-seam functions, clone/family batches with a
  proven recipe, mechanical lanes, and guided finishes of near-exact
  checkpoints (≤ a few tens of residual bytes with a recorded lead). It went
  4/4 on the easy tier at ~65–110k tokens each; the mid Proc* at ~96k.
- **Opus** — the hard tier: jump-table functions, >~1000B, heavy
  register-allocation/scheduling residuals, RTL-dump reading, novel-technique
  anchors (first of a family). The `matcher-prompt.py` top line flags
  jump-table; size and float/GTE (from `triage.py`) flag the rest.
- **Fable** — the escalation tier (owner policy, 2026-07-16): when a
  Sonnet/Opus pass on a target comes back failed or flat, ONE Fable resume of
  that target is sanctioned. Start it from the banked checkpoint plus the
  failed pass's evidence (report + STATUS updates), and point it at RTL-level
  work — the vrealloc precedent: a stronger model told to ignore the C and
  read the `-dg`/`-dl` dumps cracked a "permuter-immune" tie in one pass by
  restructuring the allocno graph (a merged variable), not by fighting
  priorities inside it. Also sanctioned for family-convention anchors where a
  wrong convention would propagate. Record the justification in the lane note;
  if the Fable pass is also flat, the park is final until a new
  compiler-level insight. Precedent: the LoadConstruction escalation took a
  643-byte "parameter-home/regalloc, unreachable" park to a fence-free exact
  MATCH by re-deriving the decomposition (spilled-locals diagnosis +
  allocation-ladder engineering — see the cookbook's LoadConstruction
  lessons).
- **Fable-exhaustion fallback (owner, 2026-07-17):** if a Fable lane dies on
  a Fable-specific usage limit (as opposed to the ordinary 5-hour window),
  salvage it and RELAUNCH THE SAME LANE ON OPUS rather than waiting for Fable
  to return — continuity beats tier. The orchestrator session itself cannot
  self-switch models; if IT is Fable-capped, the user must `/model opus` and
  nudge it (or start a fresh session from the handoff docs).

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
- **A "verified uniform" clone batch needs a FULL byte comparison, not spot
  checks.** The 108-stub `dmyGs*` batch was briefed as one uniform shape, but 15
  of 108 (the `N` no-light variants) turned out to save/return `$a2` (3 register
  args), not `$a3` — a genuine SDK ABI fact the uniformity scan missed because it
  masked exactly the words it expected to vary. Five spot matchdiffs caught one
  deviant *by luck*; a different five could have missed all 15, and `./Build
  check`'s single pass/fail wouldn't say which clones were wrong. After a batch
  builds, compare the whole cluster (or whole image) byte-for-byte against the
  target in one pass — milliseconds in Python — before declaring done. (The
  matched batch also proved 3-arg vs 4-arg formals are visible in word 4:
  `addu s0,a2,zero` vs `addu s0,a3,zero`.)
- **Generator scripts must anchor stub detection on `^INCLUDE_ASM(`** (line
  start), not a raw substring grep — the seeded boilerplate comment ("drop the
  `INCLUDE_ASM` above") contains the same substring and inflates naive counts.
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
- **Fix authorship at cherry-pick time.** Worker worktrees inherit the repo
  git config (Codex's identity), so after cherry-picking an agent commit run
  `git -c user.name="Claude Fable 5" -c user.email="noreply@anthropic.com"
  commit --amend --no-edit --author="Claude Fable 5 <noreply@anthropic.com>"`.
  Claude commits as Claude; Codex commits as Codex; never change the repo
  config itself.
- **Never pipe `git cherry-pick` through another command** (`| tail` etc.) —
  the pipe eats the conflict exit status, the `&&` chain barrels on, and you
  end up running verification builds against a tree with conflict markers in
  `config/fuzzy.main.exe.tsv` (its rows conflict whenever two branches touched
  adjacent lines). Run cherry-pick bare; on a tsv conflict, resolve by
  `git checkout --ours` + `fuzz-score.py --only <name>` (regeneration, never
  hand-merge), then `--continue`.
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

**"Fold a rule" first asks: is this a decision procedure over an artifact
(a dump, a diff, a table)?** If yes, it is a TOOL TICKET (backlog below), not a
cookbook edit — as prose it is executed by a different fallible reader per lane
per round; as code it is executed once, tested, and pinned. The cookbook keeps
only judgment: mapping a tool's verdict to a source shape. After editing the
cookbook, run `tools/cookbooklint.py` (it is also in the test suite).

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
  Keep directionally-opposite rules when both reflect real compiler choices:
  `shared-return-split` creates a second source return for epilogue scheduling,
  while `shared-result-return` hoists and funnels exactly two equal-width value
  returns to create one allocation identity. Historical replay is the gate: on
  the old `Think3chase` draft the latter changed `(1008 bytes, 18 aligned
  lines, +2 instructions)` directly to exact, so future agents should run the
  guided rule instead of rediscovering the return lifetime by hand.
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

## A park verdict is a hypothesis — and "do not retry it" is a bug

This rollout retracted a cookbook section that ended *"AdtSelect is parked on
exactly this; do not retry it."* Its reload mechanics were literally correct
(verified line-by-line against the pinned sources); its DISCRIMINATOR was wrong,
and the disproof was sitting inside the function itself — three of its four
reloads of that same pseudo already self-tie and byte-match.

Two habits follow, both cheap:
- **Test a park criterion against the function's own MATCHED instructions.** If
  the criterion forbids something the target already does elsewhere in the same
  function, the criterion is wrong, not the target.
- **Never write "do not retry" into the cookbook.** Record the mechanism, the
  measurements, and the open question. Eleven "impossible" verdicts fell in two
  days; the ones that cost the most were the ones phrased as instructions rather
  than evidence.

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

### coddog (similarity at scale)

[coddog](https://github.com/ethteck/coddog) complements findsimilar.
`tools/coddog` builds it on first use (needs network + registry nixpkgs — the
flake's rust is too old); it reads `decomp.yaml`, whose input ELF is synthesized
by `tools/coddog-elf.py` from the original bytes + the Ghidra function table
(our real ELF/map aren't consumable). **Regenerate after the Ghidra export
changes or after new matches** (also refreshes the stems dir that keeps the
"(decompiled)" tag truthful):

```console
$ tools/coddog-elf.py
$ tools/coddog cluster -m 4        # identical-function families: match one,
                                   #   apply to the whole cluster
$ tools/coddog submatch <Name> 20  # who shares a ≥20-insn chunk with <Name> —
                                   #   the "stuck on this idiom" tool
$ tools/coddog match <Name> -t 0.5 # similar whole functions; good for BIG
                                   #   ones, noisy under ~300 bytes — use
                                   #   findsimilar for small ones
```

## Gotchas (all learned the hard way)

- **Wrap a maybe-not-closing draft in the `#ifndef NON_MATCHING` guard IMMEDIATELY, not
  only at the end.** `matchdiff`/`./Build` under `NON_MATCHING=1` happily succeeds while
  the DEFAULT `./Build check` silently breaks if the guard is missing — an agent leaked
  SetBleedsDir's 88 stray bytes into the default image this way, caught only by an
  explicit default `./Build check`.

- **`matchdiff -n` means --no-build: BUILD THE DRAFT FIRST, then `-n`.** Writing
  "matchdiff -n is the metric" in a brief invites reading a stale image: an
  AddEnemy lane measured three consecutive edits against the previous build and
  got a suspiciously identical 125 each time (the identical number is the tell).
  The correct sequence is always `NON_MATCHING=<Name> ./Build` **then**
  `tools/matchdiff.py -n <Name>`. Worse, the byte count alone was misleading
  there — a LENGTH MISMATCH (1148 vs 1152) only appeared once the image was
  actually rebuilt.
- **"Identical byte count across edits" has TWO causes — distinguish them before
  alarming.** It is the tell for reading a stale image (skipping the
  `NON_MATCHING=<Name> ./Build`), but it is ALSO what a genuinely
  byte-identical object legitimately produces: Shake content-hashes the `.o`, so
  a dead-code-only or allocation-neutral edit rightly skips the relink. An
  AddEnemy lane lost real time to a self-diagnosed "build-dep gap" that turned
  out to be Shake being correct. Check the dispositions or the `.o` timestamp,
  not the byte count alone.
- **Read the matched SIBLING's source, not a park's description of it.**
  FUN_80018f00's header recorded cbAccess's identical-arm CSE fence as one of
  "cbAccess's FAILED attempts" — it is the thing that made cbAccess MATCH. Once the
  sibling was read directly, the function matched first try, and the park's detailed
  `.greg` escalation turned out to be real work aimed at a lever that was never
  available. `tools/findsimilar.py <Name>` ranks the nearest matched functions;
  OPEN them. A park's characterisation of a neighbour's technique can be exactly
  inverted.
- **A park's dilemma is often a lever-allocation error, not a wall.** "All six
  orders gave correct length XOR correct slots, never both" was true and the wrong
  conclusion: declaration order was doing two jobs, and one of them (slot placement)
  is DETERMINED arithmetic, not a search. When two constraints fight over one lever,
  find a second lever for the constraint that has one.
- **TOOLING BACKLOG — a delay-slot eligibility table.** A lane hand-derived
  `dslot`/`length` eligibility and `LABEL_NUSES` per branch target, and said outright
  "that table IS the whole answer here". An `asmdiff`/`rtldump` annotation would make
  StickonCheck's entire analysis one call.
- **`asmdiff`'s `O` column is OURS. An `insert` hunk is not a surplus instruction.**
  I read CreateHumanoid's `O`-side insert hunks as instructions the target lacked and
  briefed a divide-idiom fix. The multiset is IDENTICAL (`length ours 127 vs target
  127`) — the target has them four instructions lower, and difflib was rendering an
  INTERLEAVE. The lane's verdict: taking the brief's advice "would have BROKEN a chain
  that already matches", and "the park note in the file was more accurate than the
  brief that summarized it". **Check `length ours N vs target N` before calling
  anything surplus, and use `--context`** — which exists for this and which I did not
  run.
- **`asmdiff` NORMALISES branch targets, so a brief built from it alone is
  STRUCTURALLY INCOMPLETE — cross-check `matchdiff --max 400`.** It strips the trailing
  address to align, which makes a harmless drift and a genuine RETARGET
  indistinguishable, and hides both by default. On DrawModelArchive the hidden row was
  the entire diagnosis (`beqz v0,0x8001779c` vs `beqz v0,0x800177c4` = 44 of 48 bytes,
  the signature of a relocated block). It has now cost two lanes. asmdiff now COUNTS
  what it hides and says what it might mean; `--all` shows them. **Only matchdiff's raw
  byte view doesn't normalise.**
- **THREE briefs tonight were built from a truncated `asmdiff | head -N` and all three
  mis-stated the residual** (PadProc 4x under, CreateHumanoid called an interleave
  "surplus instructions", DrawBleed missed an entirely different operation at
  0x80034440: `lhu v0,4(v1)` vs `lw v0,4(s1)`). The pattern is mine, not the lanes'.
  **Pipe the whole diff and run `--context`, or say the excerpt is an excerpt.**
- **"MEASURED DEAD END" lists are the greedy search's blind spot, and briefs treat them
  as closed.** CreateHumanoid matched on THREE edits each of which is inert alone — two
  scored a literal `nullcheck` NO-OP and one scored zero. Three rounds recorded them
  separately as dead ends and every measurement was correct. **A brief that hands a lane
  a dead-end list is handing it a list of things not to try TOGETHER.** When a residual
  survives a clean autorules sweep AND several independent dead-end notes, say so as
  evidence FOR a joint lever, not against one.
- **I "verified" two fixes by grepping my own source for a string instead of RUNNING
  the code — and both were broken.** (1) My `import time` splice into `permute.py`
  targeted a standalone `import tempfile` that does not exist (the file uses a comma
  list), so the rescue deadline raised `NameError` on EVERY SIGTERM path — the exact
  path the contract mandates — and I "confirmed" it with
  `print("deadline enforced:", "time.monotonic" in src)`. (2) `_no_target` stripped the
  trailing address but not the leading hex ENCODING, so `branch-retarget` could never
  fire — the identical hex-prefix bug I had fixed in `_mnemonic` an hour earlier and
  failed to carry across. **A test that greps the source proves the edit landed, not
  that it works. Run the thing.**
- **NEVER screen with `matchdiff -n`.** Every `-n` screen I ran after a `./Build check`
  read whatever image was on disk — sometimes the stub, sometimes another function's
  draft. That produced a FALSE cross-validation ("--clusters and asmdiff agree on
  DrawBleed") and a phantom finding briefed to two lanes. With a real build the
  classifier is exactly right (DrawBleed: two `branch-retarget` clusters at 0x80034398
  and 0x800343b0 — precisely asmdiff's two hidden lines). **The guard catches a linked
  STUB; it cannot catch a stale draft of a DIFFERENT function.**
- **A tool's static LEGEND is data to anything that greps.** `--clusters` printed a
  `branch-retarget` explainer unconditionally; my screening grep matched the LEGEND and
  I briefed two lanes to "start at the branch-retarget" on a function that has none. The
  lane's words: "the brief read the boilerplate as a finding." The legend is now printed
  only when a retarget is actually present.
- **A tool asserted a claim its OWN output disproved, on the same screen.**
  `schedtrace` printed "Nothing can lift them: MIPS defines no ADJUST_PRIORITY, so an
  insn with LOG_LINKS (nil) is at 1 unconditionally" — while printing
  `ready list at T-2: 288 (3) 291 (7f000001)` a few lines above. 0x7f000001 is
  LAUNCH_PRIORITY. The macro was never the gate (sched.c gates `adjust_priority` on
  `reload_completed == 0`), the claim was mine, I folded it into the cookbook too, and
  it cost a full round. **A tool that both PRINTS evidence and ASSERTS a conclusion must
  derive the conclusion FROM the evidence it prints — not from what I believed while
  writing it.** schedtrace now reads the post-bump priority out of the ready lists and
  names every bumped insn.
- **A rule I built and named as THE lever had never once run.** `binop-operand-seed`
  emitted its temp as a DECLARATION at the seed site — a C89 parse error under cc1 2.8.1
  — so every candidate it ever produced scored `invalid`. A lane reported all 13 failing
  and called it "a silent gap". I had hit the identical trap building `copy-seed`, tested
  for it, and added a guard there; the lesson did not carry across. **When a sweep
  reports 0 wins, check that its candidates COMPILE before concluding the residual is not
  mechanical** — and when fixing a bug in one rule, grep the others for the same shape.
- **I pasted a tool's REFUSAL MESSAGE into two briefs under a heading claiming it was
  data.** I ran `matchdiff --clusters -n` after a `./Build check` had relinked the stub;
  the guard correctly refused, and I pasted the refusal under "### The FULL cluster
  table (both units, never truncated)". A lane caught it: *"a refusal message got pasted
  under a 'never truncated' heading and read as data."* The numbers in my prose came
  from an earlier correct run and happened to be right — which is worse, not better.
  **Never pipe a tool's output into a brief without reading it. `-n` is exactly when a
  guard fires, and a guard firing is not a table.**
- **Never quote a TRUNCATED tool head as a byte-account.** I built PadProc's brief
  from `asmdiff | head -9` and presented 4 diff lines as the residual; the real one is
  18 differing instructions across TWO `mflo` sites. The lane called it a 4x
  under-count and re-derived it. Pipe the whole diff, or say explicitly that the
  excerpt is an excerpt.
- **A tool's flag table is a claim too — verify it against the source.** `rtldump`'s
  PASS_FLAG had `jump2 -> -dj`, which is `jump_opt_dump`: `--pass jump2` silently
  dumped `.jump`, the FIRST jump pass, and a brief sent a lane to read it. And `-dR`
  is `sched2_dump`, not reorg, so there was no `sched2` key at all — a lane could not
  read the pass owning 16 of its 28 bytes and said so. Both are fixed against
  toplev.c's own `case` labels.
- **NEVER HAND-DERIVE WHAT THE DUMP PRINTS.** The `.sched` dump contains cc1's own
  `;; insn[NNNN]: priority = P, ref_count = R` trace (`sched.c:3686`, 250 lines on
  AddEnemy). A round derived a priority from the RTL by hand, read the `ref_count`
  column as priority, and inverted a correct 6-round-old park — I folded the
  inversion into the cookbook AND briefed the next round on it, which spent itself
  restoring the original reading. `tools/schedtrace.py <Name>` now tabulates the
  trace. **Before folding a rule derived by reading RTL, ask whether the dump states
  it outright.**
- **A REFUTATION CAN ITSELF BE WRONG — and I amplify whichever one arrives last.**
  Round 7 was right, round 12 "refuted" it, round 13 refuted the refutation and
  restored round 7. My failure mode is treating the newest report as authoritative
  because it is the most detailed. Weight by EVIDENCE KIND, not recency: round 12
  hand-derived, round 13 read cc1's own printed table. A claim citing a dump line
  beats a claim citing an inference, regardless of order.
- **LEAD BRIEFS WITH THE CHEAP EXPERIMENT, NOT THE DEEP MACHINERY.** DrawSnow's lane:
  "the brief's framing pushed hard toward sub-C machinery for what were two one-line
  source edits. The cheap experiment came first and ended it." I spent most of that
  brief on schedtrace / rtldump --trace / siblingdiff --demo / the permuter; the answer
  was swapping two statements and a cookbook rule applied verbatim. **Put the one-build
  experiments at the TOP and the escalation path at the bottom.**
- **TOOLING BACKLOG — a `findrule` lookup: residual SHAPE -> candidate cookbook
  sections.** DrawSnow's lane found §3459 (`arr[idx]` base-first) by grepping the
  section index and recognising "base/offset colour swap" — both its fixes were cookbook
  rules applied verbatim, and nothing ROUTED it there. The cookbook is now large enough
  that finding the right rule is itself a skill. (Its `arr[idx]` -> offset-first rewrite
  is also a clean autorules candidate — mechanical and byte-checkable.)
- **TOOLING BACKLOG — siblingdiff should say "TRANSCRIBE THIS", not diff instructions.**
  Three lanes in a row found the answer sitting in a matched sibling's C and got there
  by eye: DrawSprite is ranked 0.48 to DrawClip and shares its TU, and its body is a
  one-to-one answer key. The lane's words: "nothing in the loop told me to just
  transcribe it… autorules/permute/rtldump would all have missed this — it's block
  PLACEMENT, and none of the tools model that." When a sibling scores high AND shares a
  TU, the tool should say *transcribe this body, change only the tail*.
- **`tools/siblingdiff.py --demo` is the most underused tool in the box.** It has now
  been decisive twice and named so by both lanes: it answers "is this real codegen or
  our scaffolding?" in ONE call, before a round is spent. FadeOutDirect's demo emits
  the block instruction-for-instruction identically to retail — proof it was source,
  not scheduling. Its park had instead concluded "a decomp.me session would be the
  next lever"; the demo build was. **Name it in every brief for a function with a
  demo twin.**
- **A byte-account is not a byte account unless you checked the UNITS.** I
  propagated mission_score_screen's cluster table through three briefs as BYTES; the
  numbers were INSTRUCTION counts, ~3x off (84 insns = 254 bytes, so the "16-byte"
  GsSortSprite cluster is really 63). And its INDEPENDENCE assumption was false —
  fixing one allocation also removed two clusters the table listed separately, as
  they were downstream of it. Clusters are a hypothesis about causes, not a
  partition; state the units and re-derive them.
- **TOOLING BACKLOG — `matchdiff --clusters`.** Every round hand-rolls per-cluster
  counts because matchdiff reports only a total, and that is exactly how the insn/byte
  mix-up above survived three briefs. A flag printing range / insns / BYTES per
  cluster would prevent the class outright.
- **Do not tell a lane master has a fix before you have COMMITTED it.** I sent a
  rule correction saying "master now has the fixed version, `git merge --ff-only
  master`" — between harvesting and committing. Master was still at the lane's own
  base; it checked, found nothing to merge, and correctly used the message as
  guidance only. Commit first, then message, and cite the SHA you actually created.
- **A rule that names a MECHANISM must also name what the mechanism FORBIDS.** The
  `addu` operand-order rule named the ARRAY_REF/INDIRECT_REF tree distinction and
  stopped — so its obvious fix (`(*(T (*)[70])p)[i]`) is precisely the one construct
  the gate rejects. A rule that explains a difference without naming the dead ends
  costs a lane the round it saves.
- **A rule with a DIRECTION must say so, or it gets applied backwards.** The
  LOOP_BEG prediction flip matched LoadTIMpack (which wanted the +1 copy) and is
  exactly wrong for StickonCheck (which needs to PREVENT a target-thread steal) — I
  briefed it as a bare lever and the lane measured 90 -> 90 disproving it. When
  folding a rule that selects between two outcomes, name the outcome it selects.
- **TOOLING BACKLOG — `tools/passtrace.py <Name> <rtx-pattern>` (HIGH VALUE, asked
  for twice).** Two lanes hand-rolled the same per-pass sweep — grep an insn across
  `*.i.*` dumps in pass order to find which pass FIRST rewrites a register — and both
  called it their highest-value step. GetAreaMapVector's answer was "`.cse`/`.loop`
  keep reg 91, `.cse2` rewrites it to reg 100", which named the cause outright. The
  sweep is mechanical: print the operand per pass, in pass order.
- **TOOLING BACKLOG — a per-block load classifier.** Nothing surfaces "which load is
  free to move": printing each load's LOG_LINKS classified as FIXED (`symbol_ref`,
  floats anywhere) vs VARYING (`(plus (reg N) K)`, pinned below every preceding
  store) would have named FUN_8004c59c's 23-byte lever immediately.
- **TOOLING BACKLOG — autorules `join-store` and `guard-expr-inline`.** Both
  mechanical, both byte-safe, both matched a function by hand tonight: hoist a store
  duplicated in both if/else arms into a single post-if statement (and its inverse);
  replace a pre-loaded compare local with the direct expression.
- **A stale WARNING is as costly as a stale fact.** I told round 13 the cluster table
  was "wrong twice over" — but round 12 had already fixed it, and the lane spent effort
  re-deriving what was already correct (76 insns / 239 bytes / 23 clusters, rows summing
  exactly). Corrections expire too; re-check a warning against the current file before
  repeating it.
- **A NULL EXPERIMENT reported as a negative result is worse than no experiment — it
  warns later rounds away from the real lever.** DrawClip's park A/B'd `return -1` at
  every site against `goto shared;` and found them byte-identical, concluding the layout
  was "cc1's own cross-jump heuristic, not a source spelling choice". Both arms held the
  shared LABEL at the bottom next to `ret:`, where both collapse to the same thing — the
  one variable it never moved was label POSITION, which is the entire lever. Three later
  rounds trusted the negative. It matched on the first build once the label moved.
  **Before recording "X vs Y made no difference", ask what Z you held fixed in both
  arms** — and write the negative as "with Z fixed at W", never as "unreachable".
- **RETRACTED (and the retraction is the lesson): "park STATUS numbers are badly
  stale — don't select targets from them" was MY error, not the parks'.** I measured
  DrawSnow at 14 and "corrected" its header, which had said 14 all along — the 203 I
  quoted was **StageEndScreen's** number, which I misattributed while surveying with a
  truncating grep and filled the gap from memory. SetBleedsDir's header said thirteen
  bytes and measures 13. SetupTelop has no STATUS at all. **The headers were the honest
  artifact in the chain; the orchestrator was not.** The lane caught it: "The '203'
  claim was third-hand and false; the header was the one honest artifact."
  **When a survey and an artifact disagree, suspect the survey first** — and never
  generalize a rule from a number you did not read in the file you are talking about.
- **Rank with `config/fuzzy.main.exe.tsv` — it is free and needs no builds.** Still the
  right selection tool, but because it is cheap and total, NOT because the headers lie:

      99.79%  FUN_80057b80    98.17%  SetBleedsDir    97.81%  GetAreaMapVector
      98.61%  AddEnemy        98.16%  StageEndScreen  96.28%  SetupTelop

- **RE-SCREEN EVERY PARK WHEN A TOOL'S BLIND SPOT IS FOUND.** `asmdiff` hid branch
  retargets by default; the moment that surfaced, a sweep found DrawClip (2 hidden) and
  ControlTraceLine (4) carrying the same signature that turned out to be 44 of
  DrawModelArchive's 48 bytes. **A tool bug does not just cost the lane that hit it — it
  silently invalidates every park written while it was live.**
- **RE-PRICE THE RESIDUAL EVERY ROUND. Rounds 5-10 on AddEnemy all spent
  themselves on cluster A while 20 of 35 bytes were ordering questions NO round had
  attacked.** A byte-account is a snapshot, not a standing truth: the clusters move
  as the draft changes, and a brief that inherits last round's priorities inherits
  last round's blind spot. Round 11 re-priced first, found the unattacked 20, and
  took 6 of them.
- **I pasted round 10's E3 analysis onto a draft that is not E3 — one round after
  folding round 10's lesson that "a citation is not a proof".** The charter ("find
  one loop-1 allocno with priority > 520") was a CATEGORY ERROR: our preheader
  already emitted every one of retail's registers, so there was no tie to break at
  all, and the residual was pure emission ORDER. Before writing a brief around a
  quantitative lead, **check the lead still describes the CURRENT draft** — quoting
  a prior round's dump numbers at a changed file is the same error the prior round
  warned about, one level up.
- **A false "measured" fact propagated through FIVE rounds. Re-measure inherited
  claims that a round is about to rest on.** AddEnemy's round 5 recorded "with the
  hack deleted, gcc spilled them on its own to sp+0x7e0/sp+0x7e4 — byte-identical
  slots"; five rounds quoted it as settled, three briefs built on it, and round 10
  finally ran the experiment: gcc spills NOTHING. Worse, the supporting evidence was
  CIRCULAR — round 5 had sized the struct so those slots would land there. Rules:
  a STATUS/brief claim must name the round that measured it, and a lane must
  RE-MEASURE any inherited fact its plan depends on. "Measured in round 5" is a
  citation, not a proof.
- **Briefs on the hard functions are now 6-for-6 REFUTED, and that is the process
  working — but stop asserting a cause.** AddEnemy round 9 is the sharpest case:
  the brief said "the $a0/$a1 swap is downstream of that one choice", and cluster C
  was in fact two INDEPENDENT halves (7 bytes and 1 byte). Three rounds went into
  the 1-byte half on the strength of that sentence. A brief must hand over
  EVIDENCE and OPEN QUESTIONS, never a conclusion — and must say which parts are
  measured vs. inferred, because lanes reasonably trust the framing more than the
  numbers.
- **Suspect our own scaffolding before the compiler.** AddEnemy's biggest remaining
  cluster is blocked by a `volatile` spill hack THIS PROJECT added: it reassigns
  `weapon_base`, so `n_times_set != 1` and loop.c can never make it a movable.
  Retail's spill slots sit exactly where the allocator's own caller-save would put
  them, which suggests retail has no such hack. Fences, volatile hacks and call
  aliases are reconstruction DEBT; when a pass "refuses" to do something, check
  whether our own scaffolding is what forbids it.
- **TOOLING BACKLOG — `struct-view` autorules rule (HIGH VALUE, fully specified).**
  `member-scalar-alias` sweeps the cast direction (COMPONENT_REF -> INDIRECT_REF,
  clearing `/s`, the DrawSplash lever) but NOT its inverse, and the inverse matched
  HumanActionControl unattended-able: rewrite `*(u16 *)&p->field` to a read through
  a TU-local view struct (`typedef struct { u16 mid; } MotionManagerU;`), which
  emits the same `lhu` while KEEPING `/s`. Needs a file-scope typedef insertion, so
  it is larger than an in-function splice. Every width-forcing cast in a parked
  draft is a candidate.
- **TOOLING BACKLOG — `sched-deps <Name>`.** A lane hand-derived, from the dumps
  alone: UID -> mnemonic/C-line mapping (grepping `.combine`, correlating 13 UIDs
  against the target `.s`); inverting `.sched2`'s backward `T-1…T-25` pick order to
  forward; the priority arithmetic; and the five `anti_dependence` clause
  evaluations. All four are deterministic functions of the dump. The tool should
  emit, per displaced insn: its LOG_LINKS, its derived priority, and the verdict —
  e.g. *"REG_DEP_ANTI 50 exists only because load 50 lacks `/s` -> try a struct
  view."* That is an entire round's reasoning chain, mechanised.
- **A tool's VERDICT is a claim, and a wrong one costs a whole round.** reghist told
  a lane "Sum ZERO = pure renames … the variable decomposition already matches the
  target. Do not hunt a mega-pseudo here" — authoritative, and wrong: a zero-sum
  histogram cannot tell a rename from a same-count/different-IDENTITY copy
  structure. It now hedges when `move` copies are present. When adding a heuristic
  verdict to a tool, write down what it CANNOT distinguish, in the output, next to
  the verdict.
- **Three tools silently reported the wrong thing, all fixed together, all the same
  class.** (1) `autorules` scores every candidate through a source override, so it
  left the LAST CANDIDATE's image on disk while writing the real source — `matchdiff
  -n` straight after a sweep read the wrong build and produced a spurious LENGTH
  MISMATCH. It now rebuilds before exiting. (2) `matchdiff` truncated at `--max 40`
  and printed only "+N more"; a lane built a byte-account from the visible rows (12
  clusters/160 B where the truth was 23/239). It now states the CONSEQUENCE. (3)
  rtldump's isolation — see below. **When a tool ends, the world it leaves behind is
  part of its output.**
- **My own fix introduced the failure it was preventing, TWICE.** rtldump's dump dir was
  shared across agents (a lane nearly read a sibling's stale `.greg`), so I keyed it to
  the worktree via `tempfile.gettempdir()` — which reads TMPDIR, and **`nix develop`
  mints a fresh TMPDIR per invocation.** The path then changed every run: a lane
  captured one, reused it, silently diffed two EMPTY files and got a clean
  "IDENTICAL". Then the ROOT hash was applied only when CLAUDE_SCRATCH was UNSET — and
  agents ALWAYS have it set, to the SHARED scratchpad, so the isolation never applied
  in the one case it existed for; a lane found two OTHER lanes' stale `.sched` dumps
  there. **A path used to compare two runs must be stable ACROSS runs, and isolation
  must not be optional — verify by printing it twice and by testing the env the agents
  actually run in, not the one you tested in.**
- **A tool that can silently measure the STUB is the worst bug class in this repo.**
  Three instances now: `reghist` built the stub and reported "every register matches
  exactly" (a vacuous truth that tells an agent no lever remains); `matchdiff -n`
  reads a stale image; `rtldump` dumped the INCLUDE_ASM trampoline for guarded
  functions, whose `.greg` is **0 lines against the draft's 1338** — every pass
  reads empty, so any conclusion drawn from it is invention. All three are fixed by
  the same move: **detect `is_guarded()` and do the right thing by default**, rather
  than warn and hope. rtldump now defaults to `--draft` when guarded (`--stub` opts
  in loudly). When adding a tool that compiles a function, ask what it does on a
  guarded source before shipping it.
- **Read `n_refs` from `.lreg`, never hand-count source references.** A brief this
  session claimed a pseudo carried 16 refs (hand-counted) when the committed draft
  measured 19 — the lever built on it could not have worked, and the lane spent a
  round proving that. Every "the original's X carries K refs" claim must cite a
  dump. (`tools/reghist.py` gives the cheap cross-check: a register the target
  mentions N times has N-2 body refs.)
- **`tools/nullcheck.py <Name>` now answers "did my edit do ANYTHING?" — use it
  instead of re-deriving.** Four lanes have burned time on this: one hand-rolled
  sha256+revert+rebuild to find that a four-site edit produced a byte-identical
  object (cse1 folded it), three mis-read a fast build as skipped. Exit 1 = no-op,
  exit 0 = codegen changed.
- **A 0.1 s "build" is not automatically a stale read — and `touch` will NEVER
  trigger a rebuild.** Shake keys on CONTENT DIGESTS, not mtimes. So (a) a
  dead-code-only or codegen-neutral edit legitimately produces an identical `.o`
  and rightly skips the relink, and (b) probing the build with `touch` proves
  nothing: it changes no content, so doing nothing is CORRECT. Three separate
  lanes have now mis-diagnosed this — one filed a "build-dep gap", one distrusted
  a valid measurement, one concluded `./Build` had silently no-op'd after a
  `touch`. Verify with the `.c` -> `.o` -> `main.exe` content/timestamps, or just
  use `tools/matchdiff.py <Name>` (no `-n`), which does its own build.
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

- **An agent's worktree can be based on a STALE commit — tell every round-N
  lane to check.** Agent worktrees do not reliably branch from master's tip: a
  FUN_80057b80 round-3 worktree was based on a commit predating round 2, so its
  `.c` was a bare `INCLUDE_ASM` stub with none of the banked findings; several
  other lanes silently fast-forwarded before starting. The agent noticed and
  recovered, but a lane that does NOT notice will re-derive banked work or
  "improve" a state that master already beat. Put this in every continuation
  brief: *verify your `src/main.exe/<Name>.c` carries the checkpoint the brief
  describes; if not, `git merge --ff-only master` (or reset onto it) FIRST and
  re-measure the baseline before any edit.* The harvest-side defence is already
  standard: re-measure every returned state yourself and take the file, not the
  agent's number.
- **A fresh worktree needs `tools/wt-init.sh` before anything works.** `disks/` (the
  game images) and `.shake/` (which holds the Ghidra export) are both gitignored, so a
  new worktree has neither: `./Build` cannot find the target image, and
  matcher-prompt.py / coverage.py / triage.py / findsimilar.py all die with
  FileNotFoundError on `.shake/ghidra-export/functions.tsv`. wt-init.sh symlinks both
  from the primary worktree. Tell every worktree-isolated agent to run it first.

- **Read the actual gcc-2.8.1 sources when a residual looks "provably
  impossible".** DamageControl's last cluster fell in one step once the agent
  read `reorg.c`'s `fill_simple_delay_slots` and `config/mips/mips.md`'s
  `abssi2` template (extract them to /tmp; the nix-pinned compiler's source is
  the ground truth for every "cc1 always/never does X" claim). Several parks in
  this project rested on plausible-but-wrong models of a pass; the sources
  settle them. Pair with `rtldump --pass all`.
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

**The cookbook-restructure tickets (2026-07-17), ranked by lane-rounds already
paid.** The restructured cookbook keeps compressed decision procedures for
exactly these, each marked `TOOL TICKET (name)` inline; when one is built, the
marked prose shrinks to a router line. Specs: docs/cookbook-audit.md §4.

1. ~~`sched-deps`~~ **BUILT** (82bf8cc + 56987f0): equal-priority-group
   analyzer, `.flow`-vs-`.sched` block-leader report, and the argmove priority
   CENSUS — which REFUTED the old "argmove floor — park on sight" rule on its
   own cited function (23/30 above priority 1); schedtrace's ready-list "pick"
   reading was also fixed (the pick is the head of the LAST `, now` — on a
   hazard cycle the first list's head is the LOSER). Remaining ticket:
   **the remat-split detector** (UID rewritten to a const set + store at fresh
   UID → "reload split — not a sched tie, not fence-fixable") — deliberately
   unshipped, no live case to pin a test against; build it with the next one.
   Absorbs the backlog's "scheduler-tie classifier" entry below.
2. **`regalloc --spill-uses` / `--names` / `--local`** (≥5 rounds; three lanes
   asked) — entries below; `--order`'s self-validation is BUILT.
3. **`matchdiff --baseline <saved.json>`** — per-cluster delta table across an
   edit (+7 totals must never again hide −27/+34); `--clusters` itself is
   BUILT.
4. **`loadclass`** (~3 rounds) — per-load address-kind classification
   (FIXED symbol_ref / CHAINED lo_sum / VARYING reg+K incl. sp) + anti-dep
   lists + the struct-view verdict; cookbook §3.13 is the spec.
5. **`dslot`** (~3 rounds) — per-branch: condition class, `mostly_true_jump`
   result WITH DIRECTION, thread ownership, skip-label leader eligibility +
   LABEL_NUSES, actual fill + which reorg routine; cross-checks its prediction
   against the bytes and prints MODEL-DIVERGES. Absorbs the "delay-slot fill
   classifier" entry below.
6. **`slotcalc`** (stackplan extension, 1–2 rounds) — model assign_stack_local
   from declared sizes + target sp+K accesses → unique declaration order +
   "N compiler spill slots implied"; 0 or >1 orders fit ⇒ fail loudly.
7. **`passtrace <Name> <pattern>`** — first-rewrite sweep across pass dumps by
   PATTERN (rtldump `--trace UID` is BUILT; the pattern form remains); zero
   matches across all passes is an error, and remind that reload_cse_regs has
   no dump.
8. **`selfsim`** (1–2 rounds) — target self-similarity: residual inside
   instance k of a repeated block whose instance j matches ⇒ "spelling
   asymmetry — diff your sites, stop looking at the compiler".
9. **autorules batch** (each cites a win): `struct-view` (HumanActionControl),
   `guard-clause-invert` (GetNearestHumanoid), `invariant-inline`
   (SetBleedsDir — would have matched unattended), `chain-order-swap`
   (DrawSnow), `return-to-goto` (§3.11's join-label rule), `merge-def-swap`
   (SetBleed), `redundant-entry-guard` (KillHumanoid), `neg-mul-strength`
   (EndDrawing), plus the entries already listed below (extern-complete-size,
   narrowed-negate, signext-inplace-pair, arrayref-int-sum/multi-dim,
   cmp-swap guard extension, ChasetoTarget's two).
10. **permute-rand-message** — permute.py should print its own abort reasons:
    the rand() typemap KeyError today reads as a crash (it already prints one
    for gte.h functions).

The audit's `findrule` (T10) is dropped: the restructured router IS the static
findrule.

- **autorules `cmp-swap` site selection misses regalloc-owned guard hunks** (it
  targets combine/expression-owned hunks). DamageControl's −6 operand-order win
  sat in a regalloc drift region and had to be found by hand. Extend the site
  filter.
- **autorules: multi-dim explicit-index rule** — convert `&arr[i][j][k].field`
  on a top-level extern into `(k<<sk) + ((i<<si) + (j<<sj)) + (int)arr`
  (innermost term first, higher strides grouped, base trailing). Needs declared
  dimensions to derive strides. LoadConstruction paid −21 manually.
- **rtlguide aggregates register goals too coarsely** — it lumped five pseudos
  as one "$s2→$s3 / $s3→$s4" when only x/y had actually rotated, and the agent
  had to disentangle by reading split-piece `.s` by hand. Wanted: per-pseudo
  wrong-reg mapping, plus a "disjoint pseudos coalesced in ours but split in
  target → try split/ballast" diagnostic.
- **`reverse.py --size` should re-size an existing `c` carve** (absorb the
  trailing orphan data blob). Today it no-ops on already-carved functions, so
  the BreedLife under-sized fix needed a manual yaml line deletion.
- **Under-sized-carve detector**: teach `coverage.py` (or a small sibling) to
  flag the delay-slot signature mechanically — for every carved function,
  objdump the word at `carve_start+carve_size` and warn when it decodes as
  frame teardown / a return delay slot rather than the next prologue.
- **autorules: veto/flag width-changing type-narrows + plateau revert.**
  AddEnemy's sweep greedily kept a byte-reducing `s16→s8` narrow that emitted
  `sll 0x18` where the target has `sll 0x10` — semantically wrong AND masking
  a −14 win behind it. Extend the LOCAL-SHAPE REGRESSION mechanism into a
  hard veto when a kept type-narrow changes an aligned sll/sra shift AMOUNT
  vs the target; and on plateau, try reverting the sweep's own last kept
  type-narrow before concluding.
- **autorules: the extern-array completion rule (HIGH VALUE, fully specified).**
  For any `lui/addiu` byte-diff where target and draft differ ONLY in the `lui`
  destination register, try completing each `extern T x[];` declaration to sizes
  1..8 bytes. `ENCODE_SECTION_INFO` sets `SYMBOL_REF_FLAG` (which declines the
  address split) iff the type is complete and `0 < sizeof <= -G`. Bounded,
  byte-safe, and it would have closed RestoreItemLayout's park in one unattended
  minute — the fix was a single character.
- **autorules: reject the single-use-into-call-argument rewrite class** — combine
  folds `(set P expr)` + `(set (reg a0) P)` into one insn and flow deletes the
  dead def, so such a value can never donate a copy preference. Flagged by the
  ControlHumanoid lane as the one mechanical rule of its four findings.
- **autorules: the "narrowed negate" rule** — rewrite `narrow = -narrowVar` as a
  separate SImode negation operand (convert.c narrows NEGATE_EXPR through the
  truncation, so the narrow form can never emit the target's sign-extended
  negate). Mechanical and local; flagged as a good candidate by the lane that
  found it.
- **autorules: in-place sign-extend shift-pair rule** — rewrite `x = (s16)x` /
  `x = (s16)src` sites as `x <<= 16; x >>= 16;` (forces in-place sll/sra
  instead of a $v0 scratch; FUN_800519bc paid it manually twice). Also
  consider a bounded cross-statement move family (`fade_step = -8;` moved
  across one call to fill its delay slot was manual).
- **permute.py: report the minimal semantic delta of the best
  authoritatively-rescored candidate** (with dead-declaration flagging), and
  PERSIST the best candidate to a known path on timeout-kill (StageEndScreen's
  bounded run lost all candidate output; start_demo_'s win had to be recovered
  via `--rescore-only` plus a manual diff).
- **autorules: two guided transforms from ChasetoTarget** — "inline a
  single-use abs/min/max temp into its comparison" and "inline a single-use
  CSE'd array element into its first-use expression" (both were manual).
- ~~wt-init staleness warning~~ **BUILT** — `tools/wt-init.sh` now prints how
  many commits the branch is behind master and the exact ff command. Seven lanes
  in one rollout started 26-36 commits stale; one was missing the checkpoint its
  task described, another a tool its task told it to run first.
- **regalloc.py `--local`**: surface local_alloc quantities (lreg qty -> hard reg
  + life lengths). It currently shows only `-dg` GLOBAL allocnos, yet local
  colouring DRIVES global preferences via `set_preference` — ControlHumanoid's
  whole residual was local quantities, invisible to the tool.
- **autorules: the fixed-address indexing toggle** — pointer-local vs
  constant-macro at each fixed-address indexing site, keyed on commutative `addu`
  operand order (close in spirit to `late-pointer-direct`/`ptr-base-split`). It
  found 16 bytes that a 37-candidate default sweep missed.
- **autorules: `add-prefix-temp` should also try the FULL-sum temp**, not only the
  two contiguous two-term seams — naming the full sum is what closed
  ControlHumanoid's cluster.
- **asmdiff `--context N`**: show N unchanged instructions around each hunk.
  asmdiff prints only differing lines, but the SURROUNDING context is what reveals
  delay-slot and cross-jump structure — the ActSTICKON lane hand-rolled a 10-line
  script over `asmdiff.dis()` to get it, and that context is what exposed the
  jump2 artifact behind a 7-byte "register cycle".
- **regalloc.py `--spill-uses <Name>`**: for a spilled pseudo, list each use as
  BARE (self-tie legal) or IN-MEM (self-tie barred). That single distinction was
  the whole of AdtSelect's round — done by hand-grepping `reg/v:SI 81` in the
  combine dump and eyeballing `(mem/s:SI (reg 81))` vs `(plus … (reg 81))`. It is
  mechanical, and it retired a "do not retry it" verdict.
- ~~matchdiff `--account`~~ **BUILT as `matchdiff --clusters`** (insns AND bytes
  per cluster, units in the header). Remaining: the per-cluster classification
  (register-swap / inserted / moved) and `--baseline` (ticket 3 above).
- **regalloc.py `--names`**: join pseudo -> source variable (rtldump already
  shows `(reg/v:HI 85)`). Lanes hand-count refs to identify pseudos, and the
  "confirm identity via disposition" rule exists precisely because that misread
  costs rounds.
- **asmdiff/rtldump: flag a LOAD-FREE block** ("sched cannot reorder this — it is
  your source order"), turning a multi-step source dive into a printed line.
- ~~regalloc.py: self-validate the priority model against `.greg`'s allocno
  order~~ **BUILT as `regalloc --order`** (prints cc1's own order, ours, and a
  self-validation verdict that REFUSES divergent output; 12 functions / 267
  allocnos, zero divergence).
- **A backward-branch scan must EXCLUDE calls.** A recursive `jal` back to the
  function's own entry glabel reads as a loop; FUN_80057b80 was briefly credited
  with 4 loops when it has 0. Affects any loop-counting heuristic.
- **rtldump's output directory changes per `nix develop` invocation**
  (`/tmp/nix-shell.XXXX/`), so an A/B across two invocations silently compares
  stale files. Capture the path rtldump prints, per invocation.
- ~~per-register reference-count diff~~ **BUILT: `tools/reghist.py`** — three
  lanes hand-rolled it before it became a tool (worth 140 bytes on FUN_80057b80;
  a cheap negative on StageEndScreen and AddEnemy). It is now the recommended
  first move on any big Ghidra function, and its delta SUM doubles as an
  exhaustion proof. Note the trap it was born with: objdump prints MIPS registers
  WITHOUT the `$`, so a `\$reg` regex silently reports "everything matches".
- **asmdiff: a register-canonicalizing alignment mode.** A single register swap
  makes asmdiff invent large phantom reorder hunks (T74/O74, T30/O2) that cost
  real time to dismiss; canonicalizing register names before aligning would make
  a true 2-instruction residual obvious.
- **rtlguide: a delay-slot fill classifier** — report which reorg pass filled
  a branch's slot and why ("branch N's slot filled by fill_simple stealing
  insn M backward, so fill_eager never duplicated the shared return move");
  ChasetoTarget's final diagnosis needed a hand-read of gcc reorg.c.
- **rtlguide: a scheduler-tie classifier** — detect "priority-N tie between
  insns A/B whose order is fixed by pre-scheduling LUID at pass P;
  source-position-invariant" and advise PARK immediately. StageEndScreen's
  dominant ~140-byte cluster took ~5 manual build cycles to prove; the .sched
  `ready list`/`priority`/LUID evidence is mechanical. Highest-value gap for
  parked drafts.

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
  `tools/gte2word.py` is deleted.) **The policy call is RESOLVED (2026-07-16):
  the owner adopted the restricted `gte.h` inline-asm layer** — see
  [gte-policy.md](gte-policy.md): PsyQ INLINE_N.H-style macros in
  `src/main.exe/gte.h` (COP2 moves native, GTE commands as `.word`) plus
  pinned-register locals, ONLY for functions whitelisted in
  `config/gte-allowlist.txt`; containment + whitelist are unit-tested.
  `SetDepthQ` matched byte-exactly as the pipeline spike. The
  `GetPad`/`GetPadXY`/`FUN_8001b174` sign-extension trio and `PClseek` are
  deliberately EXCLUDED (plain-C / libsn-assembly originals — an asm body
  would be unfaithful; they stay parked). Family order: `drawF3` anchor →
  15 `draw*` clones → the three twin pairs → `FUN_80057b80` → `DrawTMD`. m2c
  can already read the region (`--input-regs`). ArrangeLocalMatrix was an old
  false positive in this list: its `$t2..$t6` values are internal loop
  temporaries and its calls use the normal ABI; it now matches in pure C.
