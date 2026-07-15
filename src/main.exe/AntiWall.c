#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AntiWall(struct GsRVIEW2 *vinfo, struct GsRVIEW2 *target);
 *     CAMERA.C:432, 91 src lines, frame 88 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     param $a0       struct GsRVIEW2 * vinfo
 *     param $s2       struct GsRVIEW2 * target
 *     stack sp+24     struct SVECTOR vsL
 *     stack sp+32     struct SVECTOR vsR
 *     reg   $s3       int lvR
 *     reg   $s0       int rmap
 *     stack sp+40     struct SVECTOR av
 *     stack sp+48     int rx
 *     stack sp+52     int ry
 *     reg   $s5       int sx
 *     reg   $s3       int sy
 *     reg   $s0       int sz
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * AntiWall (0x80030390, 0x2b4 bytes) — tests camera positions offset to
 * either side of the target, then rotates a corrective vector away from a
 * wall and applies it to the target view.
 *
 * Matching notes:
 *  - Retail changed the demo's SVECTOR outputs to VECTORs when it replaced
 *    ApplyMatrixSV with ApplyRotMatrix. Declaring vsL, vsR, and av in this
 *    order reproduces the three 16-byte stack slots at sp+0x18/0x28/0x38;
 *    the original int rx/ry locals follow at sp+0x48/0x4c.
 *  - The literal scratchpad casts are intentional. Repeated accesses keep
 *    0x1f800000 in $s2 while call arguments still materialize in $a0.
 *  - Plain signed `/ 2` expressions produce the target's bias-and-shift
 *    truncation sequence and retain its intermediate numerator registers.
 */

typedef struct
{
    s32 vpx;
    s32 vpy;
    s32 vpz;
    s32 vrx;
    s32 vry;
    s32 vrz;
    s32 rz;
    void *super;
} GsRVIEW2;

extern unsigned long *GlobalAreaMap;
extern SVECTOR D_800979F4;
extern SVECTOR D_800979FC;
extern char D_80097A04[];
extern char D_80097A08[];
extern char D_80097A0C[];

extern void GetVectorRotation(VECTOR *start, VECTOR *end, s32 *rx, s32 *ry);
extern void RotMatrixYXZ(SVECTOR *r, MATRIX *m);
extern void SetRotMatrix(MATRIX *m);
extern void ApplyRotMatrix(SVECTOR *in, VECTOR *out);
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z,
                            int mode);
extern void FntPrint(char *fmt, ...);

void AntiWall(GsRVIEW2 *vinfo, GsRVIEW2 *target)
{
    VECTOR vsL;
    VECTOR vsR;
    int lvR;
    int rmap;
    VECTOR av;
    int rx;
    int ry;
    int sx;
    int sy;
    int sz;

    GetVectorRotation((VECTOR *)target, (VECTOR *)&target->vrx, &rx, &ry);
    ((SVECTOR *)0x1F800000)->vz = 0;
    ((SVECTOR *)0x1F800000)->vx = rx;
    ((SVECTOR *)0x1F800000)->vy = ry;
    RotMatrixYXZ((SVECTOR *)0x1F800000, (MATRIX *)0x1F800040);
    SetRotMatrix((MATRIX *)0x1F800040);

    ApplyRotMatrix(&D_800979F4, &vsL);
    ApplyRotMatrix(&D_800979FC, &vsR);

    lvR = GetAreaMapLevel(GlobalAreaMap,
                          target->vpx + vsR.vx,
                          target->vpy + vsR.vy,
                          target->vpz + vsR.vz, 0);
    rmap = 0;
    if (GetAreaMapLevel(GlobalAreaMap,
                        target->vpx + vsL.vx,
                        target->vpy + vsL.vy,
                        target->vpz + vsL.vz, 0) <= target->vpy)
    {
        rmap = 1;
        FntPrint(D_80097A04);
    }
    if (lvR <= target->vpy)
    {
        rmap |= 2;
        FntPrint(D_80097A08);
    }

    rmap &= 3;
    if (rmap != 0)
    {
        av.vx = 0;
        av.vy = 0;
        av.vz = 0;
        if (rmap == 1)
        {
            ((SVECTOR *)0x1F800000)->vx = 1000;
            ((SVECTOR *)0x1F800000)->vy = 0;
            ((SVECTOR *)0x1F800000)->vz = 0;
            ApplyRotMatrix((SVECTOR *)0x1F800000, &av);
        }
        if (rmap == 2)
        {
            ((SVECTOR *)0x1F800000)->vx = -1000;
            ((SVECTOR *)0x1F800000)->vy = 0;
            ((SVECTOR *)0x1F800000)->vz = 0;
            ApplyRotMatrix((SVECTOR *)0x1F800000, &av);
        }

        sx = av.vx / 2;
        sy = av.vy / 2;
        sz = av.vz / 2;
        if (GetAreaMapLevel(GlobalAreaMap,
                            target->vpx + av.vx + sx,
                            target->vpy + av.vy + sy,
                            target->vpz + av.vz + sz, 0) <= target->vpy)
        {
            av.vx = sx / 2;
            av.vy = sy / 2;
            av.vz = sz / 2;
            FntPrint(D_80097A0C);
        }
        target->vpx += av.vx;
        target->vpy += av.vy;
        target->vpz += av.vz;
    }
}
