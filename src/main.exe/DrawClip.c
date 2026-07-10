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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
 * STATUS: NON_MATCHING — 62 of 296 bytes differ; function IS the right
 * length (74 insns both sides). Pure cross-jump/tail-layout residual, same
 * class as DrawModel's — see notes below. One bounded `tools/permute.py`
 * run (400s, ~9800 iterations) plateaued at score 720 vs baseline 730
 * (never approaching 0); parked per the cookbook's sub-C-level early-stop.
 *
 * DrawClip (0x80018320) — same TU as DrawModel.c/UpdateCoordinate.c/
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
 *  - Every `return -1;` (attribute&1 fails outright, the four inner
 *    rejects) shares the SAME physical epilogue: reorg steals the `-1`
 *    load into whichever branch's own delay slot is empty, exactly as
 *    Ghidra's literal multi-`return -1` rendering shows — write it
 *    literally, no manual goto-merging needed.
 *  - Residual: the target's cross-jump pass merges THREE of the five
 *    `return -1;` sites (attribute&1, the second box-edge reject, and the
 *    OTZ>=0x4e3 fallthrough) into ONE physical 2-insn stub
 *    (`j epilogue; li v0,-1`) reached by jumps, while the OTHER two
 *    (attribute&4, the first box-edge reject, attribute&0x10) get their
 *    OWN direct branch straight to the epilogue with `-1` in ITS delay
 *    slot. Our draft's cross-jump instead sends every site directly to
 *    the epilogue (no intermediate stub) — a length-neutral (SAME 74
 *    insns), pure layout/tail-merge-grouping difference. Tried: literal
 *    `return -1` at every site (Ghidra's own spelling) vs `goto` to a
 *    shared label — byte-identical either way, confirming this is cc1's
 *    own cross-jump heuristic, not a source spelling choice.
 */

extern s32 RotTransPers(SVECTOR *v0, s32 *sxy, void *p, void *flg);
extern SVECTOR UnitVector;
extern s32 DrawTMDmode;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawClip", DrawClip);
#else

long DrawClip(ModelType *objp, long *xy)
{
    u16 attr;
    long lv;
    s32 otz;
    s32 iv;
    short rxy[2];

    attr = objp->attribute;
    if ((attr & 1) == 0)
    {
        if ((attr & 2) == 0)
        {
            lv = RotTransPers(&objp->clip, (s32 *)rxy, 0, 0);
            otz = lv >> 2;
            if ((attr & 4) != 0 && otz == 0)
            {
                return -1;
            }
            if ((attr & 8) != 0)
            {
                iv = rxy[0];
                if (iv < 0)
                {
                    iv = -iv;
                }
                if (0xf0 < iv)
                {
                    return -1;
                }
                iv = rxy[1];
                if (iv < 0)
                {
                    iv = -iv;
                }
                if (0xb4 < iv)
                {
                    return -1;
                }
            }
            if ((attr & 0x10) != 0 && 0x4e2 < otz)
            {
                return -1;
            }
        }
        lv = RotTransPers(&UnitVector, xy, 0, 0);
        otz = lv >> 2;
        if (otz < 0x4e3)
        {
            if (xy != 0)
            {
                return otz;
            }
            if (otz < 300)
            {
                DrawTMDmode = 0;
                return otz;
            }
            DrawTMDmode = 0x20;
            return otz;
        }
    }
    return -1;
}
#endif
