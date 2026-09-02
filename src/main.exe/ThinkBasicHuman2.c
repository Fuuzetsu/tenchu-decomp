#include "common.h"
#include "main.exe.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ThinkBasicHuman2(void);
 *     THINK.C:247, 2 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 * END PSX.SYM */

s16 ThinkBasicHuman2(void)
{
    return GetPad(PAD_CONTROLLER_2);
}
