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
 * currently in the "0x400" attrib state and the field76_0xb0 hint is unset,
 * arm it (0x80000008, or 0x20000008 if Degree > 0), then just forward to
 * turn_towards_player_(0, 0). Same TU as Think1sleep.c/Think2confirm.c, but
 * THIS file's own compile reads Attrib as u16 (`lhu`), not the main.exe.h
 * `s16` Think1sleep/Think2confirm use — same field, per-file signedness
 * respelling (like Think1sleep's `&0xA000` vs Think2confirm's `&~0x5FFF`).
 * Deliberately does NOT include main.exe.h: its `extern s16 Attrib;` would
 * conflict with the `u16` this file needs, so the handful of externs used
 * here are re-declared locally instead (game_types.h alone gives
 * `Humanoid`; libgs.h is the prerequisite main.exe.h itself needs
 * first).
 */
extern Humanoid *Me_THINK_C;
extern int turn_towards_player_(int x_diff, int z_diff);
extern u16 Attrib;

s16 Think2contact(void)
{
    if ((Attrib & 0x400) && (Me_THINK_C->field76_0xb0 == 0))
    {
        s32 hint;

        hint = 0x80000008;
        if (Degree > 0)
        {
            hint = 0x20000008;
        }
        Me_THINK_C->field76_0xb0 = hint;
    }
    return turn_towards_player_(0, 0);
}
