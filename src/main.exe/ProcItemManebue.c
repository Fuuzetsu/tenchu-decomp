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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s0       struct tag_TItem * item
 *     reg   $a0       struct param_drop * param
 *     reg   $s0       struct tag_TItem * item
 * END PSX.SYM */

/*
 * ProcItemManebue (0x8004a1d8) — the manebue (lure whistle) item processor.
 * mode 0: silence the alert, set the owner's whistling state, play the sound,
 * arm a 30-frame timer; mode 1: count the timer down, then dispose of the item
 * (call its proc, drop the conflict, clear owner/proc).
 *
 * Matching notes (all verified against the original bytes):
 *  - `param = (param_drop *)item->param` mirrors the original PSX.SYM local
 *    (it lives in $s1 across the calls); indexing off `item` directly doesn't
 *    allocate $s1.
 *  - The `zero` variable and the goto ladder reproduce the original dispatch:
 *    cc1 CSEs a plain `mode == 0` chain into one load, but the original reloads
 *    `mode` after the 0xff test and keeps the case bodies out of line.
 *  - FRAMES_UNTIL_END_OF_ALERT is a plain small extern here, and this file is
 *    deliberately NOT in Build.hs's maspsxGpExterns list: the original item TU
 *    did not define it (think's TU does), so ASPSX addressed it absolutely
 *    (lui $at) — unlike Think1sleep, where the same symbol is gp-relative.
 */
extern char D_800121CC[];

void ProcItemManebue(tag_TItem *item)
{
    param_drop *param;
    u8 cVar1;
    s32 zero;

    param = (param_drop *)item->param;
    zero = 0;
    if (item->mode == 0xff)
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
    FRAMES_UNTIL_END_OF_ALERT = 0;
    item->owner->active_item = item->type;
    SoundEx((VECTOR *)0x0, 0x43);
    param->count = 0x1e;
    item->mode = item->mode + 1;
    return;
mode1:
    cVar1 = param->count - 1;
    param->count = cVar1;
    if (cVar1 == 0)
    {
        item->owner->active_item = 0;
        if (item->proc != 0)
        {
            item->mode = 0xff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
    }
}
