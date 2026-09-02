#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * FileRead(unsigned char *filename);
 *     FILEIO.C:156, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       unsigned char * filename
 *     reg   $s0       unsigned long * ret
 *
 * Globals it touches, as the original declared them:
 *     extern int AccessPower;
 *     extern int ReadMode;
 *     extern int TotalIO;
 * END PSX.SYM */

extern void CdaStop(void);
extern void cbAccess(void);
extern void VSyncCallback(void (*f)(void));
extern void PCinit(void);
extern u_long *LoadFromMEMORY(u8 *filename);
extern u_long *LoadFromDEVPC(u8 *filename);
extern u_long *LoadFromCDROM(u8 *filename);
extern void stop_access_meter_(void);

u_long *FileRead(u8 *filename)
{
    u_long *ret;

    CdaStop();
    if (AccessPower >= 0)
    {
        AccessPower = 0;
        VSyncCallback(cbAccess);
    }
    else
    {
        VSyncCallback(0);
    }
    if (ReadMode == READ_MODE_UNINITIALIZED)
    {
        TotalIO = 0;
        ReadMode = READ_SOURCE_DEVPC;
        PCinit();
    }
    switch (ReadMode & READ_SOURCE_MASK)
    {
    case READ_SOURCE_DEVPC:
        ret = LoadFromDEVPC(filename);
        break;
    case READ_SOURCE_MEMORY:
        ret = LoadFromMEMORY(filename);
        break;
    case READ_SOURCE_CDROM:
        ret = LoadFromCDROM(filename);
        break;
    default:
        ret = 0;
        break;
    }
    stop_access_meter_();
    return ret;
}
