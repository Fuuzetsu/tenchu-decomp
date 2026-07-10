#include "common.h"
#include "main.exe.h"

/*
 * InitSprite (0x8004e9d8, 0x118 bytes) — zero a GsSPRITE, set its default
 * grey/full-scale look, then (when `image` is given) derive its pixel
 * geometry/tpage/UV window from a GsIMAGE. Twins: SetPolyXF4.c/AddXF4.c/
 * AddXG4.c/StartDrawing.c (same TU, all matched).
 *
 * Matching notes:
 *  - `tp = image->pmode & 3` reads only the LOW HALFWORD of the 4-byte
 *    pmode field (lhu at offset 0) — GsIMAGE's real layout (proven
 *    elsewhere, e.g. LoadTIM.c's full-word `im.pmode`) keeps pmode a
 *    u_long; this call site narrows via an explicit pointer cast rather
 *    than through the field, same idiom as a param-union's divergent
 *    access width (cookbook Expressions: reach it via an explicit offset
 *    cast off the SAME proven pointer).
 *  - `sh = 2 - tp` is a named local: it's read again AFTER the GetTPage
 *    call (for the `u` mask), so its live range crosses the call and it
 *    needs a callee-saved register — matches if declared once and reused
 *    for both the `w` shift and the `u` mask.
 *  - `image->px`/`image->py` are re-read (fresh loads) after the
 *    GetTPage call rather than cached, since the call clobbers the
 *    caller-saved copies (same "reload across an intervening call"
 *    shape as SetupSE.c's se->VABid).
 *  - `sprite->v = (u8)image->py` is a genuinely separate BYTE load
 *    (lbu) from the earlier full `lh` read of the same field for the
 *    GetTPage argument — different machine modes don't CSE (cookbook:
 *    DeleteConflict's ConflictObjects).
 */
extern u16 GetTPage(s32 tp, s32 abr, s16 x, s16 y);
extern void *memset(void *s, s32 c, u32 n);

void InitSprite(GsIMAGE *image, GsSPRITE *sprite)
{
    s32 tp;
    s32 sh;

    memset(sprite, 0, sizeof(GsSPRITE));
    sprite->b = 0x80;
    sprite->g = 0x80;
    sprite->r = 0x80;
    sprite->attribute = 0;
    sprite->scaley = 0x1000;
    sprite->scalex = 0x1000;
    if (image != 0)
    {
        tp = *(u16 *)&image->pmode & 3;
        sprite->attribute = sprite->attribute | (tp << 0x18);
        sh = 2 - tp;
        sprite->w = image->pw << sh;
        sprite->h = image->ph;
        sprite->tpage = GetTPage(tp, 0, image->px, image->py);
        sprite->u = (u8)((image->px << sh) & ((1 << (8 - tp)) - 1));
        sprite->v = (u8)image->py;
        sprite->cx = image->cx;
        sprite->cy = image->cy;
        sprite->mx = sprite->w >> 1;
        sprite->my = sprite->h >> 1;
    }
}
