#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

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
 * CdaStop (0x8004fb00, 0x80 bytes) — stops CD-audio playback if it's
 * currently active (CdaStatus.flag bit0): re-silences the SPU stream
 * (SsSetSerialAttr/SsSetSerialVol), disarms the vsync-driven CD-audio pump
 * (VSyncCallback(NULL)), stops the drive (cd_control(9,0,0), already
 * matched — a retry wrapper over CdControlB; 9 = CdlPause), flushes it
 * (CdFlush), then resets CdaStatus.CurPos to -2 and clears .status. Same
 * proven TCdaStatus struct as FUN_8004fbf4.c/FUN_8004fc08.c.
 *
 * SsSetSerialAttr/SsSetSerialVol/VSyncCallback/CdFlush are precompiled PsyQ
 * SDK calls (all > 0x80060000, see the cookbook's toolchain-gotchas note) —
 * only this call site's own argument setup is source-shaped, not their
 * bodies. The repeated zero arguments (SsSetSerialAttr(0,0,1),
 * SsSetSerialVol(0,0,0), cd_control(9,0,0)) chain via register-to-register
 * moves rather than fresh `li`s — ordinary cc1 reuse of whichever register
 * already holds 0, not something to hand-engineer.
 */
extern TCdaStatus CdaStatus;
extern void SsSetSerialAttr(u8 a, u8 b, u8 c);
extern void SsSetSerialVol(u8 a, u8 voll, u8 volr);
extern void VSyncCallback(void *func);
extern void cd_control(u8 param_1, u8 *param_2, u8 *param_3);

void CdaStop(void)
{
    if (CdaStatus.flag & 1) {
        SsSetSerialAttr(0, 0, 1);
        SsSetSerialVol(0, 0, 0);
        VSyncCallback(0);
        cd_control(9, 0, 0);
        CdFlush();
        CdaStatus.CurPos = -2;
        CdaStatus.status = 0;
    }
}
