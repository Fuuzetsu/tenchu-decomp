#include "common.h"
#include "main.exe.h"
#include "effect.h"

void reset_effects_(void)
{
    s32 i;

    for (i = 0; i < N_EFFECT_SLOTS; i++)
    {
        if (EffectSlot[i].proc != UpdateTexScroll)
        {
            EffectSlot[i].proc = 0;
        }
    }
}
