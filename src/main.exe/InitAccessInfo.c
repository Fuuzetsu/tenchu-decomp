#include "common.h"
#include "main.exe.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitAccessInfo(void);
 *     FILEIO.C:106, 4 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_GT4 AccessImage;
 *     extern int AccessPower;
 * END PSX.SYM */

/*
 * InitAccessInfo (0x800194f4, 0x38 bytes) — one-shot setup for the file-access
 * indicator, called only from main(): fetch IMG_LOADING, bind it to a
 * GT4 at screen coordinates (214, 217), and zero its colour-cycle counter.
 *
 * AccessImage (0x800bc0c0) is the POLY_GT4 named and typed by PSX.SYM; the
 * same layout is independently exercised field-by-field by cbAccess.c and
 * FUN_80018f00.c. splat auto-named it D_800BC0C0 only because this function's
 * asm was the sole reference before those functions were matched, so the
 * explicit `AccessImage = 0x800bc0c0;` in config/symbols.main.exe.txt keeps
 * the original name (see PathFileRead.c for the same trap).
 *
 * AccessPower is %gp_rel (a small in the gp window) — same global PrepareAccess.c
 * clears.
 */

extern GsIMAGE *GetImage(s32 id);

void InitAccessInfo(void)
{
    SetupImageToPolyGT4(GetImage(IMG_LOADING), &AccessImage, 0xd6, 0xd9);
    AccessPower = 0;
}
