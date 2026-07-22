#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern short RefrectMove[16][2];
 * END PSX.SYM */

/*
 * FUN_80030644 (0x80030644, 0x128 bytes) — nudges a position's x/z (a VECTOR,
 * only vx/vz are touched, vy is left alone) away from an area-map wall: it
 * samples the area map at `pos` with GetAreaMapVector, bails out if the
 * sample is off-map (`level == INT_MIN`) or reports no wall (`vector == 0`),
 * then re-samples at HALF the requested `amount` and uses that sample's
 * `vector` code instead if it also came back non-wall (falling back to the
 * first sample's code and the halved amount). `RefrectMove[vector]` is a
 * 16-entry `[xAdjust, zAdjust]` sign table: each component's sign (not
 * magnitude — only compared `<1`/`>-1`) decides whether to nudge that axis
 * by `+amount`, `-amount`, or leave it alone. Called once from the
 * still-unmatched CameraDirection.c (`FUN_80030644(&CamLoc, 1000);`) to push
 * a candidate camera position off a wall it's testing against — a camera
 * wall-avoidance helper, not player collision. No candidate name in
 * reference/psxsym-candidates.tsv; not in the demo's PSX.SYM (CameraDirection
 * itself IS in the demo per CAMERA.C:898, so this helper is either a
 * retail-only addition or was inlined/differently-named in the demo build).
 *
 * The two MapVector out-buffers sit 0x18 bytes apart at sp+0x18/sp+0x30,
 * independently confirming the retail type's complete size.
 *
 * Matching notes:
 *  - `xAdj` and `zAdj` are same-width unsigned captures of the signed table
 *    entries. This keeps both target `lhu` loads while the casts at their
 *    signed consumers produce the visible `sll`/`sra` conversions.
 *  - The byte-neutral identical arms make raw `xAdj` a real dependency before
 *    combine, preventing its load-plus-conversion from folding to `lh`.
 *    jump2 removes the condition and duplicate assignment completely.
 *  - The positive-first labelled arms reproduce the target's physical CFG and
 *    its duplicated z conversion at the x-store/skip merge. Separate `newx`
 *    and `newz` carriers preserve `amount` in $s0 while results use $v0.
 *  - In the fallback arm, copying `half` to `amount` before loading v1.vector
 *    gives the target's final independent move/load schedule.
 */
extern s16 RefrectMove[16][2];
extern s32 GetAreaMapVector(void *area, MapVector *mvp, void *pos, s32 wide, s32 mode);

void FUN_80030644(VECTOR *pos, s32 amount)
{
    MapVector v1;
    MapVector v2;
    s32 half;
    u32 vec;
    u16 xAdj;
    s32 signedX;
    u16 zAdj;
    s32 newx;
    s32 newz;

    GetAreaMapVector(GlobalAreaMap, &v1, pos, amount, 0);
    if (v1.level == (s32)0x80000000)
    {
        return;
    }
    if (v1.vector == 0)
    {
        return;
    }
    half = amount / 2;
    GetAreaMapVector(GlobalAreaMap, &v2, pos, half, 0);
    vec = v2.vector;
    if (vec == 0)
    {
        amount = half;
        vec = v1.vector;
    }
    xAdj = RefrectMove[vec][0];
    zAdj = RefrectMove[vec][1];
    if (xAdj != 0)
    {
        signedX = (s16)xAdj;
    }
    else
    {
        signedX = (s16)xAdj;
    }
    if (signedX <= 0)
    {
        goto x_nonpositive;
    }
    newx = pos->vx + amount;
    goto store_x;
x_nonpositive:
    if (signedX >= 0)
    {
        goto skip_x;
    }
    newx = pos->vx - amount;
store_x:
    pos->vx = newx;
skip_x:
    if ((s16)zAdj <= 0)
    {
        goto z_nonpositive;
    }
    newz = pos->vz + amount;
    goto store_z;
z_nonpositive:
    if ((s16)zAdj >= 0)
    {
        return;
    }
    newz = pos->vz - amount;
store_z:
    pos->vz = newz;
}
