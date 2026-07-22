#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long DrawClip(struct ModelType *objp, long *xy);
 *     3DCTRL.C:240, 23 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     param $a0       struct ModelType * objp
 *     param $a1       long * xy
 *     stack sp+16     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

/*
 * DrawClip (0x80018320, 296 bytes) — same TU as DrawModel.c/UpdateCoordinate.c/
 * GetAbsolutePosition.c (3DCTRL.C): a visibility/clip test twin of
 * DrawModel's body without the actual DrawTMD call. `objp->clip` is
 * RotTransPers'd into a stack `rxy` pair first (skipped when attribute&2),
 * gated by attribute&4 (reject a behind-camera OTZ==0) and attribute&8
 * (reject outside a +-0xf0/+-0xb4 screen-space box) and attribute&0x10
 * (reject beyond OTZ 0x4e2); then `xy` (the caller's own out-param, or
 * NULL) is RotTransPers'd against the fixed UnitVector, and DrawTMDmode is
 * set from the resulting OTZ exactly like DrawModel's tail — but DrawClip
 * returns the OTZ instead of drawing.
 *
 * Matching notes:
 *  - `objp->attribute` reads via `lhu` although item.h's proven field is
 *    `s16` — every use here is a small positive bitwise `&`, so combine
 *    folds the sign_extend+mask into a zero_extend load (DrawModel.c's
 *    twin does the same); not a type mismatch.
 *  - This is DrawSprite.c's body transcribed one-to-one (same TU, same
 *    source template) with the sprite tail replaced by `return result;`.
 *    Read DrawSprite.c's notes for the full derivation; the two levers:
 *      * `result` ($v0) is assigned ONLY on the edges that reach `ret:`,
 *        never once at the top, so it stays caller-saved and cc1
 *        rematerialises `li $v0,-1` per reject — the target's four
 *        `li v0,-1` + three `move v0,v1`.
 *      * The reject sites split into DIRECT (own branch to the epilogue,
 *        `li v0,-1` stolen into its delay slot: sz==0, iv>=0xf1,
 *        attr&0x10 && sz>0x4e2) and SHARED (`goto reject`, routed through
 *        the block's own `j ret`: the attr&1 early-out and the iv>=0xb5
 *        box fail). We do not choose the split — reorg does — but the C
 *        controls what it CAN do: the two SHARED sites are exactly the two
 *        whose branch delay slot is ALREADY FULL (`move s1,a1` at
 *        0x80018340, `andi v0,s0,0x10` at 0x800183c0), so they cannot
 *        inline the `li` and must route through the shared block.
 *  - `reject:`'s PHYSICAL POSITION is load-bearing, and it is what the
 *    earlier 62-byte park missed: the label must sit INSIDE the
 *    `if (sz >= 0x4e3)` guard in the unit_vector block, making the shared
 *    block that guard's body (target 0x800183fc `j 0x80018434; li v0,-1`,
 *    reached by jumps from both full-delay-slot sites). The park had tried
 *    "literal `return -1` vs a goto to a shared label" and found them
 *    byte-identical — true, but the variable it never moved was WHERE the
 *    label lives. A trailing `reject:` next to `ret:` takes a free
 *    fallthrough and cross-jump swaps it with the arm that should have
 *    owned it; that swap was the whole 62-byte residual.
 *  - The `if (sz >= 300) DrawTMDmode = 0x20; else = 0;` two-armer is
 *    spelled NEGATED so the `= 0` arm sits adjacent to the tail and takes
 *    the fallthrough (target 0x8001842c) — same swap-non-invariance
 *    DrawModel and DrawSprite needed.
 */

extern s32 DrawTMDmode;

long DrawClip(ModelType *objp, long *xy)
{
    u16 attr;
    long sz;
    long result;
    s32 iv;
    short rxy[2];

    attr = objp->attribute;
    if ((attr & 1) != 0)
        goto reject;
    if ((attr & 2) == 0)
    {
        sz = RotTransPers(&objp->clip, (s32 *)rxy, 0, 0) >> 2;
        if ((attr & 4) != 0 && sz == 0)
        {
            result = -1;
            goto ret;
        }
        if ((attr & 8) != 0)
        {
            iv = rxy[0];
            if (iv < 0)
            {
                iv = -iv;
            }
            if (iv < 0xf1)
            {
                iv = rxy[1];
                if (iv < 0)
                {
                    iv = -iv;
                }
                if (iv < 0xb5)
                {
                    goto reject_check;
                }
                goto reject;
            }
            result = -1;
            goto ret;
        }
    reject_check:
        if ((attr & 0x10) != 0 && 0x4e2 < sz)
        {
            result = -1;
            goto ret;
        }
        goto unit_vector;
    }
    else
    {
    unit_vector:
        sz = RotTransPers(&UnitVector, xy, 0, 0) >> 2;
        if (sz >= 0x4e3)
        {
        reject:
            result = -1;
            goto ret;
        }
        if (xy != 0)
        {
            result = sz;
            goto ret;
        }
        if (sz >= 300)
            DrawTMDmode = 0x20;
        else
            DrawTMDmode = 0;
        result = sz;
    }
ret:
    return result;
}
