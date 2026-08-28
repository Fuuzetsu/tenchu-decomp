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

/*
 * CdaStop (0x8004fb00, 0x80 bytes) — when CD audio is enabled
 * (CdaStatus.flag bit0), stops any playback: re-silences the SPU stream
 * (SsSetSerialAttr/SsSetSerialVol), disarms the vsync-driven CD-audio pump
 * (VSyncCallback(NULL)), stops the drive (cd_control(9,0,0), already
 * matched — a retry wrapper over CdControlB; 9 = CdlPause), flushes it
 * (CdFlush), then resets CdaStatus.CurPos to -2 and clears .status. Same
 * proven TCdaStatus struct as set_cda_volume_.c/apply_cda_attr_.c.
 *
 * SsSetSerialAttr/SsSetSerialVol/VSyncCallback/CdFlush are precompiled PsyQ
 * SDK calls (all > 0x80060000, see the cookbook's toolchain-gotchas note) —
 * only this call site's own argument setup is source-shaped, not their
 * bodies. The repeated zero arguments (SsSetSerialAttr(0,0,1),
 * SsSetSerialVol(SS_SERIAL_A,0,0), cd_control(9,0,0)) chain via register-to-register
 * moves rather than fresh `li`s — ordinary cc1 reuse of whichever register
 * already holds 0, not something to hand-engineer.
 */
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
        CdaStatus.CurPos = -2;
        CdaStatus.status = 0;
    }
}
