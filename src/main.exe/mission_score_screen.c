#include "common.h"
#include "main.exe.h"
#include "misc.h"

/*
 * Post-mission score/high-score screen (0x80054B48, 0x121C bytes).
 *
 * STATUS: NON_MATCHING -- 254 of 4636 bytes differ (was 330; 346; 401; 413;
 * 433; 437; 476; 498; 525; 1266).  85 differing instructions.
 *
 * 2026-07-17 round-10 (Fable, macro-expansion escalation) -- 330 -> 254 via
 * THE LOOP.C MOVABLE BUDGET.  Two preliminary facts, then the model:
 * - EXPANSION IS FREE: every function-like draw macro was expanded to its
 *   literal per-site text and verified byte-identical (same cpp token
 *   stream via mipsel cpp -P | tr, same .o sha256).  The hypothesis that a
 *   shared macro spelling *itself* costs bytes is REFUTED -- cpp expands
 *   textually, sites already compiled per-site.  What expansion buys is
 *   EDITABILITY: the wins below are per-site variations a shared macro
 *   could not express.  17 macros deleted; only SCORE_STATE (object-like)
 *   remains.
 * - THE PREHEADER ORDER IS A move_movables CHAIN (fixed the F cluster, 4
 *   insns, and the whole draw-4/N-O scheduling cluster fell with it):
 *   target preheader is lui/ori s5 (the /10 magic), li s8,10, addiu
 *   s6,sp,24 IN THAT ORDER.  Source-preheader insns can never follow the
 *   magic (loop.c emits all moved insns immediately before LOOP_BEG, after
 *   any source code), so ten/numberSprite must themselves be MOVED
 *   MOVABLES whose chain position (= scan order of the head's SET) is
 *   after the magic's.  The per-site structure delivers exactly that:
 *   `ten = 10` inside EACH sign arm (10 sites) and `numberSprite =
 *   &number` at EACH use site (colon 1, SV32, colon 2, draw 4).
 *   combine_movables (loop.c:1239) merges same-value set-once movables
 *   (n_times_used is a COPY OF n_times_set -- multi-use members are fine)
 *   into the first-scanned head, summing savings and lifetimes; the merged
 *   set is emitted once at the head's chain position.  Draw-0's sign arm
 *   scans after draw-0's division (magic head), colon 1 after that: magic,
 *   ten, ns.  EXACTLY the target.
 * - THE BUDGET (loop.c:1620-1728, read directly): each moved movable costs
 *   `threshold -= 3` for every LATER movable in the chain, and a movable
 *   moves iff already_moved || threshold*savings*lifetime >= insn_count
 *   (this loop: threshold 29 = 1*(1+28 non-fixed regs), insn_count 795
 *   "real insns", both printed by -dL).  The function needs FIVE moves in
 *   chain order: -71 drawY group (draws 0/1/2, jump-1 y consts; hoisted
 *   head spills, reload REMATERIALIZES li t0/t2 at each store -- that is
 *   where the target's t-register y constants come from), the magic, ten,
 *   ns, and the row 0x80010000 group (scores/chars lui cse-folds into one
 *   pseudo + grades lui; hoisted+spilled => remat lui t1/t2 in the row).
 *   Gates as landed: -71 at 29 needs life >= 10: draw-0's drawY seed moved
 *   BEFORE its value seed (life 9 -> 11; at life 9 the group is refused
 *   and the y consts become sunk local v0 li's, dbr steals the else arm
 *   into the bgez slot and both jump-1 draws lose their `j` -- that is the
 *   shape the group's remat protects).  Row group at threshold 17 needs
 *   savings*life >= 47, had 2*22: fixed by `{ s32 rowScopePad; }` after
 *   the row FUN_800515b0 call -- BLOCK_BEG/END notes RECEIVE LUIDS
 *   (loop.c:404 assigns ++i to every non-line-number note) but are not
 *   real insns, so the scope block stretches the head's set-to-last-use
 *   span (19 -> 21, group 24) without touching insn_count or cse.  The
 *   scope-pad is the note-emitting sibling of the do{}while(0) fence: use
 *   pads to lengthen a movable lifetime, fences to break cse1.
 * - reg_single_usage (loop.c:740): a set-once single-use pointer whose use
 *   folds to a legitimate address is SUBSTITUTED AND DELETED by loop.c
 *   (measured: a fenced `ps = (PersistentState *)0x80010000; ...
 *   D_8008ED50[ps->stage]` collapses to (mem (const)) -- absolute const
 *   addresses ARE legitimate on MIPS via CONSTANT_ADDRESS_P).  Only
 *   index-scaled uses survive substitution (the fold produces an
 *   illegitimate reg+bigconst sum and validate_replace_rtx refuses).
 *   Dead probes are equally futile: delete_trivially_dead_insns iterates
 *   before loop.c and removes dead chains (a `base = ...; dummy = *base;`
 *   probe vanishes entirely).
 * - SV32's `sprite = (numberSprite);` copy must sit behind a do{}while(0)
 *   fence now: with the ns seed in-block, cse1 otherwise re-canons
 *   sprite's decimal-loop uses onto ns and deletes the copy (the target's
 *   move s2,s6).  Same idiom as the medal-block blocker, cheaper spelling.
 * - MEASURED AND REVERTED (do not retry blind):
 *   - x=102 as a 4-site variable group with scope pads: the group MOVES
 *     and remats as li t1 at all four sites (confirming the target's t1 is
 *     the same remat mechanism, and draw-2's -71 then remats to t2 exactly
 *     like the target), but the x-store loses its v0-reuse interlock and
 *     sched1 lifts the sll/sra sign-extension above it: net 254 -> 266.
 *     The register class is understood; the blocker is the store's sched
 *     position, not allocation.  A future round needs a way to keep the
 *     sh before the extension with the group pseudo (the target does).
 *   - The row accessed via D_80010458/D_8001044C/D_80010451 symbols:
 *     without -msplit-addresses (mips.h LEGITIMIZE_ADDRESS gates the
 *     HIGH/LO_SUM split on mips_split_addresses) a symbol+index address
 *     materializes la (lui+addiu) and the %lo never folds into the load:
 *     +3 insns.  The const (SCORE_STATE) form is correct; the target's
 *     %hi/%lo notation is just splat labeling of identical words.
 *   - Init-loop pivot writes through a (fenced) pointer local: cse1
 *     hash-unifies the seed's RHS with initSprite's value upstream of any
 *     fence and deletes the recompute (-4 insns).  MEM-context keeps the
 *     recompute but is index-first (instantiate's special case); pointer
 *     context is base-first but folds.  The pivot half of the bank-address
 *     pair is therefore closed at the C level in BOTH directions, matching
 *     the round-9 verdict from the main-address half.
 * - Residual 254 = 85 insns: entry rotation (3), LoadTIMAndFree block
 *   rotation (10, brightness-fence-parked), bank main pairs (10, CLOSED) +
 *   attr/w load order + or/srl/store order (7, source-order-neutral --
 *   measured) + pivot pairs (4, CLOSED above), draw-0 head li/lbu swap (2)
 *   + sign-arm a2/a0 sched (2), colon-1 tail + SV32 head role mirror
 *   (~12: enemies lbu lands v1 in target vs a0 in ours, everything
 *   downstream mirrors; local-alloc window/priority cascade seeded by
 *   sched placing our lbu 65(sp) one slot earlier -- needs the testbed),
 *   SV32 decimal+sign GsSortSprite arg scheduling (16), x=102 heads (13,
 *   see above), colon-2 tail move/sb swap (2), stock-tail addu chains (4).
 *
 * 2026-07-17 round-9 (Opus) — THE CONSTANT-ADDRESS OPERAND-ORDER LEVER
 * (346 -> 330), and the bank-address pair is CLOSED as a spelling problem.
 *
 * THE NEW RULE (reusable, verified in a standalone cc1-281 reduction before
 * applying, then measured):
 *   Indexing a fixed absolute address through a POINTER LOCAL
 *     `register T *st = (T *)0x80010000;  st->arr[i]`
 *   makes the element address `(plus reg_st reg_index)`, and expand_binop
 *   emits `addu t,state,index` — BASE FIRST.  Inlining the constant
 *     `#define ST ((T *)0x80010000)      ST->arr[i]`
 *   makes it `(plus reg_index CONST)`, and expand's EXPAND_SUM/form_sum sorts
 *   the CONSTANT TERM LAST, emitting `addu t,index,base` — INDEX FIRST, the
 *   target's order.  The single cse'd `lui` per block is PRESERVED, so the
 *   fresh-lui-per-block shape is a property of the constant being
 *   rematerialized per block, NOT of the pointer local.
 *   Sites: rank scan -10B, shift loop -4B, init loop -2B; insert block and the
 *   three row-loop locals byte-NEUTRAL (kept: they delete 4 locals / 13 lines).
 *   This CORRECTS the old "separate insert-block pointer (fresh lui)" identity.
 *   THE RULE IS SITE-DEPENDENT — it pays only where the access is INDEX-SCALED
 *   (`st->arr[i]`, an addu whose operands can reorder).  Applying it to the
 *   GRAND-MASTER stock tail (whose accesses are displacement-folded scalars,
 *   `state->stage` / `state->chr`) REGRESSES 330 -> 334: with no index there is
 *   no addu to reorder, and inlining only costs the shared base.  Measured and
 *   reverted; do not retry.  Keep a pointer local for displacement-folded
 *   blocks, a constant macro for index-scaled ones.
 *
 * THE BANK-ADDRESS base-first/index-first PAIR IS CLOSED AT THE C LEVEL — do
 * not spend another round re-spelling it.  Round 3 said "cc1 address-selection,
 * not spelling" from 2 spellings; that verdict is now proven at TEN, with the
 * mechanism:
 * - CORPUS ORACLE (whole-game disasm, 525 matched fns): the target's shape
 *   (`addu rX,sp,rI` + a SEPARATE `addiu rX,rX,IMM`) occurs at exactly TWO
 *   sites in the ENTIRE GAME — 0x80054ca4 and 0x80054d64, BOTH inside this
 *   function.  It is not an idiom anywhere else, so no matched source can
 *   supply it.  The 3 matched fns that DO emit `addu rX,sp,idx` (load_layout,
 *   DrawConstruction, BriefingAndInventorySelectionScreen) all DEREFERENCE
 *   immediately and fold the frame offset into the load displacement
 *   (`addu v0,sp,a0` / `lw a1,16(v0)`) — a different construct.
 * - TESTBED: `&bank[i]`, `bank + i`, the (u8*) cast macro, BOTH operand orders,
 *   the int-sum form, `&bank[i].a` / `&(bank[i].a)` (COMPONENT_REF), `&(*(bank
 *   + i))`, and the array at zero AND nonzero virtual-frame offset ALL emit
 *   byte-identical base-first.  Only `(S*)(i*36 + (int)bank)` differs — worse
 *   (an extra addu).  C folds `&a[i]` to `a + i`, so every spelling converges.
 * - MECHANISM (expand + function.c:3130 instantiate_virtual_regs_1, read
 *   directly).  At expand `p = &bank[i]` is ONE insn, `(set p (plus VIRT idx))`
 *   — the frame base is the VIRTUAL reg and there is NO constant, so it takes
 *   instantiate's GENERIC path, which forces the base (`addiu t,sp,96`) and
 *   yields base-first.  The target's index-first needs the RTL
 *   `(plus (plus VIRT idx) CONST)`, which hits the special case at :3137
 *   (`Check for (plus (plus VIRT foo) (const_int)) first`) and emits exactly
 *   `addu s0,sp,s1 / addiu s0,s0,96`.  Our expand never builds that for a
 *   pointer-VALUED address; only a MEM address (EXPAND_SUM) gets the constant
 *   sorted outermost, and then it folds into the displacement instead.
 * - THEREFORE `REG_N_DEATHS` (and every local-alloc lever) is CATEGORICALLY
 *   INAPPLICABLE here: the form is decided at expand/instantiate, long before
 *   local-alloc ever runs.  Do not re-run that lever on this class.
 * - Round 9 also re-measured: `tools/autorules.py` full default sweep = "no
 *   improving edit among 37 candidates" (independently re-confirms the
 *   rankSpriteBase fence: fence-unwrap L1030 346->359).  `tools/permute.py`
 *   REFUSES this residual as too broad (102 aligned lines in 65 blocks vs
 *   limits 128/32) — it is not an allocation cascade, so the permuter is not
 *   the tool until the structure closes further.
 * - BYTE-ACCOUNT of the 346 (17 clusters, 114 insns): 23 insns were PURE
 *   PERMUTATION (identical instruction multiset, scheduling only — largely
 *   GsSortSprite's `move a2,zero` placed early by the target, late by us).
 *   The SCORE_STATE class above was the largest REAL class, not the sp banks.
 *
 * 2026-07-17 round-8 (Opus) — THE +-4 SKEW PAIR IS LANDED (401 -> 346).
 * Round 7's (A) and (B) are both in; its blocking (C) ("move pseudo 777 off
 * $s0 onto $v0") was a MISFRAME and is now DISSOLVED, not solved.  Read this
 * before touching the medal block.
 * - (C) WAS NOT A PRE-EXISTING DEFECT: the 401 baseline ALREADY emitted the
 *   target's `lw v0,0(v0)/nop/addiu s0,v0,104/li v0,138` exactly.  Round 7's
 *   (B) INTRODUCED it.  ALWAYS diff the region a "missing piece" lives in
 *   against the CURRENT draft before hunting it: (C) was collateral from (B),
 *   so there was never a pseudo to relocate.
 * - MECHANISM (local-alloc.c, read directly).  `combine_regs` ties the lw temp
 *   into medal's quantity ONLY if `reg_qty[medal] == -2`; the guard is
 *   `if (reg_qty[sreg] >= -1) return 0;`, and local_alloc seeds
 *     reg_qty[i] = (REG_BASIC_BLOCK (i) >= 0 && REG_N_DEATHS (i) == 1
 *                   && (reg_alternate_class (i) == NO_REGS
 *                       || ! CLASS_LIKELY_SPILLED_P (reg_preferred_class (i))))
 *                  ? -2 : -1;
 *   So a tie needs medal BLOCK-LOCAL *and* DYING EXACTLY ONCE.  The 401
 *   baseline used medal after the join (r/g/b + call) => not block-local =>
 *   reg_qty -1 => no tie => the temp got its own 2-insn qty and find_free_reg's
 *   ascending scan gave it $v0.  Round 7's `medalDraw = medal; medal++;` made
 *   medal's last use fall BEFORE the join (the dead `medal++` is deleted at
 *   combine, so it never extends the range) => block-local => tie => the temp
 *   inherited medal's call-crossing callee-saved $s0 and cc1 filled the load
 *   delay with `li v0,138`.  This is the cookbook's `%hi` reload-tie guard
 *   running in the INVERSE direction (there: make combine_regs tie; here: make
 *   it refuse).
 * - THE FIX — the OTHER half of that guard, `REG_N_DEATHS (i) == 1`: give the
 *   a0 copy to BOTH arms of the brightness test:
 *       if (brightness < 0) { medalDraw = medal; brightness += 0xFFF; }
 *       else                { medalDraw = medal; }
 *   medal then dies in TWO places, so reg_qty[medal] = -1 and combine_regs
 *   refuses the tie WITHOUT medal having to outlive the join.  The temp returns
 *   to $v0 (target's nop restored) AND the copy still reaches the bgez delay
 *   slot.  Both halves of the parked +-4 pair now hold at once: (A) row +1,
 *   medal -1, net 0 at the exact 1159-insn extent.  401 -> 346.
 * - REG_N_DEATHS is a REAL, independent regalloc lever: a value that dies on
 *   two paths can never be locally tied, whatever its live range.  Reach for it
 *   whenever a temp is wrongly coalesced into a longer-lived pseudo.
 * - The medal-block reduction lives in the scratchpad testbed (25 lines,
 *   cc1-281 direct, ~1 s/variant); it reproduced BOTH known states exactly and
 *   found this in one sweep.  CAVEAT: it FALSE-POSITIVES on the delay slot —
 *   `brightness = rcos(...) * 80 / 4096 + 0x7F;` (the natural division spelling
 *   of the bgez/addiu-4095/sra-12 idiom) fills the slot in the reduction but is
 *   BYTE-NEUTRAL in the real function (measured 401, exact length; medalDraw
 *   dissolves).  With one pseudo the copy is born POST-join and only dbr's
 *   fill_slots_from_thread can steal it, which needs sched2 to leave it as that
 *   block's first insn — pressure-dependent, and the real function's pressure
 *   defeats it.  (B)'s copy is emitted in block 0, where fill_simple_delay_slots
 *   always gets it.  So: screen REGISTER questions in a reduction, but verify
 *   SCHEDULING questions in the real function.
 *
 * 2026-07-17 round-6: THE ten s7<->s8 SWAP IS SOLVED (the old INFEASIBLE
 * entry below was based on a MISIDENTIFIED pseudo; see "round-6" section).
 * s7/s8 register mentions now match the target exactly (4/4 and 13/13).
 * Exact target length (1159 instructions), frame 0x1B8, 60/60 calls; the whole
 * function is address-aligned except the LoadTIMAndFree arg copy (9 low) and
 * one +4/-4 skew pair (medal bgez delay nop vs the row draw's sra) — BOTH
 * halves of that pair are SOLVED and verified in round-7 below, but they are
 * complementary (+1/-1) and gated on one last insn (round-7 (C)).  No
 * retail-name record in PSX.SYM/demo export.
 *
 * 2026-07-17 round-5 (Fable) landed identities (do not undo):
 * - THE LADDER FLIP (433 -> 413): draw 1 (the SV32 subtraction draw) reuses
 *   the FUNCTION-SCOPED `sprite` variable as its carrier (no macro-local
 *   drawnSprite).  The merged pseudo (draw-1 range + row range, disjoint)
 *   has refs 46+14 with prio ~35000, far above rowSprite's 17229, so it is
 *   allocated FIRST and takes s2; rowSprite (conflicting: sprite's row range
 *   sits inside rowSprite's) is pushed to s3, and the row `negative` +
 *   `sprite` land s2 — the exact target rotation of the whole row body.
 *   regalloc.py --compare 82 800 said +2 weighted refs; direct ref-padding
 *   dies to cse, but merging two same-home variables moves refs wholesale.
 * - Draw 1's x/y stores go through (sprite_) BEFORE `sprite = (sprite_);`:
 *   once the copy exists, cse's make_regs_eqv makes the LONGER-LIVED copy
 *   canonical (sprite outlives numberSprite) and rewrites the stores to s2;
 *   placing the stores before the copy keeps them on s6 (target).
 *   The declaration order of sprite/numberSprite does NOT matter (measured:
 *   canon follows live length, not pseudo number).
 *
 * 2026-07-17 round-4 (Fable) landed identities (do not undo):
 * - The ROW draw is its own macro (DRAW_SCORE_NUMBER_ROW32): s16 signedValue
 *   seeded from i+1 (HImode addiu a0,s4,1 — NO pre-extension of i), u16 value
 *   = signedValue (plain HImode move a1,a0 — no andi), draws through the
 *   pre-loop rowSprite pointer (no drawnSprite copy), and declares its OWN
 *   `negative` (target row uses s2 vs s3 elsewhere: per "one variable = one
 *   hard reg", the row's flag was a distinct local in the original).  This
 *   fixed value->a1 (incl. `move a1,s0` quotient reload), negu a1,a0, the
 *   j+two-arm sign shape, the y-store delay-slot fill, the backedge
 *   `addiu a0,s4,1` delay fill, and beqz s2 — the old rowValue->a0
 *   "hard-conflict p91" dissolved once value became the copy (born second)
 *   instead of the andi owner (born first).  476 -> 437.
 * - rowSprite = &number sits between i = 0 and rankSpriteBase = rankSprites
 *   (target order: move s4,zero / addiu sp,24 / addiu sp,96).  The old
 *   "separate rowNumber pointer spills rankSpriteBase" verdict was an
 *   artifact of the drawnSprite-copy shape; with _ROW32 there is no spill.
 *
 * Proven identities added this session (do not undo):
 * - insertedRank is a PLAIN s32 LOCAL, spilled by reload to sp+392: the
 *   li t0,-1/sw + lw t1/t2/t3 spill-reload sequence is reproduced exactly.
 *   tail shrank to {oldPad, pad, background}.
 * - Both rank-scan hit arms are `insertedRank = i; break;` INSIDE the loop:
 *   cc1 relocates loop-exiting arms to the function end (goNext arms first,
 *   then these, cross-jumped into ONE island `j join; sw a0,392(sp)`).
 *   An explicit source-level island (`goto`+label at the bottom) does NOT
 *   reproduce this (breaks the shared-jal cross-jump; wrong island order).
 * - The insert region uses a one-copy guard (`found = insertedRank`),
 *   block-scoped shift pointer, separate insert-block pointer (fresh lui),
 *   inline i-1 subscripts, and grades[i]=grades[i-1] BEFORE i--.
 * - rankColour = 128 alone at entry; the number-init brightness is
 *   `brightness = rankColour;` behind a do{}while(0) fence (the loop note
 *   stops cse1 from const-folding the copy AND stops sched1 hoisting; both
 *   effects are required to keep `move v0,s3` + the v0 anti-dependence that
 *   pins the lhu w/h loads after the r/g/b stores).
 * - Medal + row brightness are chained `r = g = b =` stores (reverse 22/21/20
 *   order) with brightness a plain (caller-saved) local; row uses if/else.
 * - numberSprite is TWO variables: a block-scoped init pointer and the
 *   loop-phase `numberSprite = &number` set before the main for(;;)
 *   (target re-materializes addiu s6,sp,24 in the preheader).
 * - number-init block has NO do{}while(0) fence before number.mx=0/my=0
 *   (the old "pivot reset boundary" fence was WRONG): removing it merges the
 *   pivot reset into the rgb block and lets the LoadTIMAndFree(tim) arg copy
 *   `move a0,s2` float up (target hoists it to the block top right after
 *   InitSprite).  This closed the ~80B block-rotation to a 9-insn shift and
 *   is the 525->498 win.  The a0=tim copy now stops just past the brightness
 *   fence (line ~c1c); it cannot reach the target's bf8 without deleting the
 *   brightness fence, and deleting THAT regresses (503) — so it is parked at
 *   the 9-insn residual.
 * - DRAW2/DRAW5 (the two draws after colons) pass numberSprite (move s2,s6
 *   class); other draws pass &number (fresh addiu).  The colon x-store is
 *   the OBJECT form `number.x = x_` (sp-direct, pinned at block top).
 * - The char loop keeps retail's dead attribute load via a site-local
 *   volatile read; its w>>1/h>>1 stores go through initSprite while the
 *   zero pivots recompute the bank address (SCORE_SPRITE_AT).
 * - The stock tail is direct indexing (displacement-folded 1036) — no
 *   unlock pointer; statePtr = (PersistentState *)0x80010000 behind a
 *   do{}while(0) fence supplies the s0 base for the layout=0xFF store
 *   (the lui fills the FUN_80038ce0 delay slot).
 *
 * REJECTED this session (do not repeat):
 * - Integer-sum (leSetEnemy) spellings for the rank/char initSprite address:
 *   cse then folds the pivot recomputation into the same register and the
 *   loops SHRINK.  The two coexisting shapes (main index-first, pivot
 *   base-first) remain unsolved; current draft keeps both length-neutral.
 * - `signedValue = expr; value = signedValue;` for DRAW2/row (coalesces,
 *   one short) and pasting the expression twice (cse does NOT fold, grows).
 * - A separate rowNumber pointer for the row draw (spills rankSpriteBase).
 * - Removing the i=0 entry fence (line ~320): regresses to 578.  Removing the
 *   brightness copy fence (line ~334): regresses to 503.  Both fences needed.
 * - Moving `ten = 10;`/`numberSprite = &number;` into the loop body: loop.c
 *   hoists them to the SAME preheader slot (before the /10 magic); no change.
 * - guided autorules on the 525 and 498 residuals: no genuine win (every
 *   candidate regresses or is a LOCAL-SHAPE regression).  Do not rerun broad.
 *
 * 2026-07-17 round-6 (Opus) landed identity — THE ten s7<->s8 SWAP (413->401):
 * - The old INFEASIBLE verdict ("15x gap, priority-infeasible") compared ten
 *   against THE WRONG PSEUDO.  The `priority 99` allocno is NOT rankSpriteBase
 *   — it is the SPILLED insertedRank (it has no disposition at all).  The real
 *   competitor is rankSpriteBase at priority 544 vs ten's 1375: a 2.5x gap,
 *   and `regalloc.py --compare 798 94` says plainly "needs +4 weighted ref(s)".
 *   ALWAYS confirm a pseudo's identity via its DISPOSITION before believing a
 *   priority verdict about it.
 * - Mechanism (global.c + flow.c, read directly): MIPS defines no
 *   REG_ALLOC_ORDER, so find_reg scans hard regs ASCENDING, and pass 0 can
 *   never claim a callee-saved reg for the first time (it ORs in the complement
 *   of regs_used_so_far).  So cross-call allocnos take s0..s7 then $30(fp/s8)
 *   strictly in PRIORITY order.  s8 is $30 — the LAST register — so the target
 *   putting the high-ref `ten` in s8 means ten is allocated LAST, i.e. ten must
 *   rank BELOW rankSpriteBase.  Priority is
 *     floor_log2(n_refs) * n_refs / live_length * 10000 * size
 *   (global.c), and flow.c weights refs by `REG_N_REFS += loop_depth`.
 * - THE LEVER: `do{}while(0)` fences are REAL loop notes, so each nesting level
 *   adds +1 loop_depth to every ref inside it.  rankSpriteBase's single row-loop
 *   use, nested 4 fence levels deep, goes 4 -> 8 weighted refs; floor_log2 steps
 *   3 at 8, so its priority jumps 544 -> 1632, clearing ten's 1375.  It is then
 *   allocated first and takes s7; ten is pushed to $30.  This is surgical: 1632
 *   lands between p83 (3036) and p94 (1375), so NOTHING else moves.
 * - CAVEAT (honest): the 4-nested-fence spelling is SCAFFOLDING.  It now fences
 *   the DEF (`rankSpriteBase = rankSprites;`, lifting it 2 -> 6, refs 6+2 = 8)
 *   rather than the whole rank block (which lifted the USE, 2 -> 6, refs 2+6 =
 *   8).  Both give byte-identical 401 and the same p798 8/147 -> 1632; the def
 *   form is kept because it encloses ONE statement instead of the entire rank
 *   block.  That equivalence is itself the finding: the scaffold's ~32B cost is
 *   NOT collateral from the refs it encloses (shrinking the footprint to one
 *   assignment recovered exactly 0 bytes) — it is inherent to the loop notes
 *   themselves (they also gate cse1 const-folding and loop.c, not just flow.c
 *   depth).  So do not chase the cost by moving/shrinking the fence again.
 * - DISPROVEN (round-6, Opus): the "original reached 8 refs another way (e.g. 3
 *   cse-surviving uses)" hypothesis is DEAD.  The target's $s7 is mentioned
 *   exactly FOUR times in the whole function (prologue sw, `addiu s7,sp,0x60`
 *   def, ONE use `addu a0,s7,a0`, epilogue lw) — i.e. the original's
 *   rankSpriteBase has 1 def + 1 use, the SAME raw shape as our draft, so it
 *   carries 4 weighted refs (priority 544), not 8.  Counting the target's
 *   mentions of a register is a cheap, decisive check on any "the original had
 *   more refs" story — do it before theorising.
 * - WHY THAT MATTERS (the arithmetic, base depth = 1 per flow.c's `depth = 1`
 *   init, verified in the pinned source): ten is 1 def + 10 `mult v0,fp` uses,
 *   and all 10 uses are inside the main for(;;), so its weighted refs have a
 *   hard FLOOR of 1 + 10*2 = 21 -> priority 592 (live 1418).  rankSpriteBase at
 *   the target's 4-ref shape tops out at 544 (live 147).  592 > 544: NO
 *   fence-free spelling can flip the pair.  The scaffold is not standing in for
 *   a prettier spelling of the same structure — it is the price of the flip.
 * - THEREFORE the real lead is elsewhere: for the target's numbers to be
 *   self-consistent the original must have had NO draw-macro fences (ten at 21
 *   refs) AND either ten live >= 1545 or rankSpriteBase live <= 135 (our 147;
 *   the target's asm span between def and use is 136).  I.e. the draw-macro
 *   fences are themselves scaffolding, and the ~50B that unwrapping them costs
 *   (measured: 419/420) is a DIFFERENT unsolved problem hiding behind them.
 *   That is where the ~32B lives — not in the rankSpriteBase fence.
 * - Measured alternatives (all worse; do not retry): unwrapping the draw macros'
 *   fences drops ten 39->21 refs (1375->592) and needs only +1 ref on
 *   rankSpriteBase, but the unwrap itself costs ~50B and nets 419.  A single
 *   fence around the whole row body lifts ten's row use too (40 refs/1410) and
 *   does not flip.  Wrapper-unwrap + 3 fences (ten 874 / p798 952): 420.
 * - THE ROW GOTO-LOOP IS LOAD-BEARING: rewriting it as a real `do {...} while
 *   (i < 3)` (semantically identical, and the tempting "clean" shape since its
 *   loop note would flip the pair with NO fence scaffolding) REGRESSES TO 698 —
 *   the loop note lets loop.c hoist the row body's invariants and the whole row
 *   region re-derives.  The goto-loop must stay; that is why the flip has to be
 *   bought with fences instead.
 *
 * 2026-07-17 round-6 (Opus) — THE COLON/x=102 CLASS IS THE SAME PROBLEM, NOT A
 * LOCAL-ALLOC LEVER.  The standing brief called it "a genuinely different lever
 * (local-alloc, not global priority)".  It is not; it is a SYMPTOM of the same
 * draw-macro fences, and the fix is not independently available.
 * - The four sites are the four `type_ = s16` draws (2/5/7/8, x=0x66).  Our
 *   block is INSTRUCTION-FOR-INSTRUCTION identical to the target's; only the
 *   register differs (x const: ours $v0, target $t1; drawY: ours $t1, target
 *   $t2).  Everything else in the block (drawnSprite->s2, value->v1, the sll
 *   temp->v0, signedValue->v0) already matches.
 * - Read from the .lreg dump: the x const is pseudo 429, `Register 429 used 8
 *   times across 4 insns in block 35; 2 bytes`.  It has only TWO real refs (def
 *   1274, use 1276) — the 8 is 2 x loop_depth 4 (base 1 + main for(;;) +
 *   DRAW_SCORE_NUMBER + DRAW_SCORE_NUMBER_).  local-alloc ranks quantities by
 *   QTY_CMP_PRI = floor_log2(n_refs)*n_refs*size/(death-birth)*10000 — the SAME
 *   shape as global.c's — so the fences inflate it to 3*8*2/4*10000 = 120000.
 *   Block 35 holds exactly two local qtys: 431 (the `sll` temp, SImode, 8/2 ->
 *   480000) and 429 (120000).  431 allocates first and takes $2 over [1280,1281);
 *   429's window [1274,1276) is DISJOINT, so find_free_reg (ascending scan, no
 *   REG_ALLOC_ORDER on MIPS) finds $2 still free and takes it.  The target's 429
 *   instead lands at $9, i.e. its priority there is low enough to allocate after
 *   something that occupies $2 across its window.
 * - So refs (fences) is the only term that differs; window and size are already
 *   identical to the target's.  There is no separate local-alloc knob to turn.
 * - MEASURED this round (all at the exact 1159-insn extent):
 *     baseline (4 fences, sign arm inside DRAW_SCORE_NUMBER_)  ten 39/1375  401
 *     sign arm moved OUT of DRAW_SCORE_NUMBER_'s fence, 3 fences ten 31/874  401
 *     + outer wrapper do{}while(0) -> plain {}, 1 fence       ten 23/648  420
 *     + outer wrapper -> plain {}, 4 fences                   ten 23/648  420
 *   Conclusions: (a) relocating the ten-using sign arm out of the inner fence is
 *   byte-NEUTRAL and drops ten 1375 -> 874; (b) the scaffold's fence COUNT is
 *   byte-neutral over 1..4 (only the threshold it must clear matters); (c) the
 *   whole +19 is the OUTER WRAPPER's do{}while(0) — it is worth 19B somewhere
 *   unrelated, and that is the real thing to explain before any unwrap pays.
 * - HARD ARITHMETIC LIMIT (why no fence tuning can ever finish this): with the
 *   wrapper kept, ten's weighted-ref floor is 31 (874); with it unwrapped, 21
 *   (592).  rankSpriteBase at the TARGET's shape (1 def + 1 use, live 147) tops
 *   out at 544.  592 > 544, so even a FULL unwrap cannot reproduce the target's
 *   configuration.  The residual must therefore be a LIVE-LENGTH difference:
 *   either ten live >= 1545 (ours 1418) or rankSpriteBase live <= 135 (ours 147;
 *   80000/135 = 593 > 592).  The target's asm span between `addiu s7,sp,0x60`
 *   and `addu a0,s7,a0` is 136 insns — 11 shorter than our 147.
 * - THE ROUND-7 LEAD (a genuine reframing): the s7/s8 flip is probably a
 *   CONSEQUENCE of the row body reaching the target's length, not a cause.  Fix
 *   the remaining row-body diffs first; if rankSpriteBase's live range falls to
 *   <=135 while ten sits at its 21-ref floor, the flip falls out with NO fence at
 *   all and the scaffold deletes itself.  Do not spend another round tuning
 *   fences — the pair cannot be separated by refs.
 *
 * 2026-07-17 round-7 (Opus) — THE +4/-4 SKEW PAIR IS SOLVED IN BOTH HALVES.
 * Both halves are VERIFIED insn-for-insn against the target; neither is
 * committed because they are EXACTLY complementary (+1 / -1 insn) and a third
 * piece (below) is still missing, so any subset breaks the 1159 extent.
 * Round 4 measured each half SEPARATELY, saw each break the length, and parked
 * both ("both are the SAME parked +-4 pair") — but NEVER COMBINED THEM.
 *
 * (A) ROW HALF (+1 insn) — the target's row sign arm reproduces EXACTLY with:
 *       s16 signedValue; u16 value; s32 widenedValue;
 *       signedValue = (value_);        // addiu a0,s4,1
 *       value = signedValue;           // move a1,a0   (back at the LOOP TOP)
 *       ... x/y stores ...
 *       widenedValue = signedValue;    // sll a0,a0,0x10 / sra a0,a0,0x10
 *       if (widenedValue < 0) { value = -widenedValue; negative = 1; goto L; }
 *       else { negative = 0; }
 *   Gives target's sll/sra/bgez a0/sh v0,6(s3)[ds]/negu a1,a0 and restores
 *   `move a1,a0` to the loop top (target 0x8005597c).  WHY (convert.c, read):
 *   `value = -signedValue` with u16 value / s16 signedValue can NEVER emit a
 *   sign-extended negate — convert_to_integer NARROWS NEGATE_EXPR through the
 *   truncation, so cc1 always emits `negu` on the RAW reg and combine then
 *   collapses the compare to a bare `sll` (one insn short).  Only a separate
 *   SImode variable makes the sign_extend a real 2-use value (compare + negate),
 *   which both keeps the sra and blocks that combine fold.  Round 4's rejected
 *   `sv = (s16)(i+1) << 16; sv >>= 16;` is NOT the same thing (its pair hoists);
 *   this spelling stays put.
 *
 * (B) MEDAL HALF (-1 insn) — the bgez delay nop fills with `move a0,s0` using:
 *       medal = &ItemImage[...]->sprite;   // + the 4 x/y/scale stores
 *       brightness = rcos((GameClock << 12) / 90) * 80;   // the rcos CALL
 *       medalDraw = medal;                 // <- copy AFTER the call
 *       medal++;                           // <- blocker (see below)
 *       if (brightness < 0) { brightness += 0xFFF; }
 *       brightness = (brightness >> 12) + 0x7F;
 *       medalDraw->r = medalDraw->g = medalDraw->b = brightness;
 *       GsSortSprite(medalDraw, OTablePt, 1);
 *   Verified dispositions: `85 in 16` (medal -> s0) and `86 in 4`
 *   (medalDraw -> a0) — the target's exact configuration; whole medal
 *   store/call block then matches.  TWO rules make it work:
 *   - THE COPY MUST BE BORN AFTER THE rcos CALL.  Placed BEFORE it (round 4's
 *     and my first attempt) the pseudo lives across the call, is forced into a
 *     CALLEE-SAVED reg (measured: `86 in 16` = s0), and can never be a0 — the
 *     nop stays.  This is why round 4 concluded the second pointer always fails.
 *   - ROUND 4'S "any position" FOR THE BLOCKER IS WRONG.  `medal++` must sit
 *     BETWEEN the copy and medalDraw's uses (it defeats cse1's make_regs_eqv
 *     re-canon; it is then deleted at COMBINE, verified across dumps).  At the
 *     END of the block it is a pure no-op — cc1 emits a BYTE-IDENTICAL object
 *     (Shake content-hashes the .o and correctly skips the relink; that is not
 *     a stale build).  Without any blocker: copy dies, nop returns (4640).
 *   RTL PROOF the second pointer is unavoidable: with ONE medal pseudo the arg
 *   copy (insn 2715 `a0 = medal`) is emitted at the call and is hoisted above
 *   the r/g/b stores only in the regmove/sched1 window — at `.flow` it is still
 *   after the stores on reg 85; at `.sched` it is at the TOP of the POST-JOIN
 *   block with the stores rewritten to a0.  It can never cross the join label,
 *   so the bgez slot is unreachable.  (Also note: cc1 emits this region in
 *   `.set reorder`; the bgez slot is filled by the ASSEMBLER from the single
 *   immediately-preceding insn — not by cc1's dbr.)
 *
 * (C) THE ONLY MISSING PIECE (worth +1 insn) — the medal ADDRESS setup.
 *   Target:  lw v0,0(v0) / nop / addiu s0,v0,104 / li v0,138
 *   Ours(B): lw s0,0(v0) / li v0,138 / addiu s0,s0,104
 *   The lw temp (pseudo 777, NO .greg disposition => allocated by LOCAL-alloc)
 *   takes s0 instead of the target's v0.  With the temp on v0 the load-delay is
 *   UNFILLABLE (`li v0,138` would clobber the addiu's operand) and the target's
 *   nop is forced; on s0 cc1 fills it and we lose the insn.  So: temp==v0 <=>
 *   the nop <=> the 1159 extent with (A)+(B) both in.
 *   Measured at (A)+(B): 16 insns FIXED (0x80055940-0x8005597c medal slot +
 *   store/call block + row preheader, and 0x800559a8/ac/b4 row sign arm);
 *   ~10 real insns broken, ALL inside 0x800558d4-0x800558fc (this address
 *   setup); the rest of the "broken" set is branch targets skewed by the -4.
 *   Solve (C) and the state should land near ~117 differing insns (~350B).
 *   Tried and did NOT move the temp off s0 (all still 4632): medalDraw declared
 *   before/after medal; medalDraw block-scoped inside the `if`; role swap
 *   (fresh block-scoped setup pointer + medal as the a0 copy).  Next lead: find
 *   why local-alloc ties the temp into medal's callee-saved qty here but not in
 *   the baseline (read .lreg's qty/combine_regs decisions for that block).
 *
 * Remaining classes — RE-MEASURED at 401 (the old "498 bytes" list below was
 * stale; two of its entries are now closed).  A target-vs-draft register-mention
 * histogram (the cookbook's mega-pseudo diagnostic) comes back CLEAN: s0-s8 all
 * match exactly, so there is NO mega-pseudo here and the s2->s3 cascade the
 * round-5 ladder flip addressed is DONE (s2 148/148, s3 52/52).  s7/s8 is now
 * 4/4 and 13/13 (round-6, above).  Of the 144 differing rows, 85 are PURE
 * REORDER (identical insn text at a different address = scheduling), and only
 * 59 are real.  The real ones, largest first:
 *   - colon x=0x66 register shift: target `li t1,102`/`sh t1,4(s2)` x4 vs our
 *     li/sh v0 (8 insns, 32B).  NOTE the old name "store deferral" was a
 *     misreading: the stores are NOT deferred, our block is insn-for-insn
 *     identical to the target and only the register differs.  Diagnosed in the
 *     round-6 section above — it is fence-inflated local-alloc priority, i.e.
 *     the SAME root cause as the ten/s7-s8 pair, not an independent lever.
 *   - scan/shift/bank addu operand orders (addu v0,v0,s1 vs addu v0,sp,s1 etc).
 *   - the SV32 subtraction draw's v1/a0 role swap (lbu/subu/bgez/negu block).
 *   - entry a0/128 dbr delay-slot fill; LoadTIMAndFree arg copy 9 low
 *     (brightness-fence-blocked, above); rank/char bank-address hoist;
 *     row-loop transient +4 skew; GsSortSprite a2 schedules (the bulk of the
 *     85 reorder rows: target places `move a2,zero` early, we place it late).
 *
 * The local aggregates reproduce the original stack layout: packed
 * ScoreResult copy at sp+50, sprite banks at sp+60/sp+118, one GsIMAGE
 * scratch at sp+160 reused by all seven sprite initialisations.
 *
 * 2026-07-17 round-4 REJECTED (measured; do not repeat):
 * - Row s32 carrier with `sv = (s16)(i+1) << 16; ... sv >>= 16;` (in-place
 *   sll/sra pair like the target's 559a8/ac): correct-length head but the
 *   pair HOISTS to the block top (its statement's early LUID; the pair is
 *   sched-mobile) and the medal nop stays -> +1 insn overall.  The v2 s16
 *   spelling instead lets combine fold sra+bgez (one short) which exactly
 *   cancels the medal nop; both are the SAME parked ±4 pair.
 * - The medal bgez delay nop (target fills it with the `move a0,s0` arg
 *   copy): a second `medalSprite` pointer is REQUIRED (sbs+call via a0-born
 *   pre-branch), but every spelling that keeps its pseudo alive perturbs the
 *   global conflict web and flips draws 0/2/3's jump1 sign arms (their
 *   drawY-t0 carriers dissolve, j's vanish, -3 insns): initializer form
 *   aliases at expand (2 refs); declare-then-assign dies to cse1
 *   -fcse-skip-blocks (uses re-canoned to medal across the skipped
 *   brightness arm); do{}while(0) around the copy makes loop.c
 *   move_movables SUBSTITUTE it away (reg=reg movable); a dead `medal++`
 *   blocker (straight-line or arm-local, any position) preserves the copy
 *   and fills the slot EXACTLY like the target but always costs the
 *   drawY collateral.  4 candidate spellings measured 4624/4640; parked.
 *
 * 2026-07-16 round-3 closure (flat at 476; do not retry these):
 * - The drawY-carrier lead is CLOSED at the C level: for constant-y draws
 *   (5/7/8, y=-53/-36/-18) copy-prop/constant-rematerialization dissolves
 *   any carrier (seed AND store through it are strict no-ops), so it cannot
 *   pin y live to force v0/t1. The only computed-y site (the row draw) is
 *   separately blocked: rowValue->a0 is a p91 HARD-CONFLICT, which also
 *   produces the self-cancelling +4/-4 skew pair (medal nop / merged j).
 * - Draws 5/7/8's v0-vs-t1 shift is a sched1 y-load sink tied to their s16
 *   (lhu) value loads; jump restructuring regresses the matching sign branch.
 * - The bank-address base-first/base-last swap is cc1 address-selection, not
 *   spelling (subscript and cast-macro forms diverge identically).
 * - Other measured dead ends: rankColour entry fence (no-op), row value as
 *   i+1 (length) or (u16)(i+1) (480), y-seed/x-store fence (+2 insns),
 *   160 guided candidates (no improving path).
 * (Round-3's residual list is superseded: its "ten $s7<->$s8 x10
 * priority-infeasible" is SOLVED in round 6, and its "$s2->$s3 x13" is solved
 * by round 5's ladder flip.  Both verdicts were wrong; see above.)
 */

