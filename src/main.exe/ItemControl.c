#include "common.h"
#include "main.exe.h"
#include "item.h"

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
extern s16 motID;
extern s16 motMODE;
extern s16 SelectedItem;

extern s16 SoundEx(VECTOR *locate, s16 id);

void ItemControl(void)
{
    switch ((short)(SelectedItem + 1))
    {
    case 2:
        motID = 0xe00;
        break;
    case 1:
        motID = 0x400;
        break;
    case 3:
        motID = 0xf00;
        break;
    case 6:
        motID = 0xf02;
        break;
    case 5:
        motID = 0xf02;
        break;
    case 7:
        motID = 0xf03;
        break;
    case 0:
    case 11:
        goto item_sound;
    default:
        goto item_default;
    }
    motMODE = 1;
    return;

item_sound:
    SoundEx(Me_MOTION_C->locate, 0xc);
    return;

item_default:
    ReqItemDefault(Me_MOTION_C, SelectedItem);
}
