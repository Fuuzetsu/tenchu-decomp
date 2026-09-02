#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadFromDEVPC(unsigned char *filename);
 *     FILEIO.C:214, 29 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * filename
 *
 * Globals it touches, as the original declared them:
 *     extern int TotalIO;
 *     extern int ReadMode;
 *     extern unsigned long *MemoryLoadAddress;
 * END PSX.SYM */

extern int PCopen(char *name, int mode, int share);
extern int PClseek(int fd, int offset, TSeekMode whence);
extern int PCread(int fd, void *buf, int size);
extern int PCclose(int fd);
extern void *valloc(u32 size);
extern void AdtMessageBox(char *fmt, ...);
extern char fmt_load_pc[];       /* "%$LOAD(PC)\n%d[%s]" */
extern char msg_load_pc_error[]; /* "LOAD(PC) ERROR\n%d[%s]" */

u_long *LoadFromDEVPC(u8 *filename)
{
    s32 fd;
    s32 size;
    u_long *buff;

    TotalIO++;
    fd = PCopen((char *)filename, 0, 0);
    if (fd != -1)
    {
        size = PClseek(fd, 0, CDSEEK_END);
        if (size > 0)
        {
            if (ReadMode & READ_MODE_TRACE)
            {
                AdtMessageBox(fmt_load_pc, TotalIO, filename);
            }
            PClseek(fd, 0, CDSEEK_SET);
            if (MemoryLoadAddress == 0)
            {
                buff = (u_long *)valloc(size);
            }
            else
            {
                buff = MemoryLoadAddress;
                MemoryLoadAddress = 0;
            }
            PCread(fd, buff, size);
            PCclose(fd);
            return buff;
        }
    }
    AdtMessageBox(msg_load_pc_error, TotalIO, filename);
    return 0;
}