typedef struct
{
    u8 stageBosses;
    u8 stageEnemies;
    u8 findEnemies;
    u8 murders;
    u8 criticals;
    u8 friendHits;
    u8 pad[2];
    s32 clock;
} ScoreStats;

typedef struct
{
    u16 field0;
    u16 field2;
    u16 field4;
    u16 field6;
    u16 score;
    s16 grade;
} ScoreResult;

typedef struct
{
    u8 pad_000[0x44C];
    u8 characters[5];
    u8 grades[5];
    u8 pad_456[2];
    s32 scores[5];
} MissionScorePersistent;

typedef struct
{
    u16 oldPad;
    u16 pad;
    void *background;
} MissionScoreTail;

extern u8 CHOSEN_CHARACTER;
extern u8 CHOSEN_STAGE;
extern u8 CHOSEN_LANGUAGE;
extern u8 STAGE_LAYOUT_NUMBER;

extern char NUMBER_TIM_PATH[];
extern char *RANKS_ARCHIVE_PTRS[];
extern char *TRN_SPRITE_PTRS[];
extern char D_80013AA8[];
extern char D_80013AC0[];
extern u8 D_8001005C;

extern u8 D_8001044C[5];
extern u8 D_80010451[5];
extern s32 D_80010458[5];
extern Sprite3D *ItemImage[];
extern s16 D_8008ED50[];

