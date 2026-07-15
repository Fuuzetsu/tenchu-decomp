# Decompilation flywheel handoff — 2026-07-15

This is a dated shutdown snapshot, not a second match-status database.  The
current source, `tools/progress.py`, `tools/triage.py`, and each guarded
function's `STATUS` comment are authoritative.  Re-measure before dispatching
work; do not carry the percentages or target list below forward by assumption.

The rollout described below was the previous shutdown state. The flywheel has
since resumed with the available four-slot pool. A 2026-07-15 user correction
also supersedes this snapshot's isolated-only checkpoint policy: root may merge
a reviewed, monotonic guarded `NON_MATCHING` improvement and its current fuzzy
row because the default `INCLUDE_ASM` build remains byte-identical. Such a
checkpoint must carry an honest residual, pass its draft comparison plus the
default/global gates, and never be counted as a matched function. Exact pure-C
promotions remain the preferred outcome.

## Clean anchor

- code/tool anchor immediately before this note: `36afb3b` (`Document
  identical-arm source identity splits`)
- newest exact function commit: `2416459` (`Match small-rotation think
  handler`)
- game code: 447/555 functions (80.54%), 211856/302824 bytes (69.96%)
- all six executables passed `./Build check-all` byte-identically
- all 302 matching-tool tests passed
- guarded-draft flags, fuzzy fingerprints, symbol notes, symbol uniqueness,
  and `D_<address>` symbol names passed their global checks

The progress numbers above are merely useful confirmation that the right
anchor was checked out.  Get the current numbers with:

```console
nix develop -c python tools/progress.py
```

## Resume preflight

Run these before selecting targets:

```console
git status --short
git log -5 --oneline
nix develop -c python tools/progress.py
nix develop -c python tools/triage.py
nix develop -c python tools/triage.py --parked
nix develop -c python tools/fuzz-score.py --check
nix develop -c python tools/maspsxflags.py --check
nix develop -c ./Build check-all
```

If the anchor or metadata has moved, that is normal: use the newly measured
state.  Before launching a particular target, run `tools/triage.py <Name>`,
read the top comment in `src/main.exe/<Name>.c`, and confirm that the current
source still contains an active `INCLUDE_ASM`.  This prevents duplicate work
on functions already promoted to exact C.

Run the slower repository-wide hygiene gates before harvesting the first batch
and again at shutdown:

```console
nix develop -c python tools/symnote.py --all --check
nix develop -c python tools/dedupe-symbols.py --check
nix develop -c python tools/symcheck.py
nix develop -c python -m unittest tools.tests.test_matching_tools
```

## Work completed in this resumed rollout

Forty exact source promotions landed after the prior `dc91927` shutdown
anchor. Use `git log --oneline dc91927..36afb3b` for their source/reflection
history. The promoted functions were:

- `debug_output_edit_camera_settings`, `FUN_80058a54`, `calculate_score`,
  `SelectStage`, `FUN_8003d768`, `run_exec_file`, `AdtVsprintf`,
  `debug_menu_file_animation_test`, `jt_init4`, and `cd_open`;
- `FUN_8005679c`, `cbAccess`, `update_pressed_buttons`, `FUN_800593a0`,
  `InitializeInfoView`, `ControlAllHumanoid`, `ProcMiscFire`,
  `SelectCameraOwnerOption`, `UpdateEvent`, and `PlayMusicFormID`;
- `DoMiscProc`, `cbCheckCD`, `MemCardCallback`, `IsVisible`, `CdaPlayXA`,
  `FUN_8001b2f4`, `ItemControl`, `leSetEnemy`, `DrawTarget`, and
  `AVCameraControl`;
- `FUN_8004a6bc`, `DrawTargetS`, `FallCheck`, `ComPad`, `CVAsequence`,
  `GetAreaMapPassage`, `ProcMiscPitfall`, `Think1ninja`, `AttackShort`, and
  `think_setting_small_rotation_small_steps_`.

The last assignments closed as follows:

