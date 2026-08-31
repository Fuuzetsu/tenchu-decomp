#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "tmdfast.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawModel(struct ModelType *objp);
 *     3DCTRL.C:297, 11 src lines, frame 72 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * DrawModel (0x80017248) — same TU as DrawClip.c/UpdateCoordinate.c/
 * GetAbsolutePosition.c/DrawOrnament.c (3DCTRL.C): DrawClip's full-bodied
 * twin — builds the model's local screen matrix (GsGetLs+GsSetLsMatrix,
 * DrawOrnament's pair) then runs the exact same visibility/clip gauntlet
 * as DrawClip (attribute&1/2/4/8/0x10, the UnitVector RotTransPers,
 * DrawTMDmode), and on success actually calls DrawTMD; returns 1 drawn / 0
 * not.
 *
 * Matching notes:
 *  - Two of the three joins Ghidra needed are ordinary structure: the
 *    far-depth retest is reached by falling out of the screen-cull block
 *    (an if/else, no label), and the UnitVector projection is the
 *    fallthrough after it (2026-08-31, byte-identical - the same recipe
 *    as DrawModelArchive/DrawSprite/DrawClip). `ret` stays: it is the
 *    shared tail that draws when the running OTZ/sentinel isn't -1.
 *  - Ghidra's `iVar3` conflates TWO independent asm registers under one
 *    name: `$v1` (the OTZ from the first RotTransPers, cached and reused
 *    unchanged through the attribute&4 and attribute&0x10 tests, then
 *    reused again as the "-1 = don't draw" sentinel and finally the second
 *    RotTransPers's own OTZ — ONE variable/register the WHOLE function)
 *    and `$v0` (the transient +-0xf0/+-0xb4 box check temp). Splitting
 *    them into `sz`/`iv` reproduces the real caching (DrawClip needed
 *    the identical depth split for its own attribute&4/attribute&0x10
 *    tests) — but PSX.SYM's exact `sz` must stay ONE variable end to end (a
 *    sz1/sz2/result 3-way split was tried and left the exact same
 *    residual, so the single-variable form is kept for clarity).
 *  - The attribute&0x10 guard tests the OLD `sz` (the first RotTransPers's
 *    cached OTZ) — Ghidra's `(iVar3 = -1, 0x4e2 < lVar2 >> 2)` comma makes
 *    the assignment look like it precedes the read, but the read is of a
 *    SEPARATE value (`lVar2 >> 2`, i.e. the still-live `sz`) — the
 *    branch's delay slot just happens to carry the `sz = -1` store,
 *    executed independent of the test result (dead unless we actually
 *    take the goto). Test-then-assign-then-goto is the natural, correct
 *    C order and reproduces this exactly.
 *  - The tail is TWO literal early returns (`if (sz==-1) return 0;
 *    DrawTMD(...); return 1;`), NOT `return sz != -1;` — the latter
 *    compiles a `nor+sltu` boolean materialize that the target doesn't
 *    have (DrawBG's identical "two early returns, no computed boolean"
 *    lever).
 *  - The far-depth test is reached from THREE edges (attribute&8 skip,
 *    the box check passing cleanly, and its iv<0xb5 pass). The target
 *    lays the box-check body out FIRST, so the attribute&8==0 case is a
 *    forward branch into that test and the clean pass-through falls into
 *    the SAME test, both feeding one physical `andi s0,0x10` + `slti`.
 *    Writing it the other way around (the far-depth test first) makes
 *    cc1 reuse the same physical test through a BACKWARD jump - length
 *    correct on its own, but see the next point. The block's TEXTUAL
 *    position is load-bearing either way: a flatter spelling that moves
 *    the direct reject next to the X test costs bytes through reorg
 *    (measured on the sibling DrawSprite).
 *  - Length-only bug hiding downstream: Ghidra's decompile reads the box
 *    check's OWN "skip to the projection" tail (reached when either box-check
 *    threshold fails, i.e. iv>=0xf1 or iv>=0xb5) as skipping straight to
 *    the UnitVector projection. It doesn't — the target sends BOTH box-check failures
 *    to the REJECT tail (`sz = -1; goto ret;`) directly, never touching
 *    the UnitVector projection at all. Decompiling the shared `goto` to the projection at
 *    face value costs nothing in count (still compiles) but is
 *    semantically wrong AND — because it changes which two rejects turn
 *    out byte-identical to each other — it changes which cross-jump merge
 *    cc1 finds, which is what actually broke the LENGTH (4 extra
 *    instructions with the wrong tail).
 *  - The two-arm `if (sz < 300) DrawTMDmode = TMD_BANK_PLAIN; else DrawTMDmode = TMD_BANK_FOG;`
 *    inside the UnitVector projection must be written negated — `if (sz >= FOG_DEPTH)
 *    DrawTMDmode = TMD_BANK_FOG; else DrawTMDmode = TMD_BANK_PLAIN;` — to match which arm ends
 *    up adjacent to the shared reject/ret tail (worth 4 of the 8
 *    residual bytes on its own; the cookbook's "if(cond)A;else B" A/B
 *    labels are NOT swap-invariant once a shared tail sits past the
 *    if/else — only ONE spelling reaches it for free).
 *  - The other 4 bytes: `sz = -1;` for the "attribute&4 set, sz==0"
 *    reject and for the the UnitVector projection "sz>=0x4e3" reject must NOT be one
 *    shared trailing statement (`... } sz = -1; ret: ...`, reached by
 *    falling out of the whole if/else) — cc1's cross-jump then merges
 *    them into a stub sitting wherever the EARLIER of the two textually
 *    is (right after the far-depth test, before the UnitVector projection's own body),
 *    which is length-correct but shifts branch targets throughout the
 *    tail. The fix: give the merge point its own real label (`reject:`)
 *    placed exactly where the target's copy physically lives — inside
 *    the UnitVector projection's own `if (sz > DEPTH_LIMIT)` guard — and have every OTHER
 *    "unconditional sz=-1" site (attribute&4 reject, both box-check
 *    failures) `goto reject;` into it instead of duplicating `sz = -1;
 *    goto ret;` at each site. A named goto TARGET pins cross-jump's
 *    choice of primary copy; an implicit shared-fallthrough or a
 *    site-local duplicate both leave that choice to cc1, and it doesn't
 *    reliably pick the textually-later occurrence. the far-depth test's OWN
 *    reject (the attribute&0x10 test) stays a literal, separate `sz =
 *    -1; goto ret;` — the target compiles that one as a direct branch
 *    with `sz=-1` in its OWN delay slot, never touching the shared
 *    `reject:` stub at all, so merging it in would be wrong.
 */

extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);

