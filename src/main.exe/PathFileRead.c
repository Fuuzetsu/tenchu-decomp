#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * PathFileRead(unsigned char *path, unsigned char *name);
 *     FILEIO.C:185, 8 src lines, frame 280 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       unsigned char * path
 *     param $a1       unsigned char * name
 *     stack sp+16     unsigned char [256] filename
 * END PSX.SYM */

/*
 * PathFileRead (0x80019394, 0x38 bytes) — joins a resource prefix and a
 * resource name into a 256-byte stack buffer and loads that file. Every caller
 * passes a directory prefix like D_800127A4 ("K:\WORK\CDIMAGE\IMAGE\") plus a
 * bare filename, so the "%s%s" is a plain filename concatenation.
 */

extern char D_800976DC[]; /* "%s%s" */

extern int sprintf(char *buf, char *fmt, ...);
extern void *FileRead(char *filename);

void *PathFileRead(char *path, char *name)
{
    char filename[256];

    sprintf(filename, D_800976DC, path, name);
    return FileRead(filename);
}
