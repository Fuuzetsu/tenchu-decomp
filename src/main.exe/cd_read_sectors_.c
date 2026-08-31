#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

#define CD_SECTOR_SUBHEADER_SIZE 12
#define CD_SECTOR_PAYLOAD_SIZE 2048

/*
 * MATCHED: cd_read_sectors_ (0x8005f380, 0x1d8 bytes) reads `length`
 * bytes at (`sector`, `byteOffset`) into `buffer`. It seeks with CdlSetmode,
 * primes streaming with CdlReadN, validates each raw sector's position, copies
 * its requested payload slice, and stops the drive when complete. Although all
 * callees are libcd primitives, the address is below the 0x80060000 PsyQ/CRT
 * boundary, so this is game-TU code.
 *
 * The DMA buffer is one 0x810-byte object: a 12-byte raw-sector subheader plus
 * 2048 payload bytes. Ghidra's adjacent CdlLOC[3]/byte-array locals are two
 * views of that object. The separate 8-byte `param` and CdlLOC[2] `loc` slots
 * complete the exact 0x858-byte frame; only param[0] and loc[0] are used.
 *
 * Matching constraints:
 *  - `dst`, `curSector`, `remaining`, and `off` initialize once immediately
 *    after CdIntToPos, before the CdlSetmode and CdlReadN retry loops. Ghidra's
 *    placement inside the latter loop is a decompiler artifact.
 *  - CdReady-not-ready and CdGetSector failure both jump to `full_retry`.
 *    That re-seeks the original `sector`, resets every cursor, and discards
 *    partial progress. The two apparent assembly destinations differ only
 *    because reorg puts the shared `li 0xa0` prefix in one branch's delay slot;
 *    there is no separate mode-6-only retry.
 *  - A position mismatch is different: seek CdlReadN to `curSector` and keep
 *    the current destination/progress state.
 *  - `src = data + off` is intentionally the copy loop's first body statement.
 *    It is invariant and loop.c hoists it, but source placement inside the loop
 *    makes the entry test choose `i = 0` as its target delay-slot filler.
 *    Moving `src` before the loop preserves the instruction itself but not the
 *    schedule.
 */

extern int VSync(int mode);

void cd_read_sectors_(u8 *buffer, s32 sector, s32 byteOffset, s32 length)
{
    u8 sectorBuf[0x810];
    u8 param[8];
    CdlLOC loc[2];
    u8 *raw;
    u8 *data;
    u8 *dst;
    u8 *src;
    s32 curSector;
    s32 remaining;
    s32 off;
    s32 n;
    s32 vsyncArg;
    s32 chunk;
    s32 i;

    raw = sectorBuf;
    data = sectorBuf + CD_SECTOR_SUBHEADER_SIZE;

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
        n = CdGetSector(sectorBuf,
                        (CD_SECTOR_SUBHEADER_SIZE + CD_SECTOR_PAYLOAD_SIZE) /
                            sizeof(u32));
        if (n == 0)
            goto full_retry;

        n = CdPosToInt((CdlLOC *)raw);
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
            if (chunk > CD_SECTOR_PAYLOAD_SIZE)
                chunk = CD_SECTOR_PAYLOAD_SIZE;
            chunk -= off;
            for (i = 0; i < chunk; i++)
            {
                src = data + off;
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
