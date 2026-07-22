#include "common.h"
#include "main.exe.h"
#include "item.h"
#include <psxsdk/libgpu.h>
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct AfterimageType * SetupAfterimage(struct ModelType *model, short len);
 *     EFFECT.C:1667, 35 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s0       struct ModelType * model
 *     param $a1       short len
 *     reg   $s2       struct GsIMAGE * image
 *     reg   $s1       struct AfterimageType * afi
 *     reg   $a1       short px
 *     reg   $a2       short py
 *     reg   $v1       short ph
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

/*
 * SetupAfterimage (0x80038e9c, 0xfc bytes) — allocate an afterimage-trail
 * effect: two SVECTOR fields seeded from UnitVector (the identity SVECTOR,
 * same convention as UpdateOrnament/InsertConflict), a ring buffer of `len`
 * trail points (`p1`/`p2`, each `len * 4` bytes — matched twin
 * DisposeAfterimage.c frees these same two fields), and a POLY_GT4 sprite
 * initialized via SetupImageToPolyGT4/SetSemiTrans. The shared layout is
 * PSX.SYM's original `AfterimageType`, also independently confirmed by
 * Ghidra (reference/ghidra_types.h:4885). `len` is `short`, as recorded by
 * PSX.SYM and the shared API: `size = len * 4` compiles to the
 * sign-extend+scale-by-4 shift pair from the short parameter directly.
 *
 * Matching notes (docs/matching-cookbook.md): store order follows Ghidra's
 * literal rendering exactly, including `n` (offset 0x16) stored textually
 * BEFORE `maxn` (offset 0x14, the LOWER address) — reorg schedules the
 * `maxn` store into the following `valloc` call's delay slot, but the
 * source statement order is still n-then-maxn. `p1`/`p2` go through one
 * reused temp across both valloc calls (same cached-pointer-temp
 * convention as LoadTIMpackAndFree/GetVectorRotation's out-param). Each
 * `pAVar4->vector{1,2} = UnitVector;` is an independent align-2 SVECTOR
 * struct copy (own `lui/addiu` reload of UnitVector's address — the two
 * copies do NOT share a cached base register, matching the "TU sibling"
 * caveat that a repeated global reference isn't automatically cached).
 */
extern void *valloc(u32 size);
extern GsIMAGE *D_80097F3C;
extern void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply, s32 x, s32 y);

AfterimageType *SetupAfterimage(ModelType *model, short len)
{
    GsIMAGE *image;
    AfterimageType *pAVar4;
    long *plVar5;
    s32 size;

    image = D_80097F3C;
    pAVar4 = (AfterimageType *)valloc(0x58);
    size = len * 4;
    pAVar4->model = model;
    pAVar4->vector1 = UnitVector;
    pAVar4->vector2 = UnitVector;
    pAVar4->n = 0;
    pAVar4->maxn = len;
    plVar5 = (long *)valloc(size);
    pAVar4->p1 = plVar5;
    plVar5 = (long *)valloc(size);
    pAVar4->p2 = plVar5;
    pAVar4->sz = 0;
    SetupImageToPolyGT4(image, &pAVar4->poly, 0, 0);
    SetSemiTrans(&pAVar4->poly, 1);
    return pAVar4;
}
