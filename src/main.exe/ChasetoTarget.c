#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ChasetoTarget(long length);
 *     THINK.C:269, 21 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s5       long length
 *     reg   $s3       long xx
 *     reg   $s2       long zz
 *     reg   $s4       long * chase
 *     reg   $s3       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 *     extern long Distance;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — links 340 bytes vs the 328-byte target (3
 * instructions / 12 bytes LONG; only 10 differing asmdiff lines, ALL of them
 * from the three reorg delay-slot residuals below — register allocation now
 * matches the target exactly). Default build keeps the byte-identical
 * INCLUDE_ASM stub.
 *
 * PROGRESS (this session, from 36 differing lines -> 10): the whole register
 * allocation (me=$s1, xx=$s3, zz=$s2, chase=$s4, c0=$a2, c1=$a1-reused, the
 * abs temp in $v0) now matches the target. Three source rewrites did it,
 * each verified against the demo build (siblingdiff --demo) as the structure
 * oracle and the .greg allocation-order dump:
 *   1. The abs clamp must reach cc1's ABS_EXPR, NOT be open-coded as
 *      `t = xx; if (xx<0) t=-xx;`. The open-coded form (a) keeps `t` scheduled
 *      early into $a1 instead of the target's $v0 (its live range overlaps
 *      $v0's zz use), and (b) its neg reads zz (`t = -zz`) — 5 zz refs vs the
 *      target's 4 — which flips the .greg allocno priority so zz beats me for
 *      $s1 (me lands in $s2). A statement `t = -t` does NOT help: cse
 *      copy-propagates it back to `t = -zz`.
 *      CORRECTED IN ROUND 2 — the lever is reaching ABS_EXPR, NOT "inline vs
 *      named temp". Measured byte-identical (see ROUND 2 below): the inline
 *      `?:`, `__builtin_abs(xx)`, and an ASSIGNED `ax = __builtin_abs(xx)` all
 *      produce the same asm, because fold-const.c rewrites `A >= 0 ? A : -A`
 *      into ABS_EXPR before expand. An assigned abs is fine; what loses is an
 *      open-coded compare-and-negate, which fold cannot see and which therefore
 *      never becomes abssi2. Any earlier note claiming the win came from
 *      *inlining* (or from reorg visibility) is wrong — do not propagate it.
 *   2. c1 (`chase[1]`) is INLINED into the zz expression, not read into a
 *      named `c1` before zz. A separate `c1 = chase[1];` load schedules early
 *      (into $a2/$a3), before the target-ptr $a1 dies at `position.z`; inline,
 *      it loads during zz AFTER $a1 is free, so it reuses $a1 exactly like the
 *      target. c0 stays a named temp (target loads it distinctly into $a2).
 *   3. Me_THINK_C captured into `me` (unchanged, still required — see below).
 *
 * Matching notes (verified against the raw .s):
 *  - `chase = &Me_THINK_C->some_other_x_position;` is computed at entry,
 *    before the target null check (asm's `addiu $s4,$s1,0x80` sits ahead of
 *    the `beqz`). Index 0 (`some_other_x_position`) is always addressed
 *    DIRECTLY off `Me_THINK_C`; index 1 through `chase[1]` ($s4+4). The two
 *    spellings don't CSE, so this asymmetry is required.
 *  - `Me_THINK_C` captured into a named local (`me`): the target keeps it in
 *    a callee-saved reg across rand()/rcos()/rsin() (post-call store reuses
 *    the entry-loaded reg), which needs a real local copy.
 *  - The guard `(Attrib & 0xc000) != 0 || (c0 | chase[1]) == 0` must be a
 *    REAL bitwise-OR test `(c0 | chase[1]) == 0` (one `or`+`bnez`); the
 *    logical `c0==0 && c1==0` spelling compiles to two reload+branch pairs.
 *
 * RESIDUAL (12 bytes / 3 instructions long, ALL reorg delay-slot fills; the
 * source structure, layout, and register allocation are byte-confirmed
 * correct). Exact accounting against the retail disassembly at 0x8002bb70
 * (82 insns vs our 85):
 *   (a)+(b) The null-check guard. Target:
 *             lw a1,116(s1) / addiu s4,s1,128 / beqz a1,EPILOGUE / move v0,zero
 *           Ours:
 *             lw a1,116(s1) / beqz a1,$L5 / addiu s4,s1,128   (+maspsx load nop)
 *           fill_simple_delay_slots' BACKWARD scan steals the `chase` addiu, so
 *           $L5 keeps a reference and cannot die. That costs TWO insns: the
 *           surviving `$L5: move v0,zero` island (extra #1) and the `j $L9`
 *           needed to hop over it (extra #2). In the target all four return-0
 *           branches get their own eager-copied `move v0,zero` and retarget to
 *           the epilogue, so $L5 becomes unreferenced, is deleted, and the
 *           turn_towards_player_ tail simply falls through. The other THREE
 *           return-0 branches already match: their backward scan is blocked
 *           because each predecessor sets the branch's condition reg, and then
 *           stops outright at the preceding JUMP_INSN (stop_search_p).
 *   (c) The `(Attrib & 0xc000)` branch's delay slot is a `nop` (extra #3); the
 *       target puts the fallthrough's `or $v0,$a2,$a1` there. fill_eager's
 *       mostly_true_jump returns 1 for an NE condition, so it tries the taken
 *       thread ($L8 = `jal rand`, not eligible) first and then declines the
 *       fallthrough. Not root-caused to a source lever.
 *
 * ROUND 2 — the abssi2 hypothesis is DISPROVEN (recorded so nobody retries it).
 * The proposal was that the inline `?:` abs is "reorg-visible" and an ASSIGNED
 * `__builtin_abs` would reach the reorg-invisible mips `abssi2` multi insn and
 * deny reorg the steal. Measured: all THREE spellings compile to BYTE-IDENTICAL
 * asm (whole-file diff of the cc1 runs is empty; all link 340) —
 *   `if ((xx >= 0 ? xx : -xx) < 500)`      (baseline)
 *   `if (__builtin_abs(xx) < 500)`         (inline builtin)
 *   `ax = __builtin_abs(xx); if (ax < 500)` (the ASSIGNED form, as proposed)
 * Cause, from the pinned
 * sources: fold-const.c:5590 ("If we have A op 0 ? A : -A ...") rewrites the
 * `?:` into ABS_EXPR *before* expand, so BOTH spellings already reach abssi2.
 * mips.md:1800 abssi2 is `type "multi"`, `length "3"`, and its two-register
 * template is literally `bgez %1,1f / move %0,%1 / subu %0,$0,%0 / 1:` — which
 * is exactly the target's 0x8002bbd4 `bgez s3 / move v0,s3 / negu v0,v0`. The
 * abs region has ALWAYS matched byte-for-byte; there was never a reorg-visible
 * bgez here to remove. (abssi2 is indeed invisible to reorg — define_delay at
 * mips.md:119 requires `length 1`, and abssi2 is length 3 — but that fact buys
 * nothing at this site, because the insn reorg steals is the `chase` addiu in
 * the ENTRY block, nowhere near the abs.)
 *
 * WHY THE STEAL IS HARD TO STOP (mechanism, from reorg.c fill_simple_delay_slots
 * lines 3063-3126 + mips.md define_delay line 119). The backward scan takes the
 * FIRST eligible insn walking back from the branch and then BREAKS. Eligible =
 * `dslot no` AND `length 1`. Trace for our guard: needed={$a1} from the branch;
 * `lw a1,116(s1)` is blocked (it sets $a1, which is needed, and loads are
 * `dslot yes`); the very next insn back is `addiu s4,s1,128`, which sets $s4
 * (not needed) and reads $s1 (not in `set`) -> taken, break. It follows that in
 * the target's pre-reorg stream there MUST have been a third eligible insn
 * sitting BETWEEN the addiu and the branch, and — since that is what ended up
 * in the slot — it was `move v0,zero`. Reordering the two entry insns cannot
 * help: either order still leaves the addiu as the closest eligible candidate.
 * The open question is what C emits an unconditional `v0 = 0` into the entry
 * block: a plain `r = 0; if (p == 0) return r;` is constant-propagated by cse
 * back into the branch arm and the pre-branch `move` disappears, which is why
 * the deduction has not yet been convertible into source.
 *
 * DISPROVEN this round (do not retry):
 *  - assigned `__builtin_abs` for either/both clamps: byte-identical, 340.
 *  - early-return flattening (`if (p == 0) return 0;` + the three inner
 *    `goto ret0` rewritten as `return 0;`, body de-nested): 348, WORSE.
 *    Cross-jumping merges the four `move v0,0; j $L9` tails back into one
 *    island regardless, so `goto ret0` and `return 0` are the same shape.
 *  - `deg` widened to s32 (autorules adopts this on the whole-image metric,
 *    340->336): WRONG. It replaces the target's `sll s0,v0,16 / sra s0,s0,16`
 *    at 0x8002bc48 with a single `move s0,v0`, breaking a region that already
 *    matched; it only "improves" because a shorter function shifts less of the
 *    image. PSX.SYM independently records `reg $v0 short deg`, and retail saves
 *    and restores $s0. `short deg` is correct — do not let autorules re-adopt.
 *
 * Permuter is inapplicable (refuses a >1-instruction length delta).
 *
 * ROUND 3 (humanise pass) — mechanism CONFIRMED against the real target
 * bytes and the pinned gcc-2.8.1 reorg.c source (not just RTL-dump inference
 * as before), plus a critical NEW counter-example. Net: still 340, but the
 * search space for the next attempt is now much smaller.
 *
 * Exact target bytes for all four return-0 sites, from the raw disassembly
 * (config/../.shake/gen/main.exe/asm/nonmatchings/ChasetoTarget/ChasetoTarget.s),
 * confirm every one branches DIRECTLY to the epilogue label (.L8002BC94,
 * where the `lw ra,...`/restore sequence begins) with its OWN `addu
 * v0,zero,zero` in the delay slot — there is no surviving intermediate
 * "ret0" label in the target at all:
 *   beqz  a1,.L8002BC94 / addu v0,zero,zero      (null check)
 *   bnez  v0,.L8002BC94 / addu v0,zero,zero      (abs(zz)<500, the goto-ret0 site)
 *   bnez  v0,.L8002BC94 / addu v0,zero,zero      ((Attrib&0x400)!=0)
 *   bnez  v0,.L8002BC94 / addu v0,zero,zero      (Distance<1000)
 * Our draft already reaches this exact shape for the LAST THREE (verified in
 * our own compiled .s: `bne $2,$0,$L9 / move $2,$0`, landing directly on the
 * shared epilogue label with no intermediate island) — only the null check
 * (the first, entry-block one) fails, producing the extra `$L5: move $2,$0`
 * island plus the `j $L9` needed to hop over it, plus a THIRD cost not
 * previously itemised: since `addiu $20,$17,128` (chase) gets pulled into
 * the null check's delay slot, the `lw $5,116($17)` immediately before it is
 * now directly followed by `beq $5,...` with nothing between them, so
 * maspsx must insert its own load-delay `#nop` there (absent from the
 * target, where `addiu` naturally occupies that gap). All three costs (nop +
 * island + hop) are downstream of the SAME single reorg decision.
 *
 * Root cause, confirmed against gcc-2.8.1's actual source
 * (`tools/ccsrc.py fill_simple_delay_slots` / `stop_search_p`, reorg.c
 * :2975-3129 and :684-715): `fill_simple_delay_slots` always tries a
 * BACKWARD scan of the branch's own block FIRST (before ever considering
 * stealing from the branch's target), walking `prev_nonnote_insn` one insn
 * at a time. It only stops outright (`stop_search_p`) at a CODE_LABEL,
 * JUMP_INSN, BARRIER, or an already-filled SEQUENCE/asm insn; anything else
 * that conflicts (sets/reads a register the branch needs, or that an
 * already-rejected earlier candidate needed) is REJECTED but does NOT stop
 * the scan — its own read/write set is folded into the accumulated
 * needed/set resources and the walk continues further back
 * (reorg.c:3095-3129). For our null check: `lw $a1,...` is rejected (it SETS
 * $a1, the branch's own tested register) but is not a hard stop, so the scan
 * continues and reaches `addiu $s4,$s1,0x80` (chase) next — which touches
 * neither $a1 nor $s1 in any needed/set way — so it is taken immediately.
 * This holds regardless of textual order (matches the already-recorded
 * "reordering the two entry insns cannot help" finding) because the
 * accumulate-and-continue behaviour reaches `addiu` whichever of {lw,addiu}
 * is closer. The other three sites are protected only because their
 * immediately-preceding insn is a `slt`/`andi` that DEFINES the exact
 * register their own branch tests (an unavoidable feature of a `<`/`&`-style
 * compiled test), which is rejected AND then hits a hard stop (a CODE_LABEL
 * or a prior branch's already-filled SEQUENCE) within 1-2 more steps — so
 * they always fall through to stealing "addu v0,zero,zero" from the shared
 * tail instead. The null check has no such compare insn (a direct pointer
 * `!= 0` test compiles straight to `beqz`, no intermediate flag), which is
 * structurally why it alone is exposed. An independently-matched sibling,
 * DrawModelArchive.c, documents the exact same family (reorg steals a
 * shared reject block's leading insn into each eligible branch's delay slot
 * and retargets through it) with the identical failure mode at ONE of its
 * five sites for the identical reason ("its delay slot was already taken by
 * `andi v0,s0,0x10`, so reorg had nothing to steal with") — and that site
 * stays un-collapsed in the MATCHED target, i.e. this reorg quirk is real
 * and sometimes simply wins.
 *
 * BUT: `tools/siblingdiff.py ChasetoTarget --demo` disproves "unreachable
 * from C" as a conclusion here. The demo's ChasetoTarget (THINK.C:269, a
 * much bigger 468-byte body that inlines what became turn_towards_player_)
 * has, at its own null check, the EXACT same local shape as retail —
 * `lw a1,108(s1) / addiu s4,s1,120 / beqz a1,L1ac` — and its delay slot is
 * ALSO won by `move v0,zero`, not by `addiu`. Since the demo and retail were
 * compiled at different times against different surrounding code and BOTH
 * land on the collapsed form for this identical local instruction pattern,
 * some source construct reliably avoids the steal — the deterministic
 * backward-scan argument above must be missing a real difference between
 * our draft and the true source, not a property of the instruction shapes
 * being wrong in principle. I could not find it this round; see the ruled-
 * out list below.
 *
 * NEW this round, all measured, all confirmed genuine no-ops via
 * `tools/nullcheck.py` (not "tried and failed to help" — cc1 produced
 * byte-IDENTICAL .o both ways, so these are eliminated as levers entirely,
 * not just deprioritised):
 *  - Dropping the named `vx`/`vz` locals and storing the rcos()/rsin()
 *    results directly (`me->some_other_x_position = rcos(deg)*length>>12;`
 *    etc.): no-op. cc1's local-alloc already collapses their trivially-short
 *    live ranges to the same registers regardless of whether they are named.
 *  - Declaring locals in PSX.SYM's reversed order (the general §1 rule —
 *    `short pad`(unused)/`deg`/`vz`/`vx`/`chase`/`c0`/`zz`/`xx`, since the
 *    listed order is xx,zz,chase,vx,vz,deg): no-op. This function's pseudo
 *    numbering evidently never hits a QTY_CMP_PRI tie that declaration order
 *    would break — consistent with `regalloc.py --local` (below) already
 *    self-validating the CURRENT order as exactly correct.
 *  - Naming `me->another_camera_related_perhaps` into its own local
 *    (`target = me->another_camera_related_perhaps; if (target != 0) ...`,
 *    using `target->` throughout): no-op, byte-identical codegen either way.
 *  - `tools/regalloc.py ChasetoTarget --local` (new this session): self-
 *    validated, 0 divergences — reproduces cc1's exact 21-pseudo home table.
 *    This independently reconfirms (via a different tool than the prior
 *    rounds used) that register allocation is not where the residual lives;
 *    it is entirely a post-allocation reorg/delay-slot decision.
 *  - `tools/autorules.py ChasetoTarget`: refuses outright (length mismatch;
 *    correctly, since the greedy score would just be the length penalty).
 *  - Fresh permuter runs with the corrected (`-fno-builtin`-fixed) build,
 *    BOTH plain (which still refuses: "instruction length differs by 3")
 *    AND `--force-early` (240s bounded, ~21800 iterations): found no match.
 *    The best candidates by whole-image score were 332-336 bytes (4-8 over,
 *    down from 12) but every one relies on a construct that fails the
 *    "would a person write this" bar on its own terms — e.g. its closest
 *    candidate (332 bytes) only gets there via `c0 = 12; vz = (rsin(deg) *
 *    length) >> c0;`, reusing an already-dead, semantically-unrelated
 *    variable purely to force a variable-shift instruction in place of an
 *    immediate shift. Not ported.
 *
 * Net: the residual is unchanged at 340 (12 bytes / 3 insns), now with a
 * byte-and-source-verified mechanism, a proof (via the demo) that a fix
 * exists in principle, and a substantially narrowed set of remaining
 * candidates (declaration order, naming, and the named-temp question are
 * all now closed off). Whoever picks this up next should look at what
 * ELSE differs about the demo's wider function body that might change
 * `unfilled_slots_base` processing order or the entry block's true RTL
 * shape — not at this local neighbourhood's own spelling.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ChasetoTarget", ChasetoTarget);
#else
extern short Attrib;
extern long Distance;
extern int rand(void);
extern s32 rcos(s32 a);
extern s32 rsin(s32 a);

short ChasetoTarget(long length)
{
    character_state *me;
    long xx, zz;
    long *chase;
    long c0;
    long vx, vz;
    short deg;

    me = Me_THINK_C;
    chase = &me->some_other_x_position;
    if (me->another_camera_related_perhaps != 0)
    {
        c0 = me->some_other_x_position;
        xx = me->another_camera_related_perhaps->position.x + c0
             - me->some_kind_of_current_position->vx;
        zz = me->another_camera_related_perhaps->position.z + chase[1]
             - me->some_kind_of_current_position->vz;

        if ((xx >= 0 ? xx : -xx) < 500)
        {
            if ((zz >= 0 ? zz : -zz) < 500)
            {
                goto ret0;
            }
        }

        if ((Attrib & 0x400) == 0 && 999 < Distance)
        {
            if ((Attrib & 0xc000) != 0 || (c0 | chase[1]) == 0)
            {
                deg = rand();
                vx = rcos(deg) * length >> 12;
                me->some_other_x_position = vx;
                vz = rsin(deg) * length >> 12;
                chase[1] = vz;
            }
            return turn_towards_player_(xx, zz);
        }
    }
ret0:
    return 0;
}
#endif
