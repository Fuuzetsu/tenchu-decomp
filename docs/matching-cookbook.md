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
   finally `tools/permute.py <Name>` for pure allocation ties.
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
6. **On MATCH:** `./Build check`, add the matching-notes header, promote any
   NEW reusable rule to this cookbook, commit the function + splat.yaml
   together.

## Picking targets

`tools/findsimilar.py --targets` ranks every unmatched function by assembly
similarity to the matched corpus (best-first, smallest-first) — the top
entries are near-clones of things we've already solved and make good next
targets. Prefer functions from an original TU we've already touched (shared
headers like item.h and the gp-extern lists already exist for those). Batch
work: one function per matcher agent (.claude/agents/matcher.md), and commit
only on a green `./Build check`.

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
- A mode-dispatcher that **reloads its variable** (two `lbu` of the same
  field) and compares **signed** (`slti`) is a real **`switch`**: cc1's
  `expand_case` emits a balanced compare tree over a *fresh* index load. An
  if/goto-ladder CSEs the load into one `lbu` and compares small unsigned
  types with `sltiu`. Case bodies are laid out in source order.
- A preceding test that keeps its constant in a **callee-saved register**
  across calls (`li $s4, 0xFF` + later `sb $s4`) is a local variable
  (`u8 ff = 0xff;`) also used by a later store. A path where the same value is
  materialized fresh (`li $v1, 0xFF`) uses a literal there — the register got
  reused for something else in between.
- Branch-if-equal *into* a physically-later block with the fallthrough being
  the other body usually means the source condition was the **opposite
  polarity** of Ghidra's rendering (`if (0xe < n) {...} else {...}` vs
  Ghidra's `if (n < 0xf)` with swapped bodies).
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
- Byte-sized call arguments loaded with `lbu` and no masking are plain `u8`
  struct fields passed directly.
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
- **A callee returning a pointer to a small static struct gets a FULL struct
  copy** even when only one field is read afterward: `sr = *(SmallStruct *)
  f();` — align-2 lwl/lwr+swl/swr block copy, not a selective field load
  (debug_menu_stage_option).
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
- **Two registers holding the same computed value = an explicit source
  copy**: cc1 never splits live ranges — `trg = cur & (cur ^ opad);` into
  one callee-saved reg and then `opad = trg;` into another, with the rest of
  the body reading `opad`, is the only way one value occupies two s-regs
  (PauseProc).
- **Ghidra's short-typed call-result variable can be `int` in source**: a
  short-returning call assigned to int extends once at the assignment
  (sll/sra right after the jal, before any joins); a true `short` variable
  re-extends at each compare after the joins. Place the sll/sra relative to
  the joins to pick the type (PauseProc).
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
- Write grouped stores **through the same base expression** a later copy
  reads (`((s32 *)(buf + 0x10))[n] = ...;` rather than
  `*(s32 *)(buf + 0x18) = ...;`): with distinct address pseudos the scheduler
  loses the store→copy dependence and interleaves the copy into the divide
  latency (differently from the original).
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
- **A signed `slt` on a pointer comparison is an `(s32)` cast pair in
  source** (Ghidra renders it as `(int)ptr < -0x7ff…`).
- **Array spelling picks the addu operand order**: `p->arr[i]` emits
  `addu base,index`; `(&p->arr[0])[i]` emits `addu index,base`;
  `((T *)0x80010000)->arr[i]` emits index-first with a split `lui`+lo
  displacement that cse merges into any register already holding the
  constant; an extern-array symbol emits `la` (lui+addiu), not the split.
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

- **Source statement order is the main regalloc lever.** Storing
  register-held values first (`prm.type = ty; prm.user = own;`) before
  memory-chain stores frees their registers for later constants — in
  ProcItemKusuri that one swap released `$s0` for the division magic and
  collapsed a whole extra callee-saved register out of the prologue.
- For a guarded indirect call, null-check through a variable but **call
  through the field**: `ppu = item->proc; if (ppu == 0) return; ...
  item->proc(item);` — cse reuses the loaded value and allocation lands in
  `$v0`; calling through `ppu` flips it to `$v1`.
- Cached pointers that live in `$s`-registers across calls
  (`p = &item->param;`) are real source temporaries — indexing the base
  struct directly doesn't allocate the register (see ProcItemManebue).
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
- **Reused parameters vs fresh locals show in the prologue**: `move $s5,$a1`
  + later arithmetic on $s5 means the original overwrote the parameter
  (`x = x / 10;`) — the param pseudo gets a callee-saved home. A fresh local
  computes straight into the s-register with no entry move.
- **Assignment position around a branch is a double lever** (ReqItemDrop):
  `pp = ...;` placed *before* `if (it == 0) return 0;` lets reorg fill the
  `beqz` delay slot with its `addiu` (instead of collapsing the return-0
  block into the slot) *and* lengthens pp's live range, demoting its
  allocation priority so the function argument keeps `$s1`. One statement
  move fixed a register swap, a missing block, and two missing instructions.
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
- An identical dispose/cleanup sequence reached from two paths with a `j`
  into the middle of one copy is **the code written out twice** in source —
  GCC's cross-jump pass merges the common suffix (from the `jalr` backwards;
  the differing prefixes, e.g. two different `mode = 0xff` stores, stay
  separate). A shared `goto` label merges *more* than the original (loses the
  duplicated call-site setup) — don't.

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
draft and iterate. reverse.py detects the split case and points you at it. The
manual mechanics, for reference:

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

ASPSX gp-addresses only symbols **defined in the TU being assembled**;
externs are always absolute (`lui $at`/dest-reg expansion of the one-line
macro). Our per-file `maspsxGpExterns` list in `Build.hs` encodes "smalls the
original TU defined". Full story + evidence in
[toolchain.md](toolchain.md#gp-globals-making-small-globals-byte-match-solved--per-tu-opt-in).
Practical rule: a new function needing `$gp` for a symbol → add
`--gp-extern` for it in that file's list (and mirror in
`tools/permute.py` `GP_EXTERNS`); needing absolute → declare it a plain small
extern and keep the file off the list.

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
- **≤8-byte externs are -G8-small: their address is one `la` (lui+addiu into
  the SAME register); a split `lui rA,%hi / addiu rB,rA,%lo` across TWO
  registers means the original declaration was NOT small** — respell as an
  unknown-size array (`extern SVECTOR D_80097B88[];`): unknown size is
  non-small, cc1 emits separate HIGH/LO_SUM pseudos and the hi temp gets the
  first free caller-saved reg. Raw-constant casts give `lui+ori` (wrong), a
  pointer temp to a small extern still collapses to `la`
  (DebugMenuItemSet's smoke vector).
- **A callee-saved value dying at a call whose result lands in the SAME
  s-register is the call-result variable HOSTING the earlier load**:
  `h = pm->locate.coord.t[1]; gy = h; … h = GetAreaMapLevel(…, gy, …);` —
  the lw lands in h's pseudo, gy coalesces onto it, and the post-call
  `move sN,v0` reuses the register once gy dies. Loading gy directly leaves
  h caller-saved and drifts the whole tail (DebugMenuItemSet, permuter
  find; same family as the variable-reuse rule).

## Toolchain gotchas

- `-fdollars-in-identifiers` is required for anything including
  `reference/ghidra_types.h` (Ghidra `$`-names); the mod pipeline passes it.
- The canonical cc1 correctly reads the **low** halfword for
  `(short) = (int)` truncation; if you see a high-half `lhu` (offset+2),
  something reintroduced the old non-canonical binary.
