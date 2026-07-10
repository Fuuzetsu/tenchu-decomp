#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void RotateVector(struct VECTOR *vec, int rx, int ry, int rz);
 *     EFFECT.C:470, 9 src lines, frame 96 bytes, saved-reg mask 0x801f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       struct VECTOR * vec
 *     param $a1       int rx
 *     param $a2       int ry
 *     param $a3       int rz
 *     stack sp+16     struct MATRIX SMAT
 *     stack sp+48     struct SVECTOR rot
 *     stack sp+56     struct VECTOR vo
 * END PSX.SYM */

/* Matching notes: a direct transcription of Ghidra's decompilation using the
 * PSX.SYM local names (SMAT/rot/vo) — matched with no source-shaping needed. */
extern void *memset(void *s, int c, u32 n);
extern void RotMatrixYXZ(SVECTOR *r, MATRIX *m);
extern VECTOR *ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1);

void RotateVector(VECTOR *vec, int rx, int ry, int rz)
{
    MATRIX SMAT;
    SVECTOR rot;
    VECTOR vo;

    memset(&rot, 0, 8);
    rot.vx = (short)rx;
    rot.vy = (short)ry;
    rot.vz = (short)rz;
    RotMatrixYXZ(&rot, &SMAT);
    ApplyMatrixLV(&SMAT, vec, &vo);
    vec->vx = vo.vx;
    vec->vy = vo.vy;
    vec->vz = vo.vz;
}