| Function | Result | Integrated/preserved commits | Useful result |
|---|---|---|---|
| `AttackShort` | exact 1668 B / 417 instructions | `ad08550`, reflection `9a617b8` | an HImode round trip plus an empty loop and identical widening arms removes a false literal-zero equivalence before `jump2` |
| `think_setting_small_rotation_small_steps_` | exact 1392 B / 346 instructions | `2416459`, reflection `11a249c` | make one signed-word carrier absorb the entire condition/result chain; reuse a dead local for the other operand to remove a hard `$v0` conflict |
| `GetAreaMapVector` | guarded 548 B checkpoint, 40 bytes remain | isolated `f2d0a6e`, fuzzy `3764437`; only reflection `36afb3b` integrated | identical arms preserve distinct raw/cached source identities and fix every persistent saved-register assignment, but entry/early-return scheduling remains |
| `ProcItemNingyo` | best guarded checkpoint has 15 bytes remaining | isolated `c10c3ab`; not integrated | clearing a short-lived launch pointer after `memset` breaks stack-address CSE; destructive model-pointer reuse makes the full derived-position/modulus block exact |

The Ningyo branch is a materially better starting point than the 19-byte draft
on `master`: only the three pre-`memset` loads and three mode-1 constants are in
the wrong order. It is isolated solely because nonexact source is never merged,
not because the 19-byte form is preferred. The GetAreaMapVector branch likewise
has the exact loop and normal-return path; its 40-byte residual is confined to
setup scheduling and the `MIN` early return. Do not rerun their already-flat
broad searches: Ningyo needs a zero-code ordering dependency, while
GetAreaMapVector needs a scheduler/store-order lever. The latter already saw
guided budgets 80/160 and a bounded roughly 26k-candidate permuter run.

## Data-name recovery status

The apparent absence of `_data_` names was mostly a propagation problem, not a
lack of symbols. `PSX.SYM` data addresses belong to the earlier debug build;
they map rigidly to the demo executable by `+0x358`, but neither address set maps
directly to retail. `datamatch.py` now infers that relocation from 323/323 unique
anchors, reconstructs 201 labels omitted by the demo Ghidra export, preserves
aliases, checks translation-unit ownership for duplicate statics, and refuses
unsafe reverse-name conflicts.

Final data commits:

- `85fb921`: recover omitted demo labels and calibrate the relocation.
- `1dc1f53`: de-duplicate repeated globals generated from `PSX.SYM`.
- `2fd6b15`: adopt 71 calibrated original names.
- `1cfab59`: adopt `StageAppearance` and `AdtMessageBoxCount`; make global-note
  ownership extent-based so a preceding object cannot claim unrelated data.

`reference/data-symbols-applied.tsv` records 229 decisions: 35 earlier
`datamatch` names, 71 calibrated relocation names, 121 historical Ghidra names,
and two independently corroborated names. A fresh final run had a 172/172
control set (100%), zero remaining proposals with two or more witnesses, and 48
single-witness candidates. Leave those 48 unapplied unless independent type,
TU, semantic, or same-binary evidence corroborates one. Re-run `datamatch.py`
after every function-rename batch because new shared function names unlock new
data-reference witnesses. See [`psx-sym.md`](psx-sym.md) for the full protocol.

## Target selection

The user's standing preference is **Think\***, **Proc\***, and **Attack\***.
Honor that preference among *live* targets.  At this shutdown, most of the
remaining named functions in those families were already exact or explicitly
parked after extensive RTL/permuter work, so do not revive one solely because
its name has priority.

The earlier fresh slate is now exact. Default triage has almost exhausted
non-GTE game targets: the only straightforward fresh candidate is
`FUN_8005fe34`, while `check_for_known_button_combination` has an unmerged
near-match. A larger replacement pool can still be useful if each parked slot
gets a distinct, evidence-driven residual instead of another broad search:

