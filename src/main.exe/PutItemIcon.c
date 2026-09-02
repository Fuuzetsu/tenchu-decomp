#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutItemIcon(int ItemID, short x, short y, short scale);
 *     INFOVIEW.C:349, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int ItemID
 *     param $a1       short x
 *     param $a2       short y
 *     param $a3       short scale
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *ItemImage[25];
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

void PutItemIcon(s32 ItemID, short x, short y, short scale)
{
    GsSPRITE *sprite = &ItemImage[ItemID]->sprite;

    sprite->x = x;
    sprite->y = y;
    sprite->scalex = scale;
    sprite->scaley = scale;
    GsSortSprite(sprite, OTablePt, 0);
}
