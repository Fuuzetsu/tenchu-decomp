# Matching cookbook — cc1 2.8.1 source idioms that byte-match

What we've learned, function by function, about how the original source must
be *shaped* for our exact compiler (the nix-pinned canonical decompals
`gcc-2.8.1-psx` cc1, verified equivalent to the real Sony `CC1PSX.EXE` — see
[toolchain.md](toolchain.md)) to reproduce the original bytes. Every rule
below was verified against the original binary; the jump.c/fold-const.c ones
were confirmed by reading the actual GCC 2.8.1 sources
(`mirrors.kernel.org/gnu/gcc/gcc-2.8.1.tar.gz`).

Worked examples, in increasing difficulty:
[`Think1sleep.c`](../src/main.exe/Think1sleep.c),
[`GetRealPad.c`](../src/main.exe/GetRealPad.c),
[`ProcItemManebue.c`](../src/main.exe/ProcItemManebue.c),
[`ProcItemKusuri.c`](../src/main.exe/ProcItemKusuri.c) (1432 bytes — the most
idioms in one place; read its header comment).

## The workflow

```console
$ tools/reverse.py <Name> --ghidra-export .shake/ghidra-export
      # splits the function, seeds src/main.exe/<Name>.c with Ghidra's C,
      # verifies ./Build check stays green (INCLUDE_ASM stub)
      # the seed carries TWO references: Ghidra's typed-but-normalized C and
      # (appended after the build) m2c's `mipsel-gcc-c` output — m2c has the
      # cleaner control flow / register temps, Ghidra the real types/names.
$ # write a first draft from BOTH references + this cookbook, then loop:
$ tools/matchdiff.py <Name>
      # ./Build + byte-compare the function window vs the original;
      # prints differing instructions side by side. Iterate until MATCH,
$ ./Build check      # then confirm the whole image
```

## Iteration protocol

The ordered triage — fix categories in THIS order, re-running
`tools/matchdiff.py <Name>` after every source change:

1. **Length first.** If your function assembles to a different instruction
   count (matchdiff's whole-image number explodes), the *structure* is wrong —
   dispatch shape, loop shape, missing/extra temps, tail merging. Fix before
   anything else; operand diffs are meaningless while the length is off.
2. **Wrong/missing/reordered instructions next.** Map each to a cookbook rule
   (switch vs ladder, while(1)+break, fold reassociation, temps-vs-inline,
   buffer casts, statement order). `tools/findsimilar.py <Name>` names the
   matched functions most like yours — read their source and header notes
   first; imitate the nearest one.
3. **Register-only differences last.** They usually fall out of 1–2. The
   levers, in order: statement/assignment position (a one-line move can flip
   allocation priorities and delay-slot fills), declaration of temps, and
   finally `tools/permute.py <Name>` for pure allocation ties. **A permuter
   winner can carry red herrings alongside the real fix — bisect a multi-diff
   score-0 candidate statement-by-statement and keep only the load-bearing
   change** (bow_shoot_logic's winner touched three things; only the extern
   return-type retype mattered), don't paste it wholesale.
   - **Machine-apply the mechanical rules first.** The moment the draft
     compiles, run `tools/autorules.py <Name>`: it sweeps the *local* rules —
     every local's integer type (the s16↔s32 index/call-result width lever
     below, sign toggles), `&&`-chain split/merge (the redundant-chain-collapse
     rule under Dispatch), and single-use temp inline (the calls-inline-in-
     expressions rule below) — and greedily keeps what shrinks the asmdiff,
     telling you which edit helped. Don't hand-apply these — and if it reports
     no win, the residual is *not* one of them, so move to structure/regalloc
     instead of trying more variants. **Reject an autorules "win" that changes a
     PROVEN struct field's ACCESS WIDTH** even when it shrinks the byte count: a
     `u16`→`u8` field retype can narrow a correct `lhu` to `lbu` at another,
     already-matched site — autorules scores *total* diff, not per-site
     correctness, so it reports the net shrink as a win (Think3chase). (Structural rules that need diff-reading
     to place — loop shape, switch-vs-ladder, union-offset casts — stay manual.)
4. **A whole-image `./Build check` failure while a function is mid-match is
   EXPECTED, not a regression** — a function of the wrong length shifts every
   later object (even already-matched siblings show huge matchdiff diffs).
   Finish the in-progress function (its own `tools/matchdiff.py <Name>` window
   isolates it); downstream resolves once it reaches the right length. Don't
   bisect.
4. Don't trust the byte count alone — read the diff. A change can improve the
   count while shifting registers globally (worse), or vice versa.
5. **Attempt cap.** If ~10 meaningful source changes haven't reduced the diff,
   stop and preserve the best draft with the **NON_MATCHING convention** (see
   below): the default build keeps the byte-identical stub while the draft
   stays first-class and buildable. Mark the residual (`STATUS: NON_MATCHING —
   N of M bytes differ`) and record what blocked you. decomp.me (psyq4.3
   preset) arbitrates "is this expressible at all".
   - **Sub-C-level residual early-stop (SAVES THE MOST TOKENS).** A residual of
     **≤ ~10 bytes** that is a pure **register swap** or **adjacent-instruction
     reorder** (same instructions, same registers, only order/reg-name differs)
     is almost always below the C level — a `reload`/register-allocator
     rotation or a `sched` tie. These are *permuter-immune*: FileOption burned
     a 450k-iteration run and AdtSelect an 80k+162k run, both flat, both still
     stuck. Budget the permuter to **one bounded run (~5–10 min)**; if such a
     residual survives it, STOP — root-cause it against the gcc-2.8.1 sources
     (sched.c / reload.c / global.c), write the mechanism into the header, mark
     NON_MATCHING, and move on. Do NOT open more surgical sessions on it; that
     is how ~1.4M tokens produced zero additional matches this batch.
     - **The `la`/address-materialization reload tie is a NAMED member of this
       class — DON'T even start the permuter on it.** When the only diff is a
       symbol address built `lui $tmp,%hi / addiu $dst,$tmp,%lo` where the
       target puts both halves in one register (`lui $dst,%hi / addiu
       $dst,$dst,%lo`), or vice versa, that's a reload-pass register choice with
       no source lever. PrepareAccess (2 bytes, 87k permuter iters, flat) and
       cd_open (4 bytes, ~50 min / **353k agent tokens**, flat) are the identical
       tie. Recognize the signature, document it, mark NON_MATCHING immediately —
       a permuter run on this pattern has never once paid off.
6. **On MATCH:** `./Build check`, add the matching-notes header, promote any
   NEW reusable rule to this cookbook, commit the function + splat.yaml
   together.

## Picking targets

`tools/triage.py` rates every unmatched function EASY..VERY-HARD with a
"why" (size, jump table, float/GTE, loops, gp, frame, near-clone twin) and the
cookbook sections it needs — `--leverage` sorts by call in-degree (high-impact
first), `<Name>` details one. `tools/findsimilar.py --targets` ranks every
unmatched function by assembly
similarity to the matched corpus (best-first, smallest-first) — the top
entries are near-clones of things we've already solved and make good next
targets. Prefer functions from an original TU we've already touched (shared
headers like item.h and the gp-extern lists already exist for those). Batch
work: one function per matcher agent (.claude/agents/matcher.md), and commit
only on a green `./Build check`. **Don't hand-write the launch prompt** — run
`tools/matcher-prompt.py <Name>` to generate it (it also ROUTES: an exact
byte-identical twin -> tools/clonematch.py, a findsimilar-1.00 twin ->
near-clone recipe done inline in ~2 min, else a full agent prompt) (auto-fills address/size,
jump-table detection, the nearest matched worked examples, TU family, and the
current cross-cutting guidance). Update the GUIDANCE block in that script (and
this cookbook) as lessons accrue, so every future launch inherits them.

Two helpers for the setup phase:
- `tools/gpsyms.py <Name> [--write]` derives the gp-extern list from the
  function's `%gp_rel(...)` operands and (with --write) syncs it into both
  Build.hs maspsxGpExterns and permute.py GP_EXTERNS.
- `tools/xref.py <Name>` lists callers (their call sites pin the prototype)
  and callees (matched vs needs-extern), resolved against the original image.

**Batch efficiency (measured over the debug-menu batch):**
- **Route by model.** Sonnet matched every sub-500-byte non-jump-table
  function for ~65–75k tokens each (4/4); it cost 2–5× more on jump-table or
  regalloc-heavy ones. Give Sonnet the small/simple tier, Fable the
  jump-table / big / heavy-regalloc tier.
- **Bundle a near-clone CLUSTER into ONE agent, not one-per-function.** Five
  mutually near-clone item functions (each a 1-line `it->proc` diff) matched
  in a SINGLE session for ~141k total (~28k/fn) vs ~550–750k for five separate
  agents: the setup (contract/cookbook/worked-examples/item.h) is paid once and
  the anchor's investigation IS the clones' (copy, rename, carve, build). It
  also sidesteps the merge conflicts five agents editing Build.hs/permute/yaml
  concurrently would hit. Caveat: the amortization is mostly the shared
  investigation — 5 UNRELATED functions in one session only saves the fixed
  setup. Watch for contract drift on repetition (see matcher.md).
- **Jump-table functions cost 2–3×** (split pieces + `.rodata` carve + jtbl
  array). Detect them up front (`.shake/gen/main.exe/asm/nonmatchings/<Name>/`
  has multiple `.s` pieces) and tell the agent, so it doesn't rediscover the
  split-function protocol.
- **Fix a tool bug once, centrally, the moment the first agent hits it** —
  don't let N parallel agents each re-diagnose it (the `reverse.py`
  carve-drop cost ~every debug-menu agent real tokens before it was fixed;
  agents branched from before the fix and each repaired it by hand).
- **Respect the sub-C-level early-stop** (Iteration protocol §5) — the single
  biggest token sink was deep-diving permuter-immune residuals.

### coddog (similarity at scale)

