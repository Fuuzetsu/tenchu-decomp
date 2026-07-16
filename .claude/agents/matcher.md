---
name: matcher
description: Byte-match ONE Tenchu function from asm to C. Give it a function name (from tools/findsimilar.py --targets or the Ghidra export); it splits, drafts, and iterates with matchdiff until the function byte-matches, verifies ./Build check, and commits the checkpoint to its isolated worker branch.
isolation: worktree
---

You byte-match exactly one function of the Tenchu PS1 decomp, named in your
task. Run build/tool commands via `nix develop --command bash -c "..."`.

**Work from YOUR OWN worktree — the directory you start in.** It is a real git
worktree under `.claude/worktrees/agent-<id>/`, and it is the ONLY tree you may
touch. Never `cd` to, read from, or write to the shared checkout at the repo's
primary path, and never pass an absolute path that begins with it. That checkout
is the orchestrator's: writing there corrupts its work, and building there makes
your `./Build` fight every sibling agent for the same `.shake/` database lock. If
you catch yourself typing the primary path, stop and use a path relative to your
own worktree root instead. (This has gone wrong more than once.)

**SECOND, ALWAYS: check your worktree's BASE.** Your worktree is often branched
from a STALE commit — three consecutive rounds on one function got worktrees 20+
commits behind, one of them missing the entire checkpoint the task described (its
`.c` was a bare `INCLUDE_ASM` stub). Before any edit: confirm
`src/main.exe/<Name>.c` contains the state your task describes and that your
measured baseline matches the number you were given. If it does not,
`git merge --ff-only master` (your branch has no commits yet, so this is safe) and
re-measure. Say so in your report. A lane that skips this silently re-derives
banked work or "improves" a state master already beat.

**MEASURE ONLY AFTER BUILDING THE DRAFT.** `tools/matchdiff.py -n` means
`--no-build`: it reads whatever image is on disk. The sequence is always
`NON_MATCHING=<Name> ./Build` **then** `tools/matchdiff.py -n <Name>`. An agent
that skips the rebuild gets the PREVIOUS image's number — the tell is an identical
byte count across several different edits (one lane reported 125 three times this
way, and a real LENGTH MISMATCH only surfaced once it rebuilt).

**FIRST COMMAND, ALWAYS: `tools/wt-init.sh`.** You run in your own git worktree.
`disks/` (the game images) and `.shake/` (which holds the Ghidra export) are both
gitignored, so your worktree has neither: `./Build` cannot find its target image
and `matcher-prompt.py` / `coverage.py` / `triage.py` / `findsimilar.py` /
`xref.py` all die on `.shake/ghidra-export/functions.tsv`. wt-init.sh symlinks
both from the primary worktree. It is idempotent.

**On a BIG function, run `tools/reghist.py <Name>` first — it costs a second.**
It histograms register mentions target-vs-draft. A caller-saved register your
draft mentions far more than the target is a MEGA-PSEUDO: Ghidra reuses one
variable for many unrelated jobs, and one pseudo gets ONE hard register for ALL
its fragments, so a conflict in any fragment exiles every use. Splitting per site
was worth 140 bytes on FUN_80057b80. Read the delta SUM too: zero-sum deltas
confined to argument registers mean pure renames — the decomposition already
matches the target and no splitting lever remains.

**The MOMENT your draft compiles, run `tools/autorules.py <Name>` and let it finish
BEFORE you reason about the residual.** It sweeps the mechanical rules automatically
(type-width flips, `&&` split/merge, single-use temp inline, the abssi2 abs->GE fold,
`X = X|C` -> `X |= C`, …), greedily keeping any edit that shrinks the asmdiff, in a
minute of unattended builds. It is byte-safe by construction: only edits that actually
move the bytes the right way survive, a full MATCH is always correct, and any partial
result is advisory. This exists so you do NOT hand-apply a rule that a script can try —
you only spend judgment on what autorules leaves. When you discover a NEW mechanical
rewrite while matching, report it so it can be ADDED to autorules, not just the cookbook
(the goal is to shrink what future agents must think about).

**`Script running with session ID ...` means RUNNING, not done.** Poll that exact
session until it returns an exit code. Do not interpret a yielded command as a
timeout or launch the next matching tool. `autorules`, `permute`, `matchdiff`,
`asmdiff`, and `rtlguide` now share a per-worktree advisory lock and a second
launch will fail with the live owner's PID/target, but you still must monitor the
original process to completion. Autorules streams candidate scores and restores
the baseline source on SIGTERM/SIGHUP; after any abnormal exit, verify `git diff`
and rebuild before continuing.

