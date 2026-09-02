#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutItemCursor(short x, short y, short size, short rotdif);
 *     INFOVIEW.C:358, 5 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short x
 *     param $a1       short y
 *     param $a2       short size
 *     param $a3       short rotdif
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE CursorImage;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

void PutItemCursor(s16 x, s16 y, s16 size, s32 rotdif)
{
    CursorImage.x = x;
    CursorImage.y = y;
    CursorImage.scaley = CursorImage.scalex = size;
    CursorImage.rotate += rotdif;
    GsSortSprite(&CursorImage, OTablePt, 1);
}
