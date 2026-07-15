#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateOrnament(struct OrnamentType *objp, short ry);
 *     3DCTRL.C:532, 8 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       struct OrnamentType * objp
 *     param $a1       short ry
 *     stack sp+16     struct SVECTOR rotv
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

/*
 * UpdateOrnament (0x800187bc) — like UpdateCoordinate, but OrnamentType has no
 * own `rotate` SVECTOR: build one from UnitVector (the identity SVECTOR, see
 * InsertConflict.c) with vy overridden to `ry`, then recompute the local
 * matrix and clear the GsCOORDINATE2 dirty flag.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `local = UnitVector;` is the align-2 SVECTOR struct-copy idiom
 *    (lwl/lwr + swl/swr pairs, see InsertConflict's `.offset`/`.size`);
 *    the following `local.vy = ry;` is a plain `sh` overwriting the copied
 *    half, executed AFTER the copy (matches asm order).
 *  - OrnamentType (Ghidra: `{ GsCOORDINATE2 locate; GsDOBJ2 object; }`) isn't
 *    shared via item.h yet; declared locally here with just the accessed
 *    `locate` field to keep this function's edits self-contained.
 */
typedef struct
{
    GsCOORDINATE2 locate;        /* 0x00 */
} OrnamentType;

extern MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m);
extern SVECTOR UnitVector;

void UpdateOrnament(OrnamentType *objp, short ry)
{
    SVECTOR local;

    local = UnitVector;
    local.vy = ry;
    RotMatrixYXZ(&local, &objp->locate.coord);
    objp->locate.flg = 0;
}
