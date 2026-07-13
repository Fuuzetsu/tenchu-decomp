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

When the committed demo inventory has the same function name, run
`tools/siblingdiff.py <Name> --demo` before drafting. It disassembles both builds,
turns intra-function targets into relative labels, and turns relinked calls/jumps
into shared function names. This made SetFlyWire's earlier 900-byte body a direct
structural oracle for the 984-byte retail function: its distance clamp, midpoint,
and three jitter arms retained their statement shape even though the retail effect
pool scan and `abs` calls changed. Demo register allocation, constants, and layouts
can differ; use the comparison to recover source structure, never as a byte-match
claim.

If a function is still `FUN_…`, its block carries the recorded candidate name from
`reference/psxsym-candidates.tsv` — a suggestion that was not confident enough to
adopt. Do not rename on it without `tools/callmatch.py --verify`.

The name-recovery tools take function boundaries/sizes from Ghidra's exported TSV,
but overlay current function names from splat's named `c` subsegments by address.
Never classify unresolved names or compare callee fingerprints from the raw Ghidra
names alone: that snapshot is not rewritten when the repository adopts a name, so it
silently turns known callees back into placeholders. Call containment is necessary,
not sufficient; callbacks with the same small set of helpers still require prototype,
constant, and semantic confirmation (the historical `AttackFire` collision).
`callmatch --verify` mechanically blocks a conservative subset of these as
`AMBIGUOUS` when another full-containment candidate has no more extra named calls
and is at least as close in size; do not override that gate with positional evidence.

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

> **`autorules` scores the fully linked whole-image raw bytes; decomp-permuter's
> search score is only a proxy.** Older autorules revisions used an instruction
> alignment score that could improve while real bytes regressed; that is now
> fixed. The permuter still deliberately normalises details such as stack slots,
> relocations, and register allocation, so its lower-numbered output directory is
> not necessarily closer. `tools/permute.py` now full-links every retained output
> after a run and prints the authoritative whole-image/window ranking; after an
> interrupted run use `tools/permute.py <Name> --rescore-only`. **Re-verify every
> accepted edit with `matchdiff.py`, and the function as a whole with `./Build
> check`.** Bisect winners: they routinely carry dead statements that are not
> load-bearing.

> **Do not run the permuter as an early decompiler.** First recover the CFG,
> loop shapes, expressions, stack plan, calls, and near-final instruction
> length with the deterministic/RTL tools. Otherwise stochastic proxy wins are
> usually nonsense C that obscures the real structure. `tools/permute.py` now
> preflights the draft and refuses length deltas above one instruction or broad
> residuals above 128 aligned lines / 32 blocks. `--force-early` exists only for
> a reviewed case where RTL proves the broad diff is one allocation cascade.
> Even then, preserve the source baseline, linked-rescore, and bisect—never
> paste a wholesale permuter result into an unmatched large function.

The ordered triage — fix categories in THIS order, re-running
`tools/matchdiff.py <Name>` after every source change:

1. **Length first.** If your function assembles to a different instruction
   count (matchdiff's whole-image number explodes), the *structure* is wrong —
   dispatch shape, loop shape, missing/extra temps, tail merging. Fix before
   anything else; operand diffs are meaningless while the length is off.
   Exact length is necessary, not sufficient: compare `rtlguide`'s physical
   control-flow inventory too. AddEnemy had an exact-length guided candidate
   only because an identical-arm fence invented a conditional branch absent
   from retail. The structurally honest checkpoint stayed five instructions
   short; accepting the balanced count would have hidden the real allocation
   problem.
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
     integer width/sign, `&&` split/merge, single-use temp inline, expression
     polarity, extern-array addressing, and four-field VECTOR aggregate copies
     — and greedily keeps what shrinks the authoritative whole-image byte diff,
     telling you which edit helped. Don't hand-apply these — and if it reports
     no win, the residual is *not* one of them, so move to structure/regalloc
     instead of trying more variants. For a correct-length residual that
     survives that ordinary sweep, run `tools/rtlguide.py <Name>` followed by
     `tools/autorules.py <Name> --guided`. The guide aligns target asm to our
     output, uses cc1 debug NOTE objects to map it back to C lines, selects only
     the owning pass's advanced rules, and keeps a small beam of neutral/slightly
     worse intermediate candidates. The finite build budget is consumed in
     rtlguide's pass-ranked rule order, so dominant-pass allocation/CFG levers
     are tried before generic registry rules. This is how zero-code equivalence
     barriers and cross-jump fences become a bounded mechanical search instead
     of another manual RTL conversation. Guided mode can now fence a contiguous
     range of two-to-four statements (not just one statement) when RTL
     implicates a producer/consumer span. **Reject an autorules "win" that changes a
     PROVEN struct field's ACCESS WIDTH** even when it shrinks the byte count: a
     `u16`→`u8` field retype can narrow a correct `lhu` to `lbu` at another,
     already-matched site — autorules scores *total* diff, not per-site
     correctness, so it reports the net shrink as a win (Think3chase). Target
     asm is also a structural oracle: autorules now retains asmdiff's aligned
     differing-line count and prints `LOCAL-SHAPE REGRESSION` when a byte-count
     win at equal length worsens it. That warning caught FUN_80052ea8's tempting
     signed-`s16` to unsigned-`u16` retype; review the affected hunk instead of
     accepting a length-only win blindly. The raw linked byte count remains the
     authoritative objective, and an exact match is never warned. Target asm
     is also an anti-oracle: `extern-array` refuses any symbol whose target
     access is proven `%gp_rel`, since that symbol categorically must stay on
     the small-data path. (Structural rules that need diff-reading to place —
     loop shape, switch-vs-ladder, union-offset casts — stay manual.)
     The same semantic veto applies to a local whose signedness is recorded by
     PSX.SYM: SetupBG's `short sy` became exact-length and reduced the residual
     when mechanically changed to `u16`, but that contradicts the original
     debug type and was rejected. A better score cannot overrule original type
     evidence.
     Autorules does not rewrite the checked-in C while scoring: candidates live
     under `.shake` and Build.hs consumes them through a per-function source
     override. Only an accepted result is atomically published. TERM/HUP also
     tears down the owned build process group, and Linux parent-death handling
     prevents a dead command frontend from orphaning the search. Still poll a
     yielded command until it returns an exit status; staging makes interruption
     safe, not complete.
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
     - **Two adjacent loads in opposite order are NOT proven independent by the
       final assembly.** Compare their order through `.combine -> .sched ->
       .sched2`, inspect the candidate's sched `LOG_LINKS`, and look for a nearby
       `NOTE_INSN_LOOP_END` before declaring a tie. gcc-2.8.1 makes the first
       post-LOOP_END insn a full pending register/memory fence. In
       DefaultActionHumanoid, `.combine` had `size, y, id`; sched1 changed it to
       `size, id, y` because `size` consumed that fence. Moving a full-width id
       capture between two nested loop ends gave `pointer, id, size`; duplicating
       the y load into identical arms fenced it without adding loop weight, and
       matched all 2624 bytes. Only park an adjacent-load residual after the
       pass-order and dependency checks show a genuine equal ready-list choice.
     - **A dead constant scratch feeding one narrow store can be a reload/reorg
       hard-register choice.** When length, CFG, schedule, opcodes, and every
       surrounding register agree, but `li R,K; sb R,off(base)` uses `$v1` in
       the target and `$t1` in the draft, there may be no remaining source
       identity to steer. ProcItemDokudango reached this state at two duplicated
       cleanup sites (four bytes total); 1,089 authoritative permuter candidates
       were flat. After checking the cleanup's literal/direct/local spellings,
       preserve the pure-C near-match rather than introducing a register-asm
       escape hatch.
     - **A commutative PLUS whose destination ties to the other dying operand is
       a combine/local-allocation tie.** When the target and draft differ only
       as `addu A,A,B; sw A` versus `addu B,B,A; sw B`, inspect `.combine` and
       `.lreg`. If both source operand orders have already become the same plain
       commutative PLUS, explicit temps and a bounded permuter stay flat, the
       destination choice is no longer represented in C. AttackLong parks at
       exactly this 3-byte residual; inline asm is not an acceptable escape.
     - **A jump present in the first jump dump but threaded away only by
       `jump2` can be a real final-pass park.** AttackShort's missing explicit
       zero-return jump still exists before jump2; direct returns, one-shot
       loops, and both real-case-label fence layouts all cause jump2 to thread
       it into the shared sign-extension tail (or merge even more). Once the
       desired pre-jump2 RTL is proven and all safe CODE_LABEL layouts are flat,
       preserve the pure-C draft rather than forcing the jump.
     - **Mechanical detection:** `rtlguide` retains the historical
       `adjacent-independent-load-order` signature name, but now warns that it
       is only a hypothesis and reports nearby post-LOOP_END source lines from
       `.sched`/`.sched2`. It also names `commutative-plus-destination`.
       `autorules --guided` can try `loop-boundary-shift` and
       `identical-arm-fence`; the pass dump and bounded experiments remain the
       proof.
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
- `tools/maspsxflags.py <Name> [--write]` is the combined setup pass: it derives
  those gp externs, detects target variable divisions carrying ASPSX's
  `break 7`/`break 6` guards, and synchronizes both the gp and `--expand-div`
  Build.hs/permuter tables. Prefer it when starting a newly split function.
  After batch work, `tools/maspsxflags.py --check` audits every guarded draft
  against target asm and verifies that the two metadata tables remain mirrored.
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
those first), and pure branch-target drift is suppressed by default. An empty
`--structural` view means only that every aligned replacement has equal length;
it is **not** a byte-match result. The tool now prints both the displayed and
raw aligned residual plus `exact instruction sequence: NO`; only `matchdiff`
is the authoritative byte gate.

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
- **Retail load/store width and raw offset outrank a demo symbol's reused union
  member.** Shared parameter names often describe only a common prefix, while a
  later retail processor extends or overlays it differently. If the binary says
  `lhu/sh` at +0x14 and a pointer at +0x0c, define a TU-appropriate overlay with
  exactly those widths/offsets instead of reusing a nearby struct that shifts the
  tail. ProcItemDokudango's `param_dokudango` is the worked case: the existing
  korogari view had an extra counter in the prefix, while the retail accesses
  prove `eater`@+0x0c, `org_think`@+0x10, and a `u16 count`@+0x14.

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
- **An explicit `default:` can change CSE even when it only returns.** Omitting
  the default makes the switch's out-of-range fallthrough another predecessor
  of the normal continuation; CSE then cannot carry a value proved on the
  pre-switch path and reloads it. Give the cold switch an explicit default
  return (and named close/normal labels when layout requires them) to remove
  that false predecessor. AttackGeneral's close-range `% 4` switch matched only
  in this form. Treat default presence as CFG data, not cosmetic syntax.
- **A case with a value already proven nonzero can deliberately fall through
  another case's zero-fill guard instead of jumping past it.** A decompiler can
  render two semantically equivalent gotos: one arm enters the later case's
  `if (value == 0) value = K;`, while the other skips that test. If every path
  from the earlier case has already made `value != 0`, remove both gotos and let
  the case fall through the shared test. gcc can then put the test expression in
  the preceding branch's delay slot and branch directly into the guard, which
  is distinct from targeting the following case. DamageControl's item case 0
  (`dmg` already 20) falling through case 0xe is the worked example. This stays
  a recognition rule until the nonzero proof can be made mechanically; never
  apply it when any predecessor can still carry zero.
- **Case-body memory order reveals the source case order** (the compare tree
  always sorts by value): LayoutEnemyOption's inner switch was written
  `case 1, case 2, case 0` — only the bodies show it.
  This diverges from the TEST order: expand_case sorts the compare TREE by
  value but lays bodies in SOURCE order, and a case ending in `return` is
  pushed later in memory (reached by a forward jump). Makibishi's nested
  switch needed `case 4:` before `case 1:` in source though the tests check 1
  first — check body ADDRESSES in the `.s`, not just the test order.
  In a split jump-table function, the same source order controls the sequence
  of separately carved case text islands. LoadConstruction's target order
  `0,5,2,3,11,4` cut its draft's structural block count substantially even
  though the table's numeric destinations remained value-indexed.