extern s32 GameClock;
extern s16 SkipFrame;
extern GsOT *OTablePt;

extern void init_score_stats(ScoreStats *stats);
extern ScoreResult *calculate_score(ScoreStats *stats, s32 stage);
extern u_long *FileRead(char *path);
extern void vfree(void *ptr);
extern u_long *get_tim_from_archive(u_long *archive, s32 index);
extern void InitSprite(GsIMAGE *image, GsSPRITE *sprite);
extern void LoadTIM(u_long *tim);
extern void _PlayMusic(s32 music, s32 mode);
extern u32 GetRealPad(s32 port);
extern void StartDrawing(void);
extern void DrawBG(void *background);
extern void EndDrawing(s32 arg);
extern void DisposeBG(void *background);
extern void GsSortSprite(GsSPRITE *sprite, GsOT *ot, s32 priority);
extern void FUN_800515b0(GsSPRITE *number, s32 value, s16 x, s32 y,
                         s32 mode);
extern s32 rcos(s32 angle);
extern s32 rsin(s32 angle);
extern void FadeOutDirect(s16 time, s16 attribute, u8 r, u8 g, u8 b);
extern void FUN_80038ce0(void);
extern void FUN_800514d8(void);
extern void FUN_8004f6c0(s32 screen);

static inline void InitScoreSprite(u_long *tim, GsIMAGE *image,
                                   GsSPRITE *sprite)
{
    GetTIMInfo(tim, image);
    InitSprite(image, sprite);
}

