#include "common.h"
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
 *  - The `zero` variable and the goto ladder reproduce the original dispatch:
 *    cc1 CSEs a plain `mode == 0` chain into one load, but the original reloads
 *    `mode` after the ITEM_MODE_DISPOSE test and keeps the case bodies out
 *    of line.
 *  - EmergencyNotice is a plain small extern here, and this file is
 *    deliberately NOT in Build.hs's maspsxGpExterns list: the original item TU
 *    did not define it (think's TU does), so ASPSX addressed it absolutely
 *    (lui $at) — unlike Think1sleep, where the same symbol is gp-relative.
 */

void ProcItemManebue(TItem *item)
{
    param_drop *param;
    u8 count;
    s32 zero;

    param = &item->param.drop;
    zero = 0;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = 0;
        return;
    }
    if (item->mode == zero)
        goto mode0;
    if (item->mode == 1)
        goto mode1;
    return;
mode0:
    EmergencyNotice = 0;
    item->owner->itmctl = item->type;
    SoundEx(0, 0x43);
    param->count = MANEBUE_DURATION;
    item->mode++;
    return;
mode1:
    count = param->count - 1;
    param->count = count;
    if (count == 0)
    {
        item->owner->itmctl = 0;
        if (item->proc != 0)
        {
            DISPOSE_ITEM(item);
        }
    }
}
