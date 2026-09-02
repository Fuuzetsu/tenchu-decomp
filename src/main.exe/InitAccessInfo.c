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

void InitAccessInfo(void)
{
    SetupImageToPolyGT4(GetImage(IMG_LOADING), &AccessImage, 0xd6, 0xd9);
    AccessPower = 0;
}
