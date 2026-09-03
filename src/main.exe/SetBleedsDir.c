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
extern int rand(void);

void SetBleedsDir(VECTOR *pos, SVECTOR *vec, short grange, short n, int time, long col)
{
    int btime;

    do
    {
        if (n <= 0)
        {
            return;
        }
        {
            VECTOR npos = {
                pos->vx + (grange * 2 > 0
                               ? rand() % (grange * 2) - grange
                               : -grange),
                pos->vy + (grange * 2 > 0
                               ? rand() % (grange * 2) - grange
                               : -grange),
                pos->vz + (grange * 2 > 0
                               ? rand() % (grange * 2) - grange
                               : -grange)
            };
            SVECTOR v = {vec->vx, vec->vy, vec->vz};

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
                slot->param.bleed.vec = v;
                param->r = r;
                param->g = col >> 8;
                param->time = time;
                param->b = col;
                param->mode = 0;
                slot->proc = DrawBleed;
            }
        }
    } while (1);
}
