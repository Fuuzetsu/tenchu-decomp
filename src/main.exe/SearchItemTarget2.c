#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static struct Humanoid * SearchItemTarget2(struct Humanoid *owner, struct SVECTOR *rot, struct VECTOR *start, struct VECTOR *target);
 *     ITEM.C:352, 62 src lines, frame 136 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $fp       struct Humanoid * owner
 *     param $s1       struct SVECTOR * rot
 *     param $s5       struct VECTOR * start
 *     param $s7       struct VECTOR * target
 *     reg   $s1       int i
 *     reg   $s4       int dist
 *     reg   $s6       struct Humanoid * ret
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR rrot
 *     stack sp+56     struct SVECTOR v
 *     reg   $s0       struct VECTOR * gpv
 *     reg   $s0       struct Humanoid * human
 *     stack sp+64     struct VECTOR tv
 *     stack sp+80     struct VECTOR lv
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 * END PSX.SYM */

extern VECTOR vec_z_n17000;

extern long abs(long x);

Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot, VECTOR *start, VECTOR *target)
{
    int i;
    int dist;
    Humanoid *ret;
    Humanoid *human;
    MATRIX mat;
    SVECTOR rrot;
    VECTOR tv;
    VECTOR lv;
    int cond;
    int z;

    ret = 0;
    tv = vec_z_n17000;
    RotateVector(&tv, rot->vx, rot->vy, rot->vz);
    tv.vx += start->vx;
    tv.vy += start->vy;
    tv.vz += start->vz;
    trace_ground_(start, &tv, &lv, 0);
    setVector(target, lv.vx, lv.vy, lv.vz);
    dist = GetVectorDistance(&lv, start);
    rrot.vx = -rot->vx;
    rrot.vy = -rot->vy;
    rrot.vz = -rot->vz;
    RotMatrix(&rrot, &mat);

    i = 0;
    while (1)
    {
        if (i >= Humans)
            break;
        human = HumanGroup[i];
        tv = *GetAbsolutePosition(*human->model->object, 0, 0, 0);
        lv = tv;
        if (human->life > 0 && human != owner)
        {
            lv.vx -= start->vx;
            lv.vy -= start->vy;
            lv.vz -= start->vz;
            ApplyMatrixLV(&mat, &lv, &lv);
            lv.vz = -lv.vz;
            if (lv.vz > 100)
            {
                cond = 0;
                if (abs(lv.vx) < 500)
                {
                    cond = abs(lv.vy) < 1000;
                }
                if (cond && (z = lv.vz, z < dist))
                {
                    dist = z;
                    *target = tv;
                    ret = human;
                }
            }
        }
        i++;
    }
    return ret;
}
