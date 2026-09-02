#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawEffect(void);
 *     EFFECT.C:366, 73 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void DrawEffect(void)
{
    TEffectSlot *p;
    s32 i;

    for (i = 0; i < N_EFFECT_SLOTS; i++)
    {
        p = &EffectSlot[i];
        if (p->proc != 0)
        {
            p->proc(p);
        }
    }
}
