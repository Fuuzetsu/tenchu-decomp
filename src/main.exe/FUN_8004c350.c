#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsSPRITE sprFrame[4];
 * END PSX.SYM */

/*
 * MATCH.
 *
 * MISC type 6 handler. CREATE clears the one-shot mode flag. Active ticks
 * recolor and draw one of four frame sprites, periodically emit blood, and
 * play a sound every 79 ticks.
 *
 * Matching notes:
 *  - The explicit dispatch ladder leaves the ignored messages inline while
 *    CREATE and the active body are both forward targets.
 *  - D_80097C50 intentionally has unknown array size. The casted whole-
 *    SVECTOR copy then uses the target's two-register HIGH/LO_SUM address.
 *  - `direction[2]` followed by three VECTOR locals reproduces the complete
 *    0x40-byte stack workspace. The middle VECTOR is reused as the first
 *    effect's source and the sound call's destination.
 *  - The chained RGB assignment emits the target's b/g/r store order.
 */

extern s32 GameClock;
extern GsSPRITE sprFrame[4];
extern u8 D_80097C50[];

extern void DrawSpriteXYZ(GsSPRITE *spr, s32 x, s32 y, s32 z, s32 size);
extern void SetBleedsDir(VECTOR *pos, SVECTOR *vec, short grange, short n,
                         int time, long col);
extern s16 SoundEx(VECTOR *loc, s32 id);
extern void *memset(void *dst, s32 c, u32 n);

void FUN_8004c350(tag_TMisc *m, TMiscMessage msg)
{
    SVECTOR direction[2];
    VECTOR bleed_pos;
    VECTOR pos;
    VECTOR raw_pos;
    GsSPRITE *frame;

    direction[0] = *(SVECTOR *)D_80097C50;
    frame = &sprFrame[GameClock % 4];

    if (msg == MM_CREATE)
        goto do_create;
    if (MM_DO <= msg)
        goto do_draw;
    return;

do_create:
    m->mode = 0;
    return;

do_draw:
    if (m->mode != 0)
        return;

    frame->r = frame->g = frame->b = (u8)(rand() % 100 + 100);
    DrawSpriteXYZ(frame, m->x, m->y, m->z, m->param.init.a);

    if ((GameClock & 0xF) == 0)
    {
        memset(&pos, 0, sizeof(pos));
        pos.vx = m->x;
        pos.vy = m->y;
        pos.vz = m->z;
        bleed_pos = pos;
        SetBleedsDir(&bleed_pos, direction, 100, 10, 30, 0x64643C);
    }

    if (GameClock % 79 == 0)
    {
        memset(&raw_pos, 0, sizeof(raw_pos));
        raw_pos.vx = m->x;
        raw_pos.vy = m->y;
        raw_pos.vz = m->z;
        pos = raw_pos;
        SoundEx(&pos, 0x47);
    }
}