- **Identical-valued case bodies can still be distinct physical islands.** Do
  not factor equal case results through a temporary until the target proves
  they were merged. ActCHASE's item switch contains two separate `motID =
  0xf02` islands: writing a shared `selected_motion` collapsed them, while
  direct global assignments in target body order (`2,1,3,6,5,7`) preserved
  both. Its terminating sound/default cases also had to `goto` labels placed
  after the normal cases' shared `D_80097F0E = 1; return;` tail. This recipe—
  order value-producing cases by physical address, keep direct stores, and
  route terminating cases around the shared tail—restored the exact jump
  inventory. It is a structural recognition rule; use body addresses and
  control-flow counts as the oracle rather than blindly permuting case values.
- **Three sparse literal equalities may need `switch`, even without a jump
  table.** gcc 2.8.1's `expand_case` builds a value-sorted comparison tree but
  keeps case bodies in source order. FUN_8005b17c's `0x20`, `0x2000`, and
  `0x8000` pad tests required the exact `0x2000`-centred tree and separately
  placed tails; nested `if`s were twelve bytes short. Guided autorules rule
  `sparse-eq-switch` recognizes a three-arm equality ladder over one
  nonvolatile local and tests all six case-body orders mechanically.
- **A switch's shared continuation sitting physically BETWEEN two case bodies
  is NOT after-switch code — it is a duplicated tail.** When the target shows
  e.g. `item->mode++; return;` in the middle of the case bodies (case 0 ends in
  a `j` with its lone store in the delay slot, then the increment+return appears,
  then case 1), each of case 0 and case 1 ended in a *literal duplicated*
  `item->mode = item->mode + 1; return;` and jump2 cross-jumped them into the
  LAST copy (case 1's). Writing one shared statement after the `switch` instead
  lays the join after ALL bodies — wrong placement. Duplicate the tail into each
  case (ProcItemKawarimi, ProcItemGun, ProcItemDokudango).
- **The switch index register doubling as the constant it matched.** A
  `sw`/`sh` store of the literal case value *inside* a case body (ProcItemGun's
  `common = (void *)1;` under `case 1:`) is cse's `record_jump_equiv` on the
  `beq index,CONST` taken edge: the pseudo==constant equivalence survives calls,
  so a plain literal constant in source reproduces the store while ALSO forcing
  the dispatch index into a callee-saved register. Don't hand-substitute a
  variable for the literal, and don't be alarmed the switch index is callee-saved.
- A mode-dispatcher that **reloads its variable** (two `lbu` of the same
  field) and compares **signed** (`slti`) is a real **`switch`**: cc1's
  `expand_case` emits a balanced compare tree over a *fresh* index load. An
  if/goto-ladder CSEs the load into one `lbu` and compares small unsigned
  types with `sltiu`. Case bodies are laid out in source order. A decompiler's
  long state ladder can therefore need to be restored as one full `switch`, even
  when labels/gotos initially look closer: ProcItemDokudango recovered both the
  compare tree and the physical mode-body order only after doing so.
- **If an entry guard names that field, a dead overwrite can be required to
  preserve the switch's fresh reload.** `mode_index = item->mode;` followed by
  a guard normally leaves a CSE equivalence that lets a later
  `switch (item->mode)` reuse the first `lbu`. When the target has a second
  `lbu`, write `mode_index = 0;` immediately before the switch. The assignment
  is dead and emits no code, but evicts the equivalence before `expand_case`
  (ProcItemKaengeki). Guided `autorules` enumerates this only when the named
  automatic local is provably unused from the switch onward, as
  `switch-cse-evict`.
- **A jump-table dispatch that opens `lhu / addiu -BASE / sll 16 / sra 16 /
  sltiu N` is a cast-narrowed switch index** — spell it
  `switch ((short)(ptr->mid - 0xA00))`. The `(short)` cast narrows the subtract
  to HImode: a raw `lhu` load plus a HImode `addiu`, then the switch promotion
  itself emits the `sll 16 / sra 16` sign-extension. A plain `switch (ptr->mid)`
  with `0xA00`-based case labels subtracts in SImode off an `lh` load — no
  sll/sra, wrong shape (ActHANG, dispatching on MotionManager.mid).
- **An explicit `if (cond) goto L;` ladder decouples test ORDER from body
  LAYOUT** — reach for it when an N-way dispatch fires its tests in a different
  order than the bodies sit in memory. cd_seek's 3-way `whence` dispatch tests
  1,0,2 but lays bodies out 0,2,1 (SEEK_CUR last, falling into the merge): a
  `switch` adds a range-split `slti` and an `if/else if` inlines each body right
  after its own test, but `if (whence==1) goto a; if (whence==0) goto b; …` with
  the *labels* ordered 0,2,1 reproduces the test sequence and the body addresses
  independently.
  This also applies when one path must **skip an earlier cleanup/drop block**
  and enter a later aim/sight block. ProcItemArrow needed an explicit early
  `goto aim;` for a valid attached human, preserving the retail forward skip
  around the invalid-human path. ProcSightShot similarly placed drop/common
  dispose before `sight_mode` and jumped into the later block. Structured
  nesting or one factored tail changed physical block order and cross-jump
  scope even though the path conditions were identical.
- A preceding test that keeps its constant in a **callee-saved register**
  across calls (`li $s4, 0xFF` + later `sb $s4`) is a local variable
  (`u8 ff = 0xff;`) also used by a later store. A path where the same value is
  materialized fresh (`li $v1, 0xFF`) uses a literal there — the register got
  reused for something else in between.
  - **Corollary (caller-saved `ff`):** the `u8 ff = 0xff;` entry-compare
    variable is instead *caller-saved* (`$a1`/`$v1`) when the path from the
    entry test to the dispose store `item->mode = ff` crosses NO call. Checking
    `item->proc` INLINE (`if (item->proc)` with no `ppu` temp) is what allocates
    `$v0` for both the null test and the `jalr` (ProcItemGun/ProcItemKawarimi).
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

For paired axes, preserve the **definition order** separately too: cache the X
coefficient, then compute/apply X's delta; cache a distinct Z coefficient, then
compute/apply Z. Hoisting both coefficients together or reusing one coefficient
local changes pseudo lifetimes and load scheduling even though the arithmetic is
identical. DefaultActionHumanoid's reflection correction needs this
coefficient-X/input-X/output-X then coefficient-Z/input-Z/output-Z shape. A
decompiler's one reassigned SSA temp is not evidence that the original reused the
source variable.

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

When the target has one conditional branch but the candidate has the inverse
conditional followed by `nop; j shared_return`, the residual is physical guard
layout, not missing behavior. `rtlguide` reports this as
`guard-return-island-layout`. Try the guided `if-else-invert` spelling or one
explicit shared return label once; if the target/candidate RTL still chooses
opposite islands, park it rather than reacting to the large address-drift byte
count. `SearchTarget` was semantically complete with an exact 0x50 frame and
stack access plan, but one such extra skip instruction made 572 bytes appear
different despite only five aligned hunks.

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
- **A bottom-tested scan can require its cursor-pointer derivation at the TOP
  of a real `while` body.** `i = 0; while (buf[i]) { p = buf; p += i; ...; }`
  makes the duplicated entry test consume `buf[i]`, then forms `p` as the
  body's first operation.  At the back edge cc1 independently forms the
  address for the bottom test and the next iteration's `p` (the latter can
  fill the branch delay slot).  The logically equivalent
  `if (buf[0]) do { ...; p = buf + i; } while (buf[i]);` puts both expressions
  in one extended block, so cse reuses one address and emits an instruction
  less.  AfsFindFile proved the A/B: only the real-`while`, two-statement
  cursor spelling reproduces its entry `i=0` placement and distinct bottom
  `buf[i]`/next-body pointer calculations.
- **`while (1) { if (!(cond)) break; ...; i++; }`** keeps the original's
  **top-test + unconditional back-jump** shape (first insn after the loop
  note is a condjump → no duplication) *and* still gets loop.c's invariant
  hoisting (address pseudos, division magic constants moved to the preheader).
  LoadConstruction demonstrates the large-function diagnostic: its natural
  outer record loop and final map loop each hoist the signed-division `16000`
  magic once, while equivalent hand labels repeat the `lui`/`ori` sequence at
  every division site.
- A hand-rolled `label: if (...) goto...` loop also keeps the top test but
  **loses hoisting** (no loop notes → loop.c skips it): magic divisors and
  invariant addresses get rematerialized per iteration. Wrong.
- **Repeated per-axis normalization usually has one shared loop, not nested
  component loops.** A decompiler may render `vx`, `vy`, and `vz` as successive
  nested tests/divisions because their branches rotate into one another. When
  the target repeats the same abs-threshold test for all three components,
  updates a single scale, and shares one back-edge/tail, recover one loop that
  halves all three components per iteration. Compare the per-axis def/use order
  in `.loop`/`.jump2`; blind statement permutations obscure the rotation.
  DamageControl's passage vector is the worked large-function case.
- **A `short` loop counter SUPPRESSES loop.c's strength reduction.** loop.c needs
  a plain SImode giv to rewrite `arr[i].f` into a walking induction pointer, so an
  `int i` gets you `p += 8` per iteration. A `short i` instead keeps the target's
  recompute-from-base shape — the counter's own sign-extend fuses with the scale
  (`(i<<16)>>13` for an 8-byte element), and the address is rebuilt as
  `base + i*8` every iteration. When the target recomputes but your draft walks a
  pointer, narrow the counter before touching anything else (SetupMotionRegist).
  (Not to be confused with the *3-instruction* GetPad sign-extension class.)
- **Put a loop increment inside the array subscript when the target increments
  through a narrow working copy.** `array[i] = value; i++;` updates `i` in
  place. The equivalent `array[i++] = value;` can instead expand as
  `addiu work,i,1; move i,work`, then reuse `work` for the counter's narrow
  sign-extension/test. That apparently redundant copy was the last missing
  instruction in ActKAGI and is also the documented shape in parked
  now-exact FUN_800270f8. `autorules` tries both forms as `subscript-postinc`, limited to
  a nonvolatile, non-address-taken local used only once in the affected full
  expression; `rtlguide` names the `postincrement-working-copy` residual.
  If that indexed field is both read and written in the iteration, capture its
  address at the postincrement site—`u16 *attribute =
  &model->object[i++]->attribute; value = *attribute; ...; *attribute = value;`.
  The earlier `idx=i; i=idx+1; ...object[idx]...` spelling could not produce the
  working copy because the increment was detached from ARRAY_REF. Also keep the
  pointer/value temps block-local to **each** disjoint loop: sharing them across
  FUN_800270f8's set/clear loops merged their allocnos, raised priority, and
  swapped `$a0/$v0` even though their live ranges never overlap.
- **Disjoint narrow loops do not have to share one source counter.** Reusing a
  function's earlier `short i` for a later table scan can join their allocno
  preferences and rotate the scan's address/index registers even though the
  live ranges do not overlap. Give the later scan its own block-scoped
  `short scan_i` when the target uses a different hard-register colouring
  (ActKAGI; independently confirmed by SwimCheck's splash loop and later
  CVAhuman scan). This is a scope/regalloc lever, not a semantic loop change.
- **Conversely, repeated hard-register identities can recover WHICH named
  counter drove each disjoint loop.** When PSX.SYM records `int i, j, k` but a
  decompiler renders every nested scan with anonymous indices, compare the
  target counters across the separate loops. ArrangeLocalMatrix's first
  row-normalisation loop and deepest elimination loop both use `$a2`, while
  the intervening row loop uses `$t4`: the original reused `k` for the two
  `$a2` loops and reserved `j` for the `$t4` loop. Using `j` for the first two
  adjacent loops was semantically identical and exact-length, but rotated 44
  bytes of caller-saved registers; changing only that loop variable to `k`
  produced a full match. Use `rtlguide`'s local names and `regalloc`'s pseudo
  dispositions to corroborate the grouping before adding artificial loop
  weights or narrowing a debug-proven `int`.
- **Sibling scans in one function can deliberately use different loop forms.**
  In StartStageSequence the first scans are `while`, but the third reorder loop
  and the score loop must be `for`. Their otherwise similar bodies are not a
  reason to normalize the source: only the `for` loop notes make reorg copy the
  top-of-loop `i+1` into two branch delay slots. Writing all loops as `while`
  produced the exact frame, registers, and bodies but was exactly two
  instructions short. When target-only increments sit in branch slots, compare
  `.loop`/`.dbr` before adding explicit duplicate increments.
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

- **A bounded scan with an independent terminator can require two literal
  exits in one `while (1)`.** Spell the bound as a top-of-body `if (...) break`,
  perform the body and increment, then spell the data terminator as a second
  `if (...) break` at the bottom. Combining them in the loop condition or
  turning the bottom exit into a `while` test changes the loop notes and which
  edge owns the increment. SetupTelop's glyph scan needed this exact two-break
  shape; use the target's two separately placed back-edge/exit branches as the
  structural tell.

## Expressions

- **A constant right shift can be staged without changing the value.** Under
  this compiler's signed/unsigned shift behavior, `x >> 8` and
  `(x >> 7) >> 1` are equal, but the intermediate RTL lifetime can perturb a
  caller-register allocation cycle. SetupTelop's substantive permuter found
  the 7+1 form as one of only two load-bearing mutations in its best
  exact-length seed. Guided autorules rule `shift-stage` now tries every
  positive two-part split of shifts 2..31 mechanically.
- **Signed fixed-point truncation can require the literal bias-and-shift form.**
  When the target tests a value for negativity, adds `0xfff`, then arithmetic
  shifts by 12, spell that sequence directly:
  `if (x < 0) x += 0xfff; x >>= 12;`. A tidier `/ 0x1000` or folded expression
  can expose a different combine/delay-slot schedule even though it has the
  same truncation-toward-zero result. SetWire needed Ghidra's explicit form to
  remove its final length instruction and expose the target loop schedule;
  verify the raw `bgez`/`addiu 0xfff`/`sra` sequence rather than generalising
  this to every fixed-point divide.
- **fold reassociation** (fold-const.c `associate`/`split_tree`): any
  `A - C + B` or `A + (B - C)` gets its constant migrated between operands.
  `t - 500 + rand() % 1000` reassociates the *wrong* way (constant lands on
  the remainder). Write **`t + (rand() % 1000 - 500)`** — fold canonicalizes
  it into the original's `(t - 500) + rem`. Semantically that's also the
  natural "position + centered jitter".
- **A named typed temp can isolate a centered-modulo tail from its base add.**
  `coord = view + (delta % 6000 - 3000)` and
  `offset = delta % 6000 - 3000; coord = view + offset;` preserve the same
  arithmetic, but the second spelling gives the remainder/bias chain its own
  pseudo and can change only the final add's allocation. In `DrawSnow`,
  the split form cut a 128-byte cascade to 26 bytes without changing length;
  reusing one `u32 offset` across the three non-overlapping coordinate wraps
  kept the modulo type exact. `autorules` rule `mod-bias-temp` now tries both
  a short block-local temp at each safe identifier-only site and one typed temp
  shared across repeated wraps. This is distinct from the rand-call rule below:
  there is no side-effecting call to keep in place.
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
- **An unsigned narrow field with a LATE signed consumer stays in a wide
  working local; put the cast at the consumer, not the load.** If the target
  has `lhu field`, updates that value in a full register, and only then emits
  `sll 16 / sra 16` immediately before a scale/store/call, write
  `s32 yy = node->y; ... yy = yy + slope; wide = (s16)yy * 10;`. Writing
  `yy = (s16)node->y` folds the sign into an `lh`; making `yy` itself `s16`
  lets combine narrow the intermediate arithmetic; and reusing the persistent
  `wide` result as the working value joins two allocnos and rotates the caller
  registers. In `FUN_8002fd9c`, the early-cast/reused-result form was exact
  length but 116 bytes off; a reusable wide `yy` plus separate `height0`/
  `height1` products left only the independent clamp tail. This is a
  producer/consumer data-flow rule, not a blind field retype: preserve the
  proven `u16` field and move the cast to the target's visible extension site.
- **A `u8` local re-narrowed after arithmetic gets a defensive `andi 0xff` +
  unsigned `sltiu`** even when the value provably stays in byte range. If the
  target has a signed `slti` there instead, declare the local `s32` — the source
  `lbu` load already zero-extends so nothing is lost, and cc1 then keeps the
  signed compare with no mask (FUN_800576e8). The same boundary occurs when a
  `u8` field feeds a range guard: copying ProcMiscPitfall's `m->mode` to an
  `int mode` changed `sltiu` to the target's `slti` and improved its final
  residual from 7 to 6 bytes.
- **A boolean flag's declared width (`s16` vs `s32`) changes whether `if (flag)`
  tests it in place or spills through a fresh-register copy — and can shift the
  whole-function instruction count.** When the length is off by a few insns and
  a switch/loop-driven success flag is involved, try flipping its width: a
  `s16` flag matched where `s32` was 4 insns short (handle_char_state_using_item_,
  139→143).
  SearchTarget's `mode = (status in range)` is the large-function version:
  keeping `mode` as `s16` reproduces the repeated promotion/copy chain used by
  its table indexes and comparisons; `s32` coalesces those target-visible
  copies even though the logical value remains 0/1.
  - **A narrow switch guard can also prevent reuse of an SImode case constant.**
    In ActATTACK, `short cleanup_guard` gives each `cleanup_guard = 3` an HImode
    definition. It cannot coalesce with the switch comparison's SImode `3`, so
    reorg duplicates `li 3` into three case-0 branch delay slots before the
    shared `andi`. Declaring the guard `int` reuses the comparison constant and
    loses those target instructions. This is a type-mode decision even though
    every source value fits in either width.
  - **A narrow completion flag can preserve a target-visible copy at its join
    and stop a later call literal from borrowing the flag register.** In
    ActDAMAGE, `short done` produces the retail `move v0,s0` join chain. It also
    keeps `Sound(..., 1)` as a fresh literal instead of reusing the live `s0`
    value; `bool`/`int` coalesces both effects away. Treat the copies and literal
    rematerialisation as one HImode-lifetime clue, not two unrelated register
    accidents.
  - **Mechanized:** `autorules`' default `type-width` rule enumerates the local
    scalar integer widths and signednesses. It deliberately skips pointer,
    array, multi-declarator, and multi-line declarations: changing the shared
    base type in `s16 scalar, *view;` changes memory access width even though
    the first declarator looks scalar. `rtlguide` recommends `type-width` for
    regalloc, CSE, schedule, and length residuals, so this case does not require
    guessing which flag declaration to perturb.
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
- **A pre-call `sll`/`sra` on an otherwise live argument register proves a
  signed-short formal.** It can simultaneously reveal an argument Ghidra
  omitted: FUN_8005b17c's `$a1` line number is explicitly sign-extended before
  `SetupTelop`, proving the caller-local prototype is
  `SetupTelop(u8 *, short)`, not the one-argument prototype in the old stub.
- **PsyQ `GetTPage` has four full-width `int` arguments; do not redeclare its
  coordinates as `s16` in individual TUs.** A narrow caller-local declaration
  inserts misleading caller-side extensions and can make a register residual
  look like an expression or scheduling problem. `include/psxsdk/libgs.h` is
  the canonical ABI (`s32 tp, s32 abr, s32 x, s32 y`), and the matching-tools
  tests reject TU-local declarations. This is an SDK ABI fact, unlike the
  deliberately caller-local game-function prototypes described below;
  centralising it changed none of the already-matched bytes (SetupTelop and the
  six other callers).
- **An expanded inline helper's pointer parameters can be deliberate CSE
  barriers.** Two direct calls using `&local` let cse carry the same address
  pseudo across both calls. Wrapping them in a small `static inline` helper with
  pointer formals creates parameter bindings during expansion and can force the
  target's fresh local-address formation without moving the aggregate's stack
  slot. FUN_800519bc needed
  `TimToDemoSprite(file, &image, &sprite)` around `GetTIMInfo` + `InitSprite`;
  inspect `.rtl` to confirm the address-pseudo split before introducing such a
  helper, and keep it inside a guarded draft so the asm-stub TU cannot emit an
  unused out-of-line copy.
- **A demo call that disappears in retail may be an INLINE EXPANSION, not
  rewritten source.** When `siblingdiff --demo` shows a named helper call in
  the earlier body but retail contains the helper's instruction shape in its
  place, reconstruct a small local `static __inline__` definition and keep the
  source call.  Hand-copying the helper's logic can lose its expanded parameter
  bindings, return islands, and local identities even when the behavior is
  identical.  AfsFindFile is the measured case: the 300-byte demo calls
  AfsFilenameFix once and subAfsFindFile twice; the 560-byte retail body calls
  neither, while one helper-sized target window contains 12/19 and 27/44 of
  their retail mnemonic bigrams respectively.  A
  hand-expanded draft was 36 bytes short; the two inline calls recovered the
  target CFG and matched in pure C.  `siblingdiff --demo` now reports this only
  when a missing named call has at least six matching helper bigrams and 60%
  helper coverage.  Treat it as a drafting hint and confirm the actual target
  islands—real source edits can also remove calls.
- **One absolute-state pointer reused across distant phases can become an
  unwanted saved-register allocno.** If the target rematerialises the same
  `0x80010000` base with separate `lui` sites but the candidate carries one
  pointer across calls/loops, declare a fresh block-scoped pointer in each
  logical phase. mission_score_screen needed separate initialise, rank-display,
  and table-shift scopes; one function-wide pointer changed the frame and the
  entire saved-register assignment. This is the absolute-address analogue of
  the expanded-inline stack-address barrier above: confirm the retained pseudo
  in `.lreg/.greg`, and confirm target rematerialisation in raw asm before
  splitting scopes.
- **A natural counted loop over stack arrays can hoist their bases and enlarge
  the frame even when the target recomputes each address.** When `.greg` shows
  a loop-invariant `sp+K` address pseudo crossing the loop and occupying a saved
  register/spill slot, a hand-rolled label/back-edge can suppress loop.c's
  invariant treatment while preserving the semantic count. The three-row
  rank loop in mission_score_screen required this to remove an eight-byte late
  spill area and restore the exact `0x1B8` frame. This is justified by the RTL
  allocno plus target `addiu sp,K` rematerialisations—not by frame-size guessing.
- **A second typed byte-offset view can force a fresh stack-array address
  without changing the lvalue.** If `sprite = &bank[i]` remains live but the
  target later recomputes `sp+K + i*sizeof(*bank)` for a final field store,
  writing `bank[i].field` may still CSE back to `sprite->field`. Spell only the
  proven fresh accesses as
  `((T *)((u8 *)bank + i * sizeof(T)))->field`: the value and type stay the
  same, but cc1 creates the target's independent base/index address pseudo.
  `mission_score_screen` needed this for the final `mx/my` writes in both local
  sprite banks; together they restored four target instructions after the
  normal subscript form coalesced them away. Require the raw target's repeated
  `addiu sp,K`/index block before using this—do not turn ordinary subscripts
  into byte arithmetic merely to perturb allocation.
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
- **Keep a narrow countdown field's new value in an `s32` temp when the target
  zero-tests it with raw `sll 16`.** `n = p->count - 1; p->count = n; if
  ((n << 16) == 0)` preserves the unnormalised full-width result long enough for
  the shift test. Re-reading the `u16` field after the store instead naturally
  becomes `lhu`/`andi 0xffff`, a different shape (ProcItemDokudango).
- A two-statement remainder temp (`x = rand(); x = x % 200;`) is provable
  from the asm: the `mult`/`subu` operate **in place on the moved call
  result's register** ($sN) — inline `rand() % 200` computes on `$v0`.
  `autorules` now tries both spellings as `rand-mod` for nonvolatile local
  destinations and literal moduli. In a run of several random components, also
  respect store order: publishing the offset results before an independent
  countdown initializer can let sched fill the target's load/call gaps without
  changing the arithmetic (ProcItemDokudango).
- **Two adjacent call results consumed once by one later call may belong
  directly in that call's argument list.** Separate
  `x = rand(); y = rand(); SetSplash(..., (x&7)<<12, (y&7)<<12, ...);`
  statements finish the first transform before starting the second. Writing
  both `rand()` expressions directly in `SetSplash` lets cc1 keep the first
  masked result across the second `jal` and put its `sll` in that call's delay
  slot—the exact SwimCheck pipeline. `autorules` now performs the paired
  `call-arg-pair` rewrite atomically only for byte-identical producer calls,
  distinct nonvolatile locals with one consumer use each, and no later uses;
  `rtlguide` names the target pattern `call-result-argument-pipeline`.
- **A narrow stepwise remainder/result temp can end a hard-register conflict
  that an `int` temp creates.** In `t = rand() % 25; t -= 26; t -= other / 20`,
  an `int t` is first used as the `/25` quotient while raw rand `$v0` is still
  live, so global allocation forbids `$v0` and prefers `$a1`. Declaring `t` as
  `u8` (the Ghidra-proven type; `s16` happens to generate the same bytes here)
  keeps the quotient in a separate SImode pseudo and defines the narrow result
  only after `$v0` is free. ProcItemNapalm's final 12-byte rotation disappeared
  this way. This is already mechanical: `autorules`' `type-width` rule tests the
  adjacent `s32 -> s16` form and authoritative scoring accepts it.
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
  SearchTarget needed this across an arithmetic/stack join: update `delta_y`,
  copy the resulting height into a separate `base_y`, store it to
  `delta.vy`, and derive the later adjusted height from `base_y`. Reusing one
  scalar collapses the target's `$v1`→`$a1` copy and renumbers the following
  normalization loop.
  - **Branch variant — a second pointer feeds the guard-branch delay slot.**
    When one branch of an `if` reads a pointer/value the other computes, and the
    target combines the address *eagerly* (before the guard-flag load) with an
    explicit register copy hoisted into the guard branch's delay slot: declare
    the copy `q = p;` right after computing `p` and before the `if`, then read
    `p` in one branch and `q` in the other. This forces the full address combine
    at that statement (instead of drifting into the delay slot) AND supplies the
    move reorg steals into the branch's delay slot (PadShock).
  - **Loop-exit variant — place a tested-value copy before the break guard to
    change its allocno home.** `exit = value; if (value & MASK) break;` and
    `if (value & MASK) { exit = value; break; }` can schedule the same move in
    the guard branch's delay slot and retain identical CFG, length, and calls,
    yet the earlier definition has a different live range/reference priority.
    In `debug_menu_player_jump`, the arm-local form was an exact-length,
    two-byte register residual (`move v0,v1` and a later test from `v0` where
    the target used `a0`); the pre-guard form byte-matched all 692 bytes.
    Guided `autorules` exposes both placements as `guard-exit-copy`, restricted
    to a simple nonvolatile copy, a pure condition, the loop's unique break,
    and no other in-loop use of the destination. This is a late allocation
    lever: first prove the copy/test chain with `rtlguide`, then score it.
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
  - **The same trap applies to register-passed coordinates used only by narrow
    stores.** A decompiler (and even m2c) may call `$a2/$a3` `short` merely
    because every visible destination is `sh`. If retail immediately copies
    each incoming register to a long-lived `$sN` with a raw `move` and never
    performs an entry `sll/sra`, test full-width parameters. In
    `FUN_800515b0`, changing both apparent `short x, short y` coordinates to
    `s32` changed no arithmetic or stores, but removed the final 22-byte split
    identity (early `$a2/$a3` uses versus later `$s7/$s6`) and matched all
    1,036 bytes. Autorules `param-width` now enumerates plain integer parameter
    widths and signedness mechanically.
  - **The target stack load is direct parameter-width evidence, even when the
    earlier-build symbol prototype disagrees.** Retail SetSmokeS loads its fifth
    argument with `lhu stack+16`, proving `unsigned short time` despite the demo
    PSX.SYM prototype's `int time`. The function still needs signed division:
    keep the raw `u16` parameter for its narrow store, then use a separately
    timed full-width host, `int t; ... t = (short)time;`, for `/` and `%`. That
    combination preserves the entry/load mode, the raw stored bits, the signed
    ASPSX division guards, and the target's two-instruction sign extension.
    `param-width` now tries adjacent width+signedness changes atomically, so
    `int`→`u16` does not depend on a score-improving `int`→`short` intermediate;
    choosing which later uses need the signed host remains dataflow-guided.
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

**A short moved into the high half has two useful source spellings.** For a
declared 16-bit value, `(u16)x << 16` and `x * 0x10000` can both reach the raw
`sll`, while extra outer casts or signed shift-and-extend spellings introduce
`andi` or `sll/sra` canonicalization. They are not interchangeable once nearby
assignments and signed tests enter CSE, so score both at the exact site the RTL
maps. Guided `autorules` exposes the casted-shift→multiply spelling as
`shift16-mul`; DamageControl needs both shapes on different paths.

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

**A variable that reads a struct field ONLY within one conditional arm and writes it back
at the arm's end should be scoped LOCAL to that arm, not hoisted to function scope.**
Ghidra's early top-of-function `uVar1 = field;` is frequently an SSA-placement artifact,
not true source position -- rescoping it into the arm fixed PutStrain's length exactly
(`NumberImage.u` was read only in the digit-loop branch, not globally).

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
not an array subscript (`SetCameraMode`). `ptr-index-sum` now handles both a
pointer declaration initializer and a later bare assignment, including `T **`:
DrawConstruction's `slot = DrawList + bucket` needed
`slot = (T **)((u32)DrawList + bucket * sizeof(*slot))` for the terminal
index-first `addu`.

**Naming an extern array base before indexing is a separate scheduling lever.**
`T *stage = &StageConfig[StageNo]` and
`T *base = StageConfig; stage = &base[StageNo]` preserve base-first pointer
arithmetic but give split-addresses/CSE a distinct base pseudo and earlier UID.
CreateStage needed the second spelling for its first scheduling block. Guided
autorules rule `ptr-base-split` now enumerates it for pointer declaration
initializers over an identifier array base.

**Independent row and element scaling identifies a multidimensional table, not
a flattened array.** If retail emits `row << log2(row_bytes)`, separately emits
`column << log2(element_bytes)`, and only then adds the two offsets, declare the
extern with its real second dimension and use `table[row][column]`. A flattened
`table[row * columns + column]` lets fold combine the indices before the final
element-size shift, often changing both the instruction order and every scratch
register in the address block. `think_setting_small_rotation_small_steps_`
needed `s16 AIDHumanType[][2]` to turn a 23-byte residual into a five-byte
sub-C-level phi tie while preserving the exact 1,392-byte function length.

**Build a dynamic row base before a large constant field displacement.** For a
flattened table, `state->stock[chr * stride + field]` encourages cc1 to add the
constant into the dynamic index first, then address the struct base. If retail
uses the large field displacement directly on a register, first spell
`row = (u8 *)state + chr * stride; value = row[field];`. FUN_80052ea8 needed
this shape to produce the folded `0x41f(row)` access instead of an extra
`addiu` followed by `0x40c(base)`.

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
rot/trans tables). **cse.c shares the same `MEM_IN_STRUCT_P` alias heuristic**:
a varying-address struct-member store (`dtV->vy = -0x23;`) does NOT invalidate
the cached load of a fixed-address non-struct scalar global (`lw gp(dtM)`), so a
following `dtM->count` re-read reuses the register with no reload — only a
multi-predecessor label forces a fresh gp load. This is exactly *when* the
d-globals reload across the `Act*` family (ActHANG case 4's no-reload vs
ActSYURI's reload after the `.L80025E68` join).

**A same-width scalar alias can deliberately clear a captured member load's
`MEM_IN_STRUCT_P` marker.** On the 32-bit PS1 ABI, `long y; y = p->field;`
and `y = *(s32 *)&p->field;` read the same signed 32-bit representation, but
old cc1 gives only the direct member spelling the structure-memory marker. CSE
can therefore delete or sink a one-use direct load across fixed-address
non-struct stores and collapse its hard-register lifetime; the scalar-lvalue
spelling keeps an early independent load live. In `DrawSplash`, plain `py/pz`
captures left a 51-byte residual, while the two aliases preserved retail's
early `a1/a2` loads and matched exactly; `px` needed no alias because it was
consumed before those stores. Use this only when `.rtl` creates the expected
locals, `.cse` is the pass that deletes/sinks them, and retail retains the early
loads. Guided aggressive autorules rule `member-scalar-alias` enumerates the
pure-C toggle and adjacent same-object pairs atomically (either alias alone
worsened `DrawSplash`). This is a matching lever for the old compiler and known
field layout, not a general strict-aliasing recommendation for modern C.

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

**Expanded variable divisions feeding adjacent narrow fields may need grouped
quotient temps before any store.** The target tell is several guarded `divu`/`mflo`
pairs followed by one contiguous `sb`/`sh` block. Preserve that dataflow literally:
compute every channel into a scalar local, then assign the fields together. Direct
`field = numerator / divisor` statements consume each quotient immediately and can
make maspsx retain an extra hazard `nop` around every expanded division. The A/B in
`FUN_80036284` was exact and large enough to diagnose mechanically: two three-channel
fade arms written directly were 752 bytes versus the 728-byte target (six extra
instructions); grouped quotient locals restored 728 bytes, the target stack layout,
and the contiguous color stores. Apply this only when the target itself groups the
stores; an interleaved target still calls for interleaved source.

**A target's "redundant-looking" extra reload means the source used a fresh field
dereference, not a cached pointer.** `vrealloc` writes `vh.next = vhp->next;` even
though a `nb` variable computed moments earlier holds the numerically identical
address; spelling it `vh.next = nb;` drops the reload the target has, because cc1
will not refetch what it can already see live in a register. Provable equality is not
a licence to reuse the variable.

**A short-lived pointer-to-volatile scalar can preserve a second signed/unsigned
view and its derived base.** `volatile u16 *stg_think = &stg->think;` forces a
later `*stg_think` to remain a real halfword load. In StartStageSequence this
prevented CSE from replacing the second read with the earlier chrid value and
made its address reuse the target's derived `$s1-10` base rather than rebuild
`$s2+2`; a nonvolatile pointer deleted the load. Guided autorules now toggles
this local pointee qualifier as `pointee-volatile`, restricted to plain integer
pointer declarations. Treat a winning volatile view as evidence that the
original source required distinct accesses, and keep it site-local.

**Corroborate an otherwise dead target load in another shipped build before
encoding it.** A load whose result is never consumed can be a real volatile-style
source access, but it can also be a bad carve, alignment artefact, or mistaken
interpretation. ActATTACK has the same dead `Me_MOTION_C` load immediately before
the root-model coordinate clears in both retail and trial executables. That
independent occurrence justifies the narrow, site-local spelling
`(void)*(Humanoid * volatile *)&Me_MOTION_C;`; do not make the shared declaration
globally volatile. Without cross-build or surrounding-RTL corroboration, do not
invent a dead read merely to fill four bytes. This remains a review rule rather
than an automatic mutation.

**A value used ONLY as a conditional call argument must be the inline ternary in the
call's argument position**, not assigned to a named local first (even via a ternary).
Assigning it first makes cc1 re-read the tested field a second time, with the wrong
signedness -- three extra instructions in `AVCameraSetup`'s `ordr`.

**A comparison's operand order is an instruction-order lever, not a sched tie.**
expand evaluates op0 first, so `found > mem` emits the (short) sign-extension `sll`s
BEFORE the `lh`, while `mem < found` loads first — same `slt` either way. Order the
comparison to match which side's evaluation the target front-loads (`GetConflictResult`,
8 bytes).

**Equivalent local-vs-local comparison polarity is a register-allocation lever.**
Even when `direction < turn` and `turn > direction` lower to the same `slt`, fold
creates/references the operand pseudos in the opposite order. In ActATTACK that
alone exchanged the `$v1`/`$a1` homes used by the surrounding turn logic; no
instruction shape changed. This is distinct from the memory/local rule above:
`cmp-swap` handles evaluation order when exactly one operand is a dereference,
while guided `cmp-polarity` enumerates nonliteral local/local comparisons near a
register or scheduling residual.

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

**Declaration order can control separately-declared same-type address-taken stack
objects' slots; do not assume reference order controls them.** `leLayoutEnemy`
provides a one-change A/B: keeping all references fixed while swapping only
`VECTOR tmp; VECTOR pos;` to `VECTOR pos; VECTOR tmp;` swapped every
`sp+24`/`sp+40` access and caused 26 byte differences. The matching order declares
`tmp` first (the later `SetBleeds` argument at `sp+24`) and `pos` second (the earlier
memset scratch at `sp+40`), so reference order does not determine these slots here.
`autorules`' ordinary `stack-decl-swap` candidate enumerates this exact bounded lever
for adjacent, uninitialized, same-typedef objects whose addresses are both taken.

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
  Large file-format buffers follow the same rule: `LoadCard` and `SaveCard`
  each have one `u8 block[0x2000]` at sp+0xe0, with the 0x200-byte card header
  and payload represented by typed casts into that buffer. Ghidra's several
  arrays are views, not separately allocated objects.
  Conversely, three scalar deltas are sometimes one real `VECTOR` stack
  local even when each component appears independently in Ghidra. SearchTarget
  only recovered its exact 0x50 frame and sp+0x10 access plan with one `VECTOR
  delta`; separate scalars colored into saved registers and erased the retail
  workspace. `register_character_death` independently confirms the stronger
  stack signature: `lw/sw` at sp+0x10/0x14/0x18 followed by an address-taken
  `SVECTOR` at sp+0x20 is `VECTOR delta; SVECTOR passage;`. The aggregate form
  collapsed an s0-s6 scalar allocation to the target's s0-s2 and matched all
  820 bytes. `stackplan` treats any slot that the function both stores and
  loads as local workspace, so its fallback argument inference no longer hides
  that VECTOR merely because the following aggregate supplies the first
  formed stack address.
- **Two xyz triplets exactly 0x10 bytes apart can be one `VECTOR[2]`
  workspace, not an output plus three scalar spills.** The strong target
  signature is an address formed for the first element, reads of its first
  three words, then `sw`/`lw` pairs at `base+0x10/+0x14/+0x18`. In
  FUN_80035f44, three independent `long` captures stayed in registers and
  produced only 200 instructions; storing them in `rotated[1].vx/vy/vz`
  recovered the target's sp+0x58/0x5c/0x60 workspace and 207 of 208
  instructions. A named readback after the neighbouring field store supplied
  the final schedule and matched all 832 bytes. `tools/stackplan.py` reports
  this signature as a `vector-array hint`; treat it as layout evidence and
  still verify that the values really have VECTOR semantics.
- **For mutually-exclusive aggregate layouts, make the overlap an explicit
  union instead of hoping scopes share slots.** Run `tools/stackplan.py <Name>`:
  it reads the candidate's outgoing-argument size, the target's first
  callee-saved spill, and every target/candidate `sp+offset` access. The reported
  working window is the exact scratch-union extent. Give the union one struct
  view per mode/path and use padding to place members at the target offsets.
  ProcItemFire's ordinary block locals made a 0xb0-byte frame; its target says
  args end at sp+0x18 and saves begin at sp+0x78, so a 0x60-byte union with
  particle, drop, explosion and frame views reproduced the exact 0x98-byte
  frame and every stack offset. This is an explicit layout technique, not a
  claim that every target used a union—verify the complete byte diff.
  The same rule now covers a broad hard-function sample: ProcItemJirai's
  cleanup/request scratch, DrawBlood's `scr@0x18`/position/temp overlay,
  FUN_8003562c's 0x28 render scratch, ProcItemShinsoku's position/launch
  overlap, ProcItemHenshin's packed saved-record view, and ProcItemKaengeki's
  request/camera-output window. When several independently recovered functions
  converge on the same target workspace boundary, prefer the explicit union
  over padding unrelated locals until the frame happens to fit.
  For a large frame with no trustworthy decompiler locals, add
  `--emit-overlay`: stackplan also recognizes `addiu reg,sp,offset` as an
  address-taken aggregate boundary, infers outgoing o32 argument space when the
  split target lost its `.frame args=` comment, and emits a padded C struct with
  scalar widths at every target access. LoadConstruction's scaffold recovered
  the full sp+0x20..0x177 window and led directly to its exact 0x1a0 frame.
  The generated names are deliberately neutral; replace byte regions with
  PSX.SYM/Ghidra-proven aggregates before treating it as semantic source. Use
  `--args N` when the inferred outgoing boundary is ambiguous.
- **A stack aggregate pointer can coexist with DIRECT member spellings, and
  that mix may be the target.** Keep `p = &local` alive for the one field whose
  store is base-relative and for the later call, but spell other fields as
  `local.field`. ProcItemFire needed `launch->user` and `ReqItemDrop(launch)` to
  retain `$s0`, while direct `launch.start/end` spellings emitted the target's
  `sp+offset` loads/stores and preserved its loaded-z call delay slot. Making
  every access `p->field` added a reload and a call-delay `nop`; making every
  access direct deleted the pointer.
- **A decrement stored to `u8` but tested later can want a full-width host plus
  explicit narrow tests.** `s32 count = byte - 1; byte = count;` followed by
  `if ((u8)count == ...)` keeps one SI pseudo for the add/store/control-flow and
  emits narrowing only where each path needs it. An `u8 count` may split the
  decrement and promoted value, leaving a copy and one missing instruction.
  ProcItemFire's switch matched only with the full-width host; ProcItemSmoke is
  the same family.
- **A named snapshot of a full-word global must stay full-width even if its
  comparisons fit in 16 bits.** Declaring `s32 clock = GameClock;` preserves
  the target `lw` and one SI pseudo; a `u16`/`s16` snapshot lets combine narrow
  the read to `lhu`/`lh` and can merge it into a nearby case literal
  (ProcItemArrow). This is already mechanical through `type-width`: do not
  infer a local's width solely from the constants compared against it.
- The **cast type's alignment drives copy code**: `*(VECTOR *)a =
  *(VECTOR *)b` is a word block move (4×`lw`+4×`sw`, `$t2/$t3/$t4/$t1`
  rotation; a 0x50-byte struct assignment becomes the 16-bytes-per-iteration
  copy loop), while `*(SVECTOR *)a = *(SVECTOR *)b` (align 2) is `lwl/lwr` +
  `swl/swr` pairs.
  - **An opaque `struct { u8 bytes[N]; }` deliberately carries alignment 1.**
    Assigning that struct between two byte-buffer views makes `emit_block_move`
    generate its runtime-alignment path and aligned/unaligned loop family.
    This exactly recovered LoadCard's 0xe70-byte persistent-state copy,
    SaveCard's 0x20-byte palette copy, and its three 0x80-byte icon copies.
    Do not hand-transcribe the resulting `lwl/lwr/swl/swr` loops.
    Exception for allocator control: SaveSI had the correct block-move shapes
    but needed the 0x80-byte icon copies written explicitly as 0x10-byte
    aligned/unaligned chunk loops. That keeps semantics and the runtime
    alignment dispatch while exposing `src`, `dst`, alignment, end, and chunk
    roles as reusable C locals. Use this only after the aggregate form proves
    structurally exact and RTL shows the residual is cursor allocation.
  - **This holds for a member of a parameter union too, even when Ghidra renders
    it as `long`-by-`long` temp copies.** `ef->param.bleed.pos = *pos;` (VECTOR,
    align 4) and `.vec = *vec;` (SVECTOR, align 2) reproduce the target's batched
    load-then-batched-store block move exactly; the field-at-a-time transcription
    does not (SetBleed).
  - **Four adjacent `vx/vy/vz/pad` assignments may be one aggregate VECTOR
    assignment.** `dst = *src` goes through cc1's block-move expansion and can
    batch/interleave the four loads and stores differently from
    `dst.vx=src->vx; ...`. DamageControl's target requires the aggregate form.
    The local, exact-field-set transform is now the default `autorules` rule
    `vector-copy`; the `pad` requirement prevents it from inventing a partial
    copy or silently changing padding semantics.
  - **Three field copies plus one adjusted component can require COPY FIRST,
    ADJUST SECOND.** If the target stores an unmodified component to a stack
    slot, later modifies that register, then overwrites the same slot, write
    `dst.vx = src.vx; dst.vy = src.vy; dst.vz = src.vz; dst.vy -= C;`.
    Folding the middle pair into `dst.vy = src.vy - C;` deletes the first store
    and leaves a load-delay `nop`; the explicit copy gives sched2 independent
    address/global work with which to fill that slot. ProcItemNinken gained one
    semantic store but no machine instruction—the store replaced the `nop`—and
    matched its full vector setup. `rtlguide` names the
    `copy-then-inplace-adjust` signature and `autorules`' symmetric
    `vector-copy-adjust` rule enumerates both spellings for nonvolatile locals.
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
  - **A whole-struct copy from a nonzero enclosing member may also need a NAMED
    source pointer.** `local = Enclosing.member;` can still fold the member
    displacement into each `lwl/lwr` source access, leaving only the enclosing
    symbol's `%hi` base.  The two-statement spelling `T *src =
    &Enclosing.member; local = *src;` forces one full member address first, then
    zero-based block-copy accesses.  Use this when the target has
    `lui base; addiu src,base,%lo(member); lwl/lwr ...,0/3/4/7(src)` rather than
    loads carrying the member offset themselves.  InitEffect verified the two
    forms A/B: only the named `blood_src` form produces its target copy of the
    eight-byte table at `D_80097A38 + 8`.
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
- **Repeated unrolled digit bodies can require independent source identities
  even though every copy operates on the same stack sprite.** When the target
  rematerializes `&sprite` in each copy but the draft retains it in one saved
  register, give each source copy a block-local pointer, current value, and
  quotient. A C macro is a convenient way to repeat that scope without emitting
  a helper call. Do not mechanically localize everything: values which the
  target carries through all copies must stay function-scoped. In
  StageEndScreen, a function-wide pointer/value/quotient draft had 3732 byte
  differences and 50.23% fuzzy similarity. Per-copy pointer, `u32 value`, `s32
  quotient`, and `s16 signed_value` identities reached the exact 6084-byte/1521-
  instruction size with 1690 differences and 79.68%; localizing the shared base
  or negative flag rotated saved registers and regressed. `rtlguide.py` now
  reports `repeat-block-local-pointer` when a large rematerialization gap and a
  candidate saved-register cache make this source shape likely.

  A related bottom-test lever is a destructive narrow working copy after the
  next value has already been saved:
  `value = quotient; quotient <<= 16; } while (quotient != 0);`. If `quotient`
  is dead after the test, this produces
  the target's `move; sll; bnez` chain. Testing `(s16)value` instead can create a
  fresh temporary and the wrong loop-tail allocation. Confirm the target chain
  and the quotient's deadness before using this spelling.
- **The final copy of a repeated digit loop may need a scoped byte base while
  the preceding copies keep a shared word base.** A function-shared `u32 base`
  keeps the repeated post-call `andi base,0xff` and gives the saved byte one
  allocator identity across the run. If every complete copy is exact but the
  LAST optional-sign suffix is one instruction long, clone only that final
  source block and shadow `base` with a block-local `u8 base`: capture it in the
  main digit pass, carry it across the sprite call, and reuse it in the optional
  minus pass. The byte pseudo then needs neither the shared word base's final
  `andi` nor a new copy. StageEndScreen's first fifteen loops required the shared
  `u32`; narrowing all sixteen destroyed the target shape, while narrowing only
  the last loop removed exactly its isolated one-instruction excess. Apply this
  only after per-region instruction and CFG counts prove that the unmatched tail
  is not being cancelled by the following repeated block.

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
tools/rtldump.py <Name> --lines         # + source-line-mapped object/disassembly
```

