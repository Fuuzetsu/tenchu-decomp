#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "filesystem.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadFromCDROM(unsigned char *filename);
 *     FILEIO.C:296, 31 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * filename
 *
 * Globals it touches, as the original declared them:
 *     extern int TotalIO;
 *     extern struct TAFS systemAFS;
 *     extern int ReadMode;
 *     extern unsigned long *MemoryLoadAddress;
 * END PSX.SYM */

extern TAFSFileHandle *AfsOpen(TAFS *handle, char *path);
extern int AfsFileSize(TAFS *handle, TAFSFileHandle *fh);
extern u32 AfsRead(TAFS *volume, TAFSFileHandle *fd, void *buffer, u32 length);
extern int AfsClose(TAFSFileHandle *fd);
extern void *valloc(u32 size);
extern void AdtMessageBox(char *fmt, ...);
extern char fmt_load_cd[];       /* "%$LOAD(CD)\n%d[%s]" */
extern char msg_load_cd_error[]; /* "LOAD(CD) ERROR\n%d[%s]" */

u_long *LoadFromCDROM(u8 *filename)
{
    AdtQuietMode quiet;
    TAFSFileHandle *fd;
    s32 size;
    u_long *buff;

    TotalIO++;
    quiet = AdtQuiet(ADT_NORMAL);
    fd = AfsOpen(&systemAFS, (char *)filename);
    if (fd != 0)
    {
        if (ReadMode & READ_MODE_TRACE)
        {
            AdtMessageBox(fmt_load_cd, TotalIO, filename);
        }
        size = AfsFileSize(&systemAFS, fd);
        if (MemoryLoadAddress == 0)
        {
            buff = (u_long *)valloc(size);
        }
        else
        {
            buff = MemoryLoadAddress;
            MemoryLoadAddress = 0;
        }
        AfsRead(&systemAFS, fd, buff, size);
        AfsClose(fd);
        AdtQuiet(quiet);
        return buff;
    }
    AdtQuiet(quiet);
    AdtMessageBox(msg_load_cd_error, TotalIO, filename);
    return 0;
}
