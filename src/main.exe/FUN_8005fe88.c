#include "common.h"
#include "main.exe.h"

/*
 * FUN_8005fe88 (0x8005fe88, 0x44 bytes) — writes two bytes (0x25, 0x23) into
 * a static scratch buffer, reports it via AdtMessageBox, then stashes a
 * cursor pointing 2 bytes into the buffer (right after the two bytes just
 * written) into D_80097E98 for later code to continue filling.
 *
 * D_800C2EB0 is a non-small (unknown-size) extern array in this TU: the
 * offset-0 store folds %lo into the sb's displacement off the bare `%hi`
 * register, while the offset-1 store and the call/cursor uses force a full
 * lui+addiu materialization of the symbol's own address (docs/matching-
 * cookbook.md's "offset-0 folds, nonzero materializes" rule) — reusing the
 * same register rather than allocating a fresh one.
 * D_80097E98 is %gp_rel in this TU (tools/gpsyms.py --write).
 */
extern void AdtMessageBox(char *fmt, ...);
extern char D_800C2EB0[];
extern char *D_80097E98;

void FUN_8005fe88(void)
{
    D_800C2EB0[0] = 0x25;
    D_800C2EB0[1] = 0x23;
    AdtMessageBox(D_800C2EB0);
    D_80097E98 = &D_800C2EB0[2];
}
