#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/*
 * AfsFileSize (0x8005f008) — TAFS file-size accessor: `handle` (TAFS *) is
 * taken but never used ($a0 dead in the asm — confirmed by m2c); the real
 * work is on the second parameter, a `TAFSFileHandle *` in $a1 (see
 * filesystem.h for the PSX.SYM-proven layout), returning
 * fh->info->size — the directory-entry size cached behind the handle's
 * TAFSElement. Ghidra mistypes the second parameter `TAFSElement *element`
 * and renders the double-dereference as raw arithmetic
 * (`*(undefined4 *)(element->size + 8)`); m2c's raw `arg1->unk8->unk8`
 * disambiguates it as two pointer hops, matching TAFSFileHandle.info (0x8)
 * then TAFSElement.size (0x8) — same struct-name-distrust rule as AfsClose.
 * A NULL handle reports via AdtMessageBox() and returns 0 (not -1, unlike
 * the other four CD/AFS wrappers in this batch).
 *
 * Guard-clause polarity: written first in source per the cd_close/AfsClose
 * precedent (both if-arms return, so the literal if-body lands at the
 * branch target next to the epilogue, not the fallthrough).
 */

extern void AdtMessageBox(char *fmt, ...);
extern char D_800149B4[]; /* "AfsFileSize: invalid handle" — lives in this
                            * TU's unsplit data blob (splat auto-symbol),
                            * same pattern as AfsInit's D_80014944. */

int AfsFileSize(TAFS *handle, TAFSFileHandle *fh)
{
    if (fh == 0) {
        AdtMessageBox(D_800149B4);
        return 0;
    }
    return fh->info->size;
}