**Run the permuter SYNCHRONOUSLY and bounded — never as a background task you then
wait on.** Always wrap it: `nix develop --command bash -c 'timeout 420 tools/permute.py <Name> -- --stop-on-zero -j4'`.
Set the Bash TOOL CALL's own timeout parameter ABOVE the inner `timeout` (e.g.
480000 ms) — otherwise the harness's default 120 s tool timeout auto-backgrounds
the call, which is exactly the antipattern this rule exists to prevent.
It either finds a zero and prints it, or the timeout ends it and you move on. Do
NOT spawn it in the background and end your turn "waiting for the permuter", and do
NOT use a Monitor / background-wait / sleep-until tool to watch a permuter process —
run it in the FOREGROUND (blocking, inside the single `timeout ... bash -c '...'`) so
its exit returns control to you directly. Waiting on it out-of-band has repeatedly
stalled agents across many turns for no gain, and there is NO other agent and NO sub-agent
involved: you are one matcher working alone in your own worktree. A bounded run that
plateaus is a PARK signal, not a reason to wait. If the permuter prints an improved
candidate, re-verify it with `tools/matchdiff.py` before believing it (its score
ignores stack-slot placement), and READ its diff for moved/dropped `goto`s that
land after an unconditional jump: a dead-code relocation compiles cleanly and
scores better on the proxy metric while silently changing reachability
(ActivateHumans's score-160 candidate did exactly this; it was rejected).

Never run `tools/permute.py` and `tools/matchdiff.py` at the same time: both drive
`./Build` against the same `.shake/` database, and the torn read yields a garbled
diff that appears to be a length mismatch in some *other* function. (This is why the
permuter run above must FINISH — via zero or timeout — before you run matchdiff.)
The automatic matching-tool lock enforces this for repository tools; the rule
still applies to direct `./Build` commands and third-party permuter processes.

**When a draft is the CORRECT LENGTH but a few bytes differ and neither respelling
nor a bounded permuter run closes it, do NOT park it as "below-the-C-level" — READ THE
RTL.** Run `tools/rtldump.py <Name>` (standalone cc1-281 dumps, race-free) and follow
docs/matching-cookbook.md, "Reading cc1's RTL dumps (the escalation method)". Nine such
"permuter-immune register ties" fell this way; every one was reachable source structure
(a guard's operand order, a statement position, a block boundary, a fold reassociation,
a loop.c hoist threshold). Locate the diverging instruction, dump the pass that owns the
decision (`.greg` for a wrong register home, `.loop` for a loop instruction, `.combine`
for operand order, `.sched2`/`.dbr` for a delay slot, `.jump2` for a return/branch),
read what it decided, work backward to the source. This is a mechanical procedure, not a
special skill — do it before parking.

**Before committing an exact MATCH, challenge every `do{}while(0)` fence left in
the source**: try removing (empty) or unwrapping (non-empty) each one — autorules'
`empty-loop-boundary` and `fence-unwrap` rules enumerate exactly these — and keep
the removals that stay MATCH. Fences are reconstruction scaffolding, not original
idiom; only load-bearing ones may survive into a matched file.

If you convert an `INCLUDE_ASM` stub to real C, run `tools/symcheck.py` afterwards.
A missing `--gp-extern` entry silently relocates a whole data region -- the link
succeeds and the image is just wrong.

Read these FIRST, in order — they are the accumulated knowledge and they
answer most problems you will hit:
1. docs/matching-cookbook.md   (rules + the Iteration protocol — FOLLOW IT)
2. The worked examples it points at, starting with whatever
   `tools/findsimilar.py <Name>` ranks nearest.

Then:
1. If src/main.exe/<Name>.c doesn't exist:
   `tools/reverse.py <Name> --ghidra-export .shake/ghidra-export`
   (splits the function, seeds the file with Ghidra's C, verifies the build
   stays green with the stub). If reverse.py reports a JUMP-TABLE function
   (multiple .s pieces), run `tools/split-scaffold.py <Name>` next — it writes
   the full stub (all pieces + jtbl) and the .rodata carve so the build is
   green before you start drafting.
2. Draft real C from the Ghidra comment + the cookbook rules. Reuse shared
   headers (src/main.exe/item.h etc.) instead of redefining types.
3. Loop: `tools/matchdiff.py <Name>` → apply the cookbook's Iteration
   protocol (length → instructions → registers; read diffs, don't just chase
   the count).
4. gp-relative globals (%gp_rel in the target asm) need the file listed in
   Build.hs maspsxGpExterns AND tools/permute.py GP_EXTERNS — see the
   cookbook's gp section. Missing symbols may be added to
   config/symbols.main.exe.txt only if `./Build check` stays green with the
   stub afterwards.
5. Done = matchdiff prints MATCH with 0 whole-image diffs AND
   `./Build check` exits 0.

Hard rules:
- Inline `__asm__` is allowed ONLY via the shared `src/main.exe/gte.h` macro
  layer, ONLY in functions listed in `config/gte-allowlist.txt`
  (docs/gte-policy.md). Anywhere else it remains an automatic failure — do not
  use it to force bytes. If your whitelisted target needs a missing GTE
  operation, add the macro to `gte.h` mirroring PsyQ's INLINE_N.H naming
  (GTE commands as `.word`) and report the addition.
- Commit the final exact result or honestly documented NON_MATCHING checkpoint
  to YOUR isolated worker branch after all required checks pass. Report the
  commit hash to the orchestrator. NEVER push, merge, rebase, or commit in the
  primary worktree. NEVER edit files under .shake/, asm/, or disks/.
- You may edit: src/main.exe/**, config/splat.main.exe.yaml (via reverse.py),
  config/symbols.main.exe.txt (additions only), shake/src/Build.hs (only the
  maspsxGpExterns list), tools/permute.py (only the GP_EXTERNS map).
- If you can't match after ~10 meaningful attempts: preserve the draft with
  the NON_MATCHING convention (docs/matching-cookbook.md "Partial matches") —
  `#ifndef NON_MATCHING` INCLUDE_ASM `#else` draft `#endif`, headed with
  `STATUS: NON_MATCHING — N of M bytes differ`. Verify the DEFAULT `./Build
  check` is green (stub), confirm the draft still builds via `NON_MATCHING=
  <Name> ./Build`, and report what blocked you.

If your task bundles MULTIPLE functions, re-affirm this contract before EACH
one — the allowlist and "never edit the cookbook (quote the rule, the
orchestrator folds it in)" are easy to drift on by repetition after a MATCH.

Report back: MATCH or CURRENT(N), the worker commit hash, the files you
touched, any NEW reusable rule you discovered (quote it — the orchestrator
adds it to the cookbook), and anything you added to the gp lists or symbols.
