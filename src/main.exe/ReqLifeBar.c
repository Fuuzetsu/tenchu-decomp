#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqLifeBar(struct Humanoid *h);
 *     INFOVIEW.C:89, 26 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * h
 *     reg   $a1       int i
 *     reg   $a2       int g
 *
 * Globals it touches, as the original declared them:
 *     extern struct INFOVIEW__198fake LifeBar[4];
 * END PSX.SYM */

int ReqLifeBar(Humanoid *h)
{
    int i;
    int g;

    g = -1;
    for (i = 0; i < nLifeBar; i++)
    {
        if (LifeBar[i].count < 1)
        {
            if (g == -1)
            {
                g = i;
            }
        }
        else if (LifeBar[i].target == h)
        {
            g = i;
            break;
        }
    }
    if (g == -1)
    {
        goto ret_zero;
    }
    LifeBar[g].target = h;
    LifeBar[g].style = LIFE_BAR_STYLE_ENEMY;
    LifeBar[g].life = h->life;
    LifeBar[g].max = h->lifemax;
    if (h->life == 0)
    {
        LifeBar[g].count = 100;
    }
    else
    {
        LifeBar[g].count = 300;
    }
    if (LifeBar[g].max < 1)
    {
        LifeBar[g].max = 1;
    }
    return 1;
ret_zero:
    return 0;
}
