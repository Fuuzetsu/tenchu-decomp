#include "common.h"
#include "main.exe.h"

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
typedef struct
{
    s32 StartPos;   /* 0x00 */
    s32 CurPos;     /* 0x04 */
    s32 EndPos;     /* 0x08 */
    s16 mode;       /* 0x0C */
    s16 CheckCount; /* 0x0E */
    u8 status;      /* 0x10 */
    u8 voll;        /* 0x11 */
    u8 volr;        /* 0x12 */
    u8 flag;        /* 0x13 */
    u8 field9_0x14; /* 0x14 */
} TCdaStatus;

extern TCdaStatus CdaStatus;
extern void SsSetSerialAttr(u8 a, u8 b, u8 c);
extern void SsSetSerialVol(u8 a, u8 voll, u8 volr);
extern void VSyncCallback(void *func);
extern void cd_control(u8 param_1, u8 *param_2, u8 *param_3);
extern void CdFlush(void);

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
