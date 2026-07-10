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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
                            FUN_800274e8; item.h's proven 0x28-byte struct) */
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
