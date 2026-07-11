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

## Start from the original source's own facts

Before drafting anything, look up what the authors actually wrote. The Japanese
demo's `PSX.SYM` (see [psx-sym.md](psx-sym.md)) preserves, for 442 of the game's
functions, the **prototype with parameter names**, the **source file and line**, and
**every local variable** — name, type, and whether the compiler kept it in a register
or on the stack. `tools/matcher-prompt.py <Name>` prints all of it, plus the globals
the function touches with their original declarations.

```
- Original prototype: short DrawSprite(struct Sprite3D *sprt);   /* 3DCTRL.C:593 */
- The original locals (8):
    param $s1     struct Sprite3D * sprt
    stack sp+16   struct MATRIX mat
    reg   $s1     struct ModelType * objp
    stack sp+48   short [2] rxy
- Globals it touches:  extern struct GsOT *OTablePt;   /* 0x80097eb0 */
```

Three things this buys you:

- **The local count and their types are the single biggest lever on cc1's codegen.**
  The demo's *register allocation* may differ from retail, but how many locals there
  were and what type each had carries over. Declare those, in that order, before you
  start guessing.
- **The prototype is not a guess.** `reference/psxsym-protos.h` beats anything Ghidra
  or m2c infers. `reference/psxsym-globals.h` beats inventing a layout for `D_800BC108`
  — the original called it `struct ConflictObjectType ConflictObject[64]`.
