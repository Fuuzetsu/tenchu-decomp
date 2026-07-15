#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawSprite(struct Sprite3D *sprt);
 *     3DCTRL.C:593, 14 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *  - THREE distinct `long` locals carry the OTZ/return story, NOT one `sz`
 *    doing everything (that one-variable form is 1 instruction too SHORT):
 *      * `sz` ($v1) — the OTZ from either RotTransPers, live only within the
 *        clip gauntlet (re-tested by attribute&4/&0x10 and the 0x4e2 cutoff).
 *      * `result` ($v0) — the "-1 = reject / else the accepted OTZ" value
 *        the tail divides. It is assigned ONLY on the branches that reach
 *        `ret:` (each reject does its own `result = -1; goto ret;`; each
 *        accept does `result = sz; goto ret;`), NEVER once at the top. That
 *        keeps its live range short per-path, so cc1 leaves it in caller-
 *        saved $v0 and REMATERIALISES `li $v0,-1` at each reject site (the
 *        target has four such `li v0,-1` + three `move v0,v1`). Hoisting a
 *        single `result = -1;` to the function top instead makes it live
 *        across the whole body, forcing it into a preserved arg reg ($a2)
 *        materialised ONCE — which drops the per-site rematerialisations and
 *        comes out 1 instruction short. This "assign the sentinel only on
 *        each exit edge, never once up front" is the load-bearing lever.
 *      * `pri` ($a2) — `result - 5`, the GsSortSprite priority / divisor.
 *  - The reject sites split into TWO kinds and the split is NOT ours to
 *    choose — reorg (delay-slot fill) decides, but the C structure controls
 *    which it CAN do:
 *      * DIRECT-to-ret rejects — a plain `if (cond) { result = -1; goto ret; }`
 *        whose test is a conditional branch: reorg branches straight to the
 *        `ret:` tail and drops `li $v0,-1` into the branch's OWN delay slot.
 *        The attr&4&&sz==0, iv>=0xf1, and reject_check(attr&0x10&&sz>0x4e2)
 *        rejects are these. The attr&4&&sz==0 one MUST be a standalone
 *        `if ((attr&4)!=0 && sz==0)` (NOT the `else` of the enclosing
 *        `if ((attr&4)==0 || sz!=0)` guard) — as an `else` body it is
 *        textually identical to the shared reject block below and cc1's
 *        cross-jump MERGES it in, costing the direct-branch form (and with
 *        it the target's extra `andi $v0,s0,0x8` recompute that the
 *        clobbered-by-`li v0,-1` delay slot forces — that recompute is the
 *        very instruction the one-variable draft was missing).
 *      * The SHARED reject block (`reject: result = -1; goto ret;`) — reached
 *        by `goto reject` from the attribute&1 early-out and the iv>=0xb5 box
 *        fail, and as the FALL-THROUGH of unit_vector's own `if (sz>=0x4e3)`.
 *        It lands physically inside unit_vector (the `reject:` label sits in
 *        that guard) and compiles to `j ret; li v0,-1`, exactly like the
 *        target's shared block whose two predecessors' branch delay slots are
 *        busy (so they cannot inline the `li` and must route through it).
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
 *  - The two-arm `if (sz < 300) DrawTMDmode = 0; else = 0x20;` inside
 *    unit_vector is spelled NEGATED (`if (sz >= 300) = 0x20; else = 0;`) so
 *    the 0x20 arm sits adjacent to the shared tail, same swap-non-invariance
 *    DrawModel needed.
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

short DrawSprite(Sprite3D *sprt)
{
    MATRIX mat;
    u16 attr;
    long *xy;
    long sz;
    long result;
    long pri;
    s32 iv;
    short sVar2;
    short rxy[2];

    GsGetLs(&sprt->locate, &mat);
    GsSetLsMatrix(&mat);
    attr = sprt->attribute;
    xy = (long *)&sprt->sprite.x;
    if ((attr & 1) != 0)
        goto reject;
    if ((attr & 2) == 0)
    {
        sz = RotTransPers(&sprt->clip, (s32 *)rxy, 0, 0) >> 2;
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
    pri = result - 5;
    if (pri < 1)
    {
        return 0;
    }
    iv = (sprt->scale >> 2) * 300;
    sVar2 = (short)(iv / pri);
    sprt->sprite.scaley = sVar2;
    sprt->sprite.scalex = sVar2;
    GsSortSprite(&sprt->sprite, OTablePt, (u16)pri);
    return 1;
}
