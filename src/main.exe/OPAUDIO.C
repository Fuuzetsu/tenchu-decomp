#include "common.h"
#include "main.exe.h"
#include "filesystem.h"
#include <psxsdk/libcd.h>
#include <psxsdk/libsnd.h>

/*
 * Demo OPAUDIO.C defines the two status accessors before cbCheckCD. Retail
 * moved them behind CdaStop and added the volume helpers which follow them.
 */


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

static void cbCheckCD(void)
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int CdaPlayXA(unsigned char *fname, struct CdlLOC *start, struct CdlLOC *end, unsigned char channel, int mode);
 *     OPAUDIO.C:133, 39 src lines, frame 96 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       unsigned char * fname
 *     param $s5       struct CdlLOC * start
 *     param $s4       struct CdlLOC * end
 *     param $s6       unsigned char channel
 *     param stack+16  int mode
 *     stack sp+16     struct CdlFILE cf
 *     stack sp+40     struct CdlFILTER filter
 *     stack sp+48     unsigned char [4] param
 *     stack sp+56     struct CdlLOC loc
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

int CdaPlayXA(u8 *fname, CdlLOC *start, CdlLOC *end, u8 channel, int mode)
{
    CdlFILE cf;
    CdlFILTER filter;
    u8 param[4];

    if ((CdaStatus.flag & CDA_FLAG_ACTIVE) == 0)
    {
        return 0;
    }
    CdaStop();
    if (CdSearchFile(&cf, (char *)fname) == 0)
    {
        return 0;
    }
    CdaStatus.mode = mode;
    mode = CdPosToInt(&cf.pos);
    CdaStatus.StartPos = mode + CDA_FILE_LEAD_IN_SECTORS;
    if (end != 0)
    {
        mode = CdPosToInt(end);
        CdaStatus.EndPos = CdaStatus.StartPos + mode;
    }
    else
    {
        CdaStatus.EndPos = CdaStatus.StartPos +
                           (cf.size >> CDA_DATA_SECTOR_SHIFT);
    }
    if (start != 0)
    {
        mode = CdPosToInt(start);
        CdaStatus.StartPos += mode;
    }
    param[0] = CDA_XA_DRIVE_MODE;
    cd_control(CdlSetmode, param, 0);
    VSync(CDA_DRIVE_SETTLE_FRAMES);
    CdaStatus.command = CDA_COMMAND_READ_XA;
    CdaStatus.CheckCount = 0;
    CdaStatus.status = CDA_STATUS_IDLE;
    filter.file = CDA_XA_FILE_NUMBER;
    filter.chan = channel;
    cd_control(CdlSetfilter, (u8 *)&filter, 0);
    VSyncCallback(cbCheckCD);
    return 1;
}

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

void set_cda_volume_(u8 voll, u8 volr)
{
    CdaStatus.voll = voll;
    CdaStatus.volr = volr;
}

void apply_cda_attr_(s32 arg0)
{
    if (arg0 != 0)
    {
        SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
        SsSetSerialVol(SS_SERIAL_A, CdaStatus.voll, CdaStatus.volr);
    }
    else
    {
        SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
        SsSetSerialVol(SS_SERIAL_A, 0, 0);
    }
}
