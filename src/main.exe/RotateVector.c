#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void RotateVector(struct VECTOR *vec, int rx, int ry, int rz);
 *     EFFECT.C:470, 9 src lines, frame 96 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
