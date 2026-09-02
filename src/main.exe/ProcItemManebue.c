#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemManebue(struct tag_TItem *item);
 *     ITEM.C:1256, 30 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $a0       struct param_drop * param
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern long EmergencyNotice;
 * END PSX.SYM */

void ProcItemManebue(TItem *item)
{
    enum
    {
        MANEBUE_MODE_PLAY = 0,
        MANEBUE_MODE_WAIT = 1
    };
    param_drop *param;

    param = &item->param.drop;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = MANEBUE_MODE_PLAY;
        return;
    }
    switch (item->mode)
    {
    case MANEBUE_MODE_PLAY:
        EmergencyNotice = 0;
        item->owner->active_item = item->type;
        SoundEx(0, SE_LURE_FLUTE);
        param->count = MANEBUE_DURATION;
        item->mode++;
        return;
    case MANEBUE_MODE_WAIT:
        param->count--;
        if (param->count == 0)
        {
            item->owner->active_item = ACTIVE_ITEM_NONE;
            if (item->proc != 0)
            {
                DISPOSE_ITEM(item);
            }
        }
        break;
    }
}
