#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ControlAllHumanoid(void);
 *     HUMAN.C:97, 6 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

extern s16 VISIBLE_ENEMIES_;
extern void swap_balma_area_map_(void);

short ControlAllHumanoid(void)
{
    Humanoid *human;
    s16 i;
    s32 result;

    VISIBLE_ENEMIES_ = 0;
    i = 0;
    result = Humans;
    if (result > 0)
        do
        {
            human = HumanGroup[i];
            if ((human->attribute & ATTR_SUSPEND) == 0)
            {
                if (human->type == BALMA)
                {
                    swap_balma_area_map_();
                    ControlHumanoid(human);
                    swap_balma_area_map_();
                }
                else
                {
                    ControlHumanoid(human);
                }
            }
            i++;
        } while (result = i < Humans);
    return result;
}
