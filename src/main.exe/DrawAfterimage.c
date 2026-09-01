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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * position, red/green/blue ramping from a `(n-i)*127/n` level at the far
 * (older) edge to a constant 0x7f at the near (newer) edge, sorted by the
 * (shared, single) `afi->sz`-derived OTZ clamp. Returns the new `n`.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `AfterimageType` is the shared PSX.SYM layout
 *    (model/vector1/vector2/maxn/n/p1/p2/sz/poly).
 *  - `p1`/`p2` entries and each packet XY slot use GpuScreenPosition: the low
 *    16 bits are x and the high 16 bits y, matching RotTransPers's packed
 *    `sxy` convention. Each transfer is consequently one named word copy
 *    and one target `sw`, never two separate halfword stores.
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
 *    first/third packet-screen seed + the draw loop) — the `disp!=0` arm's own
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
    GpuPolyGT4Packet *poly;
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
            afi->n++;
        }
        for (i = afi->n - 1; i > 0; i--)
        {
            afi->p1[i] = afi->p1[i - 1];
            afi->p2[i] = afi->p2[i - 1];
        }
        GsGetLs(&afi->model->locate, &mat);
        GsSetLsMatrix(&mat);
        RotTransPers(&afi->vector1, (s32 *)afi->p1, 0, 0);
        RotTransPers(&afi->vector2, (s32 *)afi->p2, 0, 0);
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
        afi->n--;
    }

    poly->gpu.vertex[0].screen.word = afi->p1[0].word;
    poly->gpu.vertex[2].screen.word = afi->p2[0].word;

    i = 1;
    while (1)
    {
        if (i >= afi->n)
        {
            break;
        }
        poly->gpu.vertex[1].screen.word = poly->gpu.vertex[0].screen.word;
        tmp1 = afi->p1[i].word;
        poly->gpu.vertex[3].screen.word = poly->gpu.vertex[2].screen.word;
        poly->gpu.vertex[0].screen.word = tmp1;
        tmp2 = afi->p2[i].word;
        poly->packet.r1 = poly->packet.g1 = poly->packet.b1 = tplv;
        poly->packet.r3 = poly->packet.g3 = poly->packet.b3 = tplv;
        poly->gpu.vertex[2].screen.word = tmp2;

        tplv = ((afi->n - i) * 127) / afi->n;
        poly->packet.r0 = poly->packet.g0 = poly->packet.b0 = tplv;
        poly->packet.r2 = poly->packet.g2 = poly->packet.b2 = tplv;

        otz = afi->sz;
        otz = otz >> 2;
        CLAMP_SORT_DEPTH(pri, otz);
        GsSortPoly(&poly->packet, OTablePt, (u16)pri);
        i++;
    }

    return afi->n;
}
