#include "common.h"
#include "main.exe.h"

/*
 * FUN_8005fe88 (0x8005fe88, 0x44 bytes) — writes two bytes (0x25, 0x23) into
 * a static scratch buffer, reports it via AdtMessageBox, then stashes a
 * cursor pointing 2 bytes into the buffer (right after the two bytes just
 * written) into AdtMsgPtr for later code to continue filling.
 *
 * AdtMsgBuf is a non-small (unknown-size) extern array in this TU: the
 * offset-0 store folds %lo into the sb's displacement off the bare `%hi`
 * register, while the offset-1 store and the call/cursor uses force a full
 * lui+addiu materialization of the symbol's own address (docs/matching-
 * cookbook.md's "offset-0 folds, nonzero materializes" rule) — reusing the
 * same register rather than allocating a fresh one.
 * AdtMsgPtr is %gp_rel in this TU (tools/gpsyms.py --write).
 */
extern void AdtMessageBox(char *fmt, ...);
extern char AdtMsgBuf[];
extern char *AdtMsgPtr;

void FUN_8005fe88(void)
{
    AdtMsgBuf[0] = 0x25;
    AdtMsgBuf[1] = 0x23;
    AdtMessageBox(AdtMsgBuf);
    AdtMsgPtr = &AdtMsgBuf[2];
}
