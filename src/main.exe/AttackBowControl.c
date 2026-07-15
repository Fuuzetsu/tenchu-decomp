#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackBowControl(void);
 *     MOTION.C:800, 28 src lines, frame 72 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+56     struct SVECTOR vect
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 19 of 272 bytes differ. Residual is a pure
 * register-naming swap (a1<->v1 for `count`, a0<->v0 for the
 * `D_80097714`-table address, same instructions/order/length, 68 insns
 * both sides) inside the `count == 1` guard and its merge point — see the
 * matching notes below for the two source-shape fixes that got it this
 * far, and the permuter note for why it's parked here.
 *
 * AttackBowControl (0x8001f5a8, 0x110 bytes) — bow attack-frame callback.
 * PSX.SYM's demo build took NO parameters at all (and never touched
 * FieldIndex/MotionUpdateMode); retail's `.s` proves a real `s16 n` PARAMETER
 * (the `(n<<16)>>14` fused sign-extend+×4-scale on the raw incoming $a0,
 * before anything is stored) selecting a `{min,max}` trigger-frame window —
 * ActATTACK's own (still-unmatched) call sites don't show an argument either
 * (same earlier-build/retail signature drift, on both ends of the call).
 *
 * `D_80097714`/Ghidra's separately-invented `DAT_80097716` are the SAME
 * byte-addressed table read through ONE base (`&D_80097714 + idx`, `idx`
 * already scaled ×4): `min` at +0, `max` at +2 — a `{s16 min, max;}` struct
 * array, not two parallel arrays (the aggregate-splits-into-drifted-D_-
 * symbols pattern, cookbook's gp section). Bound the sole symbol
 * (`D_80097714`) in config/symbols.main.exe.txt since this was its only
 * `.s` referencer.
 *
 * `n` (the parameter) survives THREE calls (Sound/UpdateOrnament/
 * DrawOrnament) in the first half, so it's cached into a callee-saved
 * register at entry and re-read (re-extended via the same shift pair) both
 * inside the first if/else and again for the wholly separate second
 * `if (dtM->count == …max)` check — two independent, unrelated statements,
 * not one cached value.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - **Ghidra's own literal form — `iVar2 = (n<<16)>>14;` as its OWN
 *    statement, THEN `*(s16*)((u8*)D_80097714 + iVar2)` — is load-bearing,
 *    not just a reference style.** Writing the equivalent `D_80097714[n]`
 *    array subscript inline computes the SAME address but in the OPPOSITE
 *    instruction order (base lui/addiu, then the idx sll/sra, then addu);
 *    only when `idx` is a NAMED local assigned as its own prior statement
 *    does cc1 emit the idx shift-pair FIRST (from the preceding statement)
 *    and the base address second (when evaluating the pointer-arithmetic
 *    expression that consumes it) — matching the target exactly. Same
 *    value, same final instructions, pure source-shape-driven ORDER lever.
 *  - The residual 19-byte register swap (a1<->v1 for `count`, a0<->v0 for
 *    the table address) survived autorules.py (no improving edit among 8
 *    candidates: count's width, idx's width, p's width) and one bounded
 *    decomp-permuter run (`tools/permute.py AttackBowControl --stop-on-zero
 *    -j4`, ~80k+ iterations, flat at score 115, no score-0 hit) — the named
 *    sub-C-level class (same instructions/order/length, pure register
 *    choice for the very first live locals `count`/the table address).
 *    Parked per the cookbook's sub-C-level early-stop; do not re-permute.
 *
 * RTL escalation (this session, `.greg`/`.lreg`): the swap is a genuine
 * global-alloc PRIORITY tie, not a reachable respelling. `.greg` lists
 * `5 regs to allocate: 85 84 87 81 80` — that IS the descending-priority
 * processing order (`floor_log2(refs)*refs/live_length`, computed from
 * `.lreg`'s `used N times across M insns`): `p`(85, refs=5/life=8)=1.25,
 * `idx`(84, refs=4/life=9)=0.89, `count`(87, refs=4/life=13)=0.615,
 * raw-`n`(81)=0.5, cached-`n`(80)=0.07. `p`/`idx` are processed FIRST and
 * both hard-conflict with `v0` AND `v1` (`85/84 conflicts: ... 2 3 ...`),
 * so they fall through to the next class candidate, `$a0` (search order is
 * v0,v1,a0,a1,a2,a3,... for this port); `count` is processed after and
 * conflicts only with `v0` + the already-placed pseudos (no `3`/`v1` in
 * its conflict list), so it takes the next free candidate in that order —
 * `v1`. The target's own asm proves `count`(a1) and the table pointer
 * (v1) ARE simultaneously live across the same range-check span (both
 * used twice, non-overlapping recompute) exactly like our draft — so the
 * target isn't using a delay-slot/scratch-reuse trick here (unlike
 * FUN_8001b2f4's `rp`); it is coloured with `count` in `a1` and the
 * pointer in `v1`, meaning in the TARGET's compile `count` must be
 * processed at HIGHER priority than the table pointer/idx (opposite
 * order from ours) so it claims a register before `v0`/`v1` are excluded
 * for it. Since `count`'s own refs/life are fixed by real control flow
 * (read once, used in the `==1` test and both range-check compares — no
 * legitimate extra reference or shortened life available), and `p`/`idx`
 * likewise (reused across the two textually-separate `(n<<16)>>14`
 * computations per the note above), no ballast or reordering changes the
 * priority order without changing what the function computes. The SAME
 * pattern repeats a third time at the final `dtM->count` re-read
 * (`a0`<->`v1` for `dtM` itself) — consistent with one whole-function
 * priority ordering, not three independent local ties. Root cause is a
 * genuine global-alloc priority tie; not reachable from this source
 * shape.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AttackBowControl", AttackBowControl);
#else
/* Draft — turn this into matching C, then delete the #ifndef/#else/#endif
   guards. Reference: */

extern s16 D_80097714[]; /* byte-addressed; {min,max} pairs, stride 4 */
extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern void Sound(Humanoid *h, int id);
extern void bow_shoot_logic(s16 kind, VECTOR *start);
extern void UpdateOrnament(OrnamentType *objp, short ry);
extern short DrawOrnament(OrnamentType *objp);

void AttackBowControl(s16 n)
{
    s16 count;
    VECTOR *pos;
    PARAM_ITEM_USE item; /* PSX.SYM's "PARAM_ITEM_LAUNCH item" (unused here —
                            same 48-byte dead-local frame lever as
                            AttackGunControl; item.h's proven 0x28-byte struct) */
    SVECTOR vect;         /* PSX.SYM's "struct SVECTOR vect" (also unused) */
    s32 idx;
    u8 *p;

    count = dtM->count;
    if (count == 1)
    {
        Sound(Me_MOTION_C, 2);
    }
    else
    {
        idx = (n << 16) >> 14;
        p = (u8 *)D_80097714 + idx;
        if (*(s16 *)p <= count && count < *(s16 *)(p + 2))
        {
            UpdateOrnament(Me_MOTION_C->weapon[2], 0);
            DrawOrnament(Me_MOTION_C->weapon[2]);
        }
    }
    idx = (n << 16) >> 14;
    p = (u8 *)D_80097714 + idx;
    if (dtM->count == *(s16 *)(p + 2))
    {
        pos = GetAbsolutePosition(Me_MOTION_C->model->object[0xD], 0, 0, 0);
        bow_shoot_logic(0x15, pos);
        Sound(Me_MOTION_C, 3);
    }
}
#endif /* NON_MATCHING */
