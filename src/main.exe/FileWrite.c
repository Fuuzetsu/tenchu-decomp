#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int FileWrite(unsigned char *filename, void *data, long size);
 *     FILEIO.C:197, 13 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $a1       void * data
 *     param $a2       long size
 * END PSX.SYM */

/*
 * FileWrite (0x800193cc) — host-file writer over the PsyQ CRT's PC*
 * file family (PCcreat/PCwrite/PCclose, 0x80060xxx precompiled SDK block —
 * see PClseek.c): bail with 0 if there's nothing to write, create the file
 * (mode 0), bail with 0 if that failed, else write+close and report success.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The first guard is bitwise `|`, not `||`: both operands are
 *    side-effect-free, so cc1 computes each as a plain 0/1 flag
 *    (`sltiu`/`slti`) and ORs them before the single `bnez` — m2c's raw
 *    reconstruction shows the same `(a == 0) | (b < 1)` shape. The SECOND
 *    guard (the PCcreat result) is a genuine short-circuited `||` in
 *    Ghidra's rendering (a call can't be folded eagerly), so it's written
 *    as its own separate `if`.
 *  - `filename` stays live in $a0 unmodified from entry to the PCcreat
 *    call (never reassigned before the jal) — same forwarded-parameter
 *    shape as AdtFntOpen.
 */
extern int PCcreat(char *name, int mode);
extern int PCwrite(int fd, void *buf, int size);
extern int PCclose(int fd);

int FileWrite(char *filename, void *data, s32 size)
{
    s32 fd;

    if ((data == 0) | (size < 1))
        return 0;
    fd = PCcreat(filename, 0);
    if (fd == -1)
        return 0;
    PCwrite(fd, data, size);
    PCclose(fd);
    return 1;
}
