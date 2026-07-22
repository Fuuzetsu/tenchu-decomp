#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawAfterimage(struct AfterimageType *afi, short disp);
 *     EFFECT.C:1717, 49 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s2       struct AfterimageType * afi
 *     param $a1       short disp
 *     reg   $s0       struct POLY_GT4 * poly
 *     reg   $s3       short i
 *     reg   $s1       short tplv
 *     stack sp+16     struct MATRIX mat
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * MATCH.
 *
 * DrawAfterimage (0x800387f4, EFFECT.C:1717) — the weapon-trail afterimage
 * effect's per-frame update+draw: when `disp!=0` (actively swinging),
 * grows `n` (clamped to `maxn-1`), shifts the `p1`/`p2` screen-point ring
 * buffers up by one slot, and re-projects the model's two trail SVECTORs
 * (`vector1`/`vector2`) plus a throwaway UnitVector call whose OTZ becomes
 * `afi->sz`, bailing (`return 0`) if that OTZ is exactly 0; when `disp==0`
 * (trail fading out) it instead just shrinks `n` (bailing at 0). Either
 * way it then builds a POLY_GT4 strip: each of the `n-1` inner segments is
 * a Gouraud quad between the trail's point `i` and `i-1`'s screen
 * position, red/green/blue ramping from a `(n-i)*127/n` alpha at the far
 * (older) edge to a constant 0x7f at the near (newer) edge, sorted by the
 * (shared, single) `afi->sz`-derived OTZ clamp. Returns the new `n`.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `AfterimageType` is the shared PSX.SYM layout
 *    (model/vector1/vector2/maxn/n/p1/p2/sz/poly).
 *  - `POLY_GT4` is the canonical PsyQ LIBGPU record (52 bytes, independently
 *    confirmed by PSX.SYM). `p1`/`p2` entries are `long`s that PACK a screen
 *    (x,y) pair (x
 *    in the low 16 bits, y in the high 16 — RotTransPers's own `sxy`
 *    out-param convention); every `poly->xN/yN` pair is therefore written
 *    as ONE 32-bit store through `*(s32 *)&poly->xN`, matching the
 *    target's single `sw` per pair (never two separate `sh`s).
 *  - The ring-buffer shift is a plain `for (i = afi->n - 1; i > 0; i--) {
 *    p1[i]=p1[i-1]; p2[i]=p2[i-1]; }` — `i` must be `short` for the
 *    fused `(i<<16)>>14` sign-extend+scale addressing of the 4-byte `long`
 *    elements (the short-counter idiom that suppresses loop.c's strength
 *    reduction, same family as DrawModelArchive's `mad->object[i]` loop);
 *    cc1's own loop rotation supplies the entry-duplicated `<=0` guard for
 *    free, no hand-written outer `if` needed.
 *  - `GsGetLs(&afi->model->locate, &mat);` — `model->locate` is
 *    `ModelType`'s own offset-0 field, so this is byte-identical to
 *    `GsGetLs((GsCOORDINATE2 *)afi->model, &mat)`; spelled Ghidra's way
 *    for clarity.
 *  - The `disp!=0`/`disp==0` arms both fall into ONE shared tail (the
 *    `poly->x0`/`x2` seed + the draw loop) — the `disp!=0` arm's own
 *    `otz==0` early-`return 0` and the `disp==0` arm's own `n<=0` early-
 *    `return 0` are two INDEPENDENT guard-clause returns, not a shared
 *    variable; the shared tail is reached by plain fallthrough from
 *    either arm's last statement (no explicit `goto` needed if written in
 *    that order).
 *  - Inside the loop, statement order is Ghidra's own dependency order,
 *    NOT simple field-offset order: `poly->x1 = poly->x0;` (old value)
 *    THEN capture `p1[i]` THEN `poly->x3 = poly->x2;` (old value) THEN
 *    `poly->x0 = <captured p1[i]>;` THEN capture `p2[i]` THEN the six
 *    `0x7f` r1/g1/b1/r3/g3/b3 stores THEN `poly->x2 = <captured p2[i]>;`
 *    — `p1[i]`/`p2[i]` must be captured into temps BEFORE the x1/x3 "old
 *    value" copies overwrite their sources, matching the target's own
 *    load-then-store-elsewhere scheduling.
 *  - `tplv` (PSX.SYM's own name) is ONE variable playing TWO roles, not a
 *    literal `0x7f` plus a separate `alfa` — set to `0x7f` ONCE, before
 *    the `disp` dispatch even runs (matching the target's `li s1,127` in
 *    the shared prologue, alongside `poly`'s own setup), used for the
 *    r1/g1/b1/r3/g3/b3 stores, THEN REASSIGNED inside the loop to
 *    `((afi->n-i)*127)/afi->n` for the r0/g0/b0/r2/g2/b2 stores — the
 *    "one C variable = one pseudo for its whole life" rule; a hardcoded
 *    `0x7f` literal plus an independent `alfa` local costs an extra `move`
 *    the target doesn't have (it reuses the SAME register for both
 *    roles).
 *  - The draw loop must be `i=1; while(1) { if(!(i<n)) break; ...; i++; }`
 *    — a plain `for (i=1; i<n; i++)` gets its jump.c-duplicated entry test
 *    CONSTANT-FOLDED against the literal initial value `1` (a cheap
 *    `slti`), while the target's entry test is the SAME general widen-
 *    and-compare the bottom test uses — the `while(1)+break` form still
 *    gets loop.c's hoisting but the entry test is a literal source
 *    statement, not something jump.c can special-case against a known
 *    constant.
 *  - `((afi->n - i) * 127) / afi->n` divides by a runtime value: needs
 *    `--expand-div` (Build.hs maspsxGpExterns' `extra` list + permute.py's
 *    MASPSX_EXTRA).
 *  - The `[0, 0x4e1]` clamp on `afi->sz >> 2` is DrawSpriteXYZ's exact
 *    `goto zero;` shape (the trivial `pri=0` body must be the branch
 *    TARGET, not the fall-through) — and the shift needs `otz = afi->sz;
 *    otz = otz >> 2;` as TWO statements (matching DrawFrame's identical
 *    lever): fused into one expression, cc1 reuses the load's own
 *    register for the shift's destination; split, the load's destination
 *    register matches the target's directly (a 2-byte pure register tie).
 */

short DrawAfterimage(AfterimageType *afi, short disp)
{
    POLY_GT4 *poly;
    MATRIX mat;
    short i;
    s32 otz;
    s16 tplv;
    s32 pri;
    long tmp1, tmp2;

    tplv = 0x7f;
    poly = &afi->poly;

    if (disp != 0)
    {
        if (afi->n < afi->maxn - 1)
        {
            afi->n = afi->n + 1;
        }
        for (i = afi->n - 1; i > 0; i--)
        {
            afi->p1[i] = afi->p1[i - 1];
            afi->p2[i] = afi->p2[i - 1];
        }
        GsGetLs(&afi->model->locate, &mat);
        GsSetLsMatrix(&mat);
        RotTransPers(&afi->vector1, afi->p1, 0, 0);
        RotTransPers(&afi->vector2, afi->p2, 0, 0);
        afi->sz = RotTransPers(&UnitVector, 0, 0, 0);
        if (afi->sz == 0)
        {
            return 0;
        }
    }
    else
    {
        if (afi->n <= 0)
        {
            return 0;
        }
        afi->n = afi->n - 1;
    }

    *(s32 *)&poly->x0 = *afi->p1;
    *(s32 *)&poly->x2 = *afi->p2;

    i = 1;
    while (1)
    {
        if (!(i < afi->n))
        {
            break;
        }
        *(s32 *)&poly->x1 = *(s32 *)&poly->x0;
        tmp1 = afi->p1[i];
        *(s32 *)&poly->x3 = *(s32 *)&poly->x2;
        *(s32 *)&poly->x0 = tmp1;
        tmp2 = afi->p2[i];
        poly->b1 = tplv;
        poly->g1 = tplv;
        poly->r1 = tplv;
        poly->b3 = tplv;
        poly->g3 = tplv;
        poly->r3 = tplv;
        *(s32 *)&poly->x2 = tmp2;

        tplv = ((afi->n - i) * 127) / afi->n;
        poly->b0 = tplv;
        poly->g0 = tplv;
        poly->r0 = tplv;
        poly->b2 = tplv;
        poly->g2 = tplv;
        poly->r2 = tplv;

        otz = afi->sz;
        otz = otz >> 2;
        if (otz < 0)
        {
            goto zero;
        }
        pri = 0x4e1;
        if (otz < 0x4e2)
        {
            pri = otz;
        }
        goto done;
    zero:
        pri = 0;
    done:
        GsSortPoly(poly, OTablePt, (u16)pri);
        i++;
    }

    return afi->n;
}
