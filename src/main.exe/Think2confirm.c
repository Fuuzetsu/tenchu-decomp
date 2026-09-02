#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think2confirm(void);
 *     THINK_2.C:14, frame 16 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 * END PSX.SYM */

s16 Think2confirm(void)
{
    return GotoPosition(0, 0) & PAD_TURN_BUTTONS_SIGNED;
}
