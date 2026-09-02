#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void RotateVector(struct VECTOR *vec, int rx, int ry, int rz);
 *     EFFECT.C:470, 9 src lines, frame 96 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * vec
 *     param $a1       int rx
 *     param $a2       int ry
 *     param $a3       int rz
 *     stack sp+16     struct MATRIX SMAT
 *     stack sp+48     struct SVECTOR rot
 *     stack sp+56     struct VECTOR vo
 * END PSX.SYM */

extern void *memset(void *s, int c, u32 n);

void RotateVector(VECTOR *vec, int rx, int ry, int rz)
{
    MATRIX SMAT;
    SVECTOR rot;
    VECTOR vo;

    memset(&rot, 0, sizeof(rot));
    rot.vx = (short)rx;
    rot.vy = (short)ry;
    rot.vz = (short)rz;
    RotMatrixYXZ(&rot, &SMAT);
    ApplyMatrixLV(&SMAT, vec, &vo);
    setVector(vec, vo.vx, vo.vy, vo.vz);
}
