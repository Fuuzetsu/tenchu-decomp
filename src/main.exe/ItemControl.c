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

extern Humanoid *Me_MOTION_C;

void ItemControl(void)
{
    SELECT_ITEM_USE_MOTION(item_sound, item_default);
    motMODE = MOTION_MOVE_APPLY;
    return;

item_sound:
    SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
    return;

item_default:
    ReqItemDefault(Me_MOTION_C, SelectedItem);
}