short DrawModel(ModelType *objp)
{
    MATRIX mat;
    short atr;
    long sz;
    s32 iv;
    short rxy[2];

    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    atr = objp->attribute;
    sz = -1;
    if ((atr & MODEL_ATTR_HIDDEN) != 0)
        goto ret;
    if ((atr & MODEL_ATTR_NOCULL) == 0)
    {
        sz = RotTransPers(&objp->clip, (s32 *)rxy, 0, 0) >> 2;
        
        if ((atr & MODEL_ATTR_CULL_BEHIND) == 0 || sz != 0)
        {
            if ((atr & MODEL_ATTR_CULL_SCREEN) != 0)
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
                    if (iv >= 0xb5)
                    {
                        goto reject;
                    }
                }
                else
                {
                    goto reject;
                }
            }
            if ((atr & MODEL_ATTR_CULL_FAR) != 0 && sz > DEPTH_LIMIT)
            {
                sz = -1;
                goto ret;
            }
        }
        else
        {
            goto reject;
        }
    }
    sz = RotTransPers(&UnitVector, 0, 0, 0) >> 2;
    if (sz > DEPTH_LIMIT)
    {
    reject:
        sz = -1;
        goto ret;
    }
    if (sz >= FOG_DEPTH)
    {
        DrawTMDmode = TMD_BANK_FOG;
    }
    else
    {
        DrawTMDmode = TMD_BANK_PLAIN;
    }
ret:
    if (sz == -1)
    {
        return 0;
    }
    DrawTMD(&objp->object, OTablePt, 0);
    return 1;
}
