#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawModel(struct ModelType *objp);
 *     3DCTRL.C:297, 11 src lines, frame 72 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     param $s1       struct ModelType * objp
 *     stack sp+16     struct MATRIX mat
 *     reg   $s1       struct ModelType * objp
 *     reg   $v1       long sz
 *     reg   $s0       short atr
 *     stack sp+48     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — residual ~21 differing asm lines (83 target insns,
 * ours 87 — 4 instructions long). All branch polarities/values verified
 * correct; the residual is LAYOUT (which physical copy of the shared
 * `reject_check` test the two/three incoming edges land on) — see notes.
 *
 * DrawModel (0x80017248) — same TU as DrawClip.c/UpdateCoordinate.c/
 * GetAbsolutePosition.c/DrawOrnament.c (3DCTRL.C): DrawClip's full-bodied
 * twin — builds the model's local screen matrix (GsGetLs+GsSetLsMatrix,
 * DrawOrnament's pair) then runs the exact same visibility/clip gauntlet
 * as DrawClip (attribute&1/2/4/8/0x10, the UnitVector RotTransPers,
 * DrawTMDmode), and on success actually calls DrawTMD; returns 1 drawn / 0
 * not.
 *
 * Matching notes:
 *  - Ghidra's decompile needed 3 gotos for an irreducible CFG — the SOURCE
 *    really is goto-shaped, not just a decompiler artifact: reject_check
 *    (retest attribute&0x10 after either skipping or passing the
 *    +-0xf0/+-0xb4 box check), unit_vector (the shared UnitVector
 *    RotTransPers call, reached both when attribute&2 is set AND as a
 *    fallthrough), ret (the shared tail: draw if the running OTZ/sentinel
 *    isn't -1).
 *  - Ghidra's `iVar3` conflates TWO independent asm registers under one
 *    name: `$v1` (the OTZ from the first RotTransPers, cached and reused
 *    unchanged through the attribute&4 and attribute&0x10 tests, then
 *    reused again as the "-1 = don't draw" sentinel and finally the second
 *    RotTransPers's own OTZ — ONE variable/register the WHOLE function)
 *    and `$v0` (the transient +-0xf0/+-0xb4 box check temp). Splitting
 *    them into `otz`/`iv` reproduces the real caching (DrawClip needed
 *    the identical `otz` split for its own attribute&4/attribute&0x10
 *    tests) — but `otz` must stay ONE variable end to end (an
 *    otz1/otz2/result 3-way split was tried and left the exact same
 *    residual, so the single-variable form is kept for clarity).
 *  - The attribute&0x10 guard tests the OLD `otz` (the first RotTransPers's
 *    cached OTZ) — Ghidra's `(iVar3 = -1, 0x4e2 < lVar2 >> 2)` comma makes
 *    the assignment look like it precedes the read, but the read is of a
 *    SEPARATE value (`lVar2 >> 2`, i.e. the still-live `otz`) — the
 *    branch's delay slot just happens to carry the `otz = -1` store,
 *    executed independent of the test result (dead unless we actually
 *    take the goto). Test-then-assign-then-goto is the natural, correct
 *    C order and reproduces this exactly.
 *  - The tail is TWO literal early returns (`if (otz==-1) return 0;
 *    DrawTMD(...); return 1;`), NOT `return otz != -1;` — the latter
 *    compiles a `nor+sltu` boolean materialize that the target doesn't
 *    have (DrawBG's identical "two early returns, no computed boolean"
 *    lever).
 *  - Residual: `reject_check` is reached from THREE edges (attribute&8==0
 *    skip, the box check passing cleanly, and the box check's own
 *    "iv<0xb5" loop-back). The target lays the box-check body out FIRST
 *    (so its own pass-through falls straight into reject_check with a
 *    freshly-reloaded `attribute&0x10`) and reaches reject_check's TEST
 *    from the attribute&8==0 edge via a forward jump feeding the SAME
 *    test with a value computed in ITS OWN delay slot — i.e. one shared
 *    physical test, two independent `andi` computations feeding it. Our
 *    draft's cross-jump pass instead duplicates the whole reject_check
 *    body once per incoming edge. Tried: `if/else` vs `goto`/label for
 *    the attribute&8 dispatch (byte-identical either way — confirms cc1
 *    doesn't care about that spelling here), otz split into
 *    otz1/otz2/result (also byte-identical — coalesced back by the
 *    allocator regardless). Neither lever reached the shared-vs-duplicated
 *    layout choice; parked per the cookbook's attempt cap.
 */

extern void GsGetLs(GsCOORDINATE2 *coord, MATRIX *m);
extern void GsSetLsMatrix(MATRIX *m);
extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);
extern s32 RotTransPers(SVECTOR *v0, s32 *sxy, void *p, void *flg);
extern SVECTOR UnitVector;
extern GsOT *OTablePt;
extern s32 DrawTMDmode;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawModel", DrawModel);
#else

short DrawModel(ModelType *objp)
{
    MATRIX mat;
    u16 attr;
    long lv;
    s32 otz;
    s32 iv;
    short rxy[2];

    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    attr = objp->attribute;
    otz = -1;
    if ((attr & 1) != 0)
        goto ret;
    if ((attr & 2) == 0)
    {
        lv = RotTransPers(&objp->clip, (s32 *)rxy, 0, 0);
        otz = lv >> 2;
        if ((attr & 4) == 0 || otz != 0)
        {
            if ((attr & 8) == 0)
            {
            reject_check:
                if ((attr & 0x10) != 0 && 0x4e2 < otz)
                {
                    otz = -1;
                    goto ret;
                }
                goto unit_vector;
            }
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
            }
            goto unit_vector;
        }
    }
    else
    {
    unit_vector:
        lv = RotTransPers(&UnitVector, 0, 0, 0);
        otz = lv >> 2;
        if (otz < 0x4e3)
        {
            if (otz < 300)
            {
                DrawTMDmode = 0;
            }
            else
            {
                DrawTMDmode = 0x20;
            }
            goto ret;
        }
    }
    otz = -1;
ret:
    if (otz == -1)
    {
        return 0;
    }
    DrawTMD(&objp->object, OTablePt, 0);
    return 1;
}
#endif
