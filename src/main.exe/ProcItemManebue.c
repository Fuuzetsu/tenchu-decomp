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

/*
 * ProcItemManebue (0x8004a1d8) — the manebue (lure whistle) item processor.
 * mode 0: silence the alert, set the owner's whistling state, play the sound,
 * arm a 30-frame timer; mode 1: count the timer down, then dispose of the item
 * (call its proc, drop the conflict, clear owner/proc).
 *
 * Matching notes (all verified against the original bytes):
 *  - `param = &item->param.drop` mirrors the original PSX.SYM local
 *    (it lives in $s1 across the calls); indexing off `item` directly doesn't
 *    allocate $s1.
 *  - The mode dispatch is an ordinary `switch` (measured byte-identical
 *    2026-08-31). It supersedes an earlier note claiming a `zero`
 *    variable plus a goto ladder were needed to make cc1 reload `mode`
 *    after the ITEM_MODE_DISPOSE test and keep the case bodies out of
 *    line: the switch produces both by itself, and the helper local is
 *    gone with it.
 *  - EmergencyNotice is a plain small extern here, and this file is
 *    deliberately NOT in Build.hs's maspsxGpExterns list: the original item TU
 *    did not define it (think's TU does), so ASPSX addressed it absolutely
 *    (lui $at) — unlike Think1sleep, where the same symbol is gp-relative.
 */

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
