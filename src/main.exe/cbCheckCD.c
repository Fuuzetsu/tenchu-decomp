#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>
#include <psxsdk/libsnd.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void cbCheckCD(void);
 *     OPAUDIO.C:69, 55 src lines, frame 48 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     unsigned char [8] result
 *     reg   $s0       int ret
 *     reg   $v1       int com
 *     stack sp+24     struct CdlLOC loc
 *     stack sp+24     struct CdlLOC loc
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

extern int CdLastCom(void);
extern void SsSetSerialAttr(u8 a, u8 b, u8 c);
extern void SsSetSerialVol(u8 a, u8 voll, u8 volr);
extern void cd_control(u8 com, u8 *param, u8 *result);

void cbCheckCD(void)
{
    TCdaStatus *cs = &CdaStatus;
    u8 result[8];
    CdlLOC loc;
    s32 ret;
    s32 com;

    if (cs->command == CDA_COMMAND_READ_XA)
    {
        CdIntToPos(CdaStatus.StartPos, &loc);
        if ((cs->flag & CDA_FLAG_ACTIVE) &&
            CdControl(CdlReadS, (u8 *)&loc, NULL) == 0)
        {
            return;
        }
        cs->command = CDA_COMMAND_NONE;
        SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
        SsSetSerialVol(SS_SERIAL_A, cs->voll, cs->volr);
        return;
    }

    if (cs->CheckCount++ < CDA_STATUS_CHECK_THRESHOLD)
    {
        return;
    }
    cs->CheckCount = 0;

    ret = CdSync(1, result);
    com = CdLastCom();
    switch (ret)
    {
    case CdlDiskError:
        cs->command = CDA_COMMAND_READ_XA;
        cs->CheckCount = 0;
        cs->status = CDA_STATUS_IDLE;
        return;
    case CdlComplete:
        if (com == CdlPause)
        {
            return;
        }
        if (com == CdlGetlocP)
        {
            cs->CurPos = CdPosToInt((CdlLOC *)&result[5]);
            if ((cs->status & CdlStatRead) &&
                (cs->EndPos < cs->CurPos ||
                 cs->CurPos < CdaStatus.StartPos - CDA_POSITION_GUARD_SECTORS))
            {
                if (cs->mode == CDA_REPEAT)
                {
                    cs->command = CDA_COMMAND_READ_XA;
                    cs->CheckCount = 0;
                    cs->status = CDA_STATUS_IDLE;
                    return;
                }
                SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
                SsSetSerialVol(SS_SERIAL_A, 0, 0);
                cd_control(CdlPause, 0, 0);
                cs->status = CDA_STATUS_IDLE;
                return;
            }
            CdControl(CdlNop, NULL, result);
            CdaStatus.status = result[0];
            CdControlF(CdlGetlocP, NULL);
        }
        else
        {
            CdControlF(CdlGetlocP, NULL);
        }
        break;
    }
}
