#include "common.h"
#include "main.exe.h"

/*
 * GS_107_OBJ_4B8 (0x80065a9c) — stock PsyQ libgs: cache the light-colour matrix
 * in `_LC` (already a known symbol at 0x800c64c0) and load it into the GTE.
 * The cache write is a whole-MATRIX struct assignment, which cc1 expands to the
 * word-by-word copy; `SetColorMatrix` still receives the caller's pointer, not
 * the cache (a0 is untouched across the copy).
 *
 * The name is Ghidra's placeholder (object 107, offset 0x4B8). PsyQ calls this
 * one of the GsSetLight* entries — not renamed here: per the contract a rename
 * needs callmatch --verify plus a separate commit.
 */
extern MATRIX _LC;
extern void SetColorMatrix(MATRIX *m);

/* MATCHING: the GS_107.OBJ compiler profile reconstructs the library backend's
 * symbolic-address block-move split. See docs/toolchain.md. */
void GS_107_OBJ_4B8(MATRIX *m)
{
    _LC = *m;
    SetColorMatrix(m);
}
