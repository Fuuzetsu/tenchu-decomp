#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBlood(struct VECTOR *pos, struct SVECTOR *vect, short n, short time);
 *     EFFECT.C:745, 45 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct VECTOR * pos
 *     param $s5       struct SVECTOR * vect
 *     param $s7       short n
 *     param $fp       short time
 *     reg   $s4       short i
 *     reg   $s6       struct AreaNodeType * hint
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct BloodType * blood
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/* Retail dropped the demo build's SVECTOR * argument; every retail caller
 * and the callee use the three-argument form below. */
extern void DrawBlood(TEffectSlot *ef);

void SetBlood(VECTOR *pos, short n, short time)
{
    int idx;
    TEffectSlot *slot;
    int count;
    BloodType *blood;
    struct AreaNodeType *hint;
    short i;
    int half;
    int half2;

    GetAreaMapLevel(GlobalAreaMap, pos->vx, pos->vy, pos->vz,
                    AREA_LEVEL_DEFAULT);
    hint = FieldArea;
    i = 0;
    do
    {
        if (i >= n)
        {
            return;
        }
        FIND_EFFECT_SLOT(idx, count, slot, found);
    found:
        blood = &slot->param.blood;
        blood->sprite = rand() % N_AIRBORNE_BLOOD_SPRITES;
        blood->scale = rand() % FIXED_ONE + 2 * FIXED_ONE;
        blood->rotate = (rand() % 360) * FIXED_ONE;
        blood->px = pos->vx;
        blood->py = pos->vy;
        blood->pz = pos->vz;
        blood->vx = rand() % 120 - 60;
        blood->vy = rand() % 60 - 120;
        blood->vz = rand() % 120 - 60;
        half = time / 2;
        half2 = time - half;
        if (half2 > 0)
        {
            blood->time = rand() % half2 + half;
        }
        else
        {
            blood->time = half;
        }
        i++;
        blood->brightness = 0x80;
        blood->hint = hint;
        blood->mode = BLOOD_MODE_AIRBORNE;
        slot->proc = DrawBlood;
    } while (1);
}
