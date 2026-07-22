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
 *  - `reject_check` is reached from THREE edges (attribute&8==0 skip, the
 *    box check passing cleanly, and the box check's own "iv<0xb5"
 *    loop-back). The target lays the box-check body out FIRST — `if
 *    ((attr & 8) != 0) { <box check> } reject_check: ...` — so its OWN
 *    attribute&8==0 case is the negated guard's forward branch straight
 *    into reject_check's test, and the box check's clean pass-through is
 *    a plain fallthrough into the SAME test, both feeding one physical
 *    `andi s0,0x10` + `slti`. Writing it the other way around (Ghidra's
 *    literal `if (attr&8==0) { reject_check: ... } <box check>`, i.e.
 *    reject_check FIRST) makes cc1 place the box check's `goto
 *    reject_check;` as a BACKWARD jump reusing the SAME physical test —
 *    which is length-correct on its own, but see the next point.
 *  - Length-only bug hiding downstream: Ghidra's decompile reads the box
 *    check's OWN "goto unit_vector;" tail (reached when either box-check
 *    threshold fails, i.e. iv>=0xf1 or iv>=0xb5) as skipping straight to
 *    `unit_vector`. It doesn't — the target sends BOTH box-check failures
 *    to the REJECT tail (`otz = -1; goto ret;`) directly, never touching
 *    unit_vector at all. Decompiling the shared `goto unit_vector;" at
 *    face value costs nothing in count (still compiles) but is
 *    semantically wrong AND — because it changes which two rejects turn
 *    out byte-identical to each other — it changes which cross-jump merge
 *    cc1 finds, which is what actually broke the LENGTH (4 extra
 *    instructions with the wrong tail).
 *  - The two-arm `if (otz < 300) DrawTMDmode = 0; else DrawTMDmode = 0x20;`
 *    inside unit_vector must be written negated — `if (otz >= 300)
 *    DrawTMDmode = 0x20; else DrawTMDmode = 0;` — to match which arm ends
 *    up adjacent to the shared reject/ret tail (worth 4 of the 8
 *    residual bytes on its own; the cookbook's "if(cond)A;else B" A/B
 *    labels are NOT swap-invariant once a shared tail sits past the
 *    if/else — only ONE spelling reaches it for free).
 *  - The other 4 bytes: `otz = -1;` for the "attribute&4 set, otz==0"
 *    reject and for the unit_vector "otz>=0x4e3" reject must NOT be one
 *    shared trailing statement (`... } otz = -1; ret: ...`, reached by
 *    falling out of the whole if/else) — cc1's cross-jump then merges
 *    them into a stub sitting wherever the EARLIER of the two textually
 *    is (right after `reject_check`, before `unit_vector`'s own body),
 *    which is length-correct but shifts branch targets throughout the
 *    tail. The fix: give the merge point its own real label (`reject:`)
 *    placed exactly where the target's copy physically lives — inside
 *    unit_vector's own `if (otz >= 0x4e3)` guard — and have every OTHER
 *    "unconditional otz=-1" site (attribute&4 reject, both box-check
 *    failures) `goto reject;` into it instead of duplicating `otz = -1;
 *    goto ret;` at each site. A named goto TARGET pins cross-jump's
 *    choice of primary copy; an implicit shared-fallthrough or a
 *    site-local duplicate both leave that choice to cc1, and it doesn't
 *    reliably pick the textually-later occurrence. `reject_check`'s OWN
 *    reject (the attribute&0x10 test) stays a literal, separate `otz =
 *    -1; goto ret;` — the target compiles that one as a direct branch
 *    with `otz=-1` in its OWN delay slot, never touching the shared
 *    `reject:` stub at all, so merging it in would be wrong.
 */

extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);
extern GsOT *OTablePt;
extern s32 DrawTMDmode;

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
                }
                goto reject;
            }
        reject_check:
            if ((attr & 0x10) != 0 && 0x4e2 < otz)
            {
                otz = -1;
                goto ret;
            }
            goto unit_vector;
        }
        else
        {
            goto reject;
        }
    }
    else
    {
    unit_vector:
        lv = RotTransPers(&UnitVector, 0, 0, 0);
        otz = lv >> 2;
        if (otz >= 0x4e3)
        {
        reject:
            otz = -1;
            goto ret;
        }
        if (otz >= 300)
        {
            DrawTMDmode = 0x20;
        }
        else
        {
            DrawTMDmode = 0;
        }
    }
ret:
    if (otz == -1)
    {
        return 0;
    }
    DrawTMD(&objp->object, OTablePt, 0);
    return 1;
}
