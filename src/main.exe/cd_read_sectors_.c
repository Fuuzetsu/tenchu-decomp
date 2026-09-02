#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern int VSync(int mode);

void cd_read_sectors_(u8 *buffer, s32 sector, s32 byteOffset, s32 length)
{
    CdDataSector sectorBuf;
    u8 param[8];
    CdlLOC loc[2];
    CdDataSector *sector_view;
    u8 *payload;
    u8 *dst;
    u8 *src;
    s32 curSector;
    s32 remaining;
    s32 off;
    s32 n;
    s32 vsyncArg;
    s32 chunk;
    s32 i;

    sector_view = &sectorBuf;
    payload = sectorBuf.payload;

    if (length < 1)
        return;

full_retry:
    param[0] = CdlModeSpeed | CdlModeSize1;
    CdIntToPos(sector, loc);

    dst = buffer;
    remaining = length;
    curSector = sector;
    off = byteOffset;

    while (CdControlB(CdlSetmode, param, 0) == 0)
    {
        VSync(0);
    }

    vsyncArg = 3;
    do
    {
        VSync(vsyncArg);
        n = CdControlB(CdlReadN, (u8 *)loc, 0);
        vsyncArg = 0;
    } while (n == 0);

    while (remaining >= 1)
    {
        n = CdReady(0, 0);
        if (n != 1)
            goto full_retry;
        n = CdGetSector(&sectorBuf, sizeof(sectorBuf) / sizeof(u32));
        if (n == 0)
            goto full_retry;

        n = CdPosToInt(&sector_view->location);
        if (n != curSector)
        {
            CdIntToPos(curSector, loc);
            while (CdControlB(CdlReadN, (u8 *)loc, 0) == 0)
            {
                VSync(0);
            }
        }
        else
        {
            chunk = off + remaining;
            if (chunk > CD_DATA_SECTOR_PAYLOAD_SIZE)
                chunk = CD_DATA_SECTOR_PAYLOAD_SIZE;
            chunk -= off;
            for (i = 0; i < chunk; i++)
            {
                src = payload + off;
                dst[i] = src[i];
            }
            dst += chunk;
            remaining -= chunk;
            off = 0;
            curSector++;
        }
    }

    while (CdControlB(CdlPause, (u8 *)loc, 0) == 0)
    {
        VSync(0);
    }
}
