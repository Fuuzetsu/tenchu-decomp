#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawSprite(struct Sprite3D *sprt);
 *     3DCTRL.C:593, 14 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s1       struct Sprite3D * sprt
 *     stack sp+16     struct MATRIX mat
 *     reg   $a2       long sz
 *     reg   $s1       struct ModelType * objp
 *     reg   $s2       long * xy
 *     reg   $v1       long sz
 *     reg   $s0       short atr
 *     stack sp+48     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — length ours 112 insns vs target 111 (structural,
 * not a register-only tie); this is a THIRD confirmed instance of
 * DrawModel.c's/DrawClip.c's already-PARKED cross-jump/tail-merge residual
 * (see those files' headers for the full writeup): the `reject_check` test
 * (attribute&0x10, gating the 0x4e2 OTZ cutoff) is reached from TWO edges —
 * the `attribute&8==0` skip and the box-check (+-0xf0/+-0xb4) passing
 * cleanly — and the target's cc1 build MERGES both edges into one shared
 * physical test block (each predecessor independently recomputing its own
 * `andi $s0,0x10` into the SAME landing block), while our build's
 * cross-jump pass instead DUPLICATES the whole test+branch into two
 * separate physical copies (one instruction longer). Tried and confirmed
 * BYTE-IDENTICAL either way (matching DrawModel.c's own finding for this
 * exact tie): a shared `reject_check:` label reached by `goto` from both
 * edges, and literal duplication of the test at both call sites with no
 * goto/label at all. Also fixed one genuine bug in an earlier draft along
 * the way (`sz` must be pre-set to -1 before the `attribute&1` early exit,
 * matching DrawModel.c's identical `otz = -1;` placement — omitting it left
 * `sz` live-in on that edge and forced an extra callee-saved register,
 * 80-byte frame instead of the target's 72). Given the identical pattern
 * already resisted extensive attempts on two sibling functions in this same
 * TU (including one bounded `tools/permute.py` run on DrawClip that
 * plateaued without reaching 0), this is parked per the cookbook's
 * sub-C-level early-stop rather than re-running the permuter a third time.
 *
 * DrawSprite (0x80017be8, 0x1bc bytes) — same TU as DrawClip.c/DrawModel.c
 * (3DCTRL.C): the Sprite3D twin of DrawModel's visibility gauntlet (builds
 * the local screen matrix, then runs the IDENTICAL attribute&1/2/4/8/0x10
 * clip test against `sprt->clip`, then a UnitVector RotTransPers against
 * `xy = &sprt->sprite.x`), but instead of calling DrawTMD on success it
 * computes a distance-scaled sprite size (`(sprt->scale>>2)*300 / sz`) and
 * calls GsSortSprite. `xy` is always the fixed non-null address
 * `&sprt->sprite.x` (never a caller-supplied nullable pointer like
 * DrawClip/DrawModel's `xy` parameter) — the `if (xy != 0) goto ret;` guard
 * this TU's shared template carries over is therefore dead at runtime
 * (DrawTMDmode is never actually assigned here), reproduced literally
 * because it's the same source template as DrawModel/DrawClip.
 *
 * Sprite3D isn't item.h's truncated 0x68-byte view here either (this
 * function touches the trailing `sprite.x`/`.scalex`/`.scaley` fields) —
 * declared locally in full, same TU-local-shadow convention as
 * SetupSprite.c/DrawHinoko.c.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `sz` (PSX.SYM's own name, `long`) is ONE variable spanning the whole
 *    function: the first RotTransPers's OTZ, re-tested by attribute&4/
 *    attribute&0x10, then overwritten by the second (UnitVector)
 *    RotTransPers's OTZ, and finally decremented by 5 for the tail's
 *    divisor — not a fresh local per RotTransPers call.
 *  - `objp`/`dim`-style PSX.SYM register-note this TU's template shares with
 *    DrawModel/DrawClip's OWN PSX.SYM entries don't apply to any additional
 *    local here — `sprt` fills that role for the whole function.
 *  - OTablePt is %gp_rel here (tools/gpsyms.py --write; Build.hs
 *    maspsxGpExterns + permute.py GP_EXTERNS both list DrawSprite now) —
 *    unlike DrawModel/DrawClip in the SAME TU, where it's absolute; per-file
 *    gp eligibility, not per-TU.
 *  - The tail's `iv / sz` is a true divide-by-VARIABLE (ASPSX's guarded
 *    div, break 7/break 6) — needs maspsx `--expand-div` for this file
 *    (Build.hs `extra`/permute.py `MASPSX_EXTRA`), same as GetAreaMapLevel/
 *    UpdateTexScroll/GetSpline in other TUs.
 *  - The tail is genuinely TWO independent `return 0;`/`return 1;` sites
 *    (li v0,0 / li v0,1, each falling into the same epilogue), not a
 *    shared `ret` variable reassigned twice — DrawModel/DrawBG's "two
 *    literal early returns" lever, even though Ghidra renders it with one
 *    reused `sVar2`.
 */
typedef struct
{
    GsCOORDINATE2 locate; /* +0x00 */
    SVECTOR rotate;       /* +0x50 */
    s16 id;               /* +0x58 */
    s16 attribute;        /* +0x5a */
    SVECTOR clip;         /* +0x5c */
    long scale;           /* +0x64 */
    GsSPRITE sprite;      /* +0x68 */
} Sprite3D;

extern void GsGetLs(GsCOORDINATE2 *coord, MATRIX *m);
extern void GsSetLsMatrix(MATRIX *m);
extern s32 RotTransPers(SVECTOR *v0, s32 *sxy, void *p, void *flg);
extern void GsSortSprite(GsSPRITE *sp, GsOT *ot, u16 pri);
extern SVECTOR UnitVector;
extern GsOT *OTablePt;
extern s32 DrawTMDmode;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawSprite", DrawSprite);
#else

short DrawSprite(Sprite3D *sprt)
{
    MATRIX mat;
    u16 attr;
    long *xy;
    long sz;
    s32 iv;
    short sVar2;
    short rxy[2];

    GsGetLs(&sprt->locate, &mat);
    GsSetLsMatrix(&mat);
    attr = sprt->attribute;
    xy = (long *)&sprt->sprite.x;
    sz = -1;
    if ((attr & 1) != 0)
        goto ret;
    if ((attr & 2) == 0)
    {
        sz = RotTransPers(&sprt->clip, (s32 *)rxy, 0, 0) >> 2;
        if ((attr & 4) == 0 || sz != 0)
        {
            if ((attr & 8) == 0)
            {
            reject_check:
                if ((attr & 0x10) != 0 && 0x4e2 < sz)
                {
                    sz = -1;
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
        sz = RotTransPers(&UnitVector, xy, 0, 0) >> 2;
        if (sz < 0x4e3)
        {
            if (xy != 0)
            {
                goto ret;
            }
            if (sz < 300)
                DrawTMDmode = 0;
            else
                DrawTMDmode = 0x20;
            goto ret;
        }
    }
    sz = -1;
ret:
    sz = sz - 5;
    if (sz < 1)
    {
        return 0;
    }
    iv = (sprt->scale >> 2) * 300;
    sVar2 = (short)(iv / sz);
    sprt->sprite.scaley = sVar2;
    sprt->sprite.scalex = sVar2;
    GsSortSprite(&sprt->sprite, OTablePt, (u16)sz);
    return 1;
}
#endif