[coddog](https://github.com/ethteck/coddog) complements findsimilar.
`tools/coddog` builds it on first use (needs network + registry nixpkgs — the
flake's rust is too old); it reads `decomp.yaml`, whose input ELF is
synthesized by `tools/coddog-elf.py` from the original bytes + the Ghidra
function table (our real ELF/map aren't consumable: NOTYPE size-0 symbols, no
vrom). **Regenerate after the Ghidra export changes or after new matches**
(it also refreshes the stems dir that makes the "(decompiled)" tag truthful):

```console
$ tools/coddog-elf.py
$ tools/coddog cluster -m 4        # identical-function families: match one,
                                   #   apply to the whole cluster (e.g.
                                   #   dmyGsPrstF3NL ×108, ReqItemManebue ×5)
$ tools/coddog submatch <Name> 20  # who shares a ≥20-insn chunk with <Name> —
                                   #   the "stuck on this idiom" tool (found a
                                   #   79-insn run shared by ProcItemKusuri
                                   #   and ProcSightShot)
$ tools/coddog match <Name> -t 0.5 # similar whole functions; good for BIG
                                   #   functions, noisy under ~300 bytes — use
                                   #   findsimilar for small ones
```

For big or length-shifted functions use `tools/asmdiff.py <Name>` — it
takes the true size from the Ghidra export (matchdiff's window is capped by
mid-function symbols like switch tables) and difflib-aligns the instruction
sequences, so an insert shows as one insert instead of shifting every line
after it; `--structural` shows only the blocks that change the length (fix
those first), and pure branch-target drift is suppressed by default.

Treat the differing-byte count as a score to drive down (matchdiff also
reports the whole-image count: if your function assembles to a different
LENGTH, every object after it shifts and even data symbols drift — fix the
instruction count before chasing operand diffs); each structured diff
(wrong register class, wrong block order, wrong instruction) maps to one of
the rules below. When a residual diff resists source-shaping, decomp.me with
the `psyq4.3` preset arbitrates "is this expressible at all" (it runs the real
Sony compiler; our cc1 is equivalent to it — if it can't, restructure; if it
can, keep hunting). `tools/permute.py` (decomp-permuter) cracks pure
register-allocation ties.

**Trust the assembly over Ghidra's statement order.** Ghidra's EARLY temp
assignment is often an SSA artifact, not source position: `iVar1 = GLOBAL;`
rendered before a whole dispatch, used in ONE branch, usually just marks
where the tracked load sat — check whether the asm's load really spans that
far back before hoisting (PlayerOption). Ghidra's decompiler
reorders memory operations and normalizes conditions freely; the asm's store
order and branch polarity are the source's. Ghidra's SCALAR types are
gold — match its return/variable types exactly (`Think1sleep` needed
`short`/`ushort` accumulators, not `s32`). But its inferred STRUCT NAMES can
be wrong: for ReqItemJirai it invented `PARAM_ITEM_DROP` with fields shifted
4 bytes (its `p->start.vx`@4 was really `p->user`). For struct layout, trust
the **m2c reference's raw offsets** (`arg0->unk4`, seeded next to Ghidra's C)
plus the **proven shared header** (item.h's `PARAM_ITEM_USE`) over Ghidra's
struct name — item-TU Req*/Proc* functions almost always take
`PARAM_ITEM_USE *p`.

- **A bare `enum`-typed struct field is 4 bytes under this pinned cc1** (no
  `-fshort-enums`), even when every enumerator fits in one or two bytes — so a
  field spelled with the raw enum typedef silently compiles 4 bytes wide and
  shifts every later field (no warning; `-w` is on). Match the existing
  precedent: spell it `u16 name; // enum X, stored as 2 bytes` the way
  `character_kind`/`character_status` already do, never `enum X name;`
  (character_state's `weapon_kind`/`active_item`).
- **An existing Ghidra struct-field POINTER type may be an unverified guess** —
  nothing ever dereferenced it, so Ghidra typed the target from a stale/adjacent
  inference. Before trusting one, `grep` the field name: zero other usages ⇒
  it's a guess, re-derive from the bytes. Cross-check the THREE independent type
  sources — the m2c seed's raw offsets, the proven shared headers (item.h), and
  Ghidra's own fuller independently-built structs in `reference/ghidra_types.h`
  (its `Humanoid` had `think[4]`@0x60, `weapon[4]`@0x94, `illusion[2]`@0xa4 all
  correctly placed) — and match the way they agree (character_state's
  `camera_related`@0x58 was really `model`, an `ModelArchiveType*`).

## Partial matches (the NON_MATCHING convention)

When a function is close but has a residual that resists source-shaping
(usually a scheduler or register-allocator tie below the C level — see
FileOption / AdtSelect), preserve the draft as a compile-able XOR instead of a
dead `#if 0`:

```c
#ifndef NON_MATCHING
INCLUDE_ASM(...);              /* + any jtbl array for split functions */
#else
... the best-attempt draft ...
#endif
```

Default `./Build` keeps the stub → green byte-identical image; the tools still
count the file **unmatched** (the `INCLUDE_ASM` text is present). Build the
draft with `NON_MATCHING=<Name> ./Build`; `tools/matchdiff.py`/`asmdiff.py` set
that automatically (they'd otherwise compare the trivially-matching stub), and
`tools/permute.py` preprocesses with `-DNON_MATCHING`. Head the file with
`STATUS: NON_MATCHING — N of M bytes differ` and keep the root-cause in the
header. On a later full match, delete the guards (and any jtbl array) — the
plain C is the matched file.

## Dispatch

- **Put the case whose body precedes the epilogue LAST in source**: with no
  default, the textually last case needs no trailing jump if it falls into
  the join point — a real length lever (PlayerOption's order 0,1,3,2
  recovered a wasted j/nop pair).
- **Case-body memory order reveals the source case order** (the compare tree
  always sorts by value): LayoutEnemyOption's inner switch was written
  `case 1, case 2, case 0` — only the bodies show it.
  This diverges from the TEST order: expand_case sorts the compare TREE by
  value but lays bodies in SOURCE order, and a case ending in `return` is
  pushed later in memory (reached by a forward jump). Makibishi's nested
  switch needed `case 4:` before `case 1:` in source though the tests check 1
  first — check body ADDRESSES in the `.s`, not just the test order.
- A mode-dispatcher that **reloads its variable** (two `lbu` of the same
  field) and compares **signed** (`slti`) is a real **`switch`**: cc1's
  `expand_case` emits a balanced compare tree over a *fresh* index load. An
  if/goto-ladder CSEs the load into one `lbu` and compares small unsigned
  types with `sltiu`. Case bodies are laid out in source order.
- **An explicit `if (cond) goto L;` ladder decouples test ORDER from body
  LAYOUT** — reach for it when an N-way dispatch fires its tests in a different
  order than the bodies sit in memory. cd_seek's 3-way `whence` dispatch tests
  1,0,2 but lays bodies out 0,2,1 (SEEK_CUR last, falling into the merge): a
  `switch` adds a range-split `slti` and an `if/else if` inlines each body right
  after its own test, but `if (whence==1) goto a; if (whence==0) goto b; …` with
  the *labels* ordered 0,2,1 reproduces the test sequence and the body addresses
  independently.
- A preceding test that keeps its constant in a **callee-saved register**
  across calls (`li $s4, 0xFF` + later `sb $s4`) is a local variable
  (`u8 ff = 0xff;`) also used by a later store. A path where the same value is
  materialized fresh (`li $v1, 0xFF`) uses a literal there — the register got
  reused for something else in between.
- **De-Morgan layout lever — `||` vs `&&` place the bodies differently.** cc1
  always emits the THEN body physically first, so `if (a != 0 || b != 0) {BIG}
  else {SMALL}` and `if (a == 0 && b == 0) {SMALL} else {BIG}` (logically
  identical) compile to *different* code. Read the polarity off the asm: `||`
  gives `bnez a, BIG; beqz b, SMALL` (BIG falls through / is first); `&&` gives
  `bnez a, BIG; bnez b, BIG` (SMALL is first). When the small "stop" body sits
  right after the condition reached by a `beqz` on the second test, write the
  `||` form (MoveHumanoid's zero-speed guard).

- **cc1 collapses a logically-redundant `&&` chain, dropping a branch the
  binary has**: `if (x != 0 && x == 1 && other)` compiles to `if (x == 1 &&
  other)` — the provably-redundant `!= 0` test's `beqz` is gone. Ghidra shows
  the redundant form (logical, not physical, structure), so trusting it
  under-produces instructions. NEST instead of chain — `if (x != 0) { if
  (x == 1 && other) {…} }` — to keep both branches (ProcItemLightningBolt).
- Branch-if-equal *into* a physically-later block with the fallthrough being
  the other body usually means the source condition was the **opposite
  polarity** of Ghidra's rendering (`if (0xe < n) {...} else {...}` vs
  Ghidra's `if (n < 0xf)` with swapped bodies).
  - **EXCEPTION — a null-guard clause with two returns keeps Ghidra's LITERAL
    polarity.** When both arms terminate in `return` with no shared merge
    (`if (handle == 0) { error; return E; } success; return S;`), the guard body
    is the branch TARGET that falls straight into the epilogue and the success
    body is the fallthrough-plus-jump — so writing the null-check first in
    Ghidra's own `== 0` sense is what matches. Inverting to `if (handle != 0)
    {success} else {error}` transposes both (correct) blocks for a pure ~29-byte
    layout diff. Very common in the C-layer wrappers (cd_close/Afs* family); the
    opposite-polarity rule above is for a single if/else over a comparison, not
    this guard-clause-with-two-returns shape.
    - **…but flip back when the "rest" arm carries the loop's own back-edge.**
      `if (match) return X; … keep-searching goto-loop …; return FALLBACK;`
      relocates the `return X` to the function's far end — instead invert the
      condition and NEST the keep-searching + fallback inside the `if`, leaving
      the found-return as the final unnested statement (GetItemType).
    - **…and flip back when the "success" arm's last statement is a tail-called
      return.** `if (idx == -1) { return 0; } …; return f(0);` in Ghidra's
      literal polarity runs 4 bytes long (inverted branch + a skip-jump around
      the success block); write success-with-tail-call FIRST
      (`if (idx != -1) { …; return f(0); } return 0;`) so the call's result
      (already in `$v0`) is the last code before the epilogue with no extra move
      (leRemoveEnemy).
- **Don't reuse the switch variable inside case bodies across calls**: a
  dispatch-only variable dies at the case tree and lives in a caller-saved
  reg (`move $v1,$v0` after the call that produced it). Assigning a later
  call result to the *same* variable inside a case body extends one pseudo's
  life across calls → it's promoted to a callee-saved reg, adding a prologue
  save and shifting the whole function by one instruction. Use a separate
  case-local variable (DoInfoViewProc: `sel` dispatches, the helpers' `n`
  takes the sub-menu results in $s0).
- **A tests-then-bodies ladder for two values is a nested `switch`**:
  `beqz $sN → bodyA; li $v0,1; beq $sN,$v0 → bodyB; j end` with both bodies
  laid out after the tests is `switch (n) { case 0: ...; case 1: ...; }`.
  An `if (n==0) ... else if (n==1)` chain puts the first body on the
  fallthrough path instead — and because that keeps the second body in the
  same extended basic block as the `li 1` compare, cse reuses $v0 for a later
  constant-1 argument (`move $a2,$v0`); the switch's branch-taken bodies get
  a fresh `li $a2,1` like the original (cse tables don't follow the taken
  edge).

## Loops

- `for (init; cond; inc)` and `while (cond)` expand with a jump to a
  bottom test; jump.c's `duplicate_loop_exit_test` (fires only when
  `NOTE_INSN_LOOP_BEG` is immediately followed by a simplejump) then copies
  the exit test to the entry, where a provable initial value constant-folds it
  away → **bottom-test do-while shape**.
- **`while (1) { if (!(cond)) break; ...; i++; }`** keeps the original's
  **top-test + unconditional back-jump** shape (first insn after the loop
  note is a condjump → no duplication) *and* still gets loop.c's invariant
  hoisting (address pseudos, division magic constants moved to the preheader).
- A hand-rolled `label: if (...) goto...` loop also keeps the top test but
  **loses hoisting** (no loop notes → loop.c skips it): magic divisors and
  invariant addresses get rematerialized per iteration. Wrong.
- **Index the table (`T[i].f`) rather than walking a pointer (`e++`) when the
  loop touches two or more fields.** With a walking pointer cc1 strength-reduces
  the induction variable so that the LAST field it touches sits at offset 0 —
  the base comes out biased (`addiu $a2,$v0,8`, accesses at `-4($a2)`/`0($a2)`).
  Array indexing keeps the giv at the table base, so the accesses stay at their
  natural `4($a2)`/`8($a2)` (FUN_800568b8; the `addiu $a2,$a2,0xC` bottom
  increment is identical either way, so the *bias* is the only tell).
- **Keep a compared memory read INLINE in both `&&` operands (cc1 CSEs the
  address); hoisting it into a temp can swap two registers.**
  `if (p[n] != K && mx < p[n]) p[n] = mx;` with `mx` declared *before* the `if`
  colours the address into `$v1` and `mx` into `$a1`; rewriting it as
  `cur = p[n]; if (cur != K && mx < cur)` swaps them. autorules and regalloc.py
  both come up empty on this one (no copy-chain, no call) — it is a real 5-byte
  tie the permuter cracks in ~400 iterations (FUN_800568b8). Note the compared
  temp must still be `int`, or a `u8 < u8` compare narrows `slt` to `sltu`.
- **A loop-invariant store value hoists, but its `li` lands AFTER the counter's
  init unless you pre-assign it to a named local before the loop.**
  `for (i=N; i>=0; i--) arr[i].f = -1;` hoists the `-1` yet orders its `li`
  after the counter's — write `s16 dead; dead = -1; for (…) arr[i].f = dead;`
  to emit the invariant's `li` first (leResetEnemyLayout: a 2-instruction pure
  register-swap otherwise).
- **A top-test loop that never hoists its invariants is a hand-rolled goto
  loop, not while(1)+break**: `while(1){if(!cond)break;…}` still emits loop
  notes, so loop.c hoists invariant addresses/constants to the preheader and
  strength-reduces repeated field offsets into a second induction pointer
  (extra callee-saved regs, bigger frame). When the target recomputes an
  invariant `lui`/constant INSIDE the loop and walks ONE cursor with plain
  field offsets, write literal labels:
  `loop: if (!(i < N)) goto end; …; it++; i++; goto loop; end:`
  (ClearItemLayout: while(1)+break gave s0–s4/0x28 frame vs the target's
  s0–s1/0x20; the goto form matched first try).
- **No combined address bases + no rotated tests ⇒ goto loops.** Real
  for/while/do-while get loop.c strength reduction (extra induction pseudos
  folding several field offsets, e.g. base+2/base+6) and jump.c rotation (a
  break test migrating to the bottom with a compensation decrement). A
  hand-rolled `label: ... if (cond) goto label;` keeps one cursor with the
  original displacements and a top test + conditional back-jump
  (GetAreaMapLevel's node walk). Corollary: a TU that divides by a variable
  needs maspsx `--expand-div` for ASPSX's break 7/break 6 guards — per-file,
  via the `extra` list next to maspsxGpExterns in Build.hs (mirrored in
  permute.py MASPSX_EXTRA).
- An induction variable zeroed in a **branch delay slot** of the dispatch
  (`move $s1, $zero` under the `beq`) is just `i = 0;` as the first statement
  of the case body — reorg hoists the target block's head insn into the delay
  slot and retargets the branch.
- **A wrap-around search loop with its increment in the backjump's delay slot
  and an inverse-compensation on the fallthrough exit** is a plain do-while
  with the increment as the FIRST body statement:
  `i = CURR; cur = i; do { i--; if (i < 0) i = 0x19; } while (item[i] == 0 &&
  i != cur);` — reorg steals the top-of-loop `i--` into the conditional
  backjump's slot, retargets the branch to the wrap check, and patches the
  fallthrough exit with `addiu +1` (the entry path falls into the original
  loop top, executing the first decrement for free). Writing the increment
  at the loop *bottom* (`for(;;){...; i--;}`) emits `beq → exit; nop; j top`
  instead — one wasted slot. Load into `i` first then copy to `cur`
  (`i = CURR; cur = i;`) so the lh lands in i's register and the copy is
  `move cur,i`, matching the original operand order (DoInfoViewProc).

## Expressions

- **fold reassociation** (fold-const.c `associate`/`split_tree`): any
  `A - C + B` or `A + (B - C)` gets its constant migrated between operands.
  `t - 500 + rand() % 1000` reassociates the *wrong* way (constant lands on
  the remainder). Write **`t + (rand() % 1000 - 500)`** — fold canonicalizes
  it into the original's `(t - 500) + rem`. Semantically that's also the
  natural "position + centered jitter".
- Keep calls **inline in expressions**: `x = rand() % 200 - 100;` uses `$v0`
  straight from the call; a temp (`r = rand(); x = r % ...`) inserts a
  `move`. cc1 evaluates the call operand of a binary expression first, so an
  inline call still executes before the other operand's loads.
- `%` by a constant compiles to the magic-multiply sequence (`mult`, `mfhi`,
  shift/add chains) — automatic from natural `%`; the *same* magic constant
  shared by several `%`s in range gets CSE'd into one callee-saved register.
- Unsigned halfword loads (`lhu`) mean the field is `u16` even when a
  neighbouring field is `s16` (`life` is `s16`@0x8, `lifemax` is `u16`@0xA).
  Different original TUs can disagree about the same field: the item TU
  `lhu`'s lifemax (u16) but the info-view TU `lh`'s it — keep the shared
  header's proven type and cast at the divergent use
  (`(s16)CamState.Owner->lifemax`, DoInfoViewProc's PutLifeBar call).
  - **But a *narrowing* use reads even a signed-s16 global with `lhu`**: `short
    count = SignedShortGlobal - 1;` loads the global `lhu` (the result truncates
    to 16 bits, so the sign bits are dead), while a *comparison* of the SAME
    signed-s16 global in the same function (`i < SignedShortGlobal`) needs the
    full value and loads `lh` — two un-CSE'd loads of one global, one `lh` one
    `lhu` (different machine modes don't CSE). Don't collapse to one load; give
    the narrowing use its own `short` temp and both loads fall out
    (DeleteConflict's `ConflictObjects`).
- **A `u8` local re-narrowed after arithmetic gets a defensive `andi 0xff` +
  unsigned `sltiu`** even when the value provably stays in byte range. If the
  target has a signed `slti` there instead, declare the local `s32` — the source
  `lbu` load already zero-extends so nothing is lost, and cc1 then keeps the
  signed compare with no mask (FUN_800576e8).
- **A boolean flag's declared width (`s16` vs `s32`) changes whether `if (flag)`
  tests it in place or spills through a fresh-register copy — and can shift the
  whole-function instruction count.** When the length is off by a few insns and
  a switch/loop-driven success flag is involved, try flipping its width: a
  `s16` flag matched where `s32` was 4 insns short (handle_char_state_using_item_,
  139→143).
  - **Store-before-sign-extend on a capture-and-increment.** `v = Global; Global
    = v + 1; idx = (short)v;` — the store to `Global` MUST come before the
    `(short)v`. While `v` is still memory-equivalent to `Global`, cse renders
    `(short)v` as a second signed *reload* (`lh Global`) instead of an `sll/sra`
    on the register; the intervening store breaks the equivalence and forces the
    register sign-extend (InsertConflict's slot capture).
- Byte-sized call arguments loaded with `lbu` and no masking are plain `u8`
  struct fields passed directly.
- **A mask literal's SPELLING picks `andi` vs `li`+`and`.** `x & C` is a single
  `andi` only when `C` passes the unsigned-16-bit-immediate test; the *same*
  effective mask written as the complement of a positive constant
  (`~0x5FFF` = 0xFFFFA000, a negative `int`) fails it and materializes a full
  register constant (`addiu $rN,$zero,-0x6000`) + a register-register `and`.
  So `& 0xA000` (andi) and `& ~0x5FFF` (li+and) are bit-identical masks with
  different codegen — read the raw immediate to pick the spelling; don't trust
  Ghidra's positive-literal rendering (Think2confirm's `& ~0x5FFF` vs
  Think1sleep's `& 0xA000`). Corollary: function names here are splat-preserved
  debug symbols describing call-site *intent*, not this function's own control
  flow — `dispose_weapon_data_of_char_` has zero branches (2 stores + a tail
  call, no null-check/free); always read the raw `.s` before assuming a family
  shape from the name.
- **m2c and Ghidra disagree on a call's ARG COUNT in BOTH directions** — m2c
  over-counts (a leftover register at the call site read as an argument),
  Ghidra under-counts (misses stack-passed args past the 4 register ones).
  Count the actual a0-a3 + `sw …,N(sp)`-before-call sets in the raw `.s`
  (Makibishi SoundEx, Ningyo SetNowMotion, Launch SetupFly). m2c *also*
  under-counts in the other direction: a **leading** argument in a register
  carried in live from the caller and never reassigned before the `jal` is
  invisible to m2c (no local def), so it silently drops it — the real call takes
  one more leading arg than m2c shows (DrawBG's `FUN_80063b94`,
  lePackEnemyLayout's `memcpy`/`AdtMessageBox`). Always reconcile against the
  a0–a3 set in the raw `.s`, never m2c's argument list alone.
- **Two `u16` out-parameter locals with a 4-byte stack GAP are one `SVECTOR`'s
  `.vx`/`.vz`** (the write skips `.vy`), not two scalars (LightningBolt's
  GetVectorRotation output).
  Related (ReqItemArrow): two adjacent Ghidra-invented `short[2]` out-params of
  one call are really ONE 8-byte stack aggregate — two tight scalars pack 2
  bytes too close, and padding either to 4 overshoots by 4 (non-additive). Use
  one `struct { u16 a, pad0, b, pad1; }` and pass `&s.a`/`&s.b`; read back via
  `lhu` (the callee writes full words, the caller needs each low half).
- **A caller-side extern's RETURN TYPE is an extension-position lever**:
  declaring a u16-returning callee as `extern u32 f()` in the calling TU
  moves the result's derived (s16) extension after the intervening bit-chain,
  reusing the dying result copy (BIS: GetRealPad). Original TUs routinely
  disagree with the defining TU's prototype — respell per-TU, don't "fix"
  the shared header.
- **Two adjacent (s16)→int extensions the target INTERLEAVES**
  ([sll a][sll b][move][sra a][branch][slot: sra b]) can't come from two
  plain assignments (each emits sll+sra adjacent, one scratch serves both).
  Hand-split the halves: `hx = j << 0x10; hy = shown << 0x10; k = cursor;
  ddx = hx >> 0x10; ddy = hy >> 0x10;` — combine collapses
  (sign_extend)<<16 to one sll, the intermediates overlap, reorg steals the
  last sra into the branch slot (BIS cursor move).
- **A constant stored to both a narrow field and a full-word field must be a
  shared int variable**: cc1 gives the literal in `s16_field = 8;` an HImode
  pseudo and a later `s32_field = 8;` a separate SImode one — two `li`s, one
  instruction too long. `m = 8; ... p->pad = m; ... q->mode = m;` yields the
  original's single `li` in one register (the `sh` reads its low part). The
  tell: the same small constant materialized twice, once feeding `sh`, once
  `sw` (ProcItemDrop's conflict-insert path).
  Refinement: the shared-variable tie only forms when the constant feeds
  ARITHMETIC (`item->mode + one`, an `addu` reusing a compare's materialized
  `1`), not when it only feeds stores — plain `sh/sw` of the same literal to
  several fields can reuse the register with NO named C variable
  (Makibishi needed `s32 one=1;`, LightningBolt's store-only case did not).
- **A callee returning a pointer to a small static struct gets a FULL struct
  copy** even when only one field is read afterward: `sr = *(SmallStruct *)
  f();` — align-2 lwl/lwr+swl/swr block copy, not a selective field load
  (debug_menu_stage_option).
- **The countdown-decrement idiom is per-function — decode the raw immediate,
  don't assume.** `count - 1` (real `addiu -1`, tests the NEW value) and
  `count + 0xff` (literal +255, NOT -1, tests the OLD value) both occur for
  lookalike countdown-and-dispose logic (Happou vs LightningBolt). Ghidra's
  `+0xff` is real, not a decompiler artifact — confirm against the encoded word.
- A two-statement remainder temp (`x = rand(); x = x % 200;`) is provable
  from the asm: the `mult`/`subu` operate **in place on the moved call
  result's register** ($sN) — inline `rand() % 200` computes on `$v0`.
- **Halfword store constants reveal the element's signedness**: storing an
  0xFFFF terminator emits `li -1` (`addiu $rN,$zero,-1`) only for a SIGNED
  s16 element; a u16 element materializes `ori $rN,$zero,0xffff`. Same
  family: an `x != 0` test on an s16 compiles to `sll`+`beqz` (truncation
  with the sra dropped), on a u16 to `andi 0xffff` — the zero-test names the
  type (PauseProc's cheat buffer is s16). Initializers too: `u16 pad = -1;`
  materializes `ori 0xffff`; only an s16 variable's `= -1` gives the
  `addiu $rN,$zero,-1` form (fold converts the constant through the lvalue's
  type before RTL).
- **An increment shared with a call argument goes through a narrow temp
  BEFORE the call**: `addiu rX,s_i,1; addu s_i,rX; … jal f` (rX caller-saved,
  dead at the call) is `j = i + 1; i = j; f(buf, j + 1);` with `short i, j`.
  Ghidra's rendering (`f(buf, (short)(i+1)+1); i = i + 1;`) would hold the
  shared i+1 across the call — callee-saved reg + post-call move that the
  scheduler never undoes. Narrow (HImode) adds stay raw on the unnormalized
  reg (PauseProc).
- **Share one sign-extended pseudo between a zero-test and a later `& mask` with
  an explicit `int` copy.** Inline `(int)shortvar == 0` compiles to `sll+bnez`
  with the `sra` *elided* (a zero-test doesn't need the low bits), so the
  sign-extended value is never materialized and a following `(int)shortvar &
  mask` re-`and`s the raw copy — the shared pseudo never forms. Write `int io =
  shortvar;` and use `io` in both spots: that forces the `sll+sra` into one
  pseudo (callee-saved if it spans calls) that cse reuses (MoveHumanoid: the
  zero-test and the `& 0xff80` byte-resign share `io` across rsin/rcos).
- **A byte-resign that writes a *different* register than the live param is a
  separate local.** `if ((v & 0xff80) == 0x80) v -= 0x100;` whose `addiu`
  targets a fresh reg (leaving the raw param still live) is `o = v; if (…) o = v
  - 0x100;` — the multiply/store reads `o`; a single reassigned `v` compiles
  in-place. (MoveHumanoid keeps three ordr-derived regs live across the calls:
  the raw param, the `int io` test pseudo, and the `o` corrected copy.)
- **`x = -f(...)` negates at the assignment**, and reorg steals the `negu` into
  a *following* call's delay slot, making `-f` live across that call →
  callee-saved ($s3); a later `y = -g(...)` after the last call stays
  caller-saved ($v1). Keep the negation at the assignment (`s = -rsin(a); c =
  -rcos(a);`), don't write `-(short)s` in the downstream expression and don't
  hoist the sign-fix ahead of it (MoveHumanoid).
- **A conditional negate must re-read its own destination: `x = y; if (cond)
  x = -x;`, not `x = -y;`.** Same value, but `x = -x` picks `negu $rX,$rX`
  (dest = source) while `x = -y` picks `negu $rX,$rY` (a different source
  register) — match only when the negate re-reads the variable it writes
  (Think1trace).
- **A conditional value used ONLY as a call argument is an inline ternary, not
  a preceding if/else.** Ghidra renders `f(…, cond ? B : A)` as an SSA artifact
  `tmp = A; if (cond) tmp = B; f(…, tmp);` — but written as separate statements
  the conditional gets its own basic block that strictly precedes (blocks)
  evaluation of the call's OTHER arguments, so sched1 can't interleave the flag
  computation with them. Spell it as the ternary directly (SetupSoundEffect: a
  48-byte residual fixed in one edit).
- **Two registers holding the same computed value = an explicit source
  copy**: cc1 never splits live ranges — `trg = cur & (cur ^ opad);` into
  one callee-saved reg and then `opad = trg;` into another, with the rest of
  the body reading `opad`, is the only way one value occupies two s-regs
  (PauseProc).
  - **Branch variant — a second pointer feeds the guard-branch delay slot.**
    When one branch of an `if` reads a pointer/value the other computes, and the
    target combines the address *eagerly* (before the guard-flag load) with an
    explicit register copy hoisted into the guard branch's delay slot: declare
    the copy `q = p;` right after computing `p` and before the `if`, then read
    `p` in one branch and `q` in the other. This forces the full address combine
    at that statement (instead of drifting into the delay slot) AND supplies the
    move reorg steals into the branch's delay slot (PadShock).
- **Ghidra's short-typed call-result variable can be `int` in source**: a
  short-returning call assigned to int extends once at the assignment
  (sll/sra right after the jal, before any joins); a true `short` variable
  re-extends at each compare after the joins. Place the sll/sra relative to
  the joins to pick the type (PauseProc). Corollary: a short-returning call
  whose result INDEXES an array (`n = InsertConflict(...); D_800BC108[n]…`)
  wants `s32 n`, not `s16` — s16 lets the sign-extend float to the use and
  interleave with the `&arr` address calc (16-byte residual); s32 pins it at
  the assignment (Makibishi/LightningBolt).
- **Ghidra can mistype a stack-passed *parameter*'s width — cross-check m2c's
  per-register typing.** Two tells that Ghidra's narrow guess is wrong and the
  full register width is really used:
  - a full `lw` + `sll`/`sra` re-widen of a parameter used/narrowed exactly once
    with no other reference — a genuinely-`int` parameter merely narrowed for a
    callee's `short` formal instead folds to a single `lh` (cc1/combine reads
    "load then keep the low 16 bits" of a plain memory operand as one
    instruction; a local copy does NOT defeat the fold — copy-propagation erases
    the temp first). Declare the real width: SetLightning's 5th param was
    Ghidra-typed `int b` (rendered `(int)(short)b`) but is actually `short b`.
  - Ghidra spelling the parameter `CONCAT22(in_register_XXXX, narrow_var)` — the
    decompiler admitting its own narrow guess is wrong (the full register width
    is used); the real type is the wide one (PutItemCursor's `rotdif`: Ghidra
    `short`, really `s32`). m2c's per-register width inference is the tiebreaker,
    together with the absence of widen instructions before the arithmetic use.
- **A param-union write to OFFSET 0 routes through a fresh `it->param` recast,
  nonzero offsets through the live `pp` pointer** — mechanical, not stylistic:
  `pp` holds its own register so `pp+0` encodes `sw rN,0(pp)`, but a fresh
  `((T *)it->param)` recast folds the constant offset into `it`'s register
  (`sw rN,0x20(it)`) — same address, different bytes. Extends the twins'
  `hint = 0` single-field case; ReqItemKaengeki does it for a whole 6-word
  tail (p->start/p->end stored as raw s32 at it->param 0/4/8/0x10/0x14/0x18,
  each an isolated lw+sw, no temps). The divergent member can also sit at
  offsets that don't line up with the named view AT ALL (ReqItemGoshikimai:
  three u16 at it->param 0/2/4, no hint/status/count) — `pp` is still declared
  before the null check (the delay-slot lever) even when NONE of its named
  fields are read, purely as a cast vehicle.
- **A param-union store whose access WIDTH differs from the proven shared
  view at the same offset is a distinct union member** — reach it via an
  explicit offset cast off the SAME proven pointer
  (`*(s32 *)((u8 *)pp + 0xC) = 0;`), don't invent a new named struct. Ghidra
  synthesizes a different, often bogus union path per such store
  (ReqItemDokudango got four: napalm/smoke.koro/lightningbolt/gun — cross-
  function unification noise), and NEITHER Ghidra nor m2c discloses the store
  width; only the raw `.s` mnemonic (sw/sh/sb) does. `tools/access.py <Name>`
  prints each pointer offset's width/direction so you don't hand-trace it.
- **A narrowing store fed through a temp forces the full-word load**:
  `x = p->end.vx; pp->vx = x;` gives the original's `lw` + `sh`, while the
  inline `pp->vx = p->end.vx;` lets the canonical cc1 emit a truncating `lhu`
  of the low half. Loads batched before a run of stores (`lw`×3 then `sh`×3)
  mean the values went through source temps. **The stores need NOT stay
  contiguous**: the tell is just "N loads adjacent with no use between them"
  — name them as temps even if the scheduler later scatters their stores
  (ReqItemJirai: `p->user`/`p->type` load back-to-back but their
  `it->owner=`/`it->type=` stores end up 4 insns apart with other stores
  interleaved; `us = p->user; ty = p->type;` then the stores matched, while
  inline loads cost 26 bytes).

## Stack objects

- When Ghidra shows **overlapping stack locals** (its `local_40`-style merged
  buffer), the original really used **one `u8 buf[N]` + casts**
  (`(PARAM_ITEM_USE *)buf`, `*(VECTOR *)(buf + 0x10)`, ...). cc1 2.8 does
  **not** share stack slots between sibling scopes, so distinct scoped locals
  can't reproduce the overlap.
- The **cast type's alignment drives copy code**: `*(VECTOR *)a =
  *(VECTOR *)b` is a word block move (4×`lw`+4×`sw`, `$t2/$t3/$t4/$t1`
  rotation; a 0x50-byte struct assignment becomes the 16-bytes-per-iteration
  copy loop), while `*(SVECTOR *)a = *(SVECTOR *)b` (align 2) is `lwl/lwr` +
  `swl/swr` pairs.
  - **A whole-pool swap-remove is a plain struct assignment `pool[i] =
    pool[last];`** — Ghidra mis-renders it as a hand `do{}while` over invented
    per-field names with a halfword-typed (`sh`) tail; both wrong. A 0x78-byte
    word-aligned struct assignment compiles to `emit_block_move`'s
    `MAX_MOVE_BYTES`=0x10 (4-word) loop over the aligned bulk (0x70) + the
    leftover 8 bytes as two `lw`/`sw`. `tools/access.py <Name> --order` showing
    every copy insn (tail included) as `sw` disproves the field-typed rendering —
    trust it over Ghidra's struct-field names (DeleteConflict).
  - **A small table copied to a stack local is ONE struct assignment, not N
    element assignments** (`load_layout`). N array-element assignments from a
    nonzero-offset extern (`local[0]=Ext[0]; local[1]=Ext[1]; …`) make cc1 fold
    the offset-0 read directly off the `%hi` register while materializing a full
    base address only for the nonzero-offset ones — *two different addressing
    shapes in one sequence*. A genuine small (≤16-byte) whole-struct assignment
    (`local_18 = D_80012168;`, both sides typed as one struct with an inline
    array field) instead forces `emit_block_move` to materialize ONE base
    register up front and emit the batched-loads-then-batched-stores idiom.
    Write it that way whenever Ghidra renders a source table copied to a stack
    local (later indexed by a runtime variable) as separate per-element stores.
- **A truncated per-TU struct breaks when the element is array-INDEXED, not
  just pointer-dereferenced.** The "declare only the fields you touch" truncation
  (DisposeBG-style) is safe for `ptr->field`, but `arr[i]` compiles the index as
  `i * sizeof(T)` — a truncated `T` computes the wrong stride. FUN_80027304
  indexing the conflict pool with `ConflictObjectType` truncated to 0x14 bytes
  emitted `sll;addu;sll` (stride 0x14) instead of the target's `sll v0,a0,4;
  subu v0,v0,a0; sll v0,v0,3` (stride 0x78); replicating GetConflictResult.c's
  full 0x78 layout fixed it. When indexing an already-typed pool array from a new
  file, reuse the proven full-size element struct, never a truncated prefix.
- Write grouped stores **through the same base expression** a later copy
  reads (`((s32 *)(buf + 0x10))[n] = ...;` rather than
  `*(s32 *)(buf + 0x18) = ...;`): with distinct address pseudos the scheduler
  loses the store→copy dependence and interleaves the copy into the divide
  latency (differently from the original).
- **An unexplained frame-size gap — the target's frame is bigger with no
  load/store touching the extra bytes — is often a DEAD/UNUSED local.** cc1
  reserves stack for every declared automatic aggregate regardless of use; when
  the gap exactly equals a sibling function's struct size, declare that struct
  as an unused local (FUN_800274e8: an unused `PARAM_ITEM_USE p;` = 40 bytes, a
  refactoring leftover from wrapping bow_shoot_logic).
- **Overlapping big frame buffers whose addresses rematerialize at every
  call are INLINED STATIC HELPERS, not locals** (DoInfoViewProc's debug
  menus). Mechanics, all three observable in the bytes:
  (1) a frame-address argument (`&local` / decayed array, any spelling) is
  forced into a pseudo by calls.c `precompute_register_parameters` whenever
  its offset from the frame base is nonzero (the value is `(plus vreg c)`,
  not a REG) — and two same-valued forced pseudos in one extended basic
  block get cse'd into a single callee-saved register (`addiu $s0,$sp,N` +
  `move $a1,$s0` twice), costing an extra prologue save. A helper's OWN
  first local sits at inlined-frame offset 0, so its address is the bare
  remapped register → every call re-materializes `addiu $a1,$sp,N` like the
  original. (2) inline expansion allocates the helper frame as a TEMP SLOT,
  freed at the call statement's end: the next inlined helper's frame REUSES
  it if it fits (cc1 never overlaps ordinary sibling-scope locals — verified
  again by probe). DoInfoViewProc: item menu 0xC8 @ sp+0x78; the next
  helper's 0x40 layout+confirm pair reuses 0x78/0xA0; the 0xF8 effect menu
  doesn't fit the freed slot and lands fresh at 0x140; caller's own `mm`
  local sits below at 0x20. Frame = args 0x20 + 0x58 + 0xC8 + 0xF8 + saves
  = 0x248, byte-exact. The tell to look for: same sp offset written by two
  unrelated struct copies + `addiu $a1,$sp,N` repeated per call + a frame
  smaller than the sum of the visible buffers. **The inverse tell**: when
  args + sum(buffers) + saves/pad equals the frame exactly
  (LayoutEnemyOption: 0x10+0x58+0x18+0x38+8 = 0xC0), the buffers are PLAIN
  LOCALS — each struct-copy loop's CODE_LABEL breaks the cse window between
  the copy's address use and the call's, so the merge never fires and
  addresses rematerialize without helpers. A menu copied at ENTRY but only
  read in a later case body is real source order, not a helper.
  Verified mechanics (RTL dumps, BriefingAndInventorySelectionScreen): each
  frame-address arg expands to its own `reg := (plus fp c)` pseudo; the cse
  pass merges same-valued ones ACROSS CALLS (calls invalidate only hard regs
  and memory, not pseudo equivalences) — only a CODE_LABEL breaks the window.
  Passing the written object as the helper's pointer PARAMETER keeps later
  argument uses direct too: integrate substitutes the argument address and
  folds it per-use, so `Helper(buf, &hspr)` + a later `GsSortSprite(&hspr,…)`
  both emit fresh `addiu $aN,$sp,c` (probe-verified against the original's
  delay-slot placement). A pointer variable (`p = &spr;`) is the ORIGINAL's
  shape when the object is the FIRST local (its address is the bare frame
  reg, so the copy is free) and field accesses mix `p->` with direct `var.`
  spellings — keep both spellings exactly as Ghidra shows them.
- **A division variable carried around an in-loop call goes through a
  quotient temp reassigned at the loop BOTTOM**: `q = d / 10; …; call;
  …; av = q; } while ((s16)av != 0);` — the bottom test cse's to q's
  register and av's live range stays off the call (caller-saved, like the
  original). Writing `av = d / 10;` at the top extends av's range across the
  call → callee-saved reg + a whole allocation cascade
  (BriefingAndInventorySelectionScreen's item-count digits).

- **u16 init `= -1` with `addiu -1` AND `lhu` reloads = a 2-byte union**:
  `union { u16 u; s16 s; } pad; pad.s = -1;` — the unpromoted HImode pseudo
  stores the sign-extended canonical constant while `.u` reads stay `lhu`.
  Plain `u16 pad = -1;` gives `ori 0xffff` (see the initializer note above).
- **Inside an ADDRESS expression, a mult-by-constant subterm expands FIRST**
  (`arr[a + b*c]` → `addu chr32,idx` regardless of source order): EXPAND_SUM
  special-cases the mult. Spelling the scale as a SHIFT
  (`arr[idx + (ps->chr << 5)]`) skips the special case and preserves source
  order; a sum assigned to a value temp first (`int n = j + chr * 0x20;`)
  also expands in source order.
- **Pointer-local spelling extends the addu-order rule**: `tp->n[x]` through
  a struct-pointer emits `addu base,index`; `tp[x]` on a plain `char **`
  emits `addu index,base` (AddMisc).
- **A chain of N identical `table + offset` lookups software-pipelines when each
  table address goes through its OWN named local pointer** (`T *p2 = table2; …
  p2[off];`) rather than an inline extern-array cast at the use site. The named
  pointer lets cc1 schedule table K+1's address materialization to overlap table
  K's load/store latency (2-deep pipeline); the inline-cast form computes the
  shift/mask first and serializes. The LAST table in the chain needs no temp —
  nothing follows to overlap with (SetupThinkFunction).
- **A signed `slt` on a pointer comparison is an `(s32)` cast pair in
  source** (Ghidra renders it as `(int)ptr < -0x7ff…`).
- **Array spelling picks the addu operand order**: `p->arr[i]` emits
  `addu base,index`; `(&p->arr[0])[i]` emits `addu index,base`;
  `((T *)0x80010000)->arr[i]` emits index-first with a split `lui`+lo
  displacement that cse merges into any register already holding the
  constant; an extern-array symbol emits `la` (lui+addiu), not the split.
  - **Two ADJACENT data tables are separate symbols, not one array indexed with
    a constant offset.** When a load's `lh/lw` displacement is 0 and the base
    address equals a *different* nearby symbol's address, declare that second
    symbol (`atkd2` at atkd+8) rather than writing `atkd[wclass+4]` — the array
    form folds the +4 into the index and materializes `atkd`'s base, not
    `atkd2`'s (Think3firstattack).
  - **Don't factor out a cursor pointer when the target wants index-first.**
    Repeating the raw index (`map[j].a; map[j].b; …`) vs caching it
    (`rec = &map[j];`) FLIPS the addu order: the pointer-temp assignment trips
    cc1's source-order exception to EXPAND_SUM's mult-first special case. If the
    target's addu is index-first on a plainly-indexed pointer, keep the repeated
    `map[j]` and trust cse to still cache the address — the `rec` temp is wrong
    (LoadAreaMap).
  A hoisted-hi clamp loop with base-first addu is a block-local pointer
  variable (`PersistentState *cq = (PersistentState *)0x80010000;`), one per
  duplicated loop copy (separate pseudos → different hi registers).
- **Division shortening is blocked by an int dividend temp**: `d = av;
  quo = d / 10;` (both int, av s16) keeps the division SImode — quo stores
  raw, no widening pair. `av / 10` directly (or an s16 quo) gets shortened
  to HImode by the front end and widens at the int store. Write the
  remainder through a temp (`rem = d - quo * 10; … (s16)rem * w`) — folding
  the cast over the subtraction distributes it into `(s16)quo * 10` instead.
  The loop-carried copy needs quo int too: `av = quo; } while ((s16)av != 0)`
  folds the ext to the sll-only `bnez` pinned after the copy; with both s16
  the test hoists across the call and av/quo coalesce (the move vanishes).
- **`t = scale ± 0x10; scale = t; if ((s16)t < K)` — compare the temp**:
  reading `scale` in the compare lets combine collapse the pair back into
  one `addiu scale,scale,±16` (the target keeps the temp shape).

## Register allocation steering

> **Diagnose before you steer: `tools/regalloc.py <Name>`.** It runs `cc1 -dg`
> and shows which values are live across calls (forced into callee-saved
> $s0–$s7), the pseudo→hard-reg dispositions, and the register copy-chains that
> bias the coloring. When a draft is the right length + right instructions but a
> value is in the wrong register, this names the copy-chain to break or the live
> range to shorten — instead of blind statement-shuffling or permuting. A value
> is in $s0–$s7 *only* because it survives a call; if the target holds it in a
> caller-saved reg, shorten its live range so it no longer crosses the call.

- **A named `zero` local can flip a pure register-swap tie — cheaper than the
  permuter.** Comparing a loop bound against a `zero` local (`int zero = 0; …
  while (n > zero)`) rather than the literal `0` shifts global-alloc's
  pseudo-number tie-break enough to swap two coalesced registers with no other
  effect — plausible as a real sentinel/bound local. Try it before reaching for
  `tools/permute.py` on a ≤-few-byte register-swap residual (GetArcData).
- **A shared return variable copy-preferences its sources together — use early
  returns to split their live ranges.** Funnelling two values through one
  `ret` (e.g. `ret = existing_id; … ret = new_index; return ret;`) makes cc1's
  local-alloc give the short-lived one a copy-preference toward the other's hard
  register: if `new_index` is callee-saved (live across a call), `existing_id`
  gets pulled into that same `$sN` — even though *its own* live range never
  crosses a call. Writing `if (cond) return existing_id; … return new_index;`
  keeps their ranges disjoint, so `existing_id` takes a caller-saved reg like the
  target. This single change turned InsertConflict from a "mutually exclusive
  from C" 26-byte tie into a MATCH; `tools/regalloc.py` had named the
  `$s0 -> $v0` return copy-chain that pinned it. (When a value is in `$sN` but
  its C live range doesn't obviously cross a call, suspect a return/temp
  copy-chain dragging it there — regalloc.py shows the chain.)
  - **The same split fixes a *flag* return (constant 0/1); the failure shape is
    "default then override".** `ret = 0; if (cond) { …; ret = 1; } return ret;`
    puts `ret`'s default assignment *before* the condition test, so its pseudo
    is live simultaneously with the test's own `$v0` temp (regalloc.py shows an
    explicit conflict edge to hard reg `$v0`) — cc1 evicts `ret` to `$v1` and
    emits a trailing `move $v0,$v1`. Two early returns
    (`if (cond) { …; return 1; } return 0;`) let cc1 target `$v0` directly for
    each constant — 0 bytes (DrawBG). When a flag-return draft is one register
    off, try both shapes.
- **Source statement order is the main regalloc lever.** Storing
  register-held values first (`prm.type = ty; prm.user = own;`) before
  memory-chain stores frees their registers for later constants — in
  ProcItemKusuri that one swap released `$s0` for the division magic and
  collapsed a whole extra callee-saved register out of the prologue.
- **A same-address store must go through whichever pointer variable holds the
  base register the asm uses** (`it->param` vs a `pp` alias, even when
  numerically identical): value unaffected, but it decides a delay-slot
  scheduling tie — the wrong one leaves a small pure-reorder residual (Launch:
  9 bytes, `pp+0x28` → `it->param+0x28`).
- For a guarded indirect call, null-check through a variable but **call
  through the field**: `ppu = item->proc; if (ppu == 0) return; ...
  item->proc(item);` — cse reuses the loaded value and allocation lands in
  `$v0`; calling through `ppu` flips it to `$v1`.
- Cached pointers that live in `$s`-registers across calls
  (`p = &item->param;`) are real source temporaries — indexing the base
  struct directly doesn't allocate the register (see ProcItemManebue).
- **A pool-search cursor and its post-`found:` continuation can be a SEPARATE
  pseudo that's invisible under low register pressure.** Write the loop cursor
  as `cur = items + COUNTER…;`, then `it = cur;` assigned exactly twice — in
  the early-exit (`if (cur->proc == 0) { it = cur; goto found; }`) and once
  before the dispose tail. In SHORT functions `cur`/`it` get the same hard reg
  → the transfer is a deleted self-move; under higher pressure (an `st`/SVECTOR
  local surviving to a late call) they get different regs → a real `move` on
  EVERY edge into the label (cross-jump can't merge — continuations differ).
  Tell: exactly 8 bytes short at the loop-exit label, otherwise identical to
  the single-`it` version. Harmless when unneeded (Ningyo matched with one
  `it`), so try it first when a pool-search function comes up short
  (Makibishi/LightningBolt needed it).
- **Spilled u16 locals loaded several insns before their use went through
  int temps**: `int t1 = cap, t2 = taken; … av = t1 - t2;` — the u16→int
  reads are real zero_extend insns that reload turns into lhu in place;
  combine can't merge them into the later subu. u16 temps DO merge back
  (loads collapse to the use and cost a hazard nop).
- **A surviving `andi rN,reg,0xff` feeding a sum and a later sb across a
  call is `int c = (u8)var;` with var MULTI-DEF** — combine folds the zext
  only over a single-lbu def; reuse another block's variable as the host to
  keep the andi and pin the register (BIS's u0 hosted in the grid x).
- **A loop-internal skip branch whose delay slot duplicates the loop
  increment needs a real `for`**: the rotated exit test leaves
  NOTE_INSN_LOOP_VTOP at the bottom; reorg predicts the branch taken and
  fills from the taken thread (duplicate + retarget). goto/do-while forms
  fill from fallthrough — one insn short (BIS's grid loop).
- **The do{}while(0) lever NESTS and is statement-granular**: each level
  adds +1 loop_depth to flow.c's ref weighting for just the wrapped
  statements. A DOUBLE wrapper flipped a case body's {v0,v1}
  constant/reload pairing; single wrappers around each
  `t = scale ± 0x10; scale = t;` pair made (s16)t read the addiu temp's
  register in all three bounce arms (BIS).
- **A byte-neutral cse re-read works in a COMPARE too**:
  `mx < cq->stock[n]` for `mx < c` folds back to c's register but donates
  different global-alloc preferences (set_preference takes the FIRST operand
  of a compound SET_SRC; find_reg avoids regs other allocnos prefer) —
  flipped one clamp copy's addu tie while the textually identical sibling
  kept the fresh-reg shape (BIS).
- **Which operand goes through the named int temp picks the reload reg**
  (spilled-u16 pairs): temps for both operands let local-alloc hand the
  second zero_extend a low scratch; reading the second operand INLINE
  (`t1 = cap; av = t1 - taken;`) makes its zext an expression temp that
  reload materializes in source order into the next spill reg (BIS digit
  entry).
- **A one-block register tie can hinge on an UNRELATED statement's
  position**: local-alloc's tie-breaking sees the whole block's statement
  order, not just the tied pseudos' positions — when a stubborn swap resists
  every change to the statements touching the swapped registers, move a
  nearby independent assignment instead (PlayerOption: sliding a global
  store two statements later fixed an $a0/$a1 swap whose first uses were
  earlier).
- **`*p = x = expr;` vs `x = expr; *p = x;` flips a scheduling tie** between
  the load feeding expr and the load computing p's address — try the
  chained-assignment fold when two independent insns land swapped and
  nothing else differs (PlayerOption).
- **Read the permuter's non-winning output dirs**: with --stop-on-zero, every
  improvement leaves an output-<score>-<n>/source.c; a plateaued run's best
  candidate can contain the exact single-statement reorder needed — diff it
  against your base instead of waiting out the random search (PlayerOption
  found its fix in an output-30 dir of a run stuck at 805).
- **Byte-neutral respellings are permuter seed levers**: re-reading a cached
  field that cse folds back (`dsp->u = dsp->u + …` for `x + …`) or a
  do{}while(0) around an UNRELATED block shifts pseudo bookkeeping enough to
  flip global-alloc ordering ties (flipped a 40-line callee-saved mirror at
  zero byte cost). Semantic-diff permuter winners against the REPRINTED base
  — the reprint alone skews scores.
- **Anti-rules**: element-pointer temps materialize the array displacement
  as a real addiu (keep repeated `base[n]` spellings); address-taken locals
  move from spill slots to frame slots and shift everything; unreferenced
  static inline helpers still get emitted in stub TUs — keep helpers inside
  the caller's #if guard.
- **Callee-saved homes on short-lived block locals mean VARIABLE REUSE**:
  the original reuses one s16 `j` across the entry-counts loop, the case-1/3
  loops, the grid counter, the shown-items counter, the cursor-move dx and
  the epilogue loop (calls inside two of them force the shared pseudo into
  $s0 everywhere), and reuses `shown` as the cursor-move dy. Distinct
  same-shaped counters stay caller-saved. Also: `for (j = shown; ...)` right
  after `shown = 0;` reproduces a `move` loop-init from the zeroed variable
  (cse copy-propagates, like `i = 0` after `cursor = 0` giving
  `move a0,s8`). Anti-rule: DEAD early assignments cannot steer allocation —
  cse propagates the constant into unrelated code.
- **cse has a (set REG0 REG1) copy-swap special case for ADJACENT pairs**:
  `base = misc; p = base;` back-to-back gets rewritten so the la lands in
  the copy's dest and the copy flips. Keep other initializer insns BETWEEN
  the la and the copy (decl order!) — prev_nonnote_insn adjacency is what
  the swap checks (AddMisc).
- **A stack parameter used exactly once is load-sunk to its use**
  (update_equiv_regs: REG_N_REFS==2 + REG_EQUIV from assign_parms). A local
  copy (`s32 va = ry;`) defeats it — the parm substitutes into the copy, so
  the entry lw lands at the copy's DECL position (decl order = load order)
  and the copy's pseudo needs a hard reg. Every REG_EQUIV-noted parm also
  gets REG_LIVE_LENGTH×2 — raw parms lose allocation races their ref counts
  say they should win (AddMisc).
- **Loop notes are scheduler barriers** (sched adds artificial deps at
  LOOP_BEG/END): a do{}while(0) weight lever must ENCLOSE any insn that has
  to float across it, and must extend past a switch's tail if a case arm
  jumps out of the note range — otherwise jump.c moves that arm's block to
  the function end (AddMisc).
- **Global-alloc priority is floor_log2(n_refs)·n_refs/live_length·10000·size,
  ties by pseudo number** (global.c allocno_compare; flow adds loop_depth
  per ref). cc1's `-da` dump prints each pseudo's refs/length and the
  allocation order — with it, the whole caller-saved assignment can be
  ENGINEERED by placing do{}while(0) levers at computed depths (AddMisc:
  body wrapper depth 2 + nested double wrapper on the loop-bottom increment
  gave [a0,a2,a3,t0,t1] exactly).
  Refinements (AdtSelect): read refs/length from `-dl`, the final assignment
  from `-dg`'s "N in HardReg" table; do{}while(0) notes do NOT count toward
  live_length, so boosts compute exactly. floor_log2 makes priority jump
  DISCONTINUOUSLY at 16/32 weighted refs — +1 occurrence can be the right
  boost where +2 overshoots. Wrap a loop with its init HOISTED OUT
  (`i = first; do { for (; i < last; i++) … } while (0);`) to boost interior
  pseudos without boosting the init-read. Hazards: loop notes are sched1
  barriers (never place one between two defs that must interleave), and a
  wrapper around defs WITHOUT their consumer lets cse re-canonicalize +
  combine collapse the copy — the function changes length. Wrap
  def+consumer as a unit or not at all.
- **Huge-frame TUs (offsets past ±32767) spill params to their arg-home
  slots** once the callee-saved file is full; every use rematerializes
  li/addu/lw through reload's spill regs (a3/t0 rotation). Pointer-VALUE
  reads share one reg in place; pointer-DEREF reads emit a two-reg
  operand-address pair. A residual a3/t0 swap there is RELOAD structure
  (find_reloads_address + allocate_reload_reg rotation), not regalloc —
  permuter-immune; steer it by changing which earlier insns in the same
  block need reloads (AdtSelect's CURRENT(9)).
- **A `do { ... } while (0);` wrapper is a regalloc lever**: the degenerate
  loop generates no code, but its loop notes make flow.c count every ref
  inside at loop_depth 2, doubling those pseudos' priority in global-alloc.
  When the callee-saved assignment is permuted relative to the target and
  statement-level tweaks can't fix it, wrapping the body (after parameter
  setup, before the final return) re-weights the whole interior at once
  (GetAreaMapLevel: flipped four s-registers in one move; GetRealPad's old
  wrapper was the same mechanism).
- **A whole-block `if (X) {block} else {block}` with BYTE-IDENTICAL arms is a
  scheduling lever** (permuter-found; no statement reorder reaches it).
  Duplicating an interleaved multi-load sequence into both arms — even when `X`
  reads a not-yet-initialised local (harmless, always overwritten before use) —
  shifts cc1's front-end basic-block/pseudo numbering enough to fix a
  load-delay-slot interleaving order; the straight-line form was 8 bytes longer
  with a stray `nop` and a different colour for an unrelated variable
  (Think1trace).
- **Hoist a shared `result = 0;` default to the function's SINGLE entry, not
  per-branch** — reorg then folds that one store into the very first branch's
  delay slot, shared by every downstream path; writing it separately in each
  branch prevents the fold and forces per-branch reconstruction (Think1trace).
- **Reused parameters vs fresh locals show in the prologue**: `move $s5,$a1`
  + later arithmetic on $s5 means the original overwrote the parameter
  (`x = x / 10;`) — the param pseudo gets a callee-saved home. A fresh local
  computes straight into the s-register with no entry move.
  - **This extends to POINTER parameters, and the per-call offsets are the
    tell** (`LoadOrnament`). `adr = adr + 1; f(adr); g(adr + 2);` — reassigning
    the pointer parameter in place and then using a *smaller residual* offset for
    the second call — reproduces both the callee-saved register reuse and the
    exact offset split. Two fresh computations off the untouched parameter
    (`f(adr + 1); g(adr + 3);`) put the value in a different register and compute
    both offsets fresh: a register swap plus an instruction-shape mismatch.
- **A value computed into a source temp BEFORE an intervening store frees the
  return register for an earlier expression.** In `AfsGetHeader` the target
  builds `maxElements`'s shift/or chain in `$v0`, then `move $v0,zero` (the
  `return 0`) lands *between* the two field stores, so the second chain
  accumulates in `$v1`. Writing the two field assignments in plain source order
  (`a = X; flag = 0; b = Y;`) makes cc1 materialize `$v0 = 0` ahead of the first
  chain, pushing *its* accumulator to `$v1` and re-colouring both — a same-length,
  instruction-for-instruction-identical, 61-byte miss. The fix is a named local:
  `a = X; tmp = Y; flag = 0; b = tmp;`. The tell is loads for the *second*
  expression scheduled before the intervening store, while its own store stays
  after. (Related: N adjacent loads with no use between them are source temps.)
- **A delay-slot instruction can be pulled BACKWARD across calls by reorg** —
  its position in Ghidra/the disassembly ("the statement right before this
  call") is not proof of its source position. When a manually-placed
  increment resists sitting near its apparent slot, try moving it PAST
  subsequent calls (ReqItemHappou's permuter fix moved `j++;` from before
  SearchItemTarget2 to after SetupFly, crossing two calls).
- **A trivial dependency-free constant as the FIRST statement floats its `li`
  above the prologue `sw ra`** (not just above unrelated stores). Writing the
  statement that needs a LOAD first anchors the schedule so `sw ra` stays put
  and the `li` fills the load's delay slot (ReqItemStay).
  - **No-load body (one call, only constant args) → wrap it in `do { … }
    while (0);`.** When there's no load anywhere to anchor on, the loop-note is a
    genuine scheduler barrier that pins `sw ra` before the wrapped body, exactly
    as a load would — fixes the constant-args floating above the prologue
    (EVENT_OBJ_F4/11C's `DeliverEvent(...)` — though those stay NON_MATCHING for
    the separate SDK-compiler reason below).
- **Assignment position around a branch is a double lever** (ReqItemDrop):
  `pp = ...;` placed *before* `if (it == 0) return 0;` lets reorg fill the
  `beqz` delay slot with its `addiu` (instead of collapsing the return-0
  block into the slot) *and* lengthens pp's live range, demoting its
  allocation priority so the function argument keeps `$s1`. One statement
  move fixed a register swap, a missing block, and two missing instructions.
- **An independent load can schedule BEFORE unrelated stores that textually
  precede it — don't reorder source to chase its asm position.** cc1 hides
  load latency by hoisting a load past intervening stores it doesn't alias:
  ReqItemKusuri's `it->locate` reload (fresh per the no-cached-pointer-temp
  rule) appears in the asm before the `it->proc=`/`it->mode=`/`it->type=`
  stores that precede it in source, yet the natural owner/proc/mode/type/coord
  statement order matched first try. A load ahead of "earlier" stores is the
  scheduler, not a source reordering.
- A base-address pseudo appearing mid-sequence (`addiu $a0, $s1, 8` between
  two accesses) is a temp assigned between the statements:
  `t[0] = p->start.vx; st = &p->start; t[1] = st->vy; ...`.
- The reverse also holds: **cc1 does not fold away a pointer temp to a frame
  object**. `PARAM_ITEM_USE *prm = (PARAM_ITEM_USE *)buf;` allocates a
  callee-saved register for the address (frame grows, prologue shifts), and a
  `VECTOR *w = (VECTOR *)(buf + 0x10);` temp inside a loop reshuffles the
  induction/invariant registers. When the target *rematerializes* a frame
  address (`addiu $a0, $sp, 0x10` appearing repeatedly), the original wrote
  the cast at every use — repeated `((T *)buf)->field` casts, however ugly,
  are the source's real shape. `sizeof(T)` in the memsets is free
  (compile-time constant) and matches.

- **Anti-rule (open problem): sched's equal-priority tiebreak prefers the
  memory-unit insn** — a store and an independent ALU op simultaneously
  ready always emit [alu][store]. A target showing [store][alu] with all
  else matched resisted statement order, temps, multi-def hosts and
  do{}while(0) fences (fences pin the order but retie the dying operand
  into $a0). Untried levers: make the pair NOT simultaneously ready
  (lengthen the store's downstream dependence chain with a byte-neutral
  field re-read, or derive the ALU operand from a later-completing value).
  FileOption case 0xd, CURRENT(2).

## Shared tails

- **A store shared by both branch paths via the branch delay slot**: load the
  condition into a temp first, store between the temp and the branch
  (`uid = StageConfig[...].uid; q->counts[0] = 0xFF; if (uid == 0) {...}`) —
  the constant li fills the load-delay slot and reorg drops the lone `sb`
  into the branch delay slot, executed on both paths. Writing the store
  BEFORE the load lets the scheduler hoist it too early; writing it
  DUPLICATED inside both paths only cross-jumps when both copies' first
  insns are identical including registers
  (BriefingAndInventorySelectionScreen's entry).
- **Write each switch case's call INLINE rather than funnelling its arguments
  through one shared local into a single post-switch call.** The funnelled form
  (`case X: s = STR_X; break; … f(s, …);`) forces a two-register hi/lo address
  split — a temp for `%hi`, a different register for the `%lo`-completed pointer.
  Writing `case X: f((T *)STR_X, …); break;` per case instead lets gcc's
  cross-jump pass merge the identical call-tails into ONE physical copy *and*
  lets each address materialize straight into the call's own argument register,
  reproducing the target's single-register `$a0`-to-`$a0` shape (FUN_8004f6c0).
- An identical dispose/cleanup sequence reached from two paths with a `j`
  into the middle of one copy is **the code written out twice** in source —
  GCC's cross-jump pass merges the common suffix (from the `jalr` backwards;
  the differing prefixes, e.g. two different `mode = 0xff` stores, stay
  separate). A shared `goto` label merges *more* than the original (loses the
  duplicated call-site setup) — don't.
- **When two branches build the SAME call with all-different arguments, write
  the call literally twice (once per branch), not funnelled through shared
  locals + one post-merge call.** Cross-jump merges only the byte-identical
  suffix (`jal` + empty delay slot); the argument-loading prefix stays
  per-branch — including a same-valued-constant CSE chain (`a0=0` fresh, then
  `a1`/`a2` copied from `a0`) that a single shared call site won't reproduce
  (one branch reads globals, the other passes all-zero — StartDrawing area).

- **Multiple `return CONST;` statements cross-jump unpredictably; a labeled
  return body pins them**: write the constant return once, label INSIDE the
  last guarding if (`if (...) { ret_min: return 0x80000000; }`), `goto
  ret_min;` elsewhere — one fallthrough-entered body whose insns reorg steals
  into the other branches' delay slots.
- A conditional branch that lands PAST a load at its target block is reorg's
  redundant-insn elimination (the branch path already has the value): both
  source paths simply read the same variable — no restructuring needed.
- **A call+return tail shared by several loop exits, with the argument `li`
  in each exit's jump delay slot, is ONE copy written after the loop**:
  `break` from each path and write the call once (PauseProc's
  `SsSetMVol(0x7f,0x7f)`). Reorg steals the after-loop block's first insn
  (the arg li) into each break-jump's empty delay slot and retargets past
  it; a path whose slot is already full keeps jumping to the li itself.
  Duplicating the call per path merges WORSE (sched2 drags the arg setup
  from the call; cross-jump suffixes stop matching).

## Split functions (jump tables through .rodata)

**`tools/split-scaffold.py <Name>` does all of this for you** — run
`tools/reverse.py <Name>` first (adds the `c` carve, generates the pieces),
then `split-scaffold.py <Name>` writes the full NON_MATCHING stub (every
INCLUDE_ASM piece + the jump table(s) as `static const u32 …_jtbl[]`), inserts
the `.rodata` carve, and leaves `./Build check` green; fill in the `#else`
draft and iterate. reverse.py detects the split case and points you at it.

**But not every 2-piece report is a real jump table.** A
`__override__prt_<addr>_<hash>` symbol at a mid-function address makes
reverse.py/split-scaffold see two pieces, yet if the raw `.s` has NO branch to
that address — piece 1 falls straight through into piece 2 — it's ONE ordinary
function: write it as plain C with no `_jtbl` array (SetupStageSequence;
FileOption.c has a buried instance). Check for an actual jump before scaffolding.

The manual mechanics, for reference:

A table-switch splits the function into several nonmatching .s pieces and
routes the table through the C object's .rodata (see the yaml comment at the
BriefingAndInventorySelectionScreen carve). Consequences: the yaml needs
`- [fileoff, .rodata, <Name>]` at the table's original address plus a data
re-split line after it; the STUB state
must INCLUDE_ASM all pieces (entry + switchD + every caseD, address order).
Preferred protocol: DELAY the `.rodata` carve until you activate the real
switch — during the stub phase the table stays raw incbin'd data and no
placeholder is needed; add the carve (address from the
`switchD_…__switchdataD_<tableaddr>` symbol) together with the real C, whose
compiled table then supplies the bytes. (A `static const u32 …_jtbl[]`
placeholder is only needed if the carve must pre-exist the C, e.g. a
committed WIP.) reverse.py seeds only the FIRST piece's INCLUDE_ASM — copy the full list
(all pieces, address order) from splat's generated
`.shake/gen/main.exe/src/<Name>.c`;
tools/permute.py concatenates all pieces for its target (fixed after this
bit a session); tools/matchdiff.py's window stops at the first piece — use
tools/asmdiff.py (sizes from the Ghidra export) for iteration and
matchdiff's whole-image count only as the gate. permute.py orders the
concatenated pieces by first-instruction address (lexicographic sorting is
wrong: caseD_1f before caseD_2, switchD last).

## gp vs absolute globals (item-TU vs think-TU)

**A bare `lui $rN, 0xHHHH` with NO following `addiu`, whose register is then
reused as a base for several `lbu/sb`-style accesses (often hoisted into a
branch delay slot so both arms share it), means the source held the address in a
LOCAL via a literal pointer cast** — `PersistentState *ps = (PersistentState
*)0x80010000;`, the convention already used by
`BriefingAndInventorySelectionScreen.c`'s `PSTATE`, `FUN_800565f0.c`,
`FUN_800566c0.c`. Spell the same access as a plain `extern u8 D_80010047;` and
cc1 folds the absolute address into each memory operand instead, so `as`
re-materialises `%hi` per use through `$at` (`lui $at,0x8001; sb $v0,71($at)`) —
extra `lui`s and the *wrong length*, not merely a register tie. The `lui`
survives alone only because the object's `%lo` is 0, so the cast's constant
needs exactly one instruction (FUN_8001b2b8).

ASPSX gp-addresses only symbols **defined in the TU being assembled**;
externs are always absolute (`lui $at`/dest-reg expansion of the one-line
macro). Our per-file `maspsxGpExterns` list in `Build.hs` encodes "smalls the
original TU defined". Full story + evidence in
[toolchain.md](toolchain.md#gp-globals-making-small-globals-byte-match-solved--per-tu-opt-in).
Cross-TU aliasing reaches struct FIELD addresses too, not just top-level
globals: `CURRENTLY_SELECTED_CHARACTER_STATE_PTR` (item TU) is byte-identical
to `CamState + 0x10` = `CamState.Owner`'s address — check an m2c "mystery
symbol"'s arithmetic against already-proven struct layouts before treating it
as new.

Practical rule: `tools/gpsyms.py <Name> --write` derives the set from the
split asm's `%gp_rel(...)` and syncs both lists for you. Build.hs exposes the
per-file gp flags through a Shake **oracle** (`GpFlags`), so editing the list
invalidates exactly the affected `.s` on the next build — no manual clean, no
cache-busting (a gp-list edit used to survive stale because Shake can't see a
`cmd_` argument change; the oracle makes it a tracked dependency). Needing
absolute → keep the symbol off the list (a plain small extern).

- **-msplit-addresses is effectively ON in the pinned cc1**: non-small
  extern symbols compile to compiler-level HIGH/LO_SUM through ALLOCATED
  regs (`lui rA,%hi / addiu rB,rA,%lo`, two-reg pairs possible, %hi
  loop-hoistable, reorg can steal the lui into a delay slot); small (≤ -G8)
  symbols keep the one-line macro that maspsx/as expand via $at (stores) or
  the dest reg (loads). Tell: $at = small-symbol macro; allocated-reg lui =
  non-small split. Force the non-small path per-TU with an unknown-size
  extern array. A block copy whose source address builds as a two-reg
  lui/addiu pair is a plain extern struct assignment under split-addresses;
  flat `*(T *)0x…` casts become same-reg li macros instead. A hoisted
  lui-only base + per-iteration `addiu rD,base,disp` arg is a block-local
  round-constant pointer (`char *hi = (char *)0x80090000;`) whose uses sit
  inside a label-broken loop window (FileOption).
  - **Offset-0 folds, a nonzero offset materializes — and the addiu is always
    the DECLARED symbol's own address.** A non-small extern accessed at offset 0
    (bare scalar, struct field, or index 0 — verified 4/8/36/40 bytes) never gets
    a standalone `addiu`: cc1 shares one `lui` and folds `%lo(sym)` into each
    load/store's displacement (4 insns). A nonzero constant offset forces full
    materialization (`lui`+`addiu` forming the declared symbol's *own* address,
    never partially absorbing the extra offset), and the offset applies purely as
    the consuming access's displacement (5 insns). Consequence for a get/set-swap
    NON_MATCHING: if the target's `lui`+`addiu` decodes to the exact final address
    with **0 residual displacement** on every access, no "declare the global at
    its own offset" spelling reproduces it (offset 0 folds; a nonzero offset never
    leaves 0 displacement) — the real symbol must be an earlier sibling field of a
    not-yet-matched enclosing struct. Check this dead-end early (MemCardCallback).
- **≤8-byte externs are -G8-small: their address is one `la` (lui+addiu into
  the SAME register); a split `lui rA,%hi / addiu rB,rA,%lo` across TWO
  registers means the original declaration was NOT small** — respell as an
  unknown-size array (`extern SVECTOR D_80097B88[];`): unknown size is
  non-small, cc1 emits separate HIGH/LO_SUM pseudos and the hi temp gets the
  first free caller-saved reg. Raw-constant casts give `lui+ori` (wrong), a
  pointer temp to a small extern still collapses to `la`
  (DebugMenuItemSet's smoke vector).
  - **Counterexample — an 8-byte object is not automatically small in practice**:
    an 8-byte struct consumed by a whole-struct assignment can still want the
    non-small two-register hi/lo split. So don't reason from the size alone: if a
    plain scalar extern yields one shared register but the target uses two
    different ones, respell as an unknown-size array *even at exactly 8 bytes*
    (DoBriefingAndInventorySelection's `extern RECT D_80097698[];`, indexed
    `[0]` — 14 bytes off as a plain `extern RECT`).
- **A callee-saved value dying at a call whose result lands in the SAME
  s-register is the call-result variable HOSTING the earlier load**:
  `h = pm->locate.coord.t[1]; gy = h; … h = GetAreaMapLevel(…, gy, …);` —
  the lw lands in h's pseudo, gy coalesces onto it, and the post-call
  `move sN,v0` reuses the register once gy dies. Loading gy directly leaves
  h caller-saved and drifts the whole tail (DebugMenuItemSet, permuter
  find; same family as the variable-reuse rule).

## Toolchain gotchas

- **The PsyQ SDK block (`0x80060000+`: CRT/libcd/libapi) was built by a DIFFERENT
  compiler — don't source-shape it.** `Exec`/`__main`/`__SN_ENTRY_POINT`/`Cd*`/
  `PC*`/`EVENT_OBJ_*`/`DeliverEvent`/`_SN_*` are precompiled library objects, not
  game TUs. The tell: for a trivial (ra-only) frame our cc1's `mips_expand_epilogue`
  ALWAYS emits `restore-ra; addiu sp,sp,N; jr ra` with reorg pulling the `addiu`
  into `jr`'s delay slot — but these objects leave the delay slot a bare `nop` with
  the sp-restore BEFORE the jump, a shape our cc1 never produces at any `-O`/body
  (verified via `-dd`/`.dbr` dumps + the gcc-2.8.1 sources) and permuter-immune.
  So a few-byte epilogue residual on a `0x80060xxx` function is a compiler-version
  mismatch, not a source problem — don't chase it. `triage.py`/`progress.py` now
  cut the game/SDK boundary at `0x80060000`; also note `EVENT_OBJ_80/90/BC` etc.
  aren't even standalone functions (they're shared epilogues of `CdInit` and its
  retry wrapper — splat carved them only because they had symbol names).
- **`sll r,r,16 / sra r,r,k / sra r,r,16-k` (k≠16) is a sign-extension FUSED
  with a left-scale, and has no natural-C form.** A plain "index an array by a
  `short`" always compiles to the textbook 2-shift `sll16/sra16`, and cc1's
  fold/combine refolds *every* plain-arithmetic respelling back to it (nested
  expression, two statements, K&R params, explicit pointer casts, a static-inline
  helper, even a literal transcription of the asm shifts — all verified against
  cc1 `-da` RTL dumps + the gcc-2.8.1 sources). Only a genuine optimizer barrier
  (an empty register-constrained `__asm__("" : "+r"(t))`) reproduces it, which is
  the same open inline-asm policy question as `PClseek`. Whole game code contains
  exactly three, all parked: `GetPad`, `FUN_8001b174`, `GetPadXY` (see
  `GetPad.c` for the byte-exact barrier form). `triage.py` now detects the triple
  and reports `SIGNEXT-SPLIT (GetPad class)` instead of rating it TRIVIAL.
- **`as` cannot assemble PS1 GTE command opcodes — the DrawTMD/renderer region
  is un-splittable until a GTE→`.word` pass exists.** Our `mipsel-...-as`
  (binutils 2.40, `-march=r3000`) is vanilla MIPS: standard COP2 data moves
  (`lwc2`/`swc2`/`mfc2`/`cfc2`) assemble, but the GTE *command* opcodes
  (`RTPT`/`RTPS`/`NCLIP`/`DPCS`/`DPCT`/`MVMVA`/`AVSZ3`/`AVSZ4`/`GPF`/`GPL`/`OP`/
  `SQR`/`NC*`/`CC`/`CDP`/`INTPL`) are `unrecognized opcode`. splat emits them as
  *mnemonics* when you individually split a function, so `reverse.py` on any
  renderer helper (FUN_8005d1fc/d764/dd30/d49c/e330 = per-primitive-type
  handlers dispatched by `DrawTMD`) fails at `./Build check` — before the
  NON_MATCHING convention even applies (that assumes the stub assembles). maspsx
  has no GTE handling either. **The blocked set is wider than the `0x8005dxxx`
  handlers: 24 game functions (~15 KB) contain GTE commands, including
  `FUN_8001c730` and the whole `0x80057b80`–`0x8005a3cc` block (`FUN_80057b80` is
  3796 B).** `triage.py` now flags them (`GTE CMD — UN-SPLITTABLE`) and ranks them
  VERY-HARD, so they stop surfacing as attractive small targets; `SetDepthQ` is
  the lone COP2-*moves*-only function (`ctc2` ×2 — it assembles, but has no C
  spelling, so it is blocked on the inline-asm policy, not on the `.word` pass).
  objdump renders a GTE command as a bare `c2` mnemonic — that's the detector. FIX (backlogged, see orchestration.md): a filter
  that rewrites each GTE mnemonic to `.word 0x…` using the encoding on splat's
  own `/* addr vaddr WORD */` comment on the line. **BEWARE: that WORD column is
  the raw little-endian file bytes in address order, NOT the instruction word —
  you must byte-swap it before emitting a `.word`.** Verified: `jr $ra`
  (`0x03E00008`) prints as `0800E003`, and `j SystemOut` (`0x08005BA3`) prints as
  `A35B0008`; the file bytes at that offset really are `a3 5b 00 08`. Since GNU
  `as` on a little-endian target lays `.word 0xAABBCCDD` down as `dd cc bb aa`,
  copying the comment verbatim emits the bytes REVERSED. So the pass is
  `.word 0x` + byteswap(comment) — byte-exact only after the swap.
  NOTE it only makes the region *splittable*; MATCHING these still
  needs the register-pinned-locals / inline-asm treatment (non-ABI calling
  convention, values in `$t2..$t5`/`$s0`), i.e. the same open inline-asm policy
  as GetPad/PClseek — so build the pass together with that decision.
- **maspsx's `break` wants the single-value form `break 0x107`, not the
  two-operand disassembly `break 0, 263`.** maspsx (`elif op == "break"`) takes
  one immediate and splits it into the two 10-bit fields itself; the
  Ghidra/objdump-printed `break code1, code2` form crashes it with `invalid
  literal for int() with base 0: '0,'`. Relevant only when hand-writing inline
  asm for a raw BIOS trap (PClseek's `break 0x107` host-lseek — a syscall with
  no C representation, currently parked NON_MATCHING pending the inline-asm
  policy question; see GetPad.c).
- **`config/symbols.main.exe.txt` (an ld/splat script) has NO comment syntax**
  — `//` or `/* */` is a hard parse error, not a no-op. If a splat-auto-named
  `D_XXXXXXXX` data symbol resolves to the wrong address (verify against the
  `.map`; a few bytes low, shared with a neighbor, = a pre-existing drift, not
  yours), bind a FRESH non-colliding name at the correct address there — it
  resolves regardless of the drifted file's byte-accumulation.
  - **A Ghidra `__override__prt_` symbol splits an ordinary function into two
    `.s` pieces — it is NOT a jump table.** Those symbols are Ghidra call-SITE
    prototype overrides (e.g. the `jal sprintf` inside `PathFileRead`), but splat
    reads `symbols.main.exe.txt` as `symbol_addrs` and starts a new piece at every
    symbol inside a function's range. A stub carrying only piece 1 fails to link
    (`undefined reference to .L<addr>` — a branch target living in piece 2), which
    reads like a bad carve boundary but isn't. `reverse.py` now detects this
    (every extra piece's symbol contains `__override__prt_`), seeds all pieces,
    and reports green; a real jump table still routes to `split-scaffold.py`.
    30 unmatched game functions still carry an interior marker (`valloc`,
    `Camera` ×3, `StageSequence` ×4, `LoadConstruction` ×3, …). The root fix —
    dropping these 79 non-symbols from `symbols.main.exe.txt` — is blocked on
    `FileOption.c`, whose parked 18-piece stub `INCLUDE_ASM`s an override piece
    by name.
  - **Matching a function can DELETE the very `D_` symbol its C body needs.**
    splat only auto-labels a data address that some still-carved *asm* refers to.
    The moment you replace the last such function with C, the `glabel`/auto-symbol
    disappears and the link fails with `undefined reference to D_XXXXXXXX` —
    which looks like your C is wrong, but is really the reference count hitting
    zero. Fix: add a plain `D_XXXXXXXX = 0xXXXXXXXX;` line to
    `config/symbols.main.exe.txt` (PathFileRead's `"%s%s"` at `D_800976DC`;
    contrast `D_800127A4`, which keeps its label only because AddMisc's
    jump-table asm still references it).
  - **gp-output SVECTOR/aggregate whose fields splat auto-named as separate
    drifted `D_` symbols**: when a function writes a small SVECTOR to gp memory,
    splat names each *referenced field address* its own `D_XXXX` glabel and can
    place them at drifted addresses — so storing through the `D_` names puts
    every `sh`/`sw` at the wrong gp_rel offset. Bind ONE fresh correct-address
    name in `config/symbols` and declare it as the real SVECTOR; maspsx
    gp-addresses the `+2`/`+4` field offsets from that base fine (the original
    compiled it exactly that way, which is *why* splat saw separate addresses).
    Concrete: `ConflictDistance`/`ConflictModel` in GetConflictResult.
- `-fdollars-in-identifiers` is required for anything including
  `reference/ghidra_types.h` (Ghidra `$`-names); the mod pipeline passes it.
- The canonical cc1 correctly reads the **low** halfword for
  `(short) = (int)` truncation; if you see a high-half `lhu` (offset+2),
  something reintroduced the old non-canonical binary.
