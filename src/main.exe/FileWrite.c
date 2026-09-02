#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int FileWrite(unsigned char *filename, void *data, long size);
 *     FILEIO.C:197, 13 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * filename
 *     param $a1       void * data
 *     param $a2       long size
 * END PSX.SYM */

extern int PCcreat(char *name, int mode);
extern int PCwrite(int fd, void *buf, int size);
extern int PCclose(int fd);

int FileWrite(unsigned char *filename, void *data, long size)
{
    long fd;

    if ((data == 0) | (size < 1))
        return 0;
    fd = PCcreat((char *)filename, 0);
    if (fd == -1)
        return 0;
    PCwrite(fd, data, size);
    PCclose(fd);
    return 1;
}