| Candidate | Size | Starting evidence / permitted next attack |
|---|---:|---|
| `FUN_8005fe34` | 84 B | fresh 21-instruction leaf with one callee; first priority |
| `check_for_known_button_combination` | 288 B | isolated `fd303e1` is 12 bytes / three instructions away; solve the final register rotation, never merge that checkpoint |
| `ProcItemNingyo` | 2256 B | start from isolated `c10c3ab` (15 bytes); attack only the two six-instruction ordering clusters |
| `GetAreaMapVector` | 548 B | start from isolated `f2d0a6e` (40 bytes); persistent allocation and loop body are already exact |
| `FUN_80056910` | 300 B | parked with two extra instructions; seek the specific lifetime/CFG cause |
| `FileOption` | 1108 B | six-byte near-match with an instruction-count discrepancy; inspect the existing note before editing |
| `FUN_8005e8f0` | 88 B | eight bytes in two adjacent instructions; compact RTL escalation target |
| `PClseek` | 36 B | seven-byte support-code residual; small independent slot |
| `PutItemList` | 504 B | eight-byte / two-instruction-short checkpoint; needs structure, not permutation |
| `AdtSelect` | 776 B | nine-byte reload residual; only revisit from its documented huge-frame evidence |

This list deliberately fills up to eleven useful matching roles for a fresh
session, but the first four have the strongest current evidence. Re-run triage
before dispatch because the inventory may move. `DrawTMD` appears in default
triage but still lacks an acceptable pure-C indirect-call path; do not spend a
slot on it without a new compiler-level idea.

The useful old worktrees are checkpoint evidence, not active workers:

- `/tmp/tenchu-fw15-ningyo-0715` -> `c10c3ab` (15-byte Ningyo checkpoint);
- `/tmp/tenchu-fw18-get-area-map-vector-0715` -> `f2d0a6e` plus `3764437`
  (40-byte checkpoint; reflection already integrated);
- `/tmp/tenchu-fw2-known-buttons-0715` -> `fd303e1` (12-byte checkpoint).

Create fresh uniquely named worktrees from current `master` for new work. If an
old checkpoint is useful, compare it against the current guarded draft before
transplanting it. A genuinely better reviewed checkpoint may now be integrated
under rule 7; stale, equal, or worse experiments remain isolated.

Do not spend a slot on these merely because they appear difficult:

- GTE/COP2 routines and `DrawTMD` currently lack an acceptable pure-C path.
- Remaining unmatched jump-table stubs remain fiddly to harvest.
- Anything hidden by default `tools/triage.py` is parked; read its reason
  before considering it.

## Parked priority-family near-matches

These are the most tempting repeats.  Their source comments contain the full
evidence and rejected experiments.

| Function | Checkpoint | Why it stopped |
|---|---|---|
| `ProcItemNingyo` | master 19 bytes; isolated `c10c3ab` 15 bytes / 99.65% | pointer/modulus block is exact on the isolated branch; only three input loads and three mode-1 constants are reordered |
| `GetAreaMapVector` | isolated `f2d0a6e`, exact 548 B / 40 bytes left | setup load/move/store scheduling and `MIN` early-return ordering; guided 80/160 and ~26k permuter candidates flat |
| `AttackBowControl` | 75.00%; exact 272 B; 19 bytes left | first-live-local allocator priority swap; 80k+ permuter candidates flat |
| `StageEndScreen` | 91.26%; exact 6084 B / frame / branch-call inventory; 389 bytes left | complete pure-C reconstruction; broad remaining allocator/scheduling residual |
| `DamageControl` | 88.23%; exact 5812 B / 1453 instructions; 2457 bytes left | complete structural draft, but 140 aligned residual blocks remain |

Only revisit one of these when a *new, specific* compiler/tool insight attacks
the recorded residual class.  Do not rerun the same broad autorules or
permuter search.

## Matching invariants

1. Matching functions stay pure C.  Do not land arbitrary `__asm__` patches.
2. `matchdiff.py`/whole-image bytes and CFG are authoritative.  Fuzzy score is
   presentation metadata, not the acceptance test.
3. After every meaningful guarded-draft edit, immediately run
   `tools/fuzz-score.py --only <Name>`.  After exact promotion, run it again to
   remove the obsolete row.  Never leave changed C with a stale or absent
   fuzzy fingerprint.
