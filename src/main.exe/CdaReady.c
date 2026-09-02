#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int CdaReady(void);
 *     OPAUDIO.C:46, 2 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

s32 CdaReady(void)
{
    return CdaStatus.status & CdlStatRead;
}
