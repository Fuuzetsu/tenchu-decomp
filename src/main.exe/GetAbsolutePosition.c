#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct VECTOR * GetAbsolutePosition(struct ModelType *model, short x, short y, short z);
 *     3DCTRL.C:221, 15 src lines, frame 80 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       struct ModelType * model
 *     param $a1       short x
 *     param $a2       short y
 *     param $a3       short z
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR offset
 * END PSX.SYM */

/*
 * GetAbsolutePosition (0x800182a8) — world-space position of a local offset
 * (x, y, z) on a model. Pulls the model's local->world matrix (GsGetLw), makes
 * it the current local screen matrix (GsSetLsMatrix), then RotTrans's the local
 * offset into the original static VECTOR `vector`, whose address is returned.
 * &model->locate is model itself (locate is GsCOORDINATE2 at offset 0).
 */
extern VECTOR vector;

VECTOR *GetAbsolutePosition(ModelType *model, short x, short y, short z)
{
    MATRIX mat;
    SVECTOR offset;

    GsGetLw(&model->locate, &mat);
    GsSetLsMatrix(&mat);
    offset.vx = x;
    offset.vy = y;
    offset.vz = z;
    RotTrans(&offset, &vector, (long *)0);
    return &vector;
}