/* Round-10: every function-like draw macro (DRAW_SCORE_NUMBER family,
 * DRAW_SCORE_COLON, SCORE_SPRITE_AT) is EXPANDED to its literal per-site
 * text, verified byte-identical (same cpp token stream, same .o sha256).
 * cpp expands textually, so this is the same program cc1 always saw --
 * but each of the ~13 draw/colon/bank sites is now INDEPENDENTLY tunable,
 * which a shared macro spelling forbade.  */

/* The persistent high-score block, addressed by CONSTANT rather than through a
 * pointer local.  This is not cosmetic: with a pointer variable the address is
 * `(plus reg_state reg_index)` and expand_binop emits `addu t,state,index`
 * (base first); inlining the constant makes it `(plus reg_index CONST)`, and
 * expand's EXPAND_SUM/form_sum sorts the constant term LAST, emitting
 * `addu t,index,base` -- the target's operand order. */
#define SCORE_STATE ((MissionScorePersistent *)0x80010000)

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/mission_score_screen", mission_score_screen);
#else
void mission_score_screen(void)
{
    GsSPRITE number;
    ScoreStats stats;
    ScoreResult result;
    GsSPRITE rankSprites[5];
    GsSPRITE characterSprites[2];
    GsIMAGE image;
    MissionScoreTail tail;
    register u_long *tim;
    register u_long *archive;
    register GsSPRITE *initSprite;
    register GsSPRITE *sprite;
    register GsSPRITE *medal;
    register GsSPRITE *medalDraw;
    register u8 *unlock;
    register PersistentState *statePtr;
    register s16 i;
    register s16 newPress;
    s32 brightness;
    register s32 stageItem;
    register s32 goNext;
    register s32 rankColour;
    u32 baseU;
    register s32 negative;
    s32 insertedRank;

    tail.oldPad = 0;
    init_score_stats(&stats);
    /* Preserve the retail scheduler boundary between the initial index seed
     * and the colour/score setup. */
    do {
        i = 0;
    } while (0);
    rankColour = 128;
    result = *calculate_score(&stats, CHOSEN_STAGE);

    tim = FileRead(NUMBER_TIM_PATH);
    {
        register GsSPRITE *initNumber = &number;

        InitScoreSprite(tim, &image, initNumber);
        initNumber->attribute |= 0x50000000;
        initNumber->x = -140;
        initNumber->y = -40;
        do {
        } while (0);
        brightness = rankColour;
        initNumber->r = brightness;
        initNumber->g = brightness;
        initNumber->b = brightness;
        initNumber->mx = initNumber->w >> 1;
        initNumber->my = initNumber->h >> 1;
    }
    /* NO do{}while(0) fence here: merging the pivot reset into the rgb block
     * lets the LoadTIMAndFree(tim) arg copy `move a0,s2` float toward the top
     * (target hoists it right after InitSprite).  Re-adding a fence pins the
     * arg copy back down and regresses (see STATUS). */
    number.mx = 0;
    number.my = 0;
    LoadTIMAndFree(tim);
    number.w = 12;

    {
        register u32 attributeMask;

        archive = FileRead(RANKS_ARCHIVE_PTRS[CHOSEN_LANGUAGE]);
        attributeMask = 0x50000000;
score_rank_sprite_init_loop:
        {
        u32 attribute;
        u32 width;

        tim = get_tim_from_archive(archive, i);
        initSprite = ((GsSPRITE *)((u8 *)(rankSprites) + (i) * sizeof(GsSPRITE)));
        InitScoreSprite(tim, &image, initSprite);
        attribute = initSprite->attribute;
        initSprite->x = -160;
        initSprite->y = -120;
        width = initSprite->w;
        initSprite->r = rankColour;
        initSprite->g = rankColour;
        initSprite->b = rankColour;
        initSprite->mx = width >> 1;
        initSprite->attribute = attribute | attributeMask;
        initSprite->my = initSprite->h >> 1;
        ((GsSPRITE *)((u8 *)(rankSprites) + (i) * sizeof(GsSPRITE)))->mx = 0;
        ((GsSPRITE *)((u8 *)(rankSprites) + (i) * sizeof(GsSPRITE)))->my = 0;
        LoadTIM(tim);
        }
        i++;
        if (i < 5)
            goto score_rank_sprite_init_loop;
    }

    {
        register s32 characterColour;

        i = 0;
        characterColour = 128;
score_character_sprite_init_loop:
        {
            u32 attribute;
            u32 width;
            u32 height;

            tim = get_tim_from_archive(archive, i + 5);
            initSprite = &characterSprites[i];
            InitScoreSprite(tim, &image, initSprite);
            /* Retail keeps this dead attribute load (value overwritten
             * before any use) — a leftover of the rank-loop copy. */
            attribute = *(u32 volatile *)&initSprite->attribute;
            width = initSprite->w;
            height = initSprite->h;
            initSprite->x = -160;
            initSprite->y = -120;
            initSprite->g = initSprite->r = characterColour;
            initSprite->b = characterColour;
            initSprite->mx = width >> 1;
            initSprite->my = height >> 1;
            ((GsSPRITE *)((u8 *)(characterSprites) + (i) * sizeof(GsSPRITE)))->mx = 0;
            ((GsSPRITE *)((u8 *)(characterSprites) + (i) * sizeof(GsSPRITE)))->my = 0;
            LoadTIM(tim);
        }
        i++;
        if (i < 2)
            goto score_character_sprite_init_loop;
    }
    vfree(archive);

    tim = FileRead(TRN_SPRITE_PTRS[CHOSEN_LANGUAGE]);
    tail.background = FUN_8004f4f8(tim);
    vfree(tim);

    {
        for (i = 0; i < 5; i++)
        {
            if (SCORE_STATE->scores[i] == 0)
            {
                SCORE_STATE->scores[i] = 0x1A5C2;
                SCORE_STATE->characters[i] = 0;
                SCORE_STATE->grades[i] = 0;
            }
        }
    }

    insertedRank = -1;
    {
        for (i = 0; i < 5; i++)
        {
            if (SCORE_STATE->grades[i] < result.grade)
            {
                insertedRank = i;
                break;
            }
            if (result.grade == SCORE_STATE->grades[i] &&
                stats.clock < SCORE_STATE->scores[i])
            {
                insertedRank = i;
                break;
            }
        }
    }

    {
        register s32 found = insertedRank;

        if (found >= 0)
        {
            i = 4;
            if (found < 4)
            {
                do
                {
                    SCORE_STATE->scores[i] = SCORE_STATE->scores[i - 1];
                    SCORE_STATE->characters[i] =
                        SCORE_STATE->characters[i - 1];
                    SCORE_STATE->grades[i] = SCORE_STATE->grades[i - 1];
                    i--;
                } while (insertedRank < (s16)i);
            }

            {
                register s32 insertedAt = insertedRank;

                SCORE_STATE->scores[insertedAt] = stats.clock;
                SCORE_STATE->characters[insertedAt] =
                    ((PersistentState *)SCORE_STATE)->chr;
                SCORE_STATE->grades[insertedAt] = result.grade;
            }
        }
    }

    _PlayMusic(12, 1);
    for (;;)
    {
        u16 pad = GetRealPad(0);
        newPress = pad & (pad ^ tail.oldPad);
        tail.oldPad = pad;
        if (newPress & 0x20)
        {
            goNext = 0;
            break;
        }
        if (newPress & 0x800)
        {
            goNext = 1;
            break;
        }

        StartDrawing();
        DrawBG(tail.background);
        FUN_800515b0(&number, stats.clock, 0x46, -0x61, 1);

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            do
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                ((drawY) = (-0x47));
                (value = (stats.criticals), signedValue = (s32)value);
                (drawnSprite)->x = (0x16);
                ((drawnSprite)->y = (drawY));
                if (1)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_0;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_0;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_0:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            } while (0);
        } while (0);
        do
        {
            u8 oldU;
            s32 colonDigit;
            GsSPRITE *numberSprite;

            numberSprite = &number;
            number.x = (0x1F);
            oldU = (numberSprite)->u;
            colonDigit = 12;
            (numberSprite)->u = oldU + (numberSprite)->w * colonDigit;
            GsSortSprite((numberSprite), OTablePt, 0);
            (numberSprite)->u = oldU;
        } while (0);
        do
        {
            s32 dividend;
            s32 remainder;
            s32 quotient;
            u16 value;
            s32 signedValue;
            s32 drawY;
            GsSPRITE *numberSprite;

            do {
                numberSprite = &number;
            } while (0);
            signedValue = (stats.stageEnemies - stats.stageBosses);
            value = signedValue;
            drawY = (-0x47);
            (numberSprite)->x = (0x2F);
            (numberSprite)->y = drawY;
            sprite = (numberSprite);
            if (signedValue < 0)
            {
                value = -signedValue;
                negative = 1;
                goto score_number_1;
            }
            else
            {
                negative = 0;
            }
        score_number_1:
            do
            {
                dividend = (s16)value;
                quotient = dividend / 10;
                remainder = dividend % 10;
                baseU = sprite->u;
                sprite->u = baseU + (s16)remainder * sprite->w;
                GsSortSprite(sprite, OTablePt, 0);
                sprite->x -= 12;
                value = quotient;
                quotient <<= 16;
                sprite->u = baseU;
            } while (quotient != 0);
            if (negative != 0)
            {
                u32 signBaseU;
                s32 ten;

                ten = 10;
                signBaseU = sprite->u;
                sprite->u = signBaseU + sprite->w * ten;
                GsSortSprite(sprite, OTablePt, 0);
                sprite->u = signBaseU;
            }
        } while (0);
        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            do
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (result.field0), signedValue = (s16)value);
                ((drawY) = (-0x47));
                (drawnSprite)->x = (0x66);
                ((drawnSprite)->y = (drawY));
                if (1)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_2;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_2;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_2:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            } while (0);
        } while (0);

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            do
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (stats.murders), signedValue = (s32)value);
                ((drawY) = (-0x35));
                (drawnSprite)->x = (0x16);
                ((drawnSprite)->y = (-0x35));
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_3;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_3;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_3:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            } while (0);
        } while (0);
        do
        {
            u8 oldU;
            s32 colonDigit;
            GsSPRITE *numberSprite;

            numberSprite = &number;
            number.x = (0x1F);
            oldU = (numberSprite)->u;
            colonDigit = 12;
            (numberSprite)->u = oldU + (numberSprite)->w * colonDigit;
            GsSortSprite((numberSprite), OTablePt, 0);
            (numberSprite)->u = oldU;
        } while (0);
        do
        {
            GsSPRITE *drawnSprite;
            GsSPRITE *numberSprite;

            numberSprite = &number;
            drawnSprite = (numberSprite);
            do
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (stats.stageEnemies), signedValue = (s32)value);
                ((drawY) = (-0x35));
                (drawnSprite)->x = (0x2F);
                ((drawnSprite)->y = (-0x35));
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_4;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_4;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_4:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            } while (0);
        } while (0);
        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            do
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (result.field2), signedValue = (s16)value);
                ((drawY) = (-0x35));
                (drawnSprite)->x = (0x66);
                ((drawnSprite)->y = (-0x35));
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_5;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_5;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_5:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            } while (0);
        } while (0);

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            do
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (stats.findEnemies), signedValue = (s32)value);
                ((drawY) = (-0x24));
                (drawnSprite)->x = (0x23);
                ((drawnSprite)->y = (-0x24));
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_6;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_6;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_6:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            } while (0);
        } while (0);
        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            do
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (result.field6), signedValue = (s16)value);
                ((drawY) = (-0x24));
                (drawnSprite)->x = (0x66);
                ((drawnSprite)->y = (-0x24));
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_7;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_7;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_7:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            } while (0);
        } while (0);
        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            do
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (result.score), signedValue = (s16)value);
                ((drawY) = (-0x12));
                (drawnSprite)->x = (0x66);
                ((drawnSprite)->y = (-0x12));
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_8;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_8;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_8:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            } while (0);
        } while (0);

        if (result.grade == RANK_GRAND_MASTER)
        {
            medal = &ItemImage[D_8008ED50[CHOSEN_STAGE]]->sprite;
            medal->x = 0x8A;
            medal->y = -0xE;
            medal->scalex = 0x1000;
            medal->scaley = 0x1000;
            brightness = rcos((GameClock << 12) / 90) * 80;
            if (brightness < 0)
            {
                medalDraw = medal;
                brightness += 0xFFF;
            }
            else
            {
                medalDraw = medal;
            }
            brightness = (brightness >> 12) + 0x7F;
            medalDraw->r = medalDraw->g = medalDraw->b = brightness;
            GsSortSprite(medalDraw, OTablePt, 1);
        }

        {
            GsSPRITE *rankSpriteBase;
            register GsSPRITE *rowSprite;

            i = 0;
            rowSprite = &number;
            do { do { do { do
            {
                rankSpriteBase = rankSprites;
            } while (0); } while (0); } while (0); } while (0);
score_row_loop:
            {
                do
                {
                    s32 dividend;
                    s32 remainder;
                    s32 quotient;
                    u16 value;
                    s16 signedValue;
                    s32 widenedValue;
                    s32 drawY;
                    register s32 negative;

                    signedValue = (i + 1);
                    value = signedValue;
                    drawY = (i * 0x16 + 0x18);
                    (rowSprite)->x = (-0x8F);
                    (rowSprite)->y = drawY;
                    widenedValue = signedValue;
                    if (widenedValue < 0)
                    {
                        value = -widenedValue;
                        negative = 1;
                        goto score_row_number;
                    }
                    else
                    {
                        negative = 0;
                    }
                score_row_number:
                    do
                    {
                        dividend = (s16)value;
                        quotient = dividend / 10;
                        remainder = dividend % 10;
                        baseU = (rowSprite)->u;
                        (rowSprite)->u = baseU + (s16)remainder * (rowSprite)->w;
                        GsSortSprite((rowSprite), OTablePt, 0);
                        (rowSprite)->x -= 12;
                        value = quotient;
                        quotient <<= 16;
                        (rowSprite)->u = baseU;
                    } while (quotient != 0);
                    if (negative != 0)
                    {
                        u32 signBaseU;
                        s32 ten;

                        ten = 10;
                        signBaseU = (rowSprite)->u;
                        (rowSprite)->u = signBaseU + (rowSprite)->w * ten;
                        GsSortSprite((rowSprite), OTablePt, 0);
                        (rowSprite)->u = signBaseU;
                    }
                } while (0);
                FUN_800515b0(&number, SCORE_STATE->scores[i],
                             0x79, i * 0x16 + 0x18, 1);
                {
                    s32 rowScopePad;
                }

                sprite = &characterSprites[SCORE_STATE->characters[i]];
                sprite->x = -0x79;
                sprite->y = i * 0x16 + 0x16;
                if (i == insertedRank)
                {
                    brightness = rsin((GameClock << 12) / 90) * 60;
                    if (brightness < 0)
                    {
                        brightness += 0xFFF;
                    }
                    brightness = (brightness >> 12) + 100;
                }
                else
                {
                    brightness = 0x80;
                }
                sprite->r = sprite->g = sprite->b = brightness;
                do {
                } while (0);
                GsSortSprite(sprite, OTablePt, 1);

                {
                    register GsSPRITE *rankSprite =
                        &rankSpriteBase[SCORE_STATE->grades[i]];
                    rankSprite->r = rankSprite->g = rankSprite->b = 0x7F;
                    rankSprite->scalex = rankSprite->scaley = 0xB33;
                    rankSprite->x = -0x2F;
                    rankSprite->y = i * 0x16 + 0x16;
                    GsSortSprite(rankSprite, OTablePt, 1);
                }
            }
            i++;
            if (i < 3)
            {
                goto score_row_loop;
            }
        }

        SkipFrame = 2;
        EndDrawing(0);
    }

    if (result.grade == RANK_GRAND_MASTER)
    {
        register PersistentState *state = (PersistentState *)0x80010000;

        stageItem = D_8008ED50[state->stage];
        if (state->stock[stageItem + (state->chr << 5)] == 0xFE)
        {
            state->stock[stageItem + (state->chr << 5)] += 3;
        }
        stageItem = D_8008ED50[state->stage];
        if (state->backup[stageItem] == 0xFE)
        {
            state->backup[stageItem] += 3;
        }
    }

    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();
    do {
        statePtr = (PersistentState *)0x80010000;
    } while (0);
    if (D_8001005C != 0)
    {
        LoadTIMAndFree(PathFileRead(D_80013AA8, D_80013AC0));
        FUN_800514d8();
    }
    DisposeBG(tail.background);

    if (goNext == 1)
    {
        FUN_8004f6c0(0x11);
    }
    else
    {
        statePtr->layout = 0xFF;
        FUN_8004f6c0(0x10);
    }
}
#endif
