#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

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

extern void CdaStop(void);
extern void cd_control(u8 cmd, u8 *param, u8 *result);
extern void VSync(s32 mode);
extern void VSyncCallback(void (*func)(void));
extern void cbCheckCD(void);

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
