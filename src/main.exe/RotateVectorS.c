#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void RotateVectorS(struct SVECTOR *vec, int rx, int ry, int rz);
 *     EFFECT.C:480, 9 src lines, frame 88 bytes, saved-reg mask 0x801f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       struct SVECTOR * vec
 *     param $a1       int rx
 *     param $a2       int ry
 *     param $a3       int rz
 *     stack sp+16     struct MATRIX SMAT
 *     stack sp+48     struct SVECTOR rot
 *     stack sp+56     struct SVECTOR vo
 * END PSX.SYM */

/* Matching notes: RotateVector's SVECTOR-output twin — a direct transcription
 * of Ghidra's decompilation using the PSX.SYM local names, matched as-is. */
extern void *memset(void *s, int c, u32 n);
extern void RotMatrixYXZ(SVECTOR *r, MATRIX *m);
extern SVECTOR *ApplyMatrixSV(MATRIX *m, SVECTOR *v0, SVECTOR *v1);

void RotateVectorS(SVECTOR *vec, int rx, int ry, int rz)
{
    MATRIX SMAT;
    SVECTOR rot;
    SVECTOR vo;

    memset(&rot, 0, 8);
    rot.vx = (short)rx;
    rot.vy = (short)ry;
    rot.vz = (short)rz;
    RotMatrixYXZ(&rot, &SMAT);
    ApplyMatrixSV(&SMAT, vec, &vo);
    vec->vx = vo.vx;
    vec->vy = vo.vy;
    vec->vz = vo.vz;
}