The mechanical front end is `tools/rtlguide.py <Name>`. It performs the
assembly alignment and pass selection, invokes the full standalone dump with
debug line notes, and reports:

- each residual hunk's likely owner (`greg/lreg`, `cse`, `combine`, `jump2`, or
  `sched2/dbr`);
- the concrete C line(s) that emitted our side of the hunk;
- candidate→target hard-register substitutions, including C locals and `.greg`
  preferences occupying the candidate register;
- instruction/label/move counts across the passes; and
- a bounded `tools/autorules.py <Name> --guided` search command.

`tools/rtlguide.py <Name> --json` exposes the same report to other tools.
`--no-build` is only safe when the current `.shake` image is already the
NON_MATCHING draft, not the default INCLUDE_ASM stub. `matchdiff` now checks
the linked map for `<Name>.NON_MATCHING` and refuses that stale/trivial artifact
even with `-n`; this caught a concurrent default build that briefly made an
unmatched guarded draft appear byte-exact.

There is necessarily no "target RTL" to diff: the retail executable contains
only machine code. Target assembly is therefore the specification; our RTL is
an explanation of how cc1 reached the wrong machine code. We mutate readable C,
never RTL itself.

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

Treat a shared physical suffix as stronger evidence than decompiler scopes. In
`ControlHumanoid`, the player camera's small-angle store and the enemy vertical
clamp's store both use the target's `$v1`, then jump into one shared
`UpdateCoordinate` tail. Keeping both signed roles in one function-scope `direction`
local let jump2 cross-jump the identical `sh` suffix; two tidier block locals left two
copies and made the exact-length function eight bytes too long. When mutually
exclusive branches converge through the same hard register and identical store/call
tail, first try one reused variable plus literal duplicated tails and let cross-jump
perform the factoring. `rtlguide` classifies the resulting register-only residual;
do not manually factor the source before testing this identity clue.

