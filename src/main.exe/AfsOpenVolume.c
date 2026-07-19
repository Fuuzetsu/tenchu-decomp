#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/*
 * AfsOpenVolume (0x8005edb4, 0xCC bytes) — opens the AFS archive volume
 * named `path`: AfsInit()s the handle, and if `path` fits an 80-byte stack
 * buffer (< 0x4C chars, leaving room for the ".VOL" suffix), builds
 * "<path>.VOL", cd_open()s it into handle->fpVol. A failed open reports and
 * returns 1; otherwise reads the header (AfsGetHeader) — a nonzero result
 * reports "Header error" and returns 2 — then reads the file table
 * (AfsGetEntry) — a nonzero result reports "Entry error" and returns 3,
 * else 0. No PSX.SYM block (compiled without -g); types are the proven
 * TAFS/FILE layout from AfsInit.c/AfsRead.c/AfsGetHeader.c.
 *
 * Matching notes: Ghidra threads every path through one `iVar2` assigned
 * before the guards and returned once at the end — but that keeps `iVar2`
 * live across every call in the function, promoting it to a THIRD
 * callee-saved register (an extra prologue/epilogue save pair, 8 bytes
 * too long). The asm has no shared-return variable at all: each guard
 * jumps straight to an early `return N;` (the constant materializes
 * right at the jump, in the branch's own delay slot), and the function
 * falls off the end with a final `return 0;` — a plain guard-clause chain,
 * not Ghidra's fold-into-one-variable rendering. Two callee-saved regs
 * only (handle, path).
 */
extern void AfsInit(TAFS *handle);
extern u32 strlen(const char *s);
extern char *strcpy(char *dst, const char *src);
extern char *strcat(char *dst, const char *src);
extern int cd_open(char *name, int mode);
extern int AfsGetHeader(TAFS *handle);
extern int AfsGetEntry(TAFS *handle);
extern void AdtMessageBox(char *fmt, ...);
extern char D_80097E78[]; /* ".VOL" */
extern char D_80014870[]; /* "AfsOpenVolume: %s open err\n" */
extern char D_8001488C[]; /* "AfsOpenVolume: Header error" */
extern char D_800148A8[]; /* "AfsOpenVolume: Entry error" */

int AfsOpenVolume(TAFS *handle, char *path)
{
    char buf[80];

    AfsInit(handle);
    if (strlen(path) >= 0x4C) {
        return 1;
    }
    strcpy(buf, path);
    strcat(buf, D_80097E78);
    handle->fpVol = (FILE *)cd_open(buf, 0);
    if (handle->fpVol == 0) {
        AdtMessageBox(D_80014870, buf);
        return 1;
    }
    if (AfsGetHeader(handle) != 0) {
        AdtMessageBox(D_8001488C);
        return 2;
    }
    if (AfsGetEntry(handle) != 0) {
        AdtMessageBox(D_800148A8);
        return 3;
    }
    return 0;
}