4. Use target assembly, demo symbols, siblings, xrefs, `rtlguide.py`, and
   `rtldump.py` before speculative source churn.  `rtlguide.py` now recognizes
   the branch-phi register-tie signature seen in the final Think checkpoint.
5. Run `autorules.py --guided` only on a compiling draft.  Use the permuter
   late: exact or one-instruction-off length, localized allocation/scheduling
   residual, one bounded run.  Never use it as the initial decompiler and do
   not paste a globally nonsensical winning candidate. A better partial score
   is advisory, not semantic proof; in particular, do not bypass the automatic
   signed-use veto merely to improve alignment.
6. One worker owns one function or tightly coupled family in one isolated
   worktree.  Do not run matching tools concurrently against the same
   worktree/lock.
7. A worker may commit an honestly documented nonexact checkpoint. Root may
   merge it only when it is a reviewed monotonic improvement over `master`, the
   C remains behind the `NON_MATCHING` guard, the fuzzy row is refreshed, and
   both the draft comparison and default/global gates pass. Root still resolves
   additive metadata conflicts and never promotes or counts a nonexact draft.
8. Commit the function/checkpoint first.  Then perform the reflection pass and
   land any cookbook, diagnostic, autorule, test, or naming improvement in a
   separate commit.  Tooling must encode a repeatable decision, not one
   function's accidental spelling.
9. After a high-confidence function rename, re-run `datamatch.py`. Adopt a data
   name only when the tool's uniqueness controls and independent provenance
   rules in `psx-sym.md` are satisfied.

Recent mechanical lessons are already in
[`matching-cookbook.md`](matching-cookbook.md) and the tools/tests: narrow-copy
zero extension and round-trip fencing, subtraction-role fusion, in-place
branch-phi carriers, identical-arm source-identity splits, difference-role
fusion, shared-result returns, terminal call returns, default-ladder hoisting,
returning-guard fallthrough producers, and two-cursor inline byte-pack helpers.
Search the cookbook and `rtlguide.py` signatures before inventing a new manual
experiment.

## Agent and worktree hygiene

Many historical `/tmp/tenchu-*` worktrees and `codex/*` branches still exist.
They are not evidence of active work. The final visible agents
(`match_debug_camera`, `match_fun3d768`, and `match_memcard_callback`) all
completed and took no replacement targets. AttackShort and the small-rotation
handler were integrated under the master hashes above. GetAreaMapVector stayed
isolated; only its cookbook reflection was integrated. Root's Ningyo worktree
is clean at isolated commit `c10c3ab`. No matcher agent remains live.

Before touching an old worktree, inspect both its dirt and its branch delta:

```console
git worktree list
git -C /path/to/worktree status --short
git log --oneline master..branch-name
```

Do not blindly cherry-pick old worker hashes: integrated commits often have a
different hash after conflict resolution or recreation.  Prefer new uniquely
named worktrees/branches for a restarted batch, and do not delete old user
state as incidental cleanup.

## Starting the replacement Codex session

The resumed rollout that produced this note was provisioned with only four
total concurrency slots despite `~/.codex/config.toml` containing
`[agents] max_threads = 11`. A resumed rollout retains its original runtime
capacity; changing the config cannot enlarge it in place. This was a
concurrency ceiling, not an elapsed-time limit. There is no discovered
user-editable wall-clock limit for ordinary subagents;
`agents.job_max_runtime_seconds` applies only to CSV fan-out jobs. Start a
completely new session, not `codex resume`:

```console
codex -c agents.max_threads=11 -C /home/shana/programming/tenchu-decomp
```

Have the new root read this handoff and `docs/orchestration.md`, run the resume
preflight, then create a fresh parallel batch from current `master`. If a brand
new session still exposes four total slots, treat that as an external runtime
quota rather than another repository/configuration issue. Continue the user's
standing unattended flywheel: harvest each worker, refresh fuzzy metadata,
run global gates, reflect/tool, commit, and refill slots until asked to wind
down.

The detailed launch, harvest, conflict, reflection, and shutdown procedures
remain in [`orchestration.md`](orchestration.md).  This file is only the clean
launch pad for the next run.
