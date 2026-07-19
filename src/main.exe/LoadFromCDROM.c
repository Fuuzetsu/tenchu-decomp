#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadFromCDROM(unsigned char *filename);
 *     FILEIO.C:296, 31 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       unsigned char * filename
 *
 * Globals it touches, as the original declared them:
 *     extern int TotalIO;
 *     extern struct TAFS systemAFS;
 *     extern int ReadMode;
 *     extern unsigned long *MemoryLoadAddress;
 * END PSX.SYM */

/*
 * LoadFromCDROM (0x80019650, 0x118 bytes) — loads a file from the AFS
 * volume on disc: bumps TotalIO, mutes the CD-audio pump (AdtQuiet),
 * opens `filename` via AfsOpen on the global `systemAFS` handle; a NULL
 * result restores the audio state and reports via AdtMessageBox. On
 * success, optionally logs (ReadMode & 4), gets the file's size via
 * AfsFileSize, takes MemoryLoadAddress as a pre-supplied buffer if set
 * (consuming it) or valloc()s a fresh one, reads it via AfsRead, closes
 * the handle, restores the audio state, and returns the buffer. Same
 * proven TAFS/TAFSFileHandle layout as
 * AfsRead.c/AfsFileSize.c/AfsClose.c/AfsOpen.c.
 *
 * Matching notes:
 *  - `&systemAFS` is recomputed at each of its 3 uses (AfsOpen, AfsFileSize,
 *    AfsRead) rather than cached in a named pointer: the first two share one
 *    callee-saved register (CSE, no intervening redefinition), but that
 *    register gets reused for `size` right after AfsFileSize, so AfsRead's
 *    `&systemAFS` materializes fresh. Write the address-of expression
 *    directly at each call site — don't introduce a `TAFS *vol` local.
 *  - Same MemoryLoadAddress idiom as LoadFromDEVPC: `buff = MemoryLoadAddress`
 *    belongs INSIDE the else-branch (m2c already shows this literally),
 *    not hoisted before the if the way Ghidra renders it.
 *  - AfsOpen's real return type is `TAFSFileHandle *` (AfsFileSize/AfsRead/
 *    AfsClose already fixed that signature) even though AfsOpen.c's own
 *    still-unmatched Ghidra stub calls it `TAFSElement *` — Ghidra's struct
 *    naming for this family isn't reliable; follow the proven callees.
 *  - GUARD POLARITY: unlike the one-line-success guard clauses (AfsClose,
 *    AfsFileSize — literal `== 0`, error as the branch target folded next
 *    to the epilogue), this guard's success arm is LONG (several
 *    statements, a nested if, five more calls) and a shared `quiet`
 *    local live from before the guard to the tail of BOTH arms. Ghidra's
 *    literal `if (fd == 0) {error; return 0;} success; return buff;`
 *    compiles with the wrong polarity here (success ends up the branch
 *    target, error the fallthrough — backwards from the target, which
 *    keeps error unnested and LAST). Nest the long success arm instead:
 *    `if (fd != 0) { ...long body...; return buff; } error; return 0;`
 *    — the mirror image of the short-body guard-clause rule. Net rule:
 *    the short-body exception flips back once the "success" side is the
 *    LONGER of the two arms.
 */
extern s32 AdtQuiet(s32 quiet);
extern TAFSFileHandle *AfsOpen(TAFS *handle, char *path);
extern int AfsFileSize(TAFS *handle, TAFSFileHandle *fh);
extern u32 AfsRead(TAFS *volume, TAFSFileHandle *fd, void *buffer, u32 length);
extern int AfsClose(TAFSFileHandle *fd);
extern void *valloc(u32 size);
extern void AdtMessageBox(char *fmt, ...);
extern char D_80011160[]; /* "%$LOAD(CD)\n%d[%s]" */
extern char D_80011174[]; /* "LOAD(CD) ERROR\n%d[%s]" */
extern s32 TotalIO;
extern s32 ReadMode;
extern u_long *MemoryLoadAddress;
extern TAFS systemAFS;

u_long *LoadFromCDROM(u8 *filename)
{
    s32 quiet;
    TAFSFileHandle *fd;
    s32 size;
    u_long *buff;

    TotalIO = TotalIO + 1;
    quiet = AdtQuiet(0);
    fd = AfsOpen(&systemAFS, (char *)filename);
    if (fd != 0) {
        if (ReadMode & 4) {
            AdtMessageBox(D_80011160, TotalIO, filename);
        }
        size = AfsFileSize(&systemAFS, fd);
        if (MemoryLoadAddress == 0) {
            buff = (u_long *)valloc(size);
        } else {
            buff = MemoryLoadAddress;
            MemoryLoadAddress = 0;
        }
        AfsRead(&systemAFS, fd, buff, size);
        AfsClose(fd);
        AdtQuiet(quiet);
        return buff;
    }
    AdtQuiet(quiet);
    AdtMessageBox(D_80011174, TotalIO, filename);
    return 0;
}