### A target-register hard conflict means weighting is the wrong lever

Before adding one-shot-loop depth to a register-only residual, inspect the
allocno's `.greg` conflict set. If the desired hard register is listed, no
priority change can put that pseudo there: change its lifetime, identity, or
coalescing. `rtlguide` now prints `HARD-CONFLICT` for candidate allocnos in this
state instead of merely suggesting allocation priorities.

ActCHASE supplied two exact, reusable forms:

- The target loads `MotionUpdateMode` into `$v1`, branches on it, then writes
  loop index zero back to `$v1` in the delay slot. Separate mode and `i`
  pseudos made `i` hard-conflict with `$v1`. Spelling the non-overlapping roles
  as one HImode chain—`i = MotionUpdateMode; if (i != 0) { i = 0; ... }`—made
  the delay-slot overwrite legal and simultaneously moved the compared human
  pointer to the target `$a0`.
- An artificial `next_motion` local spanning several predecessor arms
  hard-conflicted with `$v0`, which still held each pad comparison. Writing
  `motID = 0x501` / `motID = 0xb00` directly in the respective arms let
  cross-jump recover the same single physical store tail while the dead
  comparison register was reused for the literal. This is the direct-store
  complement of the shared-variable rule above: when a target branch delay
  overwrites its own condition register and all arms converge on one store,
  try duplicated direct lvalue assignments and let the compiler factor them.

Also distinguish an input value from its arithmetic result when the target
does. `current = rotation->vy; result = current + turn; rotation->vy = result;`
keeps the loaded old value and the new result in separate pseudos; mutating
`current` in place forces both roles into one hard register. ActCHASE required
this in both rotation blocks before the remaining residual became a true
allocation-only problem.

The same rule applies inside one large state machine: split logically distinct
lifetimes even when Ghidra gave them one SSA name. A scan's current `candidate`
and its final `found` result, a pointer used before a call and the equivalent
pointer reacquired afterward, or a mode-local `eater` and a later `human` often
need separate C locals/scopes so each receives its own pseudo and copy preference.
ProcItemDokudango used all three patterns. Conversely, when the target explicitly
copies through the owning path, spelling an equivalent access as
`param->eater->target` instead of `target->target` can force that source identity
to materialise; algebraic pointer equivalence does not imply identical allocation.

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

