#include "common.h"
#include "main.exe.h"

/*
 * LoadFromMEMORY (0x8001962c) — memory-card/MEMORY loading stub; disabled in
 * the shipped build, so it just reports the "disabled" message and returns
 * null (filename is unused). D_8001113C is the literal string
 * "*** memory load is disabled now ***" living in this TU's rodata blob
 * (unsplit data segment; the address already has an auto symbol, no yaml
 * carve needed — same pattern as D_800121CC/D_80097CB4 elsewhere).
 */
extern void AdtMessageBox(char *fmt, ...);
extern char D_8001113C[];

void *LoadFromMEMORY(u8 *filename)
{
    AdtMessageBox(D_8001113C);
    return 0;
}
