#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBleeds(struct VECTOR *pos, short grange, short srange, short n, int time, long col);
 *     EFFECT.C:963, 11 src lines, frame 88 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param stack+0   struct VECTOR * pos
 *     param $a1       short grange
 *     param $s5       short srange
 *     param $s6       short n
 *     param stack+16  int time
 *     param stack+20  long col
 *     reg   $fp       int time
 *     reg   $s7       long col
 *     stack sp+16     struct VECTOR npos
 *     stack sp+32     struct SVECTOR v
 *     reg   $a3       struct VECTOR * pos
 *     reg   $t0       int time
 *     reg   $s7       long col
 *     reg   $v1       struct BleedType * param
 *     reg   $a2       struct tag_EffectSlot * slot
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

extern void DrawBleed(TEffectSlot *ef);
extern void *memset(void *s, int c, u32 n);

void SetBleeds(VECTOR *pos, short grange, short srange, short n, int time, long col)
{
    VECTOR npos;
    VECTOR work;
    int grange2;
    long b;
    int g;
    int z2, z3;
    int half;
    int rem;
    int btime;

    g = grange;
    grange2 = g * 2;
    z2 = 0;
    z3 = 0;
    do
    {
        if (n <= 0)
        {
            return;
        }
        memset(&work, 0, sizeof(VECTOR));
        b = pos->vx;
        if (grange2 > 0)
        {
            work.vx = b + (rand() % grange2 - g);
        }
        else
        {
            work.vx = b - g;
        }
        b = pos->vy;
        if (grange2 > 0)
        {
            work.vy = b + (rand() % grange2 - g);
        }
        else
        {
            work.vy = b - g;
        }
        b = pos->vz;
        if (grange2 > 0)
        {
            work.vz = b + (rand() % grange2 - g);
        }
        else
        {
            work.vz = b - g;
        }
        npos = work;
        memset(&((SVECTOR *)&work)[1], 0, sizeof(SVECTOR));
        if (srange * 2 > 0)
        {
            ((SVECTOR *)&work)[1].vx = rand() % (srange * 2) - srange;
        }
        else
        {
            ((SVECTOR *)&work)[1].vx = -srange;
        }
        if (srange * 2 > 0)
        {
            ((SVECTOR *)&work)[1].vy = rand() % (srange * 2) - srange;
        }
        else
        {
            ((SVECTOR *)&work)[1].vy = z2 - srange;
        }
        if (srange * 2 > 0)
        {
            ((SVECTOR *)&work)[1].vz = rand() % (srange * 2) - srange;
        }
        else
        {
            ((SVECTOR *)&work)[1].vz = z3 - srange;
        }
        *(SVECTOR *)&work = ((SVECTOR *)&work)[1];
        half = time / 2;
        rem = time - half;
        if (rem > 0)
        {
            btime = rand() % rem + half;
        }
        else
        {
            btime = half;
        }
        {
            VECTOR *pos = &npos;
            int time = btime;
            int idx;
            TEffectSlot *slot;
            int count;
            BleedType *param;
            u8 r;

            FIND_EFFECT_SLOT(idx, count, slot, found);
        found:
            n--;
            param = &slot->param.bleed;
            r = col >> 16;
            slot->param.bleed.pos = *pos;
            slot->param.bleed.vec = *(SVECTOR *)&work;
            param->r = r;
            param->g = col >> 8;
            param->time = time;
            param->b = col;
            param->mode = 0;
            slot->proc = DrawBleed;
        }
    } while (1);
}