### A two-sided scalar clamp with one return island uses assignments, not direct returns

`if (x > HI) return HI; if (x < LO) return LO; return x;` can compile into
three physically separate return paths even though the target moves the input
into one return pseudo, overwrites that pseudo in either clamp arm, and reaches
one tail. Spell the latter source shape literally:
`if (x > HI) x = HI; else if (x < LO) x = LO; return x;`. On
`FUN_8002fd9c` this was the final 29-byte residual and changed directly to an
exact match. Ordinary autorules rule `clamp-shared-return` applies this rewrite
only to adjacent literal-bound tests of a nonvolatile automatic 32-bit scalar,
so narrow conversion behavior and observable destinations are left alone.

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

The same rule applies to scalar aliases at nonzero offsets. `StageSequence`
initially read the scalar symbol at `0x80089f04`, which produced `lui v1; lw
v1,...(v1)`. The target's `lui v0; lw v1,...(v0)` instead came from
`CamState.mode` at `CamState + 0x14`. When that two-instruction residual
appears, run `tools/symnear.py <scalar-name-or-address>` to list earlier fixed
symbols and their exact offsets, then validate the candidate struct field.
`rtlguide` labels the shape `enclosing-global-field-load`.

`ActivateHumans` independently confirmed the pointer form on a much larger
exact-length, exact-CFG draft. Its source initially loaded the fixed alias
`CURRENTLY_SELECTED_CHARACTER_STATE_PTR` at `0x80089f00`; `symnear` reported
that address as `CamState + 0x10`, and the already-proven camera layout names
that field `Owner`. Spelling the same load as `CamState.Owner` reduced the
authoritative residual from 740 to 730 bytes without changing any instruction
or CFG count. Do not delete or globally rename the alias merely because the
addresses coincide: different original translation units can legitimately
name an aggregate field and an absolute alias separately. Choose the spelling
whose `%hi`/load register identity matches the target at that use site.

Several apparent globals on one fixed page can be fields of the same aggregate.
Query them together: `tools/symnear.py CHOSEN_CHARACTER CHOSEN_LANGUAGE`
intersects their preceding-symbol windows and reports `PersistentState` with
offsets +4/+0x5e. CVAsetup matched only when all three reads used
`PSTATE->chr`/`PSTATE->language`: cc1 then kept the shared 0x80010000 base in
`$s1` across three calls. Separate scalar externs emitted another `lui`, while
caching only the character value modeled the wrong lifetime.

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

The matching build now passes `-fno-builtin` to **cc1**. Consequently an
ordinary `abs(x)` stays a physical library call even when the target has the
same inline `bgez; move; negu` expansion. Use `__builtin_abs(x)` to request the
inline form at that one site without changing the translation unit's flags.
ActKAGI's three-component short-circuit loop required the explicit builtin;
`autorules` applies `builtin-abs`, and `rtlguide` recognizes a candidate
`jal abs` against the target inline sequence as `builtin-abs-inline`. Keep a
plain `abs` call when the retail function really calls the library
(GetVectorLength).

The explicit sign-fix spelling can be a different allocation shape even when
it has the same final branch sequence:

```c
r = param->r;
/* independent statements may intervene */
if (r < 0)
    r = -r;
```

Here `r` is a named pseudo spanning the negative arm. In ProcMiscDoor that
pseudo was globally allocated only after the DoorData index temporaries had
taken `$v0/$v1`, producing a 37-byte register/schedule cascade. Writing
`r = __builtin_abs(param->r);` left the builtin's temporary to local allocation
and made the entire tail exact. `autorules`' `builtin-abs` rule now recognizes
this sign-fix form too: it changes the producer assignment in place and removes
the later `if`, crossing only call-free straight-line expression statements
that do not mention the local. This keeps the original value's evaluation
point and semantics.

### A conditional store via a pre-branch address copy is one assignment, conditional RHS

`move $v0,$a0` in a branch delay slot feeding `sh …,0($v0)` means the source is ONE
assignment with a conditional right-hand side (`xyz[j] = (cond) ? (t += K) : (t -= K);`),
not two stores. expand_assignment computes the destination address BEFORE expanding the
ternary, and cse folds the recompute into a copy of the earlier load's address register.
Use compound-assignment arms (`t += K`) to keep the value updates in place, and the
ternary's condition may need to re-read the target memory (`xyz[j] < 0` rather than the
equivalent `t < 0`) to keep cse1's taken-path window from canonicalising the store back
onto the original address register (`UpdateMotion`, last 4 bytes).

### Two branch-arm stores through one global pointer need a pre-branch local

When both arms write a field through the same global pointer, spelling the global
directly in each arm can emit one `%gp_rel` load per arm.  Capture it once at the
join instead:

```c
enemy = GetNearestHumanoid(Me_MOTION_C, 3000);
human = Me_MOTION_C;
if (enemy != NULL)
    human->target = (ModelType *)enemy->model;
else
    human->target = NULL;
```

This gives both stores one address pseudo and one load before the condition.  In
AttackControl the direct-global spelling produced two loads plus an extra skip
jump; the local-base spelling, with the non-null arm first to match physical body
order, produced retail's `lw v1,Me_MOTION_C; beqz; ...; sw ...,0x74(v1)` tail
exactly.  Apply only when no intervening call/write can change the global pointer;
the target's single load before the branch is the anti-oracle.

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

### Adjacent global banks may need distinct canonical symbols to prevent base folding

When the target materializes two nearby arrays independently but the candidate
turns the second into `first_base + CONSTANT`, CSE has learned that both names
belong to one address expression. Give the second bank its own fixed-address
symbol and use that canonical name in every C file; FUN_8003562c needed
`sprBlood2` rather than expressing the second sprite bank as the first plus
0x90. This is not permission to keep two aliases for one address: DrawBlood's
old `D_800BE9E8` name stopped linking once `sprBlood2 = 0x800be9e8` was pinned,
so the integration fix was to update DrawBlood to the shared canonical symbol.
Separate physical banks; unify source names for the same physical bank.

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

This applies to long-lived game roles as well as symmetric temps. DamageControl
uses one source local first for the attack id and later for damage severity, and
another across scale/speed/bleed-counter roles; the lifetimes do not overlap, so
the merged pseudos obtain the target's `$s2`/`$s0` homes. A tidier fresh local for
each semantic role lowered/split allocation priority and rotated the saved-register
file. First split decompiler SSA roles for correctness, then deliberately re-merge
only where `.lreg` proves disjoint lives and target asm proves one hard-register
home.

The roles need not even look related. DrawImpact's near-match reused fixed-point
interpolation temporaries in the later projection/priority phase: `inverse` also
carried `pz` (`$a2`), the green/blue start product also carried `py` and the OT
priority temporary (`$v1`), and the end product also carried `px`. Those
byte-neutral reuses cut the exact-length allocator residual to 53 bytes. Use the
target's repeated hard-register identity as the clue, then confirm in `.lreg` that
the phases are actually disjoint; semantic similarity of the variable names is
irrelevant.

### `A + (B + C)` controls which operand receives the constant

