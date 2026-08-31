#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * PathFileRead(unsigned char *path, unsigned char *name);
 *     FILEIO.C:185, 8 src lines, frame 280 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * path
 *     param $a1       unsigned char * name
 *     stack sp+16     unsigned char [256] filename
 * END PSX.SYM */

/*
 * PathFileRead (0x80019394, 0x38 bytes) — joins a resource prefix and a
 * resource name into a 256-byte stack buffer and loads that file. Every caller
 * passes a directory prefix like path_image_2 ("K:\WORK\CDIMAGE\IMAGE\") plus a
 * bare filename, so the "%s%s" is a plain filename concatenation.
 */

extern char fmt_concat_2[]; /* %s%s */

extern int sprintf(char *buf, char *fmt, ...);

u_long *PathFileRead(u8 *path, u8 *name)
{
    u8 filename[256];

    sprintf((char *)filename, fmt_concat_2, path, name);
    return FileRead(filename);
}