- **TU-mates are contiguous.** `reference/psxsym-tu-map.tsv` tells you which functions
  shared a `.c` file, which pins rodata pooling and the `%gp` vs absolute choice
  (see [gp vs absolute globals](#gp-vs-absolute-globals-item-tu-vs-think-tu)).

Caveats worth internalising: the layouts are from an **earlier build**, so verify a
field offset with `tools/access.py` before relying on it; a local name repeated inside
one function is a nested-block scope, not a duplicate; and `psxsym-types.h`'s
`.NNNfake` tags were per-TU, so a struct named after one is only meaningful with its
file.

These same facts are stamped into the top of each `src/main.exe/*.c` as a
`BEGIN PSX.SYM` block (`tools/symnote.py --write --all`, regenerated in place), so you
see them whether or not you came in through a prompt. 448 of 557 files have one; the
rest are functions `PSX.SYM` never described.

If a function is still `FUN_…`, its block carries the recorded candidate name from
`reference/psxsym-candidates.tsv` — a suggestion that was not confident enough to
adopt. Do not rename on it without `tools/callmatch.py --verify`.

Parameter names are already the authors' own wherever they could be adopted
(`tools/symnote.py --params` lists the three still blocked by a name collision with a
local). Match the *locals* to the original list as you draft: that is the lever.

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

> **`autorules` and `permute.py` report their OWN score, not raw bytes.** Both
> optimise an internal objective that can move the wrong way against
> `tools/matchdiff.py`. Twice in one batch a "winning" edit scored better
> internally while being semantically wrong or literally worse under matchdiff
> (a narrow-width change in `DrawModel`/`FUN_8001b2f4`, a loop-bound change in
> `FUN_800566fc`). **Re-verify every accepted edit with `matchdiff.py`, and the
> function as a whole with `./Build check`.** Never accept a permuter candidate
> on its score alone — and bisect it: winning candidates routinely carry dead
> statements that are not load-bearing.

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
   - **When matching SEVERAL functions in one session, an EARLIER one
     (lower address) being the wrong length makes a LATER one's own
     `matchdiff`/`asmdiff` window compare against the WRONG BYTES, not just
     a bigger diff.** Both tools read the target's address straight from
     `config/symbols.main.exe.txt` and disassemble OUR build at that SAME
     fixed address; if anything before it has drifted, that address no
     longer holds the later function's actual compiled bytes — it shows a
     plausible-looking but SPURIOUS diff (a prologue instruction that looks
     "missing", garbage-looking-but-valid instructions, or gp-relative
     immediates all off by the SAME constant amount). The tell: the
     reported length is wildly short (e.g. "6 vs 63") or a data symbol's gp
     offset is off by a fixed delta consistently across every use in the
     function. Don't debug the downstream function for this — sort the
     batch by ADDRESS and fix upstream-first; once every earlier function
     in the batch reaches the target LENGTH (not necessarily byte-perfect,
     just the right size), later functions' comparisons become meaningful
     again (this batch: InitializeItem (earliest) needed fixing before
     ProcMiscSprite's diff made any sense at all).
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
     - **A guard's DELAY-SLOT FILL tie is a second named member.** When the only
       diff is which of two equally-ready, harmless-either-way instructions fills
       a guard branch's delay slot — the branch's own return-value setup (`move
       $v0,zero`) vs the fallthrough's first data-independent instruction (an
       address `lui`) — that is `reorg`, not source. StickonCheck: ~34k permuter
       iterations flat at 460, after trying guard-polarity inversion, literal
       Ghidra nesting, named temps and a `do{}while(0)` wrapper. Recognize and
       stop after one bounded run.
     - **BUT the permuter is stronger than this section's tone implies — do not
       skip it when the residual is the SAME LENGTH as the target.** It has since
       cracked: a 5-byte register tie (`FUN_800568b8`, ~400 iters), a 61-byte
       same-length schedule/colour miss (`AfsGetHeader`), and — a residual class
       worth naming — a pure **statement-order** fix (`FUN_80038c0c`: one field
       store had to sit one line *earlier* than its offset-order position, before
       the neighbouring word store, though every other store nearby was in strict
       offset order; 31 bytes). Statement order is a real, permuter-findable
       source lever, not a register swap. `autorules` reporting "no win" plus a
       right-length residual is the signal to permute, not to park.
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

- **An enum-typed dispatch parameter with all-non-negative enumerators
  compiles a range test UNSIGNED (`sltiu`) — a plain `s32`/`int` parameter
  gives `slti` instead.** `void f(struct T *m, enum TMsg msg)` with
  `if (MM_RESUME < msg)` (MM_RESUME the highest-valued enumerator, all
  values ≥0) emits `sltiu $v0,$a1,N` in the target; declaring the same
  parameter plain `s32` compiles the identical comparison to `slti` (same
  length, wrong instruction). Verified on two sibling functions in the same
  TU with the identical dispatch prologue (ProcMiscSprite, ProcMiscFire) —
  both need the parameter spelled as the real `enum` type, not just `s32`,
  to reproduce the unsigned compare cc1 picks for an enum whose range is
  provably non-negative.
- **Two independent `if (cond) goto L;` tests whose FALLTHROUGH is the
  short case is a different shape from a single if/else-if** — reach for
  it when BOTH non-fallthrough bodies are reached by a forward jump (not
  just one). `if (msg == A) goto do_a; if (B < msg) goto do_b; return;
  do_a: ...; return; do_b: ...;` reproduces a target where the SHORT
  "do nothing" case sits inline right after the tests and both real bodies
  are placed after it as jump targets — a plain `if (msg==A) {A} else if
  (B<msg) {B}` instead inlines the FIRST body (A) right after its own test
  and only jumps for the else-if chain, giving the wrong polarity for A's
  reachability (ProcMiscSprite, ProcMiscFire: MM_CREATE's body and the
  "draw" body are both forward-jump targets; the 1-in-3 `MM_DESTROY`/
  `MM_PAUSE`/`MM_RESUME` messages fall through the two tests straight to
  `return`).
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
    - **…and flip back when the SUCCESS arm is the longer one.** The literal-
      polarity rule assumes the error arm is short (a `return E;` that folds into
      the epilogue). When success is the long arm — several statements, a nested
      `if`, a handful of calls — and especially when a local is live across both
      arms, Ghidra's literal polarity compiles backwards. **Nest the long arm**
      (`if (fd != 0) { …; return buff; } error; return E;`), leaving the short
      error tail unnested and last (LoadFromCDROM). The one-line test: which arm
      would you rather have fall through into the epilogue? The short one.
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

### A boolean temp capturing a test BEFORE a narrowing adjustment is its own shape

`ch = f; hi = ch > K; if (ch > J) ch -= J2; if (hi) ch -= K2;` is genuinely distinct
from inlining the comparison *after* the adjustment (`if (ch > J) ch -= J2;
if (ch > K) ch -= K2;`). A decomp-permuter candidate may score the inlined-after
form better and still be a regression: on `FUN_800570b8` it scored 790 against the
baseline's ~1140 yet produced the wrong length (205 vs 138 bytes). Always re-verify
a permuter win with `matchdiff`.

### A "short do-nothing case falls through, both real bodies are jump targets" 3-way

When a 3-way dispatch has one trivial (do-nothing / early-return) case and two
substantial bodies both reached by forward jumps, spell it with explicit gotos to BOTH
non-trivial bodies testing the ORIGINAL conditions: `if (cond_a) goto A; if (cond_b) goto
B; return; A: …; B: …;`. Not `if/else`, and not an inline-return for the second guard --
those change which body falls through (`FUN_8004c59c`/`FUN_8004d6d4` periodic-emitter
think functions).

### A per-axis raw computation needs a temp distinct from its final assigned value

Even when Ghidra renders both roles through one reassigned SSA variable, a raw
product/subtraction (`dx*dx`, `a - b`) fed into a later result must be a SEPARATE
short-lived temp from the value ultimately assigned; reusing one variable forces the whole
computation into a callee-saved register instead of a caller-saved one (`FUN_80039ddc`).

### A dual-role dispatch test can be the opposite shape of its goto-ladder

Ghidra's `if (A == 0 && (side-effect, B == 0)) { short } else { long }` can really be
`if (A != 0) goto long; if (B == 0) goto short;` with `long` as the fall-through.
Same family as the two-independent-`goto` rule, but here BOTH bodies are
substantial — neither is a trivial do-nothing case.

### `if (cond) A; else B;` makes A the fall-through and negates `cond`

Independent of De Morgan. cc1 compiles `if (cond) A; else B;` as `if (!cond) goto B;`
with `A` falling through. So when the target's own branch tests `cond` *directly* and
branches away to the **trivial** body while the **complex** body falls through, the
source had the bodies the other way round from what you would naturally write. Spell
it `if (cond) goto trivial; <complex>; goto join; trivial: <trivial>; join:`.

This also buys the reorg trick where one branch eagerly assigns in its delay slot and
the other unconditionally overrides it on fall-through. Verified on `FUN_8003a2a8`'s
`[0, 0x4e1]` clamp: `if (t < 0) goto zero; pri = 0x4e1; if (t < 0x4e2) pri = t;
goto done; zero: pri = 0; done:` reproduces the target's `bltz` + eager `li` +
overriding `move`, where the natural `if (t < 0) pri = 0; else { … }` compiled to
`bgez` — same instructions, wrong polarity, and an extra misplaced jump.

### A guard clause with no second `return` is a plain `if`, compiled via De Morgan

`if (cond) { noreturn_call(); } <rest unconditionally>` — no `else`, no second
`return` — reproduces the target's "short fail body inline, long body reached by a
forward jump" layout. `CreateHumanoid`'s
`if (mad == 0 || Humans >= 0x28) { SystemOut(…); }` is the real shape; Ghidra renders
it backwards as `if (mad != 0 && Humans < 0x28) {…} SystemOut(…);` because there is no
second return to anchor the guard-clause exception.

### A search loop's own entry-duplicated test is the only guard needed

`if (0 < Humans) { for (i = 0; i < Humans; i++) … }` compiles the SAME `0 < Humans`
test TWICE — cc1 does not dedupe the outer `if` against the `for`'s own hoisted entry
test. Drop the outer `if` and you get one `blez` (`KillHumanoid`, and its exact twin
`GetHumanoid`).

### An `&&`-chain's body is always the fallthrough after the last test

If the target has the *opposite* body as the fallthrough, no `goto` ladder and no
restructuring that preserves the guard's polarity will fix it — the tests compile to
the same instructions either way, only the body layout differs. The single lever is
De Morgan: invert the `&&`-chain into an early-return `||` guard, which changes which
body is "the then". `CGetLevel`'s 6-way guard needed exactly this; Ghidra had rendered
the polarity backwards.

### A `return 0x80000000;`-style guard constant needs a label past the main tail

`goto ret_min;` to a label at the function's *very end*, after the normal tail.
Put the label anywhere earlier and the constant's `lui` floats up into the guard
branch's own delay slot — a 6-byte tie.

### Two-guard-then-fall-through dispatch reaches one shared return tail

`if (A) goto a; if (B) goto b; goto tail; a: …; goto tail; b: …; tail: return v;`
is what you need whenever several case bodies must converge on a *single* shared
return via cross-jump. Three separate `return EXPR;` statements each compile their
own full tail instead. (See also the shared-tails rule.)

### Guard-clause `return 0;`s all cross-jump into the LAST plain island

Every `return 0;` compiles to its own `[v0=0; j]` island, and jump2's cross-jump merges
them ALL into the last plain island in emission order. So an `if (cond) { body }
return 0;` at the end puts the shared `return 0` at the function END, while an inline
`if (cond) return 0;` guard keeps it mid-function — and which mid-function label the
guards branch to in the target tells you which form the original used. Related: a
conditional store block falling into a shared `return 0` — `if (c) { stores } return 0;`
vs `if (!c) return 0; stores; return 0;` — differ ONLY in basic-block boundaries:
same-block `v0=0` gets hoisted by sched1 into a load-delay hole (inline `move v0,zero` +
direct `j` to the epilogue), separate-block `v0=0` survives for cross-jump (`j` to a
return-0 island). Both shapes can coexist in one function (`HangCheck`).

### A guard returning the constant its own test produced wants a bare `goto`

`if (cond) return X;` followed by more code, where `X` is a compile-time constant
**equal to the value the guard's own test left in `$v0`**, compiles one instruction
too long: the literal `return X;` forces a `li $v0,X`. Write
`if (cond) goto end; ... end: return;` instead — the pre-existing test register
carries the value for free. Sharing one label across several such guards lets each
reach it with a single branch (`ItemUse`'s `item[4]==0` and `d >= 300` guards).

## Loops

### A for-loop's duplicated entry test branching to a shared `return K` is a folded guard

Write `i = 0; if (i >= N) goto ret_k;` immediately before `for (; i < N; i++)`. cse1's
`record_jump_equiv` records the guard's comparison on its FIRST slt operand's quantity,
so the guard MUST compare the INDEX against N — `if (N < 1)` records on N's quantity and
never folds (both `blez`s survive, a real trap). The loop must stay a real `for`: its
`NOTE_INSN_LOOP_VTOP` is what makes reorg predict the loop-internal skip branch taken and
duplicate the increment (`addiu v0,a2,1`) into both skip delay slots; a `do`/`while`
loses those fills (`GetConflictResult`, the deepest of its three residuals).

### A success `return X` keeping its own `jr` needs a zero-byte blocker before function end

When the target's success path has its own `jr` with the value in the delay slot (rather
than jumping to a shared epilogue), nest the entry guard —
`if (id != -1) { … } return -1;` — so the final `return -1` is the guard's else. cse
elides its `li` along the taken edge (single-pred label), and the surviving bare
CODE_LABEL blocks jump.c from deleting the success return's jump-to-next, letting reorg's
`make_return_insns` convert it to its own return and fill the slot
(`GetConflictResult`; DeleteConflict's own style).



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
- **A `short` loop counter SUPPRESSES loop.c's strength reduction.** loop.c needs
  a plain SImode giv to rewrite `arr[i].f` into a walking induction pointer, so an
  `int i` gets you `p += 8` per iteration. A `short i` instead keeps the target's
  recompute-from-base shape — the counter's own sign-extend fuses with the scale
  (`(i<<16)>>13` for an 8-byte element), and the address is rebuilt as
  `base + i*8` every iteration. When the target recomputes but your draft walks a
  pointer, narrow the counter before touching anything else (SetupMotionRegist).
  (Not to be confused with the *3-instruction* GetPad sign-extension class.)
- **Index the table (`T[i].f`) rather than walking a pointer (`e++`) when the
  loop touches two or more fields.** With a walking pointer cc1 strength-reduces
  the induction variable so that the LAST field it touches sits at offset 0 —
  the base comes out biased (`addiu $a2,$v0,8`, accesses at `-4($a2)`/`0($a2)`).
  Array indexing keeps the giv at the table base, so the accesses stay at their
  natural `4($a2)`/`8($a2)` (FUN_800568b8; the `addiu $a2,$a2,0xC` bottom
  increment is identical either way, so the *bias* is the only tell).
  - **At 3+ touched offsets from one base, a walking pointer doesn't just bias
    — it can split into a whole SECOND parallel induction register**, one
    walking pointer materializing a constant offset (e.g. `entry+5`) to serve
    two of the offsets while the original walking pointer serves the third,
    both incrementing by the same stride every iteration (confirmed: every
    walking-pointer spelling tried — plain pointer, byte-cast, named struct
    field — reproduced the identical extra register). The fix is the same
    lever, just needed harder: index by the loop's OWN counter variable
    (`T[i].f`, reusing whatever `int i` already drives the loop-exit test, not
    a separately incremented cursor) so cc1 strength-reduces to exactly ONE
    walking pointer no matter how many offsets/fields you touch off it
    (FUN_8004a6bc's `D_8008E4B4[idx]` table, touched at offsets 0/4/5).
  - **Neither shape may be perfect when the loop ALSO conditionally
    dispatches through a field.** DoMiscProc's 200-slot cull loop touches
    five fields (`proc`,`x`,`y`,`z`,`pause`) with two indirect calls through
    `proc`: a raw walking pointer reproduces the SAME bias this rule warns
    about (the last-written field, `pause`@0x14, biases the base by +0x14
    — confirmed by direct trial, 117 vs 113 target instructions), while
    array indexing (`misc[i].field`) avoids the bias but degrades the
    loop's OWN exit test from the target's plain counter compare
    (`slti v0,s1,200`) to a pointer-vs-pointer bound check
    (`addiu v0,base,7200; slt v0,cursor,v0`) plus an extra register (cc1
    copies the array base into a second register to serve as the walking
    cursor once a null-checked field access forces materialization). A
    third attempt — caching `T *p = &arr[i];` just inside the null-check,
    narrowing p's scope to avoid the raw-pointer bias — fixed the exit test
    back to a counter but added its own extra copy (preserving `p` across
    the call), netting worse (115 vs 113). Parked NON_MATCHING; array
    indexing was the best of the three (right instruction COUNT, residual
    confined to register choice) — try this cache-inside-the-if idea again
    with a LOWER register-pressure sibling before assuming it never works.
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

- **A `while (1) { … break; }` search loop whose give-up path takes the address
  of a global (`return &dmy;`) gets that `la` hoisted into the preheader by
  loop.c's invariant motion** — the address is loop-invariant, so loop.c lifts
  it above the loop even though the target only materialises it at its own use,
  on the give-up path. There is no source spelling of the loop that suppresses
  this. Hand-roll the loop instead (`top: … goto top;`) so loop.c never sees a
  `NOTE_INSN_LOOP_BEG` and does no invariant motion at all. Applies even when the
  loop is an otherwise ordinary top-tested pool search (SetFrame/SetSplash/
  SetBleed, the `EffectSlot[200]` scan).

  **But check where the give-up fallback actually lives before reaching for the
  hand-rolled `goto`.** The hazard above exists only when the fallback (`ef = &dmy;`)
  sits *inside* the loop body, giving loop.c an invariant `la` to hoist. When it sits
  *after* the loop — `SetImpact`, `SetExplosion`, same `EffectSlot[200]` pool — there
  is nothing to hoist, and only a genuine bottom-tested `do { … } while (count < 200);`
  reproduces the target's `idx++`/`idx--` delay-slot-steal-and-compensate pattern; the
  hand-rolled `goto` form does not. Two valid shapes, selected by fallback placement:
  do not assume the family's established idiom carries over.

- **You cannot flip a `break`'s branch polarity by negating the test.** `if (c)
  break;` and `if (!c) { … } break;` canonicalise to byte-identical code — cc1's
  break/continue-to-loop-exit machinery erases the difference (verified: the
  rewrite produced a 0-byte diff). The only lever is *where the work lives*: to
  make the `break` the fallthrough rather than the branch-away, move the real
  work for the other case bodily **inside** the `if` block, ahead of the `break`.

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
  a0–a3 set in the raw `.s`, never m2c's argument list alone. NB this is about
  m2c's rendering of a *call site*, not the callee's own arity: `AdtMessageBox`
  itself is plainly `void AdtMessageBox(char *fmt, ...)`, and all 87 call sites
  already declare it so.
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

**`x & (1 << n)` for non-constant `n` always canonicalises to `(x >> n) & 1`**
(`srav`+`andi`), never the literal `sllv`+`and` — verified standalone against the
pinned cc1. If the target has the literal shape, name the shifted mask first:
`int mask = 1 << n; if (x & mask)`.

**A `<<2` scaling of a truncated difference must be a separately-reused variable's
own second use**, or fpeephole fuses the truncating `sra` and the shift into one
instruction — leaving you one instruction short of the target.

**Increment-first beats read-one-ahead, even for a plain forward scan.**
`while (pad != -1) { point++; pad = point->pad; }` is two instructions shorter than
`while (pad != -1) { pad = point[1].pad; point++; }` — the one-ahead form needs a
`+12`/`-12` compensation pair the increment-first form never creates
(`SetupTraceLine`).

**A narrow (`short`) accumulator can be right for an arithmetic ladder whose value is
only narrowed at the end.** `short t` rather than `s32 t` removed both a spurious
`andi 0xffff` and a whole register-allocation cascade in `ControlTraceLine`'s degree
ladder.

**A field compared then re-read inside the hit body wants a named temp captured via a
comma expression, to preserve `&&` short-circuit.** `if (cond && (z = lv.vz, z < dist))
{ dist = z; … }` gives both the short-circuit AND one load: a bare re-read
(`… && lv.vz < dist; … dist = lv.vz;`) forces a second stack reload (the `slt` clobbers
the compare's register), and an unconditional `z = lv.vz;` before the `if` reads the
field even when `cond` is false (2 extra insns). Only the assignment INSIDE the RHS
operand gives both (`SearchItemTarget2`; and its hit body's `dist = z;` must precede the
struct copy `*target = tv;`, or the copy's temps schedule first and swap a move pair).

**A callee that really returns `s16` can be declared `extern u16 f()` per-TU so the
caller tests its sign as `(f() << 16) < 0`.** u16 promotes to int, so bit 15 lands in
bit 31 for a plain `sll`+`bltz` with no `sra` (the shifted value is never reused). Same
per-TU return-type lever as GetRealPad, here applied to GetMotionID's not-found
sentinel (`JumpControl`).

**The EffectSlot pool-scan `slot = base + idx` addu is index-first ONLY without an
outer loop.** Single-shot spawners (SetImpact/SetExplosion/SetBleed) emit it index-first
(`addu dst, scaled_idx, base`). The moment the same expression runs inside an outer
count loop with `base` loop-invariant callee-saved, cc1 canonicalises the invariant
operand FIRST (`addu dst, base, scaled_idx`) — a post-`fold` RTL operand order with NO
source lever: `base+idx`, `idx+base`, `&base[idx]` all fold identical, and moving `base`
in/out of the loop changes the LENGTH instead. A 1-instruction, same-length,
permuter-immune sub-C tie (SetBlood, SetHinoko). For such an outer-loop spawner, use
`while (1) { if (!(i < n)) break; …; i++; }` (NOT a hand-rolled goto) to place `base` in
the target's callee-saved register — SetHinoko's goto form shifted every param register
up one (55-byte cascade); the while/break form collapsed it to the 2-byte addu tie.

**`a - (C - b)` and `a - (b - C)` survive fold; `a - C + b` gets reassociated.**
split_tree builds `a - (C - b)` as `(a - C) + b` and `a - (b - C)` as `(a + C) - b`
(it builds, does not refold), so the constant folds into an existing operand. The naive
`a - C + b` is reassociated to `a + (b - C)`, whose `b - C` is an INDEPENDENT instruction
that drifts into a delay slot — one off. Spell the subtraction with the parenthesised
`C - b` / `b - C` grouping the target's `addiu`+`addu`/`subu` pair implies (`HangCheck`:
`dtL->vy - (0x69 - y)` → `addiu vy,-105; addu +y`).

**Two loop-invariant, mutually-independent assignments that must land in a specific
relative order: cc1's list scheduler breaks the tie by RTL-creation (UID) order, which
follows source scan order.** To force one earlier, capture whichever must come first as
an explicit statement before the other — hoisting a dispatch comparison into an
unconditional boolean, `found = (a == b); k = 1; if (found) …`, gives the comparison's
widen (`sll/sra`) a lower UID than `k = 1`, ordering it first. Zero semantic effect, pure
UID control (`SetCommand`). Related loop lever: the bottom `i++` must be the LAST body
statement (after the whole `if`), so reorg hoists it into the entry-comparison branch's
delay slot.

**An address-taken local's loads never schedule above ANY pointer store** (the scheduler
is alias-conservative: a true dep to every store). If the target reads such a local
BETWEEN two field stores, the source statement order put that read earlier — reorder the
STATEMENTS, do not chase the scheduler (`HangCheck`'s vx/vz-then-vy commit order: `vect`
escaped, so its `lh` reads carry deps on every earlier `*dtL` store, and storing vy last
lets the vz read fill the load-delay hole with no hazard nop).

**`x++` (postfix) is the general spelling for "test the OLD value; the incremented value
is stored unconditionally because the other path overwrites it anyway."**
`if (field++ < K) return; field = 0;` reproduces a single-load + delay-slot-store shape;
an intermediate temp (`v = field; if (v < K) field = v+1;`) sometimes fails to reach the
fused form.

**A captured field copy used later in arithmetic must be declared `s32`, not the field's
narrow width.** `s16 size = param->size;` triggers cc1's narrowing-store-loads-`lhu` rule
even when `size` is later used in signed arithmetic, costing a spurious `sll/sra`
re-widen. Declare the capturing local `s32` (`DrawFrame`'s size, `DrawSmoke`'s vz_old).

**Pointer arithmetic normalises to base+index; only INTEGER addition keeps operand
order.** `p = (SVECTOR *)(idx * 0x20 + (s32)tbl);` emits `addu p,index,base`, whereas
`tbl[i]`, `&tbl[i][0]` and `(u8 *)tbl + n` all emit `addu p,base,index`. When the
target's `addu` has the index first, spell the address as an explicit integer sum,
not an array subscript (`SetCameraMode`).

**A `slti/xori/bnez` (set-condition + branch) success test is a NAMED flag variable.**
`hitf = call(...) > 0x7ff; if (hitf) goto hit;` compiles the three-instruction
scc+branch; the inline `if (call(...) > 0x7ff)` compiles a two-instruction
`slti/beqz` and is one short (`SetCameraMode`).

**Serialised stores/loads at absolute scratchpad addresses are small-extern SYMBOL
accesses, not flat pointer casts.** A `<= -G8` extern's store is the one-op `$at`
macro AND is `MEM_IN_STRUCT_P`; gcc 2.8.1 `sched.c`'s `true_dependence` MEM-in-struct
heuristic then dismisses the store→load dependence between a non-struct fixed-address
store and a struct varying-address load, so a flat `*(s16 *)0x1F8000xx` cast lets
sched1 wrongly batch the following struct loads past the stores, while a
struct-pointer cast loses the constant-address macro (base forced into a register).
Bind the symbol in `config/symbols.<target>.txt` and access it as a plain extern; GTE
CALL arguments, by contrast, stay literal casts (`SetCameraMode`'s scratchpad
rot/trans tables).

**Two divisions in one function must be computed back-to-back before either's store
block.** Interleaving store-then-next-division puts stores BETWEEN two reads of the same
field, invalidating cse1's cached load and forcing a needless reload+move
(`UpdateSplineControl`; matches Ghidra's literal order). And a store destined for a
branch's delay slot must be written BEFORE the load whose result feeds the next test --
an intervening store between a load and its first use blocks combine from fusing
`lhu`+sign-extend into `lh` (`AttackContinuousCheck`).

**Several divisions by the SAME runtime divisor compile eagerly, back-to-back,
before any intervening call** — even where Ghidra renders one lazily folded into a
later call's argument (`f(a, b / d)`). `IsVisible`'s three divisions by one variable
`d` all execute before either `abs()` that consumes them. Give each quotient its own
named temp and compute them all, in argument order, before the first consuming call.

**A target's "redundant-looking" extra reload means the source used a fresh field
dereference, not a cached pointer.** `vrealloc` writes `vh.next = vhp->next;` even
though a `nb` variable computed moments earlier holds the numerically identical
address; spelling it `vh.next = nb;` drops the reload the target has, because cc1
will not refetch what it can already see live in a register. Provable equality is not
a licence to reuse the variable.

**A value used ONLY as a conditional call argument must be the inline ternary in the
call's argument position**, not assigned to a named local first (even via a ternary).
Assigning it first makes cc1 re-read the tested field a second time, with the wrong
signedness -- three extra instructions in `AVCameraSetup`'s `ordr`.

**A comparison's operand order is an instruction-order lever, not a sched tie.**
expand evaluates op0 first, so `found > mem` emits the (short) sign-extension `sll`s
BEFORE the `lh`, while `mem < found` loads first — same `slt` either way. Order the
comparison to match which side's evaluation the target front-loads (`GetConflictResult`,
8 bytes).

**Put a pointer-field publish (`Global = p->field;`) FIRST in a multi-field store
group.** sched1 then sinks its `lw`/`sw` into the following pairs' load-delay slots one
group at a time; placing it mid-group leaves a `nop` in the first slot and drags the
`sw` below the next `subu` (`GetConflictResult`; made an old `do{}while(0)` hack
unnecessary).

**Two stores of the same struct field to DIFFERENT objects, even with zero
intervening statements, need a named temp for a single shared load** — if the struct's
address has already escaped (passed to a callee earlier), cc1 stops CSEing repeat reads
of it even across adjacent statements. `sx = out.vx; sp2->x = sx; sp1->x = sx;`
(`FUN_80039fb0`). The clamp counterpart: a clamp's final expression must re-read the
struct field directly (`scr.vz`) rather than reuse an already-live local, because the
target's asm shows a fresh `lh` reload there even at a numerically-identical value
(`FUN_8003a148`).

**Ghidra's own SSA rendering tells you when the target reloads.** If Ghidra shows a
second, separately-named dereference of an address instead of reusing its earlier
temp, the target really does refetch: reusing your cached local there costs an
instruction (`CVArun`'s `GsSortSprite` re-read, `CVAsetup`'s post-loop nudge). This
is the Ghidra-side tell for the `vrealloc` fresh-field-dereference rule above.

**`GsSortSprite`'s `int pri` argument needs an explicit `(u16)` cast at the call
site** to reproduce the `andi $a2,$a2,0xffff` that sits in the `jal`'s delay slot.

**A pure narrowing struct-field copy uses `lhu`/`lbu` even for signed fields.** A
field read and immediately written back at the same width, with no arithmetic in
between, loads unsigned regardless of the field's signedness — the sign bits can't
matter. Do not fight it with named temps or casts.

## Stack objects

**Two same-type address-taken stack objects get their slots by REFERENCE order, not
declaration order.** When a memset scratch VECTOR and its struct-copy destination
(passed to a call) land in swapped stack slots vs the target, swapping the
DECLARATIONS does nothing — swap which one is memset'd and which is the call argument.
`leLayoutEnemy`: making `pos` the memset scratch and `tmp` the SetBleeds argument put
`pos@sp+24`/`tmp@sp+40` to match, cutting the diff in half.

**cc1 pads EACH separately-declared aggregate stack local up to a multiple of 8
bytes.** Four adjacent aggregates that were really contiguous fields of one original
struct must be declared as ONE combined struct local, or each is individually
rounded (`DRAWENV` 0x5c → 0x60, `DISPENV` 0x14 → 0x18), inflating the frame and
shifting every later offset — even though each was already 4-byte aligned. Field
offsets *inside* one struct follow plain member alignment, with no such rounding
(`AdtMessageBox`).

**A varargs argument pointer must be computed lazily, at the call.** Writing
`s32 *args = (s32 *)((char *)&fmt + sizeof(fmt));` as a named local at function
entry forces cc1 to hold the address in a callee-saved register across the whole
function when the vararg call is far away — an extra register and 8 bytes of frame.
Inline the expression at the call site instead and cc1 computes it with a
caller-saved temp. (`FUN_8005fe38` wants the named-local form because its call is
near entry; `AdtMessageBox` wants the inline form.)

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
  - **This holds for a member of a parameter union too, even when Ghidra renders
    it as `long`-by-`long` temp copies.** `ef->param.bleed.pos = *pos;` (VECTOR,
    align 4) and `.vec = *vec;` (SVECTOR, align 2) reproduce the target's batched
    load-then-batched-store block move exactly; the field-at-a-time transcription
    does not (SetBleed).
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
- **A TU sibling can mislead: two functions in the same TU touching the same
  region need not share an addressing shape.** `FUN_80039544` reaches the
  scratchpad through flat per-statement `*(T *)0x1F8000xx` casts (a repeated
  `lui $at` per store) while its twins `FUN_800396c0`/`FUN_80039684` cache a
  `MATRIX *`/`SVECTOR *` local. Check the raw `.s` for the repeated `lui $at`
  before importing a twin's cached-pointer idiom.
- **One C variable is ONE pseudo for the whole function, not one per
  occurrence.** So a value playing the same role in two mutually-exclusive
  switch-case bodies must reuse the SAME C variable across the cases when the
  target reuses the same hard register in both. Pick the variable to reuse by
  matching the ROLE across cases: reusing a *different* already-used variable
  (one tied to another role) drags that case's own allocation off-target too
  (EquipWeapon's `a` = "the `wp[0]` value" in both the 4-swap and 2-swap cases).
- **Array spelling picks the addu operand order**: `p->arr[i]` emits
  `addu base,index`; `(&p->arr[0])[i]` emits `addu index,base`;
  `((T *)0x80010000)->arr[i]` emits index-first with a split `lui`+lo
  displacement that cse merges into any register already holding the
  constant; an extern-array symbol emits `la` (lui+addiu), not the split.
  - **The `(&p->arr[0])[i]` respelling only works on a struct-member array
    reached THROUGH A POINTER.** It has zero effect on a top-level
    `extern T arr[N]` indexed directly — verified by trying it on `leSetEnemy`'s
    identical-looking tie after it had fixed `leAddPath`. Don't reach for it when
    the base is a plain extern array; the tie there is local-allocation ordering.
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

## Reading cc1's RTL dumps (the escalation method)

**When a draft is the CORRECT LENGTH but a handful of bytes differ, and both C
respelling and a bounded permuter run have failed, STOP guessing and read the RTL.**
Every such "permuter-immune, below-the-C-level" residual this project has escalated —
nine in a row — turned out to be *reachable source structure* that the dumps name
outright: a guard's operand order, a statement's position, a block boundary, a `fold`
reassociation, a loop.c hoist threshold. The permuter can't find these (it transforms
the C AST; the divergence is chosen in a later RTL pass), and blind respelling is a
lottery. The dump tells you which pass diverged and why, so you solve for the fix.

This is not a Fable-only skill — it is a mechanical procedure. Any agent can do it.

**Re-verify a park header's "permuter-immune" / "below-the-C-level" verdict before
trusting it — the header can be WRONG.** `FUN_8004a368` was parked as "the same
PrepareAccess/cd_open la-reload tie" and closed in minutes once someone checked it
against a DOCUMENTED rule instead: it was the offset-0-alias-vs-struct-member `%hi`
lever (grep the target address's OTHER name in sibling TU files — 0x80089f00 is both
`CURRENTLY_SELECTED_CHARACTER_STATE_PTR` and `CamState.Owner`, and the two spellings
choose different `%hi` register homes). Before spending an RTL dive or accepting a park,
match the residual against the cookbook's named classes first.

### The tool

`tools/rtldump.py <Name>` compiles `src/main.exe/<Name>.c` exactly as `./Build` does
(same cpp defines, same cc1-281 flags) but STANDALONE in the scratchpad — no Shake, no
`.shake/` database, so it never races a concurrent build and each run is ~1 s. It writes
the requested RTL dump passes next to the `.s`:

```
tools/rtldump.py <Name>                 # greg, lreg, jump, combine (the usual four)
tools/rtldump.py <Name> --pass loop     # loop.c invariant hoisting
tools/rtldump.py <Name> --pass all      # every pass (-da): cse, flow, sched, dbr, …
tools/rtldump.py <Name> --draft         # the #else NON_MATCHING draft
```

Pair it with a scoring loop to make each hypothesis a ~5-second test: edit the `.c`,
`NON_MATCHING=<Name> ./Build` (or a standalone `cc1-281 | maspsx | as | objdump`),
`tools/asmdiff.py <Name> -n`. Read the dump ONCE to form the hypothesis; iterate with
the fast loop, not by re-dumping.

### What each pass tells you

| dump | pass | read it for |
|---|---|---|
| `.greg` | global-alloc (`-dg`) | `N in H` = pseudo N landed in hard reg H; `N preferences: H`; `N conflicts: …`. THE register-tie dump. |
| `.lreg` | local-alloc (`-dl`) | per-block allocation before global; where a copy chain starts. |
| `.loop` | loop opt (`-dL`) | biv/giv analysis with `benefit`/`used`/`lifetime` — the invariant-hoist threshold economy (see its own cookbook section). |
| `.combine` | combine (`-dc`) | where an `addu`/`subu` operand order or a `fold` reassociation is fixed. |
| `.cse`/`.cse2` | CSE (`-ds`) | which repeat load/address got folded to a copy; `record_jump_equiv` guard folds. cse1 stops at `NOTE_INSN_LOOP_END`, cse2 does not. |
| `.jump2` | jump opt (`-dj`) | cross-jump merges, `make_return_insns`, which `return K` island survives. |
| `.sched`/`.sched2` | scheduling (`-dS`) | load-delay/latency fills; why a `nop` or an independent insn lands in a slot. |
| `.dbr` | reorg (`-dd`/`-dR`) | delay-slot branch fills; `mostly_true_jump` prediction (needs `NOTE_INSN_LOOP_VTOP` from a real `for`). |

Hard reg numbers: `0` zero, `1` at, `2` v0, `3` v1, `4`–`7` a0–a3, `8`–`15` t0–t7,
`16`–`23` s0–s7, `24`–`25` t8–t9, `28` gp, `29` sp, `30` fp/s8, `31` ra.

### The procedure

1. **Locate the diverging instruction** with `tools/matchdiff.py`/`asmdiff.py`, and which
   register or offset differs (e.g. our `$v1` vs target `$v0`, or a store at gp+28 vs
   gp+32, or a missing/extra instruction).
2. **Dump the pass that owns that decision.** A wrong register home → `.greg`
   (`N in H`, `N conflicts`, `N preferences`). An extra/missing loop instruction →
   `.loop`. An operand-order or fold difference → `.combine`. A delay-slot `nop` →
   `.sched2`/`.dbr`. A `return`/branch-target difference → `.jump2`/`.cse`.
3. **Read what the pass DECIDED and work backward to the source construct** that forced
   it. The load-bearing invariants of THIS cc1 (2.8.1), all confirmed against its
   sources this session:
   - **A pseudo's live range is NEVER split.** One C variable is exactly one hard reg
     for its whole life. So two roles sharing one hard reg = ONE variable doing double
     duty; one value appearing in two hard regs across disjoint regions = TWO variables.
   - **`global.c`'s `find_reg`** makes an earlier (higher-priority) allocno AVOID a reg a
     later allocno prefers — donate a call-arg preference to steer the high-priority one.
   - Allocno **priority sums refs and live-length across the whole range** and moves in
     `floor_log2` jumps, so merging two disjoint same-role temps into one variable can
     vault it over a rival (vfree).
   - **`loop.c` hoisting is a threshold economy** — its own section.
   - **`fold`/`combine`** fixes operand order and reassociation: `a - (C - b)` survives
     as `(a-C)+b`, `a - C + b` reassociates; a comparison evaluates op0 first.
   - **`cse` + `jump2`** decide `return`-island merges and guard-test folds; block
     boundaries (a nested `if` vs a wrapper, a label between two blocks) are the lever.
   - **`sched`/`reorg`** are alias-conservative: an address-taken local's load never
     schedules above ANY pointer store — reorder STATEMENTS, don't chase the scheduler.
4. **Form ONE hypothesis, test it with the fast scoring loop, repeat.** The worked
   examples: vrealloc, vfree, valloc, Sound, turn_towards_player_, UpdateMotion,
   HangCheck, GetConflictResult, SetSmoke — read their file headers for the exact
   dump-to-source reasoning on each.

## Register allocation steering

### A default-then-override temp keeps the object's address alive

If your draft is **N instructions SHORT** and the missing instructions are an
address recomputation (`lui`/`addiu` + the `sll`/`addu` index scaling) for an
lvalue you already stored to earlier, look for a temp feeding that store:

```c
cnt = 100;                     /* or: if (h->life == 0) cnt = 100; else cnt = 300; */
if (cond) cnt = 300;
LifeBar[g].count = cnt;        /* one store, address pseudo now live... */
if (LifeBar[g].max < 1) { … }  /* ...so cc1 REUSES it here. Target recomputes. */
```

The shared temp keeps `&LifeBar[g]` live in a register across the following
access. Write the two arms as **independent stores** instead:

```c
if (h->life == 0) LifeBar[g].count = 100;
else              LifeBar[g].count = 300;
```

cc1 cross-jump-merges those into the *identical single store*, but the later
`LifeBar[g]` access is now a genuinely fresh pseudo and gets recomputed from
scratch — base and index registers swapped, exactly as the target does
(ReqLifeBar; 9 instructions / 36 bytes recovered). Verified empirically; the
`cse.c` mechanism that declines to merge the second occurrence is not root-caused,
so treat this as a shape to try, not a law.

### gcc 2.8.1 never splits a pseudo's live range: one C variable = one hard register

For its whole life. So when the target shows one *logical* value in two different
registers across disjoint regions, the source had **two variables**. And when two
different-looking roles share one register across disjoint regions, it was **ONE
variable doing double duty** — and its call-argument copy-preference travels with it
into the other region. (`vrealloc`: the grow-branch mask and the memcpy length are
the same variable.)

### A promoted `short` parameter is TWO pseudos for free

cc1 gives a promoted `short` parameter two pseudos with no source-level copy: the
raw SImode argument-register copy, and the HImode declared variable. So a target
whose early tests read the raw arg register (`$a1`) while one later use reads a
copied register (`move $v1,$a1` in a branch delay slot) needs NO explicit copy
variable in the source — it needs the USES split per path. Two literal
`return SoundEx(...)` calls rather than one shared call let cse serve the early
uses from the SImode copy while only the divergent-path use reads the variable;
cross-jump re-merges the identical `sll/sra/jal` tails. Refines the
"one logical value in two registers = two variables" rule: the second "variable"
can be cc1's own parameter split (`Sound`).

### A negative non-power-of-two multiply is spelled as its strength reduction

`sync * -0xF0` must be written `((sync - (sync << 4)) << 4) - 0xA` (the shift/sub
sequence cc1's own strength reduction emits) to match; the literal `* -0xF0` compiles
differently. And a `(u16)x << 1` widening needs a `u32` intermediate — a `u16` one
collapses the double-shift to a single `sll` (`EndDrawing`).

### A narrowing store through a widened temp differs by a reload's signedness

`dp = sk - (u16)DrawingPage; DrawingPage = dp;` (with `sk` an `s16`, `dp` an `s32`)
matches, where the direct `DrawingPage = sk - (u16)DrawingPage;` reads as a narrowing
use and lets cc1 satisfy it with a wrongly-unsigned `lhu` reload; spelling `sk` as
`s32` instead lets cse substitute a live constant. One instruction off in opposite
directions (`EndDrawing`).

### A `(short)` cast on an int `|` call argument is a SCHEDULER lever

Not just an instruction-shape lever. `(short)(seid | human->sound)` versus the
uncast `int` expression changes where the truncating `sll/sra` sits: uncast,
extend-then-or shortens the argument's dependence chain, and sched1 then floats a
SIBLING argument's load (`$a0 = human->locate`) above it — a hard-`$a0` def while
the base pointer is still live, which evicts the base from `$a0` and costs an
instruction plus a whole-image shift. The cast (or-then-extend) restores the chain
depth so the eviction never happens. When a two-call restructuring seems to
"repartition `$a0`/`$a1` wholesale", check the argument expression's TYPE before
abandoning the structure (`Sound`; this is why an earlier two-call attempt looked
worse and was wrongly parked).

### A conditional min/max against a MEMORY operand must be the ternary

`i = (a < b) ? a : b;` reaches fold/expr.c's min path, which loads both operands into
SI registers and makes the conditional arm a register MOVE. The two-statement
`x = a; if (b < x) x = b;` re-expands the arm's memory read in the DESTINATION's narrow
mode (HImode for an `s16` local), which cannot CSE with the compare's SImode load — a
reload plus a full coloring cascade. On `UpdateMotion` this one spelling change
(`i = (mmp->motion->n < mmp->model->n) ? mmp->motion->n : mmp->model->n;`) fixed 22 of
26 residual bytes. This SUPERSEDES the older "min/clamp reload is a coloring tie, park
it" note below for the memory-operand case — it IS reachable from C, via the ternary.

### Min/clamp reload-vs-reuse is a coloring tie, not a source bug

`x = a; if (b < x) x = b;` where the target reuses the already-loaded `b` in the
assignment (`move x,b`) but our cc1 RELOADS it (a second `lbu`/`lh` in the branch)
is a register-coloring tie, not something the C controls. It happens when cc1 keeps
the POINTER to `b` alive into the assign-branch, freeing `b`'s value register for
the compare result so it must be reloaded. Permuter-immune. Do not "fix" it with an
explicit temp for `b` — a fresh local adds a move and shifts the frame; field-vs-
local and counter-reuse are all identical or worse. Park after one bounded permuter
run (`UpdateMotion`'s `min(model->n, motion->n)` — one such decision cascaded into
all 13 of its diffs).

### Reuse a DYING variable to carry a later call-crossing value (coalesce upward)

The inverse of the usual "reduce callee-saved usage" levers. Sometimes the TARGET
uses one MORE callee-saved register than your draft, because it coalesces two values
into one register across a call. When a call result dies exactly where a
call-crossing value is born, spell the second value by REUSING the dying variable,
not as a fresh local: `dist = SquareRoot0(...); ...; dist = 0x7f - (dist<<7)/18000;
dist = (dist*(10000-dy))/10000; vol = (angle<<8) | dist;` coalesces `dist` and the
volume into one `$s1` (frame 0x30), where a separate `vol` local keeps `dist`
caller-saved in a distinct register and spends an EXTRA callee-saved one (frame 0x28)
— rotating the whole register file (`SoundEx`: 61 diff lines → 5). Reach for this
when the target's frame is LARGER than yours and a call result dies near a
call-surviving definition.

### An offset-0 alias vs the enclosing struct member is a `%hi`-register lever

Under -msplit-addresses, an offset-0 alias symbol (`ALIAS->field`, ALIAS at the struct
base) folds `%hi` into the destination register (`lui $a2; lw $a2,0($a2)`), while
reaching the same pointer as a NONZERO member of its enclosing struct
(`CamState.Owner`, +0x10) splits the base into a separate register
(`lui $v1; lw $a2,0x10($v1)`). When a pointer-load reload tie (`$v1` vs the dest for
`%hi`) is the only residual, switch between the alias spelling and the struct-member
spelling (`CheckCheatCodes`, last 2 bytes).

### A shared `f(0, x)` tail vs two explicit `f(0, lit)` calls place the const arg differently

A shared `f(0, x)` after an if/else hoists one `$a0 = 0` into the merged call's delay
slot; writing TWO explicit per-branch `f(0, literal)` calls keeps `$a0 = 0` in each
predecessor (cross-jump merges only the common `jal` tail, leaving a `nop` delay slot).
Use the two-explicit-calls form when the target materialises a shared constant argument
in each predecessor (`CheckCheatCodes`: 17-byte diff → 2).

### An abs assigned to a variable reaches `abssi2` only as the GE ternary

`dy = (raw >= 0) ? raw : -raw;` folds to ABS_EXPR and expands to ONE `abssi2` insn
writing `dy` (`bgez raw,1f; move dy,raw; negu dy,dy` — negu of the COPY, `raw` dies at
the abs), IN ANY CONTEXT, and the result CAN be a reusable variable. The LT spelling
`(raw < 0) ? -raw : raw` NEVER folds: fold-const.c's COND_EXPR inversion swaps the arms
but then re-binds `arg1` from the already-swapped tree, so the abs check's
`operand_equal_for_comparison_p` misses and expansion takes the branchy singleton path
(`negu dst,src` — the extra-instruction residual). So: spell the abs ternary **GE**.
This supersedes the older "the abs `negu` source is not a C-level choice" note AND the
"reach abssi2 only inline in a comparison" refinement — it is reachable as a plain
`x = (a>=0)?a:-a;` assignment (`SoundEx`, `UpdateMotion`).

### A conditional store via a pre-branch address copy is one assignment, conditional RHS

`move $v0,$a0` in a branch delay slot feeding `sh …,0($v0)` means the source is ONE
assignment with a conditional right-hand side (`xyz[j] = (cond) ? (t += K) : (t -= K);`),
not two stores. expand_assignment computes the destination address BEFORE expanding the
ternary, and cse folds the recompute into a copy of the earlier load's address register.
Use compound-assignment arms (`t += K`) to keep the value updates in place, and the
ternary's condition may need to re-read the target memory (`xyz[j] < 0` rather than the
equivalent `t < 0`) to keep cse1's taken-path window from canonicalising the store back
onto the original address register (`UpdateMotion`, last 4 bytes).

### The abs idiom's `negu` source register is not a C-level choice

`at = t; if (t < 0) at = -at;` and `at = (t < 0) ? -t : t;` compile identically:
cc1 folds `-at` back to `negu <dst>,<t_reg>`, negating the original `t`, not the
copy. Which register `negu` sources from is a coloring artifact — do not chase it
by respelling the conditional (`UpdateMotion`).

### `arr[idx]` on a top-level extern array expands base-first (ARRAY_REF), not mult-first

An address computation reused across N field stores of `arr[idx]` on a TOP-LEVEL extern
array (a single index, not a compound `arr[a+b*c]` subscript) takes ARRAY_REF's own
base-first expansion, NOT the `EXPAND_SUM` mult-first special case (which fires only for
compound subscripts). So the base symbol's `high`/`lo_sum` gets a LOWER insn UID than the
`idx*stride` offset chain, and local-alloc hands the base first pick of the low scratch
registers -- possibly reversed from the target. This is NOT reachable by the
`(&arr[0])[i]` respelling (that is the struct-member-through-pointer form) nor a cached
pointer local (identical tree post-fold): the divergence is baked into which front-end
expansion path a `SYMBOL + reg*CONST` address takes, not the source array spelling
(`leSetEnemy`, a genuine un-source-reachable local-alloc order).

### An offset-0 LOCAL-pointer dereference is cse1-canonicalised back to base+const

The sibling of the offset-0-alias `%hi` lever, but for a LOCAL pointer variable and cse
rather than -msplit-addresses. cse1 substitutes a bare offset-0 dereference (`*p` /
`p->field0`) with its defining `base+const` expression (so `sched->next` at offset 0
compiles as `this+24` through the base register, not through `sched`'s own register),
but does NOT do this for the SAME pointer's nonzero-offset fields. So if the target
reaches a struct through a local pointer at offset 0 where your draft goes through the
enclosing base, or vice versa, that is the lever -- confirmed on FUN_8004c59c via the
`.cse`/`.cse2`/`.lreg` dumps (`.greg` shows the pointer correctly in $s1, but cse1 had
already rewritten the offset-0 store to base-relative).

### A huge-offset-spilled pointer dereference can NEVER self-tie (un-matchable)

If the target self-ties the `%hi` address and the value-load registers of a `ptr->field`
where `ptr` itself is spilled at a frame offset beyond the signed-16-bit range, that
shape is UN-MATCHABLE in this cc1 — the original did not use one combined dereference,
and no C respelling forces it. `find_reloads_address`'s `reg_equiv_address` branch always
pushes the address-of-the-spill-slot reload (`RELOAD_FOR_INPADDR_ADDRESS`) BEFORE the
value-used-as-address reload (`RELOAD_FOR_INPUT_ADDRESS`); `reload_reg_free_p`'s
`RELOAD_FOR_INPUT_ADDRESS` case hardcodes a reject on the first reload's register
(`reload_reg_used_in_inpaddr_addr[opnum]`); and `reload_reg_class_lower`'s tiebreak
`return r1 - r2` makes the push order deterministic. `combine.c` refolds any intermediate
single-use pointer temp (`q = menu; name = q->choice_name;`) back to the same mem-of-mem
before reload runs, so you cannot break it up either. Contrast a PLAIN whole-value read of
the same spilled variable (`if (title == 0)`) — that is `RELOAD_FOR_INPUT`, has no reject
rule, and self-ties freely. `AdtSelect` (`menu->choice_name`, frame offset 0x80CC) is
parked on exactly this; do not retry it. Proven by reading reload.c/reload1.c, twice
empirically.

### A `%hi` reload tie is `combine_regs` refusing to tie a block-crossing pseudo

The "la/address-materialization reload tie" — target keeps a `lui`/`addiu` in the same
register as a call argument, your draft puts the `%hi` temp somewhere else — is
`local-alloc.c`'s `combine_regs` guard: `if (sreg >= FIRST_PSEUDO_REGISTER &&
reg_qty[sreg] == -1) return 0;`. A pseudo that CROSSES A BLOCK BOUNDARY (e.g. a
`void (*f)(void)` local assigned differently in each if/else arm and used once after the
join) has `reg_qty == -1`, so combine_regs refuses even to attempt tying the `%hi` temp
to it; with no tie and no suggestion, find_free_reg's default ascending search grabs the
first free reg (`$v0`), not the call's `$a0`.

Fix: make the argument pseudo BLOCK-LOCAL — call the function DIRECTLY IN EACH ARM
(duplicated call) instead of funneling a shared local. Then `reg_qty[sreg] != -1`,
combine_regs ties the `%hi` temp to it, the tied qty inherits the call-argument's `$a0`
copy-suggestion (both `lui`/`addiu` halves land in `$a0`), and jump.c's cross-jump merges
the two identical call tails back into the single `jal` the target has — length unchanged.
Do this WITHIN the existing `if/else` — do NOT also convert to an early-return +
fallthrough shape, which can relocate a nearby loop's early-exit block relative to the
call and pull the call's address computation inside `loop.c`'s scanned range (an unwanted
hoist, one instruction short; AfsOpen). This is one of the two classes previously
(wrongly) called permuter-immune; it is
reachable. Now matched by FOUR functions (FileRead, PrepareAccess, AfsOpen + …). Found by a *Sonnet* agent from `rtldump FileRead --draft` + reading
local-alloc.c. Worth retrying `PrepareAccess`'s park with it.

### When GE-ternary abs costs bytes elsewhere, absorb the cascade with a scratch reuse

The GE-ternary abs fold (abssi2) is C-correct but can cost bytes elsewhere via a
`global.c` priority cascade (it evicts another local from its target register). Check
whether reusing an EXISTING nearby local as a THROWAWAY scratch for an unrelated value —
BEFORE that local's "real" assignment — absorbs the pressure: extending its pseudo's live
range raises its allocation priority and can fix the cascade AND unrelated ties at once
(`Think1random`: reusing `adx` as scratch for `vx` before its abs role fixed both the
abssi2 cascade and a tied `mfhi $a3` vs `$a2`). The scratch choice matters: pick a
variable whose real role is UNCONDITIONALLY (re)assigned before it is ever read in its
scratch capacity — else it is semantically unsafe (using `result`, whose reset path never
reassigns) or cross-contaminates its own later use (using `adz`).

### A pure callee-saved SWAP residual is an `allocno_compare` inequality — add ballast

When two locals are swapped between `$s0`/`$s1` (or any callee-saved pair) with no other
diff, it is a priority tie in `global.c`'s `allocno_compare`:
`priority = floor_log2(refs) * refs / live_length`, read from the `.lreg` dump's
`Register N used X times across Y insns` lines; the higher priority takes `$s0` first.
Live length counts RTL insns at local-alloc time, so you tilt it by adding an
instruction inside one allocno's live range — hoist a one-arm constant into a variable
defined BEFORE the branch (`maxvol = 0x7f; … dist = maxvol - expr;`): +1 insn to every
allocno live across it, and FREE in bytes when the target materialises that constant in
a register anyway (`SoundEx`: a 14-byte `$s0`/`$s1` swap → 0). Compute the two
priorities from the dump, then add ballast to the one that must win.

### Two same-role temps with DISJOINT live ranges may be ONE reused variable

The exact inverse of the rule above, and also a global-alloc priority lever.
`global.c` sums refs and live-range lengths across a pseudo's whole life, and
priority moves in `floor_log2` jumps, so merging two disjoint temps can vault the
merged pseudo over a rival. `vfree`: the forward-merge's `next->size` (3 refs / 6
insns = 5000) and the backward-merge's `prev->size` (3 / 4 = 7500) are ONE local
`s`; merged they are 6 / 10 = 12000, outranking `next` (4 / 10 = 8000) and pushing
it off `$v1` into `$a0` — the target's exact colouring.

The tell: **the target loads two different values into the SAME register in disjoint
regions** (`lw $v1, 0($a0)` in both merge blocks) while your draft colours them
apart. A `do {} while (0)` wrapper can force the same order, but it cascades (it
boosted `pt` past `header` across a `floor_log2(8)` jump and swapped `$s0`/`$s1`);
a natural variable merge is the stabler lever when one exists.

### `A + (B + 2)` and `B + 2 + A` pick different destination registers

fold canonicalises `A + (B + C)` to `(A + C) + B`, so the `addiu` lands on A's
register — where it can fill a guard's delay slot — and the `addu`'s destination
ties to that dying operand. The mirrored spelling emits the same instructions with
swapped operands and a different result colour (`vfree`'s tail sum: `$v1` vs `$v0`).

### Steering allocation: donate a call-arg preference to a low-priority pseudo

`global.c`'s `find_reg` makes earlier (higher-priority) allocnos AVOID a free
register that a later allocno *prefers*. So you can push a high-priority pseudo off
a free `$a2` and into `$a3` by donating an `$a2` call-argument preference to a
low-priority pseudo — e.g. by merging that pseudo with a call-argument temp whose
live range is disjoint. This is the lever when a same-length register rotation has
no C-level fix. Found by reading `-dg` on `vrealloc`: pseudo 85 was the top-priority
allocno with an explicit `preferences: 6` (hard `$a2`) inherited from the give-up
path's `memcpy` third argument; it took `$a2` first and rotated everything after it.

### A shared-return `sra` above the epilogue restores means a SECOND `return`

If the epilogue's truncating `sra` sits ABOVE the `lw` register restores (target)
but your build emits `[lw restores][sra]`, the source had a second literal `return`
statement. sched2 runs BEFORE jump2/cross-jump in this cc1 (verified: the `.jump2`
dump already carries sched2's dependency lists), so with a single structured return
the `[sll][sra][use]` flows into the threaded epilogue as one block and sched2 sinks
the `sra` below the restores (the loads have longer latency chains to the blockage
insn). A second `return` makes `expand` jump to `return_label`, whose CODE_LABEL pins
the `sra` above the loads at sched2 time; cross-jump then deletes the duplicated
`[sll][sra][use][jump]` body and jump2 re-inverts the guard, so the early return
costs nothing elsewhere. Constraint: place the early-return site so its inline body
does NOT sit between two blocks sharing a cse'd value (a shared `lhu` of a global, a
shared struct-pointer load) — cse follows only the fallthrough path
(`turn_towards_player_`; its `cached != 0x80000000` guard is the unique safe site).

### cse2 ignores loop notes; evict a merged `&local` arg by reassigning a pointer

`cse1` ends its basic block at `NOTE_INSN_LOOP_END` but `cse2` does NOT, so a
`do {} while (0)` protects a value from cse1 only. When cse2 keeps folding a repeated
`&local` call-argument onto an earlier call's argument pseudo (the target
re-materialises `addiu $aN,$sp,N` fresh each time), evict the register from cse's
value class by threading one pointer variable through the call sites:
`f(.., pv = &vc, ..); f(.., pv = &vd, ..); pv = 0;` — each reassignment drops `pv`
from the previous value's class, and the dead `pv = 0;` (flow deletes it, zero bytes)
evicts the last, so the later `&vc`/`&vd` arguments find no equivalent register and
rebuild the address (`SetCameraMode`).

### An "unconditional" delay-slot move after a compare is NOT a comma-expression

reorg steals the fallthrough arm's first instruction into a conditional branch's
delay slot whenever the written register is dead on the taken path, so a plain
`if (a && b != K) { x = K; ... }` produces the identical "executed on both paths"
placement that a comma-expression `(x = K, b != K)` would. Check register deadness on
the taken path before reaching for the comma spelling — the comma form assigns `x`
too early and changes the allocation (`turn_towards_player_`; the parked draft had
been blocked by exactly this misreading).

### `x |= C; store x;` vs `store = x | C;` is a local-alloc lever

The expression form creates a fresh or-temp whose refs/length priority beats a
longer-lived sibling temp in the same block, rotating which one gets `$v0` and
cascading into a global pseudo's colouring. The in-place compound leaves the sibling
alone (`turn_towards_player_`: in-place `d2 |= 0x1e` kept `Me_THINK_C` in `$v0` and
`d2` in `$v1`; the expression form pushed both).

### A `li` in a branch delay slot means the constant was defined in that test's block

Signature: `lw / nop / bltz / lui`, i.e. a constant `li` sitting in a conditional
branch's delay slot with a load-use hazard `nop` before the branch. That means the
constant's definition lived in that test's OWN basic block — the source assigned it
*between* two tests of what looks like a single `&&` chain. Write nested `if`s with a
`goto` to the shared else, not one `&&`. sched1 slots the `li` into the load-delay
stall, reorg pulls it into the branch slot, and the assembler re-inserts the hazard
`nop`. Spelling the constant literally per-arm instead compiles TWO `lui`s: cse's
path-following cannot unify constants across divergent arms (`vrealloc`'s
`0x80000000`).

### Which zero-initialised local is the "master" decides the whole allocation

When several locals are zeroed together, one is literally assigned the constant and
the rest are copied from it. Which one is discoverable from the register the first
`move`/`li` of the init chain targets. Get it wrong and a local can collide with a
still-live incoming parameter's register, forcing an extra callee-saved save/restore
across the whole function. `GetCenterAndSize` zeroes six shorts; naming the wrong
master put one of them in the live `center` parameter's register. Reordering the
declarations so the master matches the asm's first-in-chain register fixed it with no
other change. The tell is `regalloc.py` showing a parameter's register reused by a
local.

### A parameter that walks stays in its incoming register; the fixed base gets copied

When two live pointers into one buffer are a fixed base plus a walking cursor, cc1
keeps the **incoming parameter register** as the walker and copies the fixed base to
a fresh callee-saved register. In `LoadTIMpack`, writing `puVar2 = adr;` and then
walking `adr` (`adr = adr + 1`) while reading the fixed base through `puVar2` matched
the target's register roles. Walking `puVar2` instead — keeping `adr` fixed — rotated
every callee-saved register by one and mismatched the whole prologue and epilogue.
Pick which name walks to match the target's `$a0` role, not for readability.

### A struct-typed local cached at a post-branch join, not at entry

Two sibling field checks that straddle an intervening conditional block will each
reload the base pointer, because a label is a hard CSE boundary. Naive hoisting to
function entry does not fix it — but a **named local** whose liveness crosses the
label does. `ItemUse` needed `character_state *me = Me_THINK_C;` introduced right at
the join, so `item[1]` and `item[4]` reuse one register across the branch instead of
reloading twice.

### A constant used ONCE across a call is rematerialised, not allocated

cc1 costs a cheap constant that is live across a call as *rematerialisable*: it
re-emits it at the point of use rather than paying a callee-saved register plus
the prologue/epilogue save. If the target clearly keeps such a value in `$s2`
across a call and your draft re-materialises it, **use the same named constant a
second time somewhere else in the function** — a second use tips the cost model
into giving it a real register. (`vfree`'s `mask` needed to appear in both the
double-release guard and the forward-merge test to land in `$s2`.)

### Statement order steers which value gets tail-duplicated at a goto-merge

The "source statement order steers scheduling" family has a second, distinct
member: at a merge point reached by several `goto`s, cc1 duplicates *one* of the
values computed before it into each predecessor and computes the other once.
Which one it picks is decided by **textual order**, and it can pick backwards.

With `r = col >> 16;` written before `fp = &ef->param.bleed;`, cc1 duplicated the
path-*invariant* `r` onto both merge-entry paths and computed the necessarily
path-*dependent* `fp` once — the opposite of the target. Swapping the two
statements (address first) fixed both at once (SetBleed). If a diff shows one
value duplicated across predecessors and another that "should" be, try reordering
their definitions before reaching for the permuter.

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

**A named `goto` label pins cross-jump's choice of primary copy.** When several
unconditional exits share a tail (e.g. every reject does `x = -1; goto ret;`), route them
all through ONE named label placed exactly where the target's copy lives — cross-jump
then merges onto that copy, not whichever it would pick by scan order. Placing the
`reject:` label inside an existing guard where the target keeps it fixed DrawModel's
length (Ghidra's literal `goto unit_vector;` on the box-check failures had sent them to
the wrong tail, changing the merge).

**Assign a sentinel ONLY on each exit edge, never once at the top, to keep it
rematerialised in a caller-saved reg.** `result = -1; goto ret;` per reject keeps
`result`'s live range short, so cc1 leaves it in caller-saved `$v0` and re-emits
`li v0,-1` per site; hoisting `result = -1;` to the top forces ONE `$a2` materialisation
and drops the rematerialisations (one instruction short). And a reject condition that is
textually identical to the shared tail must be a STANDALONE `if`, not the enclosing
guard's `else` — as an `else` body cross-jump merges it away, costing the direct branch
and a recompute (`DrawSprite`, the exact missing instruction).


**An early `return !flag;` right after `flag = 1;` gets constant-folded to a
literal.** `IsVisible` sets a `fail` flag and has exactly ONE `return !fail;`, at the
bottom; making the failing branch return locally lets cc1 fold `!1` to `0` there —
three instructions shorter than the target. Both branches must fall through to the
single shared `return !fail;` for the real runtime `xori` to appear on both paths.

**When both arms of an if/else end in the same statement on an address-taken
variable, duplicate it into BOTH arms.** Writing it once after the if/else forces a
fresh stack reload at the merge point (the variable is memory-resident because its
address escapes); duplicating lets each arm feed the statement from the register it
already has. cc1's own cross-jump then collapses the identical code, so duplicating
costs nothing and matches the target (`AdtMessageBox`).

**The shared-return lever fires only on INDEPENDENTLY-WRITTEN identical returns, not an
explicit goto to one.** Two separately-written identical `return EXPR;` statements let
jump2's cross-jump merge them into ONE shared call site placed EARLY; routing both
through an explicit `goto label;` instead pins that code at the label's own textual
position -- a byte regression. So: write the identical returns out in full, do not
factor them into a goto (`Think4abandon`; the inverse of the cases where a named goto
label is what you want -- see the shared-tails section).

**Two otherwise byte-identical `return expr;` tails can fail to cross-jump-merge in
this cc1.** Route all but one through `goto` to a single shared `return` — in
`CGetLevel`, funnelling three arms into one `ret` variable plus one
`goto done; … done: return ret;` was what merged the epilogues. Related but distinct
from the `li`-suppressing bare-`goto` rule under Dispatch: that one is about the value,
this one is about the tail.


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

### A false Ghidra/splat multi-piece split is sometimes just ONE function

When `reverse.py` seeds several `INCLUDE_ASM` lines for one target but
`config/splat.main.exe.yaml` carves it as a SINGLE `c` segment, the extra pieces are
usually consecutive calls (e.g. two `FntPrint`s with different literal-string addresses)
that tricked Ghidra's function-boundary detection. Write one real C function and DELETE
every stub -- do NOT reach for `split-scaffold.py` (that is for genuine jump tables).
The literal-string addresses may need pinning in `config/symbols.<target>.txt` (Camera's
two FntPrint format strings). (`Camera`.)

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

**An absolute global that is read, modified and re-read across a call wants a raw
address-literal macro, not a named `extern`.** `InitGraphicsSystem`'s tail is
`STARTING_RNG_SEED = STARTING_RNG_SEED + VSync(-1); srand(STARTING_RNG_SEED);`.
A named `extern s32 STARTING_RNG_SEED;` compiles the load and the store through the
generic assembler pseudo-ops (`lw $r,SYM` / `sw $r,SYM`), each independently
`lui`-expanded: one instruction too many, and the store cannot be stolen into the
following `jal srand` delay slot. Spelling it
`#define STARTING_RNG_SEED (*(s32 *)0x80010e70)` makes cc1 commit to explicit
`lui`+`ori` constant-load RTL, which it CSEs across the read and the write, freeing
reorg to fill the call's delay slot with the store — the exact target sequence.
Verified against the pinned cc1: the named extern and a pointer temp both fail; only
the raw literal matches. This is the read-modify-reread-across-a-call counterpart to
the bare-`lui` rule above.

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

**A missing `--gp-extern` entry silently relocates a whole data region.** Drop one
file's entry from `maspsxGpExterns` and maspsx synthesizes stray local placeholders
that the linker prefers over the real symbols; the link SUCCEEDS and the image is
simply wrong. Measured: removing `CVArun`'s entry moved **388 symbols by +16 bytes**
each. `tools/symcheck.py` catches this in one second by asserting the invariant that
a symbol named `D_<HEX>` must resolve to `0x<HEX>`. Run it after any build in which
you converted an `INCLUDE_ASM` stub to real C.

Two corrections to what was first believed about this bug, both established by
removing each change independently on a clean tree:
- The load-bearing fix is **the matched function's own gp-extern list**. Nothing else.
- Pinning `D_XXXX = 0xXXXX;` in `config/symbols.<target>.txt`, and adding gp-extern
  entries for *sibling stub files* still on `INCLUDE_ASM`, are **defensive no-ops**
  today: the build is byte-identical without them. Keep them (they cost nothing and
  may matter once the siblings are matched) but do not reach for them first.

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

### The permuter aborts on rand()-calling functions (KeyError 'rand')

decomp-permuter throws `KeyError: 'rand'` in `perm_add_mask` on EFFECT.C spawners (a
typemap gap for `rand()`-calling functions; also seen on FUN_8003944c). It could not
reach the pool-scan addu operand tie anyway — that order is chosen after `fold` in RTL,
below the C AST the permuter transforms. Recognise these and park; do not budget
permuter time.

### Declaring scratch jitter values as one array forces stack residency

For an EFFECT spawner whose three jitter scratch values (px/py/pz) must live on the
stack (the target block-copies them, so they cannot stay in registers), declaring them
as one `long p[3];` instead of three scalars forces genuine stack residency and moves
the residual toward the target (`SetBleedsDir`: 88 → 68 bytes). Part of the still-open
"throwaway scratch + second block-copy" residual that SetBleeds/SetSmoke share.

### Hoist-invariant vs widen-parameter can be a 2-instruction allocator tie

A hand-rolled `do {} while (1)` loop that has BOTH a repeatedly-used address/constant
invariant worth hoisting to a dedicated register AND a parameter compared only via the
no-`sra` double-left-shift trick can tie: the target dedicates a register to the raw
parameter (re-deriving its widened form cheaply each iteration) so another register is
free to cache the invariant across the loop, while a draft that fully widens the
parameter into one persistent register leaves nothing free and recomputes the invariant
each iteration. Both cost the same 2 instructions; no operand-order or De-Morgan rewrite
of the compare reliably picks the target's path. RTL-escalation territory, not
respelling (`SetSmoke`).

**Any literal reused across >=2 calls inside one loop needs its own named local,
assigned once before the loop, to be promoted to a callee-saved register instead of
re-materialising (`li`/`lui`) at every use.** Not just sentinels -- a bitmask, a fixed
call argument, a comparison constant all qualify (`RestoreItemLayout`'s compare sentinel,
`LoadOrnamentArchive`'s OR-mask + literal call-flag).

## loop.c hoisting is a threshold economy (the invariant-hoist model)

`move_movables` hoists a loop invariant iff `threshold * savings * lifetime >=
insn_count`. `threshold` starts at `(loop_has_call ? 1 : 2) * (1 + n_non_fixed_regs)`
— **29** for a call-containing loop on this target — and **decays by 3 for each movable
already moved**. Every decision is printed in the `-da` `.loop` dump
(`Insn N: regno R (life L), savings S moved / not desirable`), so you can solve for the
threshold and PREDICT the fix instead of permuting for it. Three levers follow:

- **Guard operand order controls whether a short parameter's widening hoists.** In a
  call-containing loop, `if (i >= n)` (counter first) emits `n`'s sign-extension pair
  adjacent to the `slt` → lifetime 2 → stays in-loop, and combine folds both extensions
  into a no-`sra` `sll/sll/slt`, keeping raw `short n` in one callee-saved reg. The
  reversed `if (n <= i)` gives the pair lifetime 4 → loop.c hoists the widening into its
  own register and the compare degrades to `sll/sra/slt`. Which operand's `sll` comes
  first in the target's compare reads the source operand order off directly.
- **A USER-variable invariant assignment is hoistable only if it precedes any conditional
  exit.** `base = EffectSlot;` as the FIRST body statement (before `if (…) return;`) is a
  movable (its `lui/addiu` lands in the prologue, both `base+idx` and the wrap-reset read
  one cached reg). The same statement AFTER the guard is `maybe_never` and ineligible —
  and assigning `base` ABOVE the loop in source mismatches too (next point).
- **The −3-per-move decay is load-bearing: an early hoist keeps a LATER invariant
  in-loop.** SetSmoke needs `time<<16` un-hoisted: after `base`'s two moves plus the
  `%100` magic, threshold is 29−9=20 → `20*2*3 < 153` → stays. Writing `base` above the
  loop instead moves only the magic (−3), and `26*2*3 >= 153` hoists the `time<<16` chain
  and spills n/time to stack. WHERE an invariant is assigned changes unrelated
  expressions' fates later in the body.

**Identical invariant else-arm constants COMBINE, so their hoist savings SUM.** cse
unifies `-shortvar` across arms into one pseudo and `combine_movables` does
`m->savings += m1->savings`, so three per-arm `negu`s hoist together
(`29*3*3 >= insn_count`) even though one alone stays (`29*1*3 < insn_count`). To keep the
literal per-arm `negu` the target has: leave ONE arm as plain `-x`, and spell the others
`zN - x` with `int zN = 0;` single-set/single-use vars initialised ABOVE the loop. Each
minus is then a SOLO movable (different regs → no rtx match → stays), zN's alloc priority
is rock-bottom (its REG_EQUIV note doubles its live length), and update_equiv_regs/reload
substitute const-0 into `subsi3`'s `reg_or_0_operand`, emitting `negu` with zero trace of
zN in the binary (`SetBleeds`). 

Two corollaries: loop.c DELETES single-use register copies inside a call-containing loop
(`reg_single_usage` substitution in scan_loop), so a `tmp = n;` copy cannot block
invariance — don't bother; and fold reassociates `(x - 1) - (a + b)` into `x - (a+b+1)`,
so an own-statement temp (`m = smoke->mode - 1;`) is needed to keep the literal `addiu -1`
on x's side. (`SetSmoke`, whole outer loop. `-da` dumps the `.loop` numbers; `-dd` dumps
reorg/dbr for delay-slot questions.)

### The loop.c "unbiased giv + surviving counter" tie is unreachable from C

When the target walks a strength-reduced pointer at NATURAL field offsets (unbiased) AND
keeps an independent counter biv for a constant-bound top test, no single C loop spelling
reproduces both: `items[i]` under `while(1){…;i++;}` gives the unbiased giv + hoisted
bases but loop.c makes the giv a function of the counter and `maybe_eliminate_biv` DROPS
the counter (pointer-bound exit); an explicit `it++` walking pointer keeps the counter as
its own biv but biases the field offsets +stride and spends an extra callee-saved reg; a
hand-rolled goto loop is unbiased but hoists nothing. The `.loop` dump names it
("giv … mult N add (reg base)" then "biv M can be eliminated"). Same-length,
permuter-immune -- park it (UpdateItemState).

## Dividing by a variable needs `--expand-div`

**A `%` or `/` by a runtime VALUE (not a constant) compiles ~10 instructions shorter
than the target unless the function is on the `--expand-div` list.** ASPSX guards a
variable division with `break 7` (divide-by-zero) and `break 6` (overflow, the
INT_MIN/-1 case); maspsx only reproduces those guards under `--expand-div`. Without
it maspsx emits a bare `div`/`mflo` and the draft assembles short, shifting
everything after it (this is a common cause of the "40 bytes short" length mismatch).

The tell, before you even build: **Ghidra renders the guards as `trap(0x1c00)` and
`trap(0x1800)`** — seeing those in the reference C means the TU divides by a variable.
In the target `.s` it is a `div`/`divu` followed by two `break` instructions. Division
by a CONSTANT is a magic-multiply with no guards and needs nothing.

Fix: add the function to BOTH `extra "<Name>" = ["--expand-div"]` in
`shake/src/Build.hs` and `"<Name>": ["--expand-div"]` in `tools/permute.py`'s
`MASPSX_EXTRA`. The EFFECT.C spawners are prone to this (`rand() % time`,
`rand() % srange` etc.): `SetSmoke` (1 variable div, 2 breaks) and `SetBleeds`
(3 variable divs) both need it; the already-matched `IsVisible`/`ComputeAreaLevel`/
`bow_shoot_logic` are on the list for the same reason.

## Matching `main`

**`main` never calls `__main()` explicitly.** gcc auto-inserts the C-runtime `__main()`
at the top of any function literally named `main`, and reorg fills its delay slot with
the first callee-saved save. Writing `__main();` in the source compiles a SECOND call --
2 bytes over. Leave it out; only the implicit one is real.

**A `main` with an outsized frame but no matching stack access has a dead aggregate
local.** Retail `main`'s frame holds a large local (~248 B) that neither Ghidra nor m2c
renders (both optimise it away). Size a `u8 dead[0xNN];` to hit the frame -- the frame
size is the only evidence (the demo's own `i`/`dat`/`rect` locals were stripped in
retail). The "unexplained frame gap = unused aggregate" rule applied to main.

## Toolchain gotchas

- **A literal added to a value that only ever feeds a NARROW (byte) store can
  canonicalize to its negative-equivalent immediate**: `x + 0x80` compiles as
  `addiu ...,-128` because only the low 8 bits survive the `sb`. If the target
  shows the positive encoding, route the literal through a named variable
  (`s32 bias = 0x80;`) used at both addition sites (SelectCameraOwnerOption).

- **A repeated `&local` passed to the same callee twice does NOT reliably get
  recomputed.** cc1 can CSE the address across an intervening conditional even
  when a genuine two-predecessor `CODE_LABEL` sits between the call sites — which
  contradicts the "a label breaks the CSE window" expectation elsewhere in this
  file. If the target recomputes the stack address fresh at each site and your
  draft caches it in two extra callee-saved registers, there is currently no
  known source lever (LoadSI, parked; wants a `-dg`/`-di` dump session).


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
  - **Do NOT confuse this with the ordinary 2-instruction `sll r,r,16 / sra
    r,r,K` (K≠16)**, which is just a `short`/HImode index sign-extended *and*
    scaled in one go — exactly what `arr[short_i]` compiles to for a 4-byte
    stride. That shape is perfectly matchable (`GetHumanoid`, `DisposeWeapon`).
    The blocked class is specifically **three** instructions: a pure sign-extend
    split into two `sra`s summing to 16, with **no** scale folded in.
    `triage.py`'s detector already requires the 3-insn form, so it will not
    false-positive here.
- **PS1 GTE command opcodes: handled by splat, still blocked for matching.**
  Our `mipsel-...-as` (binutils 2.40, `-march=r3000`) is vanilla MIPS: the COP2
  *data moves* (`lwc2`/`swc2`/`mfc2`/`mtc2`/`cfc2`/`ctc2`) assemble, but the GTE
  *command* opcodes (`RTPS`/`RTPT`/`NCLIP`/`DPCS`/`DPCT`/`MVMVA`/…) are
  `unrecognized opcode` on their own. **splat >= 0.4x generates
  `include/gte_macros.inc` (68 macros) and has `macro.inc` include it**, so it
  emits them as lowercase mnemonics that assemble through those macros. The whole
  renderer region is splittable and all 25 GTE functions are carved;
  `./Build check` is byte-identical. (We briefly carried our own `tools/gte2word.py`
  that rewrote each mnemonic to a `.word`; splat 0.41 superseded it and it is gone.
  If you ever need the encoding by hand, note splat's `/* addr vaddr WORD */`
  comment column is the raw little-endian file bytes in address order, NOT the
  instruction word — `RTPT` prints `3000284A` and is `.word 0x4A280030`.)
  - **What is still blocked is MATCHING**: no C construct emits a GTE opcode, and
    these functions also use a non-ABI calling convention (values live in
    `$t2..$t5`/`$s0` at entry). That needs the register-pinned-locals / inline-asm
    policy — the same open question as `GetPad`/`PClseek`. `triage.py` says
    `GTE CMD — split OK, no C form (inline-asm policy)` and keeps them VERY-HARD.
    `DrawTMD` and `ArrangeLocalMatrix` are blocked the same way (they pass the
    handlers their arguments in `$t2..$t6`) despite containing no GTE opcodes.
  - **m2c can read them**: our m2c carries a PSX GTE/COP2 patch series
    (`nix/m2c/*.patch`). You MUST pass `--input-regs`, or every entry-live value
    becomes `M2C_ERROR(Read from unset register)`. Hand it the whole caller-saved
    file and let m2c ignore the unused ones:
    `--input-regs v0,v1,a0,a1,a2,a3,t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,s0`
    → zero `M2C_ERROR` on `FUN_8005d1fc`.
- **maspsx's `break` wants the single-value form `break 0x107`, not the
  two-operand disassembly `break 0, 263`.** maspsx (`elif op == "break"`) takes
  one immediate and splits it into the two 10-bit fields itself; the
  Ghidra/objdump-printed `break code1, code2` form crashes it with `invalid
  literal for int() with base 0: '0,'`. Relevant only when hand-writing inline
  asm for a raw BIOS trap (PClseek's `break 0x107` host-lseek — a syscall with
  no C representation, currently parked NON_MATCHING pending the inline-asm
  policy question; see GetPad.c).
- **An UNDER-SIZED `functions.tsv` entry carves a truncated function that still
  builds GREEN — the failure is silent.** `reverse.py` takes the function's size
  from the Ghidra export. If Ghidra sized it short, splat carves only the first
  N bytes; the tail is emitted as a `.data` blob (`asm/data/<off>.data.s`) which
  *defines the `.L` labels the carved asm jumps into*, so it links, and
  `./Build check` stays byte-identical. Nothing complains — you simply can never
  write C that byte-matches, because the real body is longer than the carve.
  Run **`tools/coverage.py`** before trusting a size; it finds code claimed by no
  function and names the culprit. Two exist in game code: `LoadCard`
  (0x114 → true 0x168) and `FUN_800593a0` (0x12C → true 0x27C). Carve those with
  `reverse.py <Name> --size 0x<true>` (verified green), and fix them in Ghidra.
  - **Variant: the missing tail can land in a plain anonymous
    `{ start: 0x…, type: data }` splat entry sandwiched between two real
    functions** — no phantom label, no rename, just delete the entry. Same tell
    (a bare `jr ra` with no delay slot ending the preceding function's `.s`).
    Third confirmed instance: `valloc`, carved 456 bytes where the truth is 460
    (0x1CC); the stolen word at file `0x5F34` decodes to `addiu sp,sp,0x438`,
    sitting in `jr ra`'s delay slot. Note Ghidra's own `functions.tsv` was
    under-sized too, so `coverage.py` is the check, not the export.

  - **Variant: the missing tail can land inside a NEIGHBOURING phantom function
    instead of an orphan `.data` blob, if Ghidra also independently placed a
    (bogus) function label exactly at the boundary.** Symptom: a trivial
    `void f(void)`-shaped target's FIRST instruction is a small **positive**
    `addiu sp,sp,+N` (not the usual negative frame-open) that doesn't fold into
    anything sensible no matter how you shape the source. Check the PRECEDING
    function's own carve: if it ends in a bare `jr ra` with no trailing
    delay-slot instruction at all (`endlabel` right after `jr ra` in its `.s`),
    that missing delay slot IS your function's leading `+N` — our cc1's
    `mips_expand_epilogue` ALWAYS reorgs the sp-restore into `jr ra`'s own
    delay slot (verified: no source shape leaves it bare), so the preceding
    function's TRUE carve is one word (4 bytes) longer than its
    `functions.tsv`/splat entry, and yours starts 4 bytes later than currently
    split. Corroborate before touching the boundary: `tools/xref.py <Name>`
    should show zero `jal` callers, and a raw byte-grep of the function's own
    address as a little-endian word across `disks/tenchu/main.exe` should find
    zero hits (no real function pointer/table targets it) — if both hold, it's
    a phantom Ghidra label, not a genuine callable entry. Fix: move the
    boundary 4 bytes later in `config/splat.main.exe.yaml` (shrinking the
    phantom entry, growing the preceding one by the same 4 bytes) and rename
    the file/function to match its TRUE start address (FUN_8005fe34, 84 bytes,
    was really AdtVsprintf's own missing epilogue word + FUN_8005fe38, 80
    bytes — both now match).
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
    - **Worse variant: it doesn't always fail loudly — it can silently resolve
      to the WRONG address instead, even past your own same-name rebinding.**
      If TWO already-matched `.c` files both plain-`extern` the same `D_`
      symbol with no explicit binding (each relying on splat's auto-name from
      whichever raw asm still referenced it), converting the LAST asm
      reference to C removes that anchor — but `undefined_symbols_auto.main.exe.txt`
      (a **generated meta linker script**, listed with `-T` *after*
      `config/symbols.main.exe.txt`) can still auto-derive an address for that
      same symbol NAME from an unrelated, drifted accumulation elsewhere, and
      GNU ld's plain `SYM = ADDR;` assignment takes the LAST script's value —
      so adding `D_XXXXXXXX = 0x<real>;` to `config/symbols.main.exe.txt`
      yourself does **not** win; the auto meta file processed after it
      silently overrides it back to the wrong address, and BOTH consumer
      files break together with no link error (`./Build check`'s hash just
      fails). Diagnose via the `.map`: `grep D_XXXXXXXX .shake/build/tenchu/main.exe.map`
      shows the address actually in effect. Fix (same recipe as the sibling
      bullet above, one level firmer): bind a **FRESH, non-colliding name**
      (not the old `D_` one) at the correct address, and update **every**
      already-matched consumer file's `extern` to the new name, not just the
      one you're currently drafting (FUN_80056e30 and DeleteCard.c both
      `extern`'d `D_80097D04`/`D_80097D08`; rebound as `CardVolumeIdPtr`/
      `CardPathFormat` in both files at once).
    - **The same fix applies to a string/constant that NO carved asm ever
      referenced, from a shared TU-wide rodata pool a sibling function
      already carved.** MISC.C's functions share ONE rodata block (splat
      auto-named `D_800127A4`/`D_800127BC` from it, both still-referenced by
      AddMisc's case-5 table); "unknown sprite type" — ProcMiscSprite's own
      error string, living in that SAME block a few bytes earlier — was
      NEVER referenced by any carved asm at all (ProcMiscSprite was still an
      `INCLUDE_ASM` stub), so splat never named it. Writing a fresh C string
      literal there places it in THIS file's own compiled rodata at the
      WRONG address (a different `.rodata` page entirely — not a small
      offset drift, verified 8+ pages off); the fix is the same
      `D_XXXXXXXX = 0xADDR;` binding (address confirmed by reading the raw
      ROM bytes at the offset), then `extern char D_XXXXXXXX[];` +
      `AdtMessageBox(D_XXXXXXXX)` instead of a literal. Tell: a Ghidra/m2c
      string-literal argument whose TU already has OTHER proven shared
      strings at nearby addresses (check the sibling's header comment).
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