fold canonicalises `A + (B + C)` to `(A + C) + B`, so the `addiu` lands on A's
register — where it can fill a guard's delay slot — and the `addu`'s destination
ties to that dying operand. The mirrored spelling emits the same instructions with
swapped operands and a different result colour (`vfree`'s tail sum: `$v1` vs `$v0`).

This is visible across long arithmetic expansions, not only adjacent additions.
`MoveKorogari` needed the constant on `abs(vy) / 2` before cc1 expanded
`rand() % 25`: `A + 25 + B` put 25 on B, and `A + B + 25` emitted it after the
final sum, while **`A + (B + 25)`** produced the target `(A + 25) + B`. When
SequenceMatcher sees that `addiu` once as a deletion and once as an insertion
across the remainder sequence, `rtlguide` now rejoins the pair, classifies it as
`combine/expression`, and reports `constant-add-reassociation` instead of two
false structure/length hunks.

**Now mechanized:** guided autorules rule `plus-group` enumerates the six
association/constant-position trees for exactly two expressions and one integer
literal. It preserves the relative source order of both nonconstant expressions,
so two calls are never exchanged merely to move the literal. Exact byte scoring
chooses the surviving fold tree. Once the baseline has zero length delta,
autorules also rejects any non-matching candidate with a known nonzero length
delta before greedy adoption or beam ranking: a one-instruction shift can make a
large residual's aligned byte score look dramatically better without improving
the function.

StageEndScreen also showed why the right-associated address form is worth trying
after regional accounting: `(character * 0x1d4 + (stage * 0x24 + BASE))`
restored one instruction in an independently one-instruction-short post-game
region. Combined with the scoped final digit base above, all five semantic
regions reached their exact target lengths; this was not accepted as mere
whole-function size compensation.

### A narrowed three-term sum may need a named promoted prefix

An outer `(s16)` can make cse treat every leaf of `A + B + C` as narrow-use-only.
For signed halfword fields that means `lhu` is sufficient until the final
`sll/sra`, and fold is free to accumulate into the last operand's register. If the
target instead has signed `lh` loads and preserves `A+B` in its own register, name
only that promoted seam:

```c
s32 pair;
pair = object[0]->rotate.vy + object[1]->rotate.vy;
arg = (s16)(pair + body->rotate.vy);
```

Do not automatically name the whole three-term result: that extends a different
allocno and can rotate all call-argument registers. In `ControlHumanoid`, the
two-term prefix reduced an exact-length residual from 97 to 57 bytes; a full-sum or
third-leaf temp regressed to 68. Guided/aggressive autorules rule
`add-prefix-temp` now tries both contiguous two-term seams for exactly three
side-effect-free leaves under an explicit signed-16 cast, using a nested C89 block
to bound the temp's lifetime. The generated `_match_sum` is a probe; rename a
winning temp for the domain before committing it.

### Steering allocation: donate a call-arg preference to a low-priority pseudo

`global.c`'s `find_reg` makes earlier (higher-priority) allocnos AVOID a free
register that a later allocno *prefers*. So you can push a high-priority pseudo off
a free `$a2` and into `$a3` by donating an `$a2` call-argument preference to a
low-priority pseudo — e.g. by merging that pseudo with a call-argument temp whose
live range is disjoint. This is the lever when a same-length register rotation has
no C-level fix. Found by reading `-dg` on `vrealloc`: pseudo 85 was the top-priority
allocno with an explicit `preferences: 6` (hard `$a2`) inherited from the give-up
path's `memcpy` third argument; it took `$a2` first and rotated everything after it.
Run `tools/regalloc.py <Name> --prefer a2` to list only allocnos carrying that
preference, their final register, priority, and call-crossing count. SaveSI used
disjoint later calls as deterministic preference donors for reused copy-loop
roles (`src`→`$a0`, `dst`→`$a1`, alignment/result→`$v1`, third icon→`$a2`,
end/result→`$v0`). The donor need not remain adjacent in final assembly; it is
the merged pseudo and `.greg` preference edge that matters.

Duplicating one call in existing mutually exclusive arms is a related donor
lever: each source call can keep a block-local result preference, while jump2
cross-merges the identical output back to one physical call. SaveSI combined
that with a block-local duplicate of the symbol-address producer and a one-shot
loop fence, fixing a `%hi` coalescing/schedule tie without a surviving branch.

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

`Think3attack` independently confirmed the same signature at an ordinary
value-producing early edge: changing `goto return_pad;` to `return pad;` moved
only the final `sra`, closing a 10-byte residual with no surviving extra code.
`rtlguide` reports this exact epilogue permutation as
`shared-return-extension-schedule`; guided `autorules` then enumerates
`shared-return-split` only for gotos whose destination immediately returns.
The residual's source line normally belongs to the final return, so the rule
uses that label line to admit the distant goto without opening a global CFG
search.

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

### Sink a pre-guard flag assignment onto both edges to expose one delay-slot definition

When a flag is assigned immediately before a rejecting guard, these two shapes
are semantically equivalent for a nonvolatile local that the condition does not
read:

```c
done = true;
if (kind != EXPECTED) {
    goto check_done;
}
```

```c
if (kind != EXPECTED) {
    done = true;
    goto check_done;
}
done = true;
```

The edge-local spelling begins both definitions after the comparison. jump/reorg
can merge them into the comparison branch's delay slot, while the pre-guard form
usually materialises the flag before the compared value. This was ActDAMAGE's
last instruction-placement residual. Combined with a narrow (`short`) flag, it
also preserved the retail join copy and kept an unrelated call's literal `1`
from reusing the flag's saved register.

Guided `autorules` exposes both directions as `guard-flag-assign`, restricted to
an adjacent literal 0/1 (or false/true), a compound single-`goto` guard, a
nonvolatile non-address-taken local, and a call/macro-free condition containing
only known locals or parameters. It also rejects call-like uses of the flag and
locally defined macro expansions that name it: before preprocessing,
`#define REJECT (done && reject)` hides the read from an AST-only check, while
`CAPTURE(done)` may hide `&done`. Unary-address detection follows parentheses
and comments, so `&(done)` cannot hide an alias read in the guard. `rtlguide`
prioritises the rule for regalloc and schedule/delay residuals.

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

### Define a boolean after each arm's comparisons to reuse their result register

`flag = 0; if (outer) { flag = 1; if (inner) ... }` makes `flag` live across
both comparison pseudos, so global allocation cannot put it in their `$v0`.
When the target uses `$v0` for each `slt`, then writes `move v0,zero` / `li v0,1`
in the corresponding branch delay slots, put each definition after the work
whose comparison it can reuse:

```c
if (outer) {
    if (inner)
        update();
    flag = 1;
} else {
    flag = 0;
}
```

The assignments execute on the same paths, but their RTL live ranges now begin
after the tests. reorg can place both definitions in the delay slots with no
register conflict (ProcItemNinken's final three-byte tie). `rtlguide` reports
`post-comparison-flag-definition`; guided `autorules` can try the bounded
`flag-arm-assign` rewrite only when the local is nonvolatile, the arm does not
read it, and no early exit can bypass the moved assignment.

The reverse spelling is a separate control-flow lever.  If retail falls through
the false path while the candidate has an unconditional jump around `else`, use
the pre-zero form:

```c
flag = 0;
if (value < 0) {
    value = -value;
    flag = 1;
}
```

In `mission_score_screen`, a physical CFG audit showed 14 candidate
unconditional jumps versus eight in retail.  The first three decimal expansions
and the row loop use the two-arm form, but the next six pre-zero the sign flag;
preserving those as two macro variants made the inventory exactly 8/8 without
changing behavior.  That macro split and the affected invocation routing were
applied manually: current `autorules` operates on explicit function-body
statements and does not clone macro definitions or redirect their call sites.

For explicit statements, `flag-arm-assign` now enumerates the reverse rewrite
only for an unshadowed, nonvolatile function-scope local with a single
default-only false arm and the opposite 0/1 override at the true arm's end.  It
follows unary `&` through parentheses and rejects early exits, calls, macro
names, unknown identifiers, or direct/aliased flag observations in the
condition and true-arm prefix.  These restrictions matter because moving the
default across either region can otherwise change what a call, macro expansion,
or pointer alias sees.  Continue to review macro bodies manually, and do not
blindly normalize sibling expansions when their target CFGs differ.

### Split a path-produced result from the final flag at its CFG join

A target sequence such as `move v1,v0; move v0,v1; beqz v0,...` is not random
redundancy.  It records two distinct pseudos: a value produced only by the
computed paths is copied into the flag that all paths test.  If the candidate
has only `move v1,v0; beqz v1,...`, cc1 has coalesced those roles.

Keep the entry/default result and the path-computed result separate, and funnel
only the computed paths through one join before assigning the final flag:

```c
if (entry_case) {
    active = initial_active;
    goto active_done;
}
computed_active = expensive_path_test();
computed_active_done:
active = computed_active;
active_done:
if (active) ...
```

This is a CFG and liveness lever, not merely a rename.  In `ActivateHumans` it
recovered the target's path-result register and first join copy, reducing the
draft from 1,004 to 740 differing bytes while retaining the exact 1,608-byte
length and 0x68-byte frame.  The compiler still coalesced the final reverse
copy, so do not escalate automatically to `volatile`: confirm the copy chain in
`.lreg`/`.greg`, try at most a bounded zero-code liveness fence, and park a flat
result.  `rtlguide` detects this narrow residual as `path-result-copy-join`.

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

### Adjacent literal field stores are a bounded scheduling permutation

Two adjacent nonvolatile stores such as `frame->size = 0x3000;
frame->mode = 0;` have the same final state in either order, but sched2 can use
their source order to break a later independent scheduling tie. In
FUN_80037e0c, putting `mode` first fixed the final eight-byte tail residual even
though both stores themselves already aligned. Guided `autorules` exposes only
this narrow safe form as `adjacent-field-store-swap`: same base, distinct
fields, literal RHSes, and no intervening statement. Broader store permutations
can alias or move side effects and remain out of bounds.

> **Diagnose before you steer: `tools/regalloc.py <Name>`.** It runs `cc1 -dg`
> and shows which values are live across calls (forced into callee-saved
> $s0–$s7), the pseudo→hard-reg dispositions, and the register copy-chains that
> bias the coloring. When a draft is the right length + right instructions but a
> value is in the wrong register, this names the copy-chain to break or the live
> range to shorten — instead of blind statement-shuffling or permuting. A value
> is in $s0–$s7 *only* because it survives a call; if the target holds it in a
> caller-saved reg, shorten its live range so it no longer crosses the call.

- **Source liveness across a call wins over the final scheduled position.** A
  value assigned immediately before a projection/helper call can be scheduled
  to an instruction after that call, yet its pseudo still crosses the call and
  therefore forces a callee-saved register and a larger frame. Moving the
  assignment after the call may make the visible instruction order look the
  same while deleting that saved register. DrawBlood and FUN_8003562c both
  needed the source-before-call form; use `regalloc.py`/stackplan to decide,
  not the final instruction address alone. SaveCard is the smaller inverse-
  looking example: assigning `chan = 0` before the third `GetArcData` call lets
  sched move that definition after the call but before the `$v0` result copy.
  Writing it where the final assembly appears loses that schedule.

- **A named `zero` local can flip a pure register-swap tie — cheaper than the
  permuter.** Comparing a loop bound against a `zero` local (`int zero = 0; …
  while (n > zero)`) rather than the literal `0` shifts global-alloc's
  pseudo-number tie-break enough to swap two coalesced registers with no other
  effect — plausible as a real sentinel/bound local. Try it before reaching for
  `tools/permute.py` on a ≤-few-byte register-swap residual (GetArcData).

### A `goto`-label's placement decides which copy keeps the source's register

cse's `make_regs_eqv` (cse.c) will rewrite later uses of `x` to a copy `flag = x`
*only if* `flag`'s live range ends both **beyond the block** and **later than
`x`'s**. The C-level lever is **creation order**: placing the hit-handler block
(the `goto hit;` target) *after* the after-loop check extends the handler-set
variable's range past `x`'s, so the store keeps `x`'s register instead of the
copy's. Moving the labelled block earlier/later flips which of two coalesced
values owns the register (ProcItemSmoke's hit handler). Same family as "statement
order steers tail-duplication," but the knob is *label position*, not def order.

### `optimize_reg_copy_1` propagates a copy within a block; a `do{}while(0)` walls it off

local-alloc's `optimize_reg_copy_1` propagates `[copy dest,src] … [src REG_DEAD]`
by rewriting src→dest across the block, and its scan **stops only at JUMP/LABEL/
LOOP notes** — plain statement order does not stop it, because sched1's REG_DEAD
rank-boost hoists the copy regardless of where you write it. So when the target
keeps the copy un-propagated (two live registers, not one), the only source-level
barrier is a real basic-block edge between the copy and the death: wrap that span
in `do { … } while (0);` (its back-edge note halts the scan). Reach for this when
a draft collapses two registers into one that the target keeps distinct
(ProcItemSmoke).

The fence can be **local**, around one conditional scan rather than the whole
function. ActATTACK wraps each `if (MotionUpdateMode) { for (...) ... }` scan in
a one-shot `do`; the loop notes prevent the unwanted load/copy propagation and
change weighted reference order, while optimized output gains no branch. Guided
`autorules` now enumerates eligible `if`, `for`, and `while` statements as
`loop-fence`. It rejects an `if` containing `break` or `continue`, because the new
wrapper could capture those statements; byte scoring still decides whether an
otherwise safe fence helps.

An assignment alone can be the fenced span. LoadSI's `msg = 0` normally
propagated the literal directly into later card calls and delayed the real
`$s1 = 0` definition into a branch slot. Wrapping only that assignment in a
one-shot `do` stopped the constant/copy propagation, scheduled the definition
before `valloc`, and made `$s1` feed `MemCardAccept`/`MemCardSync`, with no
surviving branch. This is already an ordinary guided `loop-fence` candidate;
the RTL symptom is one missing long-lived zero pseudo, not a wrong constant.

A producer can sit inside the fenced assignment's **comma RHS** when the
residual is only instruction order: `do { table[1] = (offset = i * 4, 58); }
while (0);`.  This keeps the literal definition and store on opposite sides of
the producer in RTL without adding runtime control flow.  InitEffect's A/B was
exact and local: separate statements produced either `li; sw; sll` or
`sll; li; sw`, while the comma form alone produced the target `li; sll; sw`.
Reserve this for independent nonvolatile local assignments whose final values
and later uses are unchanged; moving an effectful call, volatile access, or
possibly aliasing store into a comma expression is not semantics-preserving.

An expression fence can also supply exactly one allocator-priority weight.
ProcMiscDoor's final three-byte tie had `t` at 3 refs / 5 live insns (priority
6000) below `angle` at 3 / 4 (7500); `regalloc.py --compare 217 218` reported
that one weighted ref was sufficient. A one-shot loop around only the ratan2
assignment emitted no control-flow instruction, raised `t` above `angle`, and
let `t`, `angle`, and the later `dir` reuse `$v0` at disjoint lifetimes. Prefer
the smallest `rtlguide`-selected expression fence when the comparison says one
weight is enough; broader loops can perturb unrelated scheduling.

Sometimes the target needs only the **post-LOOP_END scheduler barrier**, not the
enclosed reference weight. Insert a standalone `do { } while (0);` *between*
the producer and consumer in that case. Its empty body contributes no weighted
references, while sched still gives the first following instruction the full
pending-register/pending-memory dependency. CameraDirection supplied the
direct A/B: no boundary made the draft one instruction short; wrapping
`CamLoc.vx -= sin` restored the schedule but reweighted `sin` and exchanged its
saved-register home with `cos`; an empty loop immediately after that assignment
restored the 215-instruction retail length without the register exchange.
SetupImageToPolyFT4's exact empty boundary after `y += ph` is the earlier
independent example. Guided `empty-loop-boundary` now enumerates this
weight-free fence at adjacent statement boundaries; use ordinary `loop-fence`
when the enclosed value really does need more allocation priority.

A redundant write to an **unobserved automatic aggregate** can be an allocation
donor too. CameraDirection's bounded permuter first repeated
`target.vrx = CamLoc.vx - r.vx` after the neighbouring `vpx`/`vry`
initialisers, reducing the exact-size residual from 72 to 56 bytes. Once
mechanised, exhaustive local scoring found the stronger `target.vpx` repeat at
the same boundary (72→47). cse removed the second store in both cases, so
function length and behaviour were unchanged, but its temporary references
re-coloured the tail. Guided `redundant-field-donor` tries that narrow shape
across one to three sibling field initialisers. It requires a nonvolatile,
non-address-taken local aggregate, direct `.` fields, a
call/update/dereference-free RHS using only automatic locals, and no
intervening non-field statement. This is not permission to duplicate pointer,
global, volatile, or call-bearing assignments: those writes may be observable,
and a retained extra store is a structure regression rather than evidence.

The fence may need to cover a **contiguous producer/consumer range**, not one
AST statement. AttackIndirect's final copy-propagation tie closed only when one
`do { ... } while (0)` enclosed three adjacent `if` statements together; fencing
any member alone did not create the needed note boundary around the whole live
range. Guided `autorules` therefore also enumerates `loop-range`: two-to-four
adjacent statements, excluding declarations and any nested break/continue/case/
label whose target or scope could change. This is a bounded RTL-line-guided
search, not permission to scatter dummy loops blindly.

`paired-loop-fence` is the atomic complement: it splits two-to-four adjacent
statements into two separate one-shot loops. DrawConstruction needed both
slot/plimit fences together to keep two allocation priorities ordered; either
alone crossed a rival priority or created a target-absent scheduler `nop`. The
paired rule lets scoring see the combination without requiring a regressing
intermediate beam state. Single `loop-fence` also accepts a plain expression
statement, covering LoadSI's exact `msg = 0` fence.

Loop weighting and expression splitting can be one allocation operation rather
than two independent cleanups. ControlHumanoid first needed three weights on a
final `direction = (target_y - y) / 2` to put `direction` in `$v1`; splitting the
same expression through the now-dead, same-type `magnitude` local then donated
the numerator's `$a0` preference and reduced the exact-length residual from 42
to 17 bytes:

```c
magnitude = target_y - y;
direction = magnitude / 2;
```

The inserted assignment still vanished. In this accepted case the pre-split
RTL recorded `REG_EQUAL (div:SI ...)`, and both candidate and target used the
signed bias-plus-`sra` sequence for division by two; the field expression's
signed division was therefore established independently of the rewrite.

Guided `dead-host-split` mechanically tries a deliberately safer subset of this
composition. Destination and host must be unaddressed, nonvolatile,
function-scope signed-32 scalars of the same type; the host must have an earlier
use and no later source occurrence; and `(A +/- B) / literal` must have a
provably signed numerator (signed-32 locals/parameters or an explicit signed
cast) and divisor. It rejects a function containing `goto`, a labeled loop, a
repeating-loop ancestor, or any later call-like token that is not a declared C
function and could therefore be a local-capturing macro. Existing one-shot
weighting loops are allowed. These conservative guards mean the rule need not
fire retroactively on ControlHumanoid's raw field expression and earlier gotos;
they keep future automatic probes semantics-preserving. Do not reuse a merely
*apparently* dead local by hand when control-flow re-entry, unsigned conversion,
macro capture, address escape, or a later iteration can observe the new value.

When one value needs more than one unit of loop weight, `nested-loop-fence`
tries two and three nested one-shot loops as one candidate. This avoids making
a beam discover a useful depth through a neutral or regressing single wrapper.
FUN_8005b17c needed two nested weights on `scan = text` to place its four
call-crossing values in `s0`/`s1`/`s2`/`s3` order.

### Inline extended asm is an anti-rule, not a matching transformation

An empty compiler barrier can make old GCC retain a desired copy without
emitting an instruction, but it encodes no recovered game logic and is too easy
to use as an opaque register-allocation escape hatch. Do not land it in a
matching function. If an experiment containing inline asm is the only known
match, keep the function nonmatching (or park the experiment) and continue
working backward from `.rtl`, `.lreg`, `.greg`, and `.jump2` to a real C cause.

StateTransition is the worked correction: its experimental barriers were
removed and the exact match recovered with repeated field access, relational
polarity, a real referenced `case` label, and a one-shot `do` that supplies the
needed loop notes. Those constructs explain source-level intent and survive
review; a synthetic clobber does not.

`autorules` contains no asm-producing rule, and `rtlguide` never recommends one.
`matchdiff` also treats an otherwise byte-exact result as incomplete while its
source still contains a `NON_MATCHING` guard, `INCLUDE_ASM` fallback, or inline
`__asm__`; it returns failure until the unconditional plain C is rebuilt and
verified. This prevents an exact shadow draft from being committed as “matched”
while the default executable still links retail assembly.
A target-only move is reported only as a register goal for inspecting the
candidate's copy chain. This is a deliberate stopping rule: exact bytes are the
verification gate, but they do not make an arbitrary compiler directive a
credible decompilation.

### A search-flag's initialiser placement flips it callee- vs caller-saved

`found = 0;` written at the **top of the search loop** is loop-invariant → loop.c
hoists it into the preheader → the flag survives the loop body's calls and goes
callee-saved ($s), shifting the whole allocation. Written **inside the
exhaust/break arm** instead, it stays caller-saved and reorg can steal its `li`
into the exit branch's delay slot. Pick the placement that matches where the
target's flag lives (ProcItemLaunch/Smoke search loops).

### A block copy through `*p` coalesces the copy cursor across a multi-pred join

`rparam = *p;` (copy the pointee) lets the block-move's source cursor coalesce
with the pointer variable across a multi-predecessor label (`case 2: case 4:`),
matching a target that reuses the pointer register as the copy source;
`rparam = param;` (copy a named aggregate) re-materialises the source address
after the join and adds an `addiu`. When a struct-assignment's source register
should be the incoming pointer itself, spell it as `*p` (ProcItemLaunch).

### reorg deletes a same-address reload only if the register is untouched between

`reorg`/`dbr` (delay-branch, post-allocation) does redundant-insn elimination: a
`[sw R,x … lw R,x]` pair with **R never redefined between** collapses to just the
store — cse/combine never fold these (they're a pure coloring/reorg interaction;
verified absent from `.cse`/`.cse2`/`.combine`). So when the target KEEPS a
"redundant" reload (e.g. re-reading `p->end.*` after storing it), the stored
register must be REDEFINED in the interval — funnel the stored values through one
serially-reused temp (`t = v.vx; p->end.vx = t; t = v.vy; …`) so R is live-again
and reorg can't delete the reload. Conversely, to make a reload vanish, keep its
register untouched across the span. ReqItemUse's lightningbolt end-vector tail
initially exposed a tension between this rule and the required coloring order;
it was resolved by turning the tail's values into function-scope pseudos and
giving them byte-neutral uses in a later case (see the multi-anchor rule below).
Note: a `sz = 0;`-style dead def can't steer this —
`delete_trivially_dead_insns` runs post-cse and the ref counts are recomputed
before local-alloc, so the def is gone before it can bias anything.
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
    off, try both shapes. The default `autorules` rule `flag-return-split`
    now performs the guarded default/override-to-literal-return direction even
    when useful statements follow the override. In FUN_8005adbc this reduced
    a replay of the shared-result form from 27 whole-image differing bytes to
    an exact 0-byte, 240/240 pure-C match; no permuter run was needed.
- **Source statement order is the main regalloc lever.** Storing
  register-held values first (`prm.type = ty; prm.user = own;`) before
  memory-chain stores frees their registers for later constants — in
  ProcItemKusuri that one swap released `$s0` for the division magic and
  collapsed a whole extra callee-saved register out of the prologue. It also
  decides the final call delay-slot candidate: Think1target's independent
  target-pointer store had to precede its Attrib update, moving the `sw` out of
  `SetNowMotion`'s delay slot and leaving `li $a2,1` there (24→0 bytes).
- **A chained assignment is a reverse-store and shared-constant lever.** C
  evaluates `a = b = c = K` right-to-left, so distinct fields store in
  `c,b,a` order and share the assignment-value lifetime; three standalone
  statements naturally store forward. SetupBG required chains for RGB,
  scale-x/scale-y, and cell-width/cell-height to reproduce retail scheduling.
  `assignment-chain` now mechanically merges two-to-four adjacent same-literal
  stores to distinct fields of one nonvolatile local aggregate, and splits an
  existing chain into forward/reverse explicit orders.
- **Capture an overwritten field inside the new value's RHS when RTL wants the
  independent arithmetic before the old-field load.** The conventional
  `saved = p->u; p->u = saved + expr;` makes the capture a separate producer;
  `p->u = expr + (saved = p->u);` puts it in the store's dependency tree and
  lets old cc1 build/schedule `expr` first while preserving the old `u` for a
  later restore. StageEndScreen needed this shape in every decimal-rendering
  loop. Guided `field-capture-rhs` tries both fused addend orders and both
  conventional split orders, but only for adjacent statements, one exact
  field path rooted in a proven nonvolatile automatic object/parameter, an
  automatic saved local, and an other operand containing only proven automatic
  scalar/member reads (no calls, updates, assignments, raw dereferences, or
  subscripts). It also parses private helper-macro bodies used only by the
  target function, mapping captured names only from those safe outer objects
  and rejecting macro formals. Do not use the shape to move volatile reads or
  effectful work: C does not specify the evaluation order of `+` operands.
- **A comparison literal can be named before an earlier guard so its `li`
  fills that guard's delay slot.** If the target materializes a constant before
  the comparison that logically owns it, assign a local before the preceding
  zero/null guard and use that local in the later comparison. Its mode still
  matters: ProcItemArrow's aim bound had to be `s16`, not `s32`; `type-width`
  found the narrow form that removed the final `$v0/$v1` tie. Do this only when
  the local is live on every path reaching the comparison—the early placement
  is a scheduling/lifetime claim, not cosmetic hoisting.
- **A three-instruction load/literal/equality reversal is a bounded
  commutative tie.** When target and candidate are identical except
  `load $v0; li $v1,K; bne $v0,$v1` versus
  `load $v1; li $v0,K; bne $v1,$v0`, try `K != value` versus `value != K`
  once (`eq-literal-swap`). If the score is flat, the logical comparison and
  branch shape are already right; do not widen into unrelated permutations.
  ActJUMP stayed at its final 3-byte residual under both forms (and swapping
  inequality arms worsened it), so `rtlguide` now names this exact pattern
  `commutative-equality-register-order` as an early-stop signature.
- **A same-address store must go through whichever pointer variable holds the
  base register the asm uses** (`it->param` vs a `pp` alias, even when
  numerically identical): value unaffected, but it decides a delay-slot
  scheduling tie — the wrong one leaves a small pure-reorder residual (Launch:
  9 bytes, `pp+0x28` → `it->param+0x28`).
- For a guarded indirect call, null-check through a variable but **call
  through the field**: `ppu = item->proc; if (ppu == 0) return; ...
  item->proc(item);` — cse reuses the loaded value and allocation lands in
  `$v0`; calling through `ppu` flips it to `$v1`.
- **A cross-predecessor indirect cleanup can need one function-scope callee
  pointer.** Assign `call_item = item->proc` separately on every predecessor
  that reaches the shared cleanup, then null-check/call through that pointer at
  the join. This gives `jalr` one merged incoming pseudo without forcing a
  fresh field reload at the label; predecessor-local pointers or a direct
  field call produce different copy/hazard shapes (ProcItemJirai). This is the
  complement of the duplicated-cleanup rule below: follow the target's actual
  join placement before choosing one.
- Cached pointers that live in `$s`-registers across calls
  (`p = &item->param;`) are real source temporaries — indexing the base
  struct directly doesn't allocate the register (see ProcItemManebue).
- **A call result used as an array index can require naming the array base
  before the call.** `base = Global; r = GetResult(); use(base[r]);` makes the
  address live across the call, where direct `Global[r]` materializes it only
  afterward. In ProcMiscPitfall, the direct `ConflictObject[r]` form produced
  218 instructions; a block-local `ConflictObjectType *conflict` assigned
  before `GetConflictResult` restored the target's 217-instruction length.
  Keep the declaration at the start of the path-specific block so it remains
  valid for the original compiler dialect and does not extend the base across
  unrelated paths. Guided `autorules` mechanizes this immediate
  call/consumer shape as `ptr-base-split`.
- **Initialize a pointer alias at its first path-specific use, not
  automatically at function entry.** An early `locate = dtL` keeps the alias
  live across unrelated guards and hoists the gp load. Assigning it only in the
  successful conflict-object arm places the load at the target use and can
  keep the alias caller-saved (SwimCheck). Ghidra's entry SSA name is not proof
  of source placement; follow the first machine use and predecessor set.
  A call can be the boundary between two aliases of the same address:
  FUN_80037e0c needed `position_base = &work.pos` before `rand()` and
  `position = position_base` after it. The two names create the target's
  preserved pointer copy; one alias throughout lets cc1 coalesce it away.
- **Separate a scanning cursor from the result captured at loop exit.** Even
  when `found_slot` always receives the current `slot`, assigning it before a
  shared post-loop label creates the target's pool-exit move and lets the scan
  cursor die. Reusing `slot` through the consumer extends its allocno and
  changes the tail colouring (FUN_80037e0c).
- **But a dead pointer chain may need to remain direct.**
  `human->model->rotate.vy += K` lets cse overwrite/coalesce the dead `human`
  and model intermediates into the target register. Naming
  `Model *model = human->model; model->...` creates a distinct allocno and can
  rotate the caller-saved registers even though the load/store addresses are
  identical (ProcItemKaengeki). Cache a pointer only when the target keeps it
  live or reuses it; direct chains are also an allocation lever.
  The useful boundary can be the **enclosing object**, not the final member:
  ProcSightShot needed `ModelArchiveType *model = owner->model;` and then
  `&model->rotate`. Precomputing `SVECTOR *model_rot = &model->rotate` created
  a second address pseudo, changed the owner→model chain from `$a1` to
  `$v1/$v0`, and inserted one hazard `nop`. When `rtlguide` points at a
  pointer-chain CSE knot, name each possible prefix boundary separately; do
  not assume the deepest pointer is the best cache.
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
- **Rank those directories by the linked executable, never by the directory
  number.** `tools/permute.py` automatically recompiles each retained `source.c`,
  substitutes its object into a private copy of splat's linker script, resolves
  gp/branch/rodata relocations at the retail addresses, and reports
  `whole image / function window / .text size`. It does not touch the source or
  Shake object tree. Use `tools/permute.py <Name> --rescore-only` after Ctrl-C or
  an external timeout. `ControlHumanoid` proved why: proxy output-525 was the
  real 57-byte winner, while several numerically lower proxy candidates were
  68 bytes away or the wrong length and therefore hundreds of thousands of
  whole-image bytes worse.
- **A late reuse of an existing pointer local can recolor an early block at
  zero byte cost**: global.c allocates one pseudo for the variable across the
  whole function, so assigning a far-later same-kind pointer to it changes its
  refs/lifetime/preferences even when the late block emits the same instructions.
  SetupAppearance's 1024-byte draft was otherwise exact but swapped `$a0`/`$v1`
  in six early instructions; reusing its HumanData pointer temp for the final
  `StageMotion` null-check/free made the whole function MATCH. The permuter found
  this as a nonzero `output-25` candidate; bisecting that one edit against raw
  matchdiff proved it was the load-bearing change.
- **The inverse lever is to stop a late block from redefining an existing
  pointer local.** `p = Global; ... p = Global; p->x ...` makes `p` one
  multi-definition global allocno even when both assignments load the same
  address. If the late region has no mutating calls or writes to the global pointer,
  use `Global->x` directly there: CSE still emits one load for that block, but
  the named `p` remains single-definition and its earlier coloring can change.
  ActJUMP's final three-byte `motID == 0x906` `$v0/$v1` tie was fixed solely by
  dropping a far-later `velocity = dtV` and spelling four air-steering fields
  through `dtV`; the 1,396-byte function then matched exactly. Autorules rule
  `late-pointer-direct` now enumerates this bounded, mutation-free form.
- **Several byte-neutral late reuses can anchor an entire caller-saved
  coloring**: when RTL dumps show that a short-lived block local is being
  claimed by local-alloc, promote the affected same-mode values to
  function-scope locals and reuse each in a distant block whose target hard
  register is already known. ReqItemUse reused three SImode locals in its
  kaginawa case for the items base (`$a1`), `ProcKaginawa` address (`$v0`),
  and scaled item index (`$v1`). Those statements emit the same instructions
  as the natural kaginawa spelling, but make all three pseudos globally
  allocated; reusing two for the earlier lightningbolt sums fixed its final
  15-instruction register permutation and matched all 5272 bytes.
- **One local can intentionally span several disjoint semantic roles.** If
  lifetimes do not overlap, old cc1 can keep page offset, line number, and final
  result in one source `int`, preserving one callee-saved home across all three
  regions. FUN_8005b17c reused `n` this way to keep every lifetime in `$s3`; a
  final `(short)n` supplied the retail return extension. Separate descriptive
  locals changed allocation even though their live ranges did not overlap.
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
  LOOP_BEG/END): the FIRST real insn after `NOTE_INSN_LOOP_END` receives a full
  pending-register/pending-memory dependency (`reg_pending_sets_all`), so moving
  one source statement across an existing one-shot loop boundary can reverse
  apparently independent loads. `autorules`' guided `loop-boundary-shift`
  mechanically tries `do { A; } while (0); B;` ->
  `do { A; B; } while (0);`. More generally, a do{}while(0) weight lever must ENCLOSE any insn that has
  to float across it, and must extend past a switch's tail if a case arm
  jumps out of the note range — otherwise jump.c moves that arm's block to
  the function end (AddMisc).
  The barrier can also preserve a target `nop`: in SetLightningI, naked
  `priority = depth >> 18` moved the `sra` into the preceding positive-depth
  guard's delay slot, making the function one instruction short. Fencing only
  that assignment with `do { ... } while (0)` kept the guard delay slot empty
  and the shift after the following loads, recovering the exact function
  length and cutting the residual from 229 to 155 bytes. This is a sched-order
  lever as well as a copy-propagation/regalloc lever; guided `loop-fence`
  already enumerates expression statements for this shape.
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
  `tools/regalloc.py <Name>` now reads both `.lreg` and `.greg`, prints only the
  real allocnos with their computed priority, and can quantify a proposed
  reweighting: `--compare FIRST SECOND` reports the exact additional weighted
  refs needed; `--enclosed-refs N` converts that delta into wrapper depths when
  one proposed fence encloses N uses. `--between SUBJECT LOWER UPPER` handles
  the harder three-way case: it reports the exact weighted-ref interval and
  wrapper-depth interval that keeps SUBJECT above LOWER but below UPPER. The
  comparison assumes equal register
  mode/size—use the dump, not the number alone, for mixed-mode pseudos.
- **Disjoint semantic phases may need separate block-scoped locals even when
  their arithmetic names repeat.** Reusing one function-scope `dx/dy/dz` set
  for an early distance calculation and a late rotation calculation lets cse
  and local/global allocation coalesce unrelated lifetimes into one broad
  register cycle. Separate nested scopes give gcc distinct pseudos without
  changing runtime code. In SetWire this removed the false distance/final-
  rotation cycle and cut the residual from 418 to 376 bytes; confirm distinct
  pseudos in `.lreg`, since merely renaming overlapping live values does not
  help. A repeated name in PSX.SYM is stronger evidence: each row is a separate
  source declaration, not one variable reported twice. AttackControl records an
  early and a late `enemy`; merging them into one function-scope pointer carried
  the early saved-register identity into the final retarget block, while two
  nested declarations let the late call result remain directly in `$v0` and
  removed the extra move. `matcher-prompt.py` now detects repeated debug-local
  names and emits an explicit scope-identity hint with their demo register homes.
- **Sometimes the target is a priority WINDOW, not an outrank relation.** In
  ProcItemNingyo, the saved-register order required
  `param=4901 < item=4964 < bounce=5000`; +2 weighted refs left `item` too low
  and +4 overshot the upper peer, while exactly +3 landed in the only useful
  interval. An outer wrapper around the null-check/store/indirect-call plus
  three inner null-check weights produced 70 refs / 846 live insns = 4964 and
  the target `$s3/$s4/$s5` window. Use `regalloc.py --between`, then distribute
  the computed depth across the smallest producer/consumer ranges whose loop
  notes also preserve scheduling—do not blindly nest the whole function.
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
  The same applies to a **macro's outer `do/while(0)` safety wrapper**: it is
  still a real loop-note range after preprocessing, even when the macro body
  already contains the only runtime loop. In StageEndScreen, changing the
  private decimal-drawing macros' outer wrapper to a plain compound block
  removed accidental weight and scheduler barriers across every expansion.
  Compare preprocessed RTL before adding nested fences to compensate for a
  wrapper the likely original macro never had. Only make this substitution
  when every invocation is a standalone statement and the body has no
  `break`/`continue` whose target would change.
- **A whole-block `if (X) {block} else {block}` with BYTE-IDENTICAL arms is a
  scheduling lever** (permuter-found; no statement reorder reaches it).
  Duplicating an interleaved multi-load sequence into both arms shifts cc1's
  front-end basic-block/pseudo numbering enough to fix a load-delay-slot
  interleaving order; the straight-line form was 8 bytes longer with a stray
  `nop` and a different colour for an unrelated variable (Think1trace).
  Use an INITIALISED discriminator (`s32 donor = 0`), never an indeterminate
  read: old cc1 still builds the useful CFG before jump2 merges the arms, while
  the dead initialisation costs no code (ProcItemNemuri).
  When a loop fence fixes scheduling but rotates registers by weighting the
  enclosed loads, prefer an already-initialised live local as the discriminator:
  `if (id) y = p->y; else y = p->y;`. DefaultActionHumanoid used this exact
  zero-code CFG fence after nested LOOP_ENDs; it retained the target `$a1/$a2`
  allocation where wrapping `y` in a third one-shot loop swapped them.
  Guided `identical-arm-fence` enumerates only nonvolatile locals already used
  by the statement or unconditionally defined in the preceding statements.
  A second use is to break a **hard conflict caused by a stack-address
  pseudo**: ProcItemNingyo's address of its memset/request workspace occupied
  `$s3` across calls, preventing `item` from taking the target `$s3`. Duplicating
  the address-producing statement into identical arms made that pseudo cross a
  CFG edge differently and removed the conflict with no surviving branch.
  Confirm this in `.greg`'s conflict set before adding the fence; the visible
  C pointer name alone does not reveal the stack-address allocno.
- **A final register swap may need an earlier ALLOCATION DONOR site, not an
  edit at the residual's mapped source line.** In FUN_80033bc0, `pos` had 4
  refs / 252 live insns (priority 317) and lost `$s5` to the EffectSlot base at
  priority 330. Duplicating an earlier conditional store into arms selected by
  `if (pos)` added one temporary RTL reference; jump/cross-jump erased the
  branch and duplicate store, while global allocation saw `pos` at 5 refs / 260
  live insns (priority 384) and assigned the target `pos=$s5/base=$s6`. The
  guided `allocation-donor-fence` rule now takes parameter names directly from
  rtlguide's register substitutions and searches assignments in existing
  conditional arms even when they are away from the final diff line. Replaying
  that rule on the 11-byte baseline tried four bounded `pos` sites and found an
  independent exact fence around the earlier EffectSlot-cursor reset (11→0).
  Absent source-level proof of initialization, restrict this lever to
  nonvolatile parameters: an arbitrary automatic local may be indeterminate at
  the artificial test.
- **An automatic local already read by an enclosing guard is also a safe
  ALLOCATION DONOR.** Reaching either arm proves the guard value was initialized
  and evaluated, so it can discriminate byte-identical assignments without an
  invented indeterminate read. In Think1target, `distance` narrowly lost the
  global-allocation race to the z coordinate (7692 vs 7714); duplicating the
  guarded absolute-value assignment under `if (distance)` raised it to 8000,
  swapped the two saved-register homes, and reduced the exact-length residual
  from 35 to 24 bytes. Guided `allocation-donor-fence` now searches this bounded
  enclosing-guard case as well as initialized parameters, including donor sites
  away from the residual's mapped line.
- **A dead-until-overwrite local can donate a disjoint source identity without
  runtime work.** If uninitialized `later` is untouched before `later = L`, then
  `early = E; use(early); ... later = L` can be written as
  `later = early = E; use(later); ... later = L`. The extra automatic-local
  assignment is unobservable on defined paths, while old cc1 can join the two
  live ranges and inherit the later local's allocation preferences.
  Think1target's x alias moved an exact-length residual from 63 to 38 bytes; the
  corresponding z alias exposed the final 7714/7692 priority tie at 35 bytes.
  Guided `disjoint-local-alias` enumerates only same-type function-scope integer
  locals whose address is never taken, which are not shadowed or loop-carried,
  whose alias is uninitialized and untouched before the site, whose first later
  occurrence is a plain overwrite, and whose substituted read is in the
  current compound block.
- **An identical-arm discriminator can also be a PREFERENCE DONOR.** Declare it
  at function scope, initialise it for the artificial `if`, then overwrite it
  with the real value immediately before a call that wants a particular
  argument register. The branch and initial value disappear, but the one pseudo
  has participated in both CFGs and inherited the later call preference.
  ProcItemNemuri used `s32 bleed_n = 0`, identical arms, then `bleed_n = 2`
  before `SetBleeds`; this changed allocation without leaving a materialised
  zero or branch in the final binary.
- **Measure nested zero-trip-loop weighting instead of guessing its depth.** A
  statement in N nested `do { ... } while (0)` wrappers receives N extra
  loop-depth counts per reference. In ProcItemNemuri, putting the split colour
  construction in three nested wrappers inside BOTH identical arms gave its
  `env` pseudo 9 weighted refs / 49 live insns, priority 5510, above `wave` at
  5 / 35, priority 2857; that produced target `$t0`/`$t1`. A shallower wrapper
  was insufficient. Confirm the exact threshold with
  `tools/regalloc.py <Name> --compare FIRST SECOND --enclosed-refs N`.
- **Keep the small scheduling levers around that weighted identical block.** A
  large literal written as `env = 0x6e0000; env |= 0x6e6e;` exposes separate
  `lui`/`ori` definitions that can straddle the vanished branch. Assigning a
  call temp (`bleed_range = 300`) immediately AFTER a duplicated multi-load
  update lets its hard-register copy fill that update's load delay. And when an
  `lbu` field feeds a multiply, a full-width named temp can schedule the load
  early without the `andi 0xff` that an `u8` temp may introduce. These three
  source shapes were all required by ProcItemNemuri's exact case-2 schedule.
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
  - **A nullable pointer reassigned to a stack fallback must continue to be the
    base of fallback initialisation.** Use `center = &local; center->y = ...;
    center->z = ...`, not stores through `local` followed by a later pointer
    assignment, when the target addresses those fields through the pointer's
    hard register. SetWire's target used `$s4` for both fallback stores; those
    two extra references also moved `center` through the exact narrow priority
    window relative to `end` and the loop flag. This is simultaneously an
    addressing fact and a measured regalloc lever—inspect the target store
    bases and `regalloc.py --between` before adopting it.
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

**A referenced `case` label is a hard cross-jump fence.** gcc-2.8.1
`jump.c:find_cross_jump` scans backward from two jumps, but immediately gives
up when stream 1 reaches a `CODE_LABEL` (it merely skips labels in stream 2).
If two branches should share their final tail but must retain separate copies
of the check immediately before it, put the exceptional copy behind a real
switch label.  StateTransition uses a boolean `switch` with `case 0` and
`default`: the three independently written Findenemies tails still
cross-jump into one block and carry their already-loaded Humanoid pointer into
the join, while the case-0 post-alert type check stays distinct from case 1.
An ordinary `if`, `do{}while(0)`, or an unreferenced alias label did not stop
that over-merge; the label must survive as an actual branch target.

**Now mechanized (advanced/opt-in):** when `rtlguide` classifies a residual as
`jump/cross-jump`, `autorules --guided` can try the two semantics-preserving
two-way-switch spellings (`case-fence`) at the implicated `if/else` lines. It
rejects arms containing a source `break`, whose meaning would change inside a
switch. Exact-byte scoring decides whether either surviving CODE_LABEL is the
one the target requires.

**Algebraically equal affine multiply tails can be kept distinct without a
control-flow fence.** jump2 compares RTL, so `x * 3 + 480` and
`(x + 160) * 3` compute the same value but do not present the same backward
instruction suffix. In FUN_800519bc, using one spelling in only one brightness
arm prevented an unwanted merge that had erased exactly three instructions and
an unconditional jump; an explicit center label then pinned the retained tail.
Guided rule `mul-affine-shape` now toggles `x*M+C` with `(x+C/M)*M` whenever
the constant is exactly divisible, letting scoring place this bounded
cross-jump barrier mechanically.

**Machine-identical calls can be different to jump2.** Cross-jump compares the
whole `CALL_INSN`, including its result mode and trailing
`CALL_INSN_FUNCTION_USAGE`, not only the final `jal` opcode. If the target keeps
more physical calls than the candidate, inspect `.rtl` against `.jump`/`.jump2`
before restructuring the CFG. In ActATTACK, the original caller's old-style
`DeleteConflict` declaration allowed already-live `$a1` values at some sites,
while one typed call expression supplied an HImode result. The five eventual
`jal DeleteConflict` instructions look alike, but those RTL fingerprints stop
jump2 from merging the required sites.

`rtlguide` now detects a target physical-call surplus, names target-only aligned
callees, counts calls through the RTL passes, and prints each candidate call's
result/use fingerprint. Treat that output as a declaration and liveness clue,
not permission to add guessed arguments: m2c frequently mistakes stale argument
registers for source arguments, and changing a shared callee prototype can alter
other translation units. Caller-local old-style declarations or typed call
expressions require case-by-case semantic review, so this diagnosis is not an
automatic rewrite.

**One physical call can originate as duplicated branch-local calls.** Keep the
call written in both arms when the target prepares different arguments in each
predecessor but then has a single tail-merged `jal`. Factoring it into
`arg = ...; f(arg);` creates join-crossing argument pseudos, changes `%hi` ties,
and can reorder loads/delay slots even though jump2 still emits one call. This is
especially visible for narrow parameters and stack-passed arguments: a correct
caller-local prototype controls HImode extension and which setup remains in the
predecessor. DefaultActionHumanoid's call tails and DamageControl's duplicated
MoveHumanoid arms are the worked cases. Inspect call fingerprints through
`.rtl`→`.jump2`; do not infer source factoring from the final call count.

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

**Duplicate a whole cleanup/advance tail when its physical address is inside the
state bodies.** Writing one factored helper block or one post-switch tail pins the
code at that source location. Writing the complete pure-C sequence independently
at each exit lets jump2 cross-jump onto its chosen last copy, including indirect
call/null-check cleanup and `mode++; return;` tails. ProcItemDokudango needed the
cleanup at both disposal origins and the advance tail at three mode exits; the
final binary contains one shared copy at the target's internal address.

**A direct terminal global tail can replace a named value local and preserve
every constant island.** ActENGAGE's target has many blocks shaped like
`li v0,K; j shared_store`, including two separate item cases that both load
`0xf02`. A named `short next_motion` became a `reg/v` pseudo that conflicted
with `$v0` across the intervening comparisons: it allocated to `$a0`, and
jump2 also collapsed the two identical `0xf02` blocks. Write the semantically
complete terminal sequence at each successful edge instead:

```c
motID = 0xf02;
D_80097F0E = 1;
return;
```

Leave the desired last physical edge (ActENGAGE's `motID = 0x602`) as the
fallthrough anchor. Each literal store first gets its own HImode producer;
global allocation puts those short producers in `$v0`, then jump2 merges only
the common `sh motID; li 1; sh D_80097F0E; return` suffix onto the anchor.
Referenced switch labels stop the backward merge before the constant loads, so
even equal case values retain separate `li/j` islands. This is both a
cross-jump lever and a way to avoid an impossible named-local `$v0` allocation.

The same principle fixes late scheduling joins. A shared `D_80097F0E = 1`
after a motion-selection if/switch made ActENGAGE emit `sh zero,motID` before
`li v0,1`; writing that flag assignment in each terminal selection arm let
jump2 retain one D-store while sched2 put the load before the zero store, as in
the target. Guided `shared-tail-assign` now enumerates the narrow adjacent
if/else form: two compound arms followed immediately by one plain assignment.
Larger switch-wide terminal-tail expansion remains a deliberate CFG rewrite;
verify the final fallthrough anchor in `.jump2` rather than assuming which copy
jump2 will choose.

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
  ProcItemNinken sharpens the indirect-call case: in EACH duplicate, cache
  `proc = item->proc` for the null check but call through `item->proc(item)`.
  cse reuses that predecessor-local load and gives the `jalr` the target `$v0`;
  jump2 then cross-jumps the two cleanup suffixes from the `jalr` backward. If
  the call is factored behind one explicit `dispose:` label, that join is a CSE
  boundary and the field spelling reloads `item->proc` plus a hazard `nop`;
  calling the cached variable instead keeps the wrong `$v1`. ProcItemNapalm is
  the matched family precedent. Duplicate the source idiom and let jump2—not a
  hand-written label—recover the shared machine tail.
  LoadSI applies the same rule to ordinary error cleanup: spelling
  `vfree(temp); temp = 0; return 0;` in both error arms prevents cse from
  caching the earlier `&cmd`/`&result` stack addresses across a shared source
  label, while jump2 still merges the duplicate cleanup into one physical
  tail. A factored cleanup was shorter C but produced extra saved pointers.
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

Practical rule: `tools/maspsxflags.py <Name> --write` derives the set from the
split asm's `%gp_rel(...)`, syncs both lists, and also handles guarded variable
division flags. (`gpsyms.py` remains the gp-only primitive.) Build.hs exposes the
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
    `[0]` — 14 bytes off as a plain `extern RECT`; ProcItemGun's two 8-byte
    `extern SVECTOR D_80097B0C[];`/`D_80097B14[];` on that same `D_80097Bxx`
    table, +1 insn as plain `extern SVECTOR` — the `lui` reorg only hoists into
    the dispatch delay slot once the split forces a two-register HIGH/LO_SUM).
  - **Interleave tell — how to SPOT the split from the asm**: when another
    instruction sits *between* a symbol's `lui %hi` and its `lw/sw %lo` half, the
    original was NOT a small extern — a small extern is ONE `la`/macro insn whose
    two halves sched1 cannot pull apart, so anything landing between them proves a
    two-register HIGH/LO_SUM that only an unknown-size array (`extern T SYM[];` …
    `SYM[0]`) reproduces. Read it straight off the diff: hi and lo of the same
    symbol not adjacent ⇒ respell as the array (ReqItemUse's guard-pointer and
    the `..._[0] = 3` store).
  - **Now mechanized:** `tools/autorules.py` (rule `extern-array`) sweeps this
    automatically — it flips every file-local `extern T NAME;` to
    `extern T NAME[];` + `NAME`→`NAME[0]` and keeps the flip if it moves bytes.
    So on a fresh draft you rarely apply it by hand; run autorules first and it
    finds the split for you (it only touches decls in the `.c`, never a shared
    header). It first parses the target split asm and **skips every symbol used
    with `%gp_rel`**: an unknown-size array would force precisely the wrong
    absolute-address path there, and a misleading partial-score improvement is
    not evidence against the target. Reach for the manual respelling only when
    the decl lives in a header autorules won't edit.
    FUN_80037e0c independently confirms the data-object form: declaring the
    fixed SVECTOR bytes as unknown-size `u8 D_80097A58[]` and casting at the
    load kept HIGH/LO_SUM in separate registers and allowed the target's
    intervening stack-copy schedule.
- **`lui`+`addiu(base)`, scaled index, then load at `0(base+index)` identifies a
  real named array.** A magic absolute pointer plus indexing often lets cc1 fold
  the address's low half into the final load displacement, losing the target's
  standalone base materialization. Resolve the address against the symbol/data
  map, declare the table with its real element width/count, and index it by
  name. DamageControl's eight-entry `D_80086B6C[deg]` motion table restored both
  the correct sequence and the exact function length.
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

**Separate single-definition rand temps preserve return-register coalescing; a
named base temp preserves which side owns the bias.** For repeated
`coordinate - BIAS + rand() % RANGE` expressions, use one `random_x/y/z` per
call and split `base_x = coordinate - BIAS; result = base_x + random_x % RANGE`.
Each single-def call result coalesces with `$v0`; one reused multi-def `random`
creates a copy after every call. The named base prevents combine from
reassociating the bias onto the remainder, which otherwise sinks the coordinate
load and removes its target load-hazard `nop`. This exact three-expression shape
fixed ProcItemFire's last structural gap. When the target instead has the
reassociated form, inline `rand()` and the bias—the two spellings are deliberate
opposites. DrawShadow independently confirmed the return-register half of this
rule with four `rand()` calls. Its first call had to stay before an unrelated Y
adjustment, so blindly inlining the call at the use would have changed side-effect
order. `tools/autorules.py NAME --rules call-result-split` mechanizes the safe
version: it atomically gives each direct-call definition a single-use local while
leaving every call at its original statement position.

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

**A `%` or `/` by a runtime VALUE (not a constant) compiles shorter than the target
unless the function is on the `--expand-div` list.** ASPSX guards both signed and
unsigned division with `break 7` (divide-by-zero). Signed `div` additionally has
`break 6` for the INT_MIN/-1 overflow case; unsigned `divu` has no corresponding
overflow guard. maspsx only reproduces those guards under `--expand-div`. Without
it maspsx emits a bare division/result sequence and the draft assembles short,
shifting everything after it (a common cause of a whole-function length mismatch).

The tell, before you even build: **Ghidra renders the guards as `trap(0x1c00)` and
`trap(0x1800)`** — seeing those in the reference C means the TU divides by a variable.
In the target `.s`, signed `div` is followed by `break 7` and `break 6`, while
unsigned `divu` is followed by `break 7` alone. Division by a CONSTANT is normally
a magic-multiply with no guards and needs nothing.

Fix: add the function to BOTH `extra "<Name>" = ["--expand-div"]` in
`shake/src/Build.hs` and `"<Name>": ["--expand-div"]` in `tools/permute.py`'s
`MASPSX_EXTRA`. The EFFECT.C spawners are prone to this (`rand() % time`,
`rand() % srange` etc.): `SetSmoke` (1 variable div, 2 breaks) and `SetBleeds`
(3 variable divs) both need it; the already-matched `IsVisible`/`ComputeAreaLevel`/
`bow_shoot_logic` are on the list for the same reason.

This edit is now mechanical: `tools/maspsxflags.py <Name> --write` reads the
target split assembly, recognizes the signed guard pair or unsigned single guard,
and synchronizes this flag alongside any `%gp_rel` externs. ProcItemNapalm is the
worked combined case (one guarded character-model remainder plus six gp-relative
`D_80097F60` loads). `FUN_80036284` is the unsigned worked case: its six `divu`
operations each have only `break 7`; that exposed and fixed the detector's former
signed-only assumption.

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
  The same principle applies when one computed coordinate fans out to several
  byte fields: compute it once in an `s32` temp and then assign the bytes. This
  prevents modulo-256 folding and gives allocation one full-width pseudo to
  reuse (SetupTelop's final `u`/`v` stores). If a scratch is dead before that
  tail, reusing the later temp name earlier can also be a zero-code allocation
  lever: SetupTelop fed its halfword bitmap store through the later `final_u`
  pseudo, reducing the final register cycle without changing the value or
  adding a live range across the intervening calls. Confirm the change in
  `.lreg`; do not create an artificial long-lived local blindly.

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
  cc1 `-da` RTL dumps + the gcc-2.8.1 sources). A synthetic optimizer barrier
  can reproduce it experimentally, but under the inline-asm anti-rule above that
  is evidence that the natural C remains unresolved, not a matching solution.
  Whole game code contains exactly three, all parked: `GetPad`, `FUN_8001b174`,
  `GetPadXY`. `triage.py` detects the triple and reports `SIGNEXT-SPLIT (GetPad
  class)` instead of rating it TRIVIAL; `autorules` deliberately does not try the
  barrier form.
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
    `DrawTMD` is blocked the same way despite containing no GTE command opcode.
    ArrangeLocalMatrix was previously listed here too, but that was a false
    inference from its dense use of `$t2..$t6`: those are ordinary internal
    loop temporaries, while both calls use ABI `$a0/$a1`. Its complete
    636-byte body now matches in pure C.
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
