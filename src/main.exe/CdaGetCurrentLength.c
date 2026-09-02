#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int CdaGetCurrentLength(void);
 *     OPAUDIO.C:29, 13 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

extern s32 CdaReady(void);

int CdaGetCurrentLength(void)
{
    if ((CdaStatus.flag & CDA_FLAG_ACTIVE) == 0)
    {
        return 1;
    }
    if (CdaReady() == 0)
    {
        return -1;
    }
    return CdaStatus.CurPos - CdaStatus.StartPos;
}
