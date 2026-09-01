#include "common.h"
#include "main.exe.h"
#include "images.h"
#include "tim.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitSprite(struct GsIMAGE *image, struct GsSPRITE *sprite);
 *     IMAGES.C:79, 23 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct GsIMAGE * image
 *     param $s2       struct GsSPRITE * sprite
 * END PSX.SYM */

/*
 * InitSprite (0x8004e9d8, 0x118 bytes) — zero a GsSPRITE, set its default
 * grey/full-scale look, then (when `image` is given) derive its pixel
 * geometry/tpage/UV window from a GsIMAGE. Twins: SetPolyXF4.c/AddXF4.c/
 * AddXG4.c/StartDrawing.c (same TU, all matched).
 *
 * Matching notes:
 *  - `TIM_PIXEL_MODE(*(u16 *)&image->pmode)` reads only the LOW HALFWORD of
 *    the 4-byte pmode field (lhu at offset 0) — GsIMAGE's real layout (proven
 *    elsewhere, e.g. LoadTIM.c's full-word `im.pmode`) keeps pmode a
 *    u_long; this call site narrows via an explicit pointer cast rather
 *    than through the field, same idiom as a param-union's divergent
 *    access width (cookbook Expressions: reach it via an explicit offset
 *    cast off the SAME proven pointer).
 *  - `width_shift = 2 - texture_mode` is a named local: it's read again
 *    AFTER the GetTPage call (for the `u` mask), so its live range crosses
 *    the call and it needs a callee-saved register — matches if declared
 *    once and reused for both the `w` shift and the `u` mask.
 *  - `image->px`/`image->py` are re-read (fresh loads) after the
 *    GetTPage call rather than cached, since the call clobbers the
 *    caller-saved copies (same "reload across an intervening call"
 *    shape as SetupSE.c's se->VABid).
 *  - `sprite->v = (u8)image->py` is a genuinely separate BYTE load
 *    (lbu) from the earlier full `lh` read of the same field for the
 *    GetTPage argument — different machine modes don't CSE (cookbook:
 *    DeleteConflict's ConflictObjects).
 */
extern void *memset(void *s, s32 c, u32 n);

void InitSprite(GsIMAGE *image, GsSPRITE *sprite)
{
    s32 texture_mode;
    s32 width_shift;

    memset(sprite, 0, sizeof(GsSPRITE));
    sprite->b = 0x80;
    sprite->g = 0x80;
    sprite->r = 0x80;
    sprite->attribute = 0;
    sprite->scaley = FIXED_ONE;
    sprite->scalex = FIXED_ONE;
    if (image != 0)
    {
        texture_mode = TIM_PIXEL_MODE(*(u16 *)&image->pmode);
        sprite->attribute =
            sprite->attribute | GS_ATTR_TEXTURE_MODE(texture_mode);
        width_shift = 2 - texture_mode;
        sprite->w = image->pw << width_shift;
        sprite->h = image->ph;
        sprite->tpage = GetTPage(texture_mode, 0, image->px, image->py);
        sprite->u = (u8)((image->px << width_shift) &
                          ((1 << (8 - texture_mode)) - 1));
        sprite->v = (u8)image->py;
        sprite->cx = image->cx;
        sprite->cy = image->cy;
        sprite->mx = sprite->w >> 1;
        sprite->my = sprite->h >> 1;
    }
}
