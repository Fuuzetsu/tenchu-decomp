# Decompilation flywheel handoff — 2026-07-15

This is a dated shutdown snapshot, not a second match-status database.  The
current source, `tools/progress.py`, `tools/triage.py`, and each guarded
function's `STATUS` comment are authoritative.  Re-measure before dispatching
work; do not carry the percentages or target list below forward by assumption.

The current rollout was intentionally wound down so the user can start a wholly
new Codex session with a larger agent pool. There are no matcher agents still
running, all three final worker results were harvested onto `master`, and the
root worktree was clean before this handoff update. Do not resume this old
rollout or launch more work from it.

## Clean anchor

- code/tool anchor immediately before this note: `dc91927` (`Guard signed
  autorule semantics`)
- newest exact function commit: `31aa9d1` (`Match AfsGetEntry`)
- game code: 408/555 functions (73.51%), 194680/302824 bytes (64.29%)
- all six executables passed `./Build check-all` byte-identically
- all 299 matching-tool tests passed
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

## Work completed in the final batch

Three final agents all reached exact, unconditional pure C and were harvested:

| Function | Retail extent | Master commit | Useful source-shape result |
|---|---:|---|---|
| `TurnAroundAllItems` | 408 B | `764aec7` | the inherited 84.31% fuzzy row was already byte-exact; always run authoritative `matchdiff` before editing a checkpoint |
| `death_camera_something_` | 520 B | `75ea59f` | put the fallthrough producer after a returning guard so reorg can use it as that guard's delay slot |
| `AfsGetEntry` | 564 B | `31aa9d1` | an inline endian helper may need both already-live cursor identities to select the original load bases |

The reflection is committed separately as `dc91927`. It adds both rules to the
cookbook and matcher prompt. It also fixes a real `autorules type-width` hazard:
a signed local explicitly compared with zero/negative or arithmetically
right-shifted is no longer offered as an unsigned candidate. An improving
partial score had otherwise removed the camera routine's negative correction
and changed `sra` to `srl`.

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

The previously suggested gameplay slate (`AntiWall`,
`death_camera_something_`, `create_ninken_character_`, `FUN_8004c350`,
`TurnAroundAllItems`, and `AfsGetEntry`) is now exact. On the 2026-07-15
snapshot, the best unparked slate is:

| Candidate | Size | Why it is useful |
|---|---:|---|
| `FUN_8003d768` | 672 B | largest fresh non-GTE game target; multiply/divide and three callees |
| `debug_output_edit_camera_settings` | 628 B | substantial camera/debug loop with a clean C path |
| `FUN_80058a54` | 540 B | medium-large loop with four callees |
| `SelectStage` | 420 B | two loops and a diagnostic 0x528-byte frame |
| `calculate_score` | 352 B | gameplay scoring, multiply/divide, useful call-graph leverage |
| `debug_menu_file_animation_test` | 260 B | four callees and a 0x328-byte frame |
| `AdtVsprintf` | 248 B | compact support-code loop, useful if a slot needs a lower-risk target |

Clean, untouched worktrees already exist for all seven under
`/tmp/tenchu-fw-*-0715`, but they still point at the old `3aa896a` anchor. Do not
dispatch them as-is. Either recreate them from current `master` with new unique
names or deliberately fast-forward/rebase only after confirming they remain
clean. Fresh worktrees are less error-prone.

Do not spend a slot on these merely because they appear difficult:

- GTE/COP2 routines and `DrawTMD` currently lack an acceptable pure-C path.
- `cd_open` and other unmatched jump-table stubs remain fiddly to harvest.
- Anything hidden by default `tools/triage.py` is parked; read its reason
  before considering it.

## Parked priority-family near-matches

These are the most tempting repeats.  Their source comments contain the full
evidence and rejected experiments.

| Function | Checkpoint | Why it stopped |
|---|---|---|
| `AttackShort` | 99.52%; exact 1668 B / 417 instructions / CFG; 8 bytes left | two `move v0,s0` vs `andi v0,s0,0xffff` sites; `narrow-copy-zero-extension`; 22,046 permuter iterations flat |
| `ProcItemNingyo` | 98.40%; exact 2256 B / 564 instructions / CFG; 19 bytes left | launch-position identity, modulus register, and constant scheduling; guided rules and late permuter flat |
| `ProcMiscPitfall` | exact 868 B / 217 instructions; 6 bytes left | global register-colouring tie; 160 guided candidates and 22,097 permuter candidates flat |
| `ProcMiscFire` | exact 356 B; 10 bytes left | confirmed `la` reload tie; RTL excludes the known compiler patch; 27,390 permuter candidates flat |
| `Think1ninja` | exact 356 B; 6 bytes left | fixing local abs shape triggers a worse global-allocation cascade; bounded permuter flat |
| `think_setting_small_rotation_small_steps_` | 98.84%; exact 1392 B / CFG; 5 bytes left | QImode branch-phi register tie; 160 guided candidates and 8,850 permuter candidates flat |
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
7. A worker commits either an exact result or an honestly documented improved
   checkpoint to its branch.  Root reviews and cherry-picks it, resolves only
   additive metadata conflicts, refreshes fuzzy data, and runs the gates.
8. Commit the function/checkpoint first.  Then perform the reflection pass and
   land any cookbook, diagnostic, autorule, test, or naming improvement in a
   separate commit.  Tooling must encode a repeatable decision, not one
   function's accidental spelling.
9. After a high-confidence function rename, re-run `datamatch.py`. Adopt a data
   name only when the tool's uniqueness controls and independent provenance
   rules in `psx-sym.md` are satisfied.

Recent mechanical lessons are already in
[`matching-cookbook.md`](matching-cookbook.md) and the tools/tests: narrow-copy
zero extension, subtraction-role fusion, branch-phi register ties,
difference-role fusion, shared-result returns, identical-arm fences, terminal
call returns, default-ladder hoisting, returning-guard fallthrough producers,
and two-cursor inline byte-pack helpers. Search the cookbook and `rtlguide.py`
signatures before inventing a new manual experiment.

## Agent and worktree hygiene

Many historical `/tmp/tenchu-*` worktrees and `codex/*` branches still exist.
They are not evidence of active work. The final three visible agents
(`turn_items_resume`, `death_camera_resume`, and `afs_entry_resume`) all
completed, and their exact changes were integrated on `master`. Their worker
commits are `33c99c8`, `b94a60f`, and `2a4c6a3`; use the master commits listed
above instead. No matcher agent remains live.

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
capacity; changing the config cannot enlarge it in place. Start a completely
new session, not `codex resume`:

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
