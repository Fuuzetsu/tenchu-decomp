#include "common.h"
#include "main.exe.h"

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
