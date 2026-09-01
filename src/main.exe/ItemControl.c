#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ItemControl(void);
 *     MOTION.C:896, 13 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 * END PSX.SYM */

/*
 * ItemControl (0x80027818) — maps the selected inventory item to its use
 * motion, or dispatches the default item handler / rejection sound.
 *
 * STATUS: MATCHING — 0xC4 bytes plus the 12-word jump table. The case order
 * and shared labels are the same item-dispatch shape used by the matched
 * ActNORMAL/ActCHASE/ActSWIM functions in this translation-unit family.
 */

extern Humanoid *Me_MOTION_C;

void ItemControl(void)
{
    switch (SelectedItem)
    {
    case ITEM_SHURIKEN:
        motID = MOT_SYURI;
        break;
    case ITEM_KAGINAWA:
        motID = MOT_KAGI;
        break;
    case ITEM_MAKIBISHI:
        motID = MOT_ITEM;
        break;
    case ITEM_SMOKE:
        motID = MOT_ITEM + 2;
        break;
    case ITEM_FIRE:
        motID = MOT_ITEM + 2;
        break;
    case ITEM_JIRAI:
        motID = MOT_ITEM + 3;
        break;
    case ITEM_NONE:
    case ITEM_KAWARIMI:
        goto item_sound;
    default:
        goto item_default;
    }
    motMODE = MOTION_MOVE_APPLY;
    return;

item_sound:
    SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
    return;

item_default:
    ReqItemDefault(Me_MOTION_C, SelectedItem);
}
