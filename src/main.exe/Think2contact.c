#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think2contact(void);
 *     THINK_2.C:21, frame 16 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 * END PSX.SYM */

/*
 * Think2contact (0x8002fa54, 0x68 bytes) — a think-handler
 * (installed into a think_settingN slot elsewhere; despite the name it's a
 * per-frame handler like Think1sleep/Think2confirm, not an installer): if
 * currently in the "0x400" attrib state and the AI pad hold is unset,
 * arm it (0x80000008, or 0x20000008 if Degree > 0), then just forward to
 * turn_towards_player_(0, 0). Same TU as Think1sleep.c/Think2confirm.c.  The
 * recovered Attrib object is a signed `short`; this retail site reads its raw
 * flag bits with `lhu`, so it uses the shared `ATTRIB_BITS` view while signed
 * consumers use `Attrib` directly.
 */
extern Humanoid *Me_THINK_C;
extern int turn_towards_player_(int x_diff, int z_diff);

s16 Think2contact(void)
{
    if ((ATTRIB_BITS & 0x400) && (Me_THINK_C->pad_hold == 0))
    {
        s32 hint;

        hint = 0x80000008;
        if (Degree > 0)
        {
            hint = 0x20000008;
        }
        Me_THINK_C->pad_hold = hint;
    }
    return turn_towards_player_(0, 0);
}
