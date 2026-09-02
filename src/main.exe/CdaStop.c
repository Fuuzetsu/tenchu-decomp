#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>
#include <psxsdk/libsnd.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CdaStop(void);
 *     OPAUDIO.C:176, 10 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

extern void SsSetSerialAttr(u8 a, u8 b, u8 c);
extern void SsSetSerialVol(u8 a, u8 voll, u8 volr);
extern void VSyncCallback(void (*func)(void));
extern void cd_control(u8 com, u8 *param, u8 *result);

void CdaStop(void)
{
    if (CdaStatus.flag & CDA_FLAG_ACTIVE)
    {
        SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
        SsSetSerialVol(SS_SERIAL_A, 0, 0);
        VSyncCallback(0);
        cd_control(CdlPause, 0, 0);
        CdFlush();
        CdaStatus.CurPos = CDA_STOPPED_POSITION;
        CdaStatus.status = CDA_STATUS_IDLE;
    }
}
