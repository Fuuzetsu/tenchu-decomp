#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBleedsDir(struct VECTOR *pos, struct SVECTOR *vec, short grange, short n, int time, long col);
 *     EFFECT.C:1115, 12 src lines, frame 88 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s7       struct VECTOR * pos
 *     param $fp       struct SVECTOR * vec
 *     param $a2       short grange
 *     param $s5       short n
 *     param stack+16  int time
 *     param stack+20  long col
 *     reg   $s4       int time
 *     reg   $s6       long col
 *     stack sp+16     struct VECTOR npos
 *     stack sp+32     struct SVECTOR v
 *     reg   $a3       struct VECTOR * pos
 *     reg   $t0       int time
 *     reg   $s6       long col
 *     reg   $v1       struct BleedType * param
 *     reg   $a2       struct tag_EffectSlot * slot
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

extern void DrawBleed(TEffectSlot *ef);
extern void *memset(void *s, int c, u32 n);
extern int rand(void);

void SetBleedsDir(VECTOR *pos, SVECTOR *vec, short grange, short n, int time, long col)
{
    VECTOR npos;
    VECTOR work;
    long b;
    int btime;

    do
    {
        if (n <= 0)
        {
            return;
        }
        memset(&work, 0, sizeof(VECTOR));
        b = pos->vx;
        if (grange * 2 > 0)
        {
            work.vx =
                b + (rand() % (grange * 2) - grange);
        }
        else
        {
            work.vx = b - grange;
        }
        b = pos->vy;
        if (grange * 2 > 0)
        {
            work.vy =
                b + (rand() % (grange * 2) - grange);
        }
        else
        {
            work.vy = b - grange;
        }
        b = pos->vz;
        if (grange * 2 > 0)
        {
            work.vz =
                b + (rand() % (grange * 2) - grange);
        }
        else
        {
            work.vz = b - grange;
        }
        npos = work;
        memset(&((SVECTOR *)&work)[1], 0, sizeof(SVECTOR));
        ((SVECTOR *)&work)[1].vx = vec->vx;
        ((SVECTOR *)&work)[1].vy = vec->vy;
        ((SVECTOR *)&work)[1].vz = vec->vz;
        *(SVECTOR *)&work = ((SVECTOR *)&work)[1];

        if (time - time / 8 > 0)
        {
            btime = rand() % (time - time / 8) + time / 8;
        }
        else
        {
            btime = time / 8;
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
