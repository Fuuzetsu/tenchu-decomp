#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateSplineControl(struct SplineControlType *spc);
 *     ACTION.C:365, 19 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct SplineControlType * spc
 *     reg   $a2       struct MotionElementType * key0p
 *     reg   $a3       struct MotionElementType * key1n
 * END PSX.SYM */

/*
 * UpdateSplineControl (0x8001bfc0, 0x1B0 bytes) — recompute the per-frame
 * delta vectors (dd0 for [key0p,key1), ds1 for [key0,key1n)) once the
 * bracketing keyframes move. key0p/key1n are key0/key1 nudged one slot
 * outward whenever the frame lands exactly on a keyframe boundary.
 *
 * Matching note: `dt` (the byte-truncated Q8 time delta — keyframe times
 * wrap at 256) is an s16 difference narrowed through (s8) then shifted:
 * combine fuses the narrowing pair with the shift into retail's
 * sll 24 / sra 16. The s16 `diff` temp matters: casting the raw
 * subtraction directly lets cc1's value-range narrowing recompute the
 * operand LOADS in QImode (lbu instead of lh).
 *
 * Both quotients (slope1 for dd0, slope2 for ds1) must be computed
 * BACK-TO-BACK, before either's store block — i.e. exactly Ghidra's
 * original statement order (iVar4, iVar2, [trap guards], iVar1, [trap
 * guards], then the two multiply/store passes). Interleaving them
 * (computing slope1, storing dd0, THEN computing slope2) puts three stores
 * between the two `spc->key0->time` reads that feed dt/slope2; the
 * intervening stores through `spc` invalidate cse1's cached load of that
 * field, forcing a needless reload + reload-preserving `move` sequence (4
 * extra bytes at 432 vs the target's direct register reuse).
 */
void UpdateSplineControl(SplineControlType *spc)
{
    MotionElementType *key0p;
    MotionElementType *key1n;
    s32 dt;
    s32 slope1;
    s32 slope2;

    key0p = spc->key0;
    if (spc->key0->time != 0)
    {
        key0p--;
    }
    key1n = spc->key1;
    if (spc->key1->time < spc->dd0.pad)
    {
        key1n++;
    }
    {
        s16 diff;

        diff = spc->key1->time - spc->key0->time;
        dt = (s8)diff << 8;
    }
    slope1 = (s16)(dt / (spc->key1->time - key0p->time));
    slope2 = (s16)(dt / (key1n->time - spc->key0->time));
    spc->dd0.vx = (s16)(slope1 * (spc->key1->x - key0p->x) >> 8);
    spc->dd0.vy = (s16)(slope1 * (spc->key1->y - key0p->y) >> 8);
    spc->dd0.vz = (s16)(slope1 * (spc->key1->z - key0p->z) >> 8);
    spc->ds1.vx = (s16)(slope2 * (key1n->x - spc->key0->x) >> 8);
    spc->ds1.vy = (s16)(slope2 * (key1n->y - spc->key0->y) >> 8);
    spc->ds1.vz = (s16)(slope2 * (key1n->z - spc->key0->z) >> 8);
}
