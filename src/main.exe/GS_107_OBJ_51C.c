#include "common.h"
#include "main.exe.h"

/*
 * GS_107_OBJ_51C (0x80065b00) — stock PsyQ libgs: read back the cached
 * light-colour matrix. The exact mirror of GS_107_OBJ_4B8 (which writes `_LC`
 * and loads the GTE); this one just copies `_LC` out into the caller's MATRIX,
 * so it needs no frame and ends with the copy's last store in the `jr ra` delay
 * slot.
 *
 * Names are Ghidra placeholders (object 107, offset 0x51C) — not renamed here;
 * a rename needs callmatch --verify and its own commit.
 */
extern MATRIX _LC;

/* MATCHING: this is the natural inverse of GS_107_OBJ_4B8. The original-object
 * compiler profile is documented in docs/toolchain.md. */
void GS_107_OBJ_51C(MATRIX *m)
{
    *m = _LC;
}
