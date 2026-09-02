#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"
#include "humanoid.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think2contact(void);
 *     THINK_2.C:21, frame 16 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 * END PSX.SYM */

extern Humanoid *Me_THINK_C;
extern s16 GotoPosition(s32 vx, s32 vz);

s16 Think2contact(void)
{
    if ((Attrib & ATTR_WALL) && (Me_THINK_C->pad_hold == 0))
    {
        s32 hint;

        hint = PAD_HOLD((u32)PADLleft, 8);
        if (Degree > 0)
        {
            hint = PAD_HOLD(PADLright, 8);
        }
        Me_THINK_C->pad_hold = hint;
    }
    return GotoPosition(0, 0);
}
