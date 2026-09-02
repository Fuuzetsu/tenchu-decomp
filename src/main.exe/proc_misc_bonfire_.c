#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "effect.h"
#include "misc.h"

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
 *  - svec_y_n60_2 intentionally has unknown array size. The casted whole-
 *    SVECTOR copy then uses the target's two-register HIGH/LO_SUM address.
 *  - `direction[2]` followed by three VECTOR locals reproduces the complete
 *    0x40-byte stack workspace. The middle VECTOR is reused as the first
 *    effect's source and the sound call's destination.
 *  - The chained RGB assignment emits the target's b/g/r store order.
 */

extern SVECTOR svec_y_n60_2[];

extern void DrawSpriteXYZ(GsSPRITE *sprt, s32 x, s32 y, s32 z, s32 scale);
extern void *memset(void *dst, s32 c, u32 n);

void proc_misc_bonfire_(TMisc *m, TMiscMessage msg)
{
    SVECTOR direction[2];
    VECTOR bleed_pos;
    VECTOR pos;
    VECTOR raw_pos;
    GsSPRITE *frame;

    direction[0] = svec_y_n60_2[0];
    frame = &sprFrame[GameClock % MaxFrames];

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
    DrawSpriteXYZ(frame, m->x, m->y, m->z, m->param.bonfire.scale);

    if ((GameClock & 0xF) == 0)
    {
        memset(&pos, 0, sizeof(pos));
        pos.vx = m->x;
        pos.vy = m->y;
        pos.vz = m->z;
        bleed_pos = pos;
        SetBleedsDir(&bleed_pos, direction, 100, 10, 30, RGB24(100, 100, 60));
    }

    if (GameClock % 79 == 0)
    {
        memset(&raw_pos, 0, sizeof(raw_pos));
        raw_pos.vx = m->x;
        raw_pos.vy = m->y;
        raw_pos.vz = m->z;
        pos = raw_pos;
        SoundEx(&pos, SE_BONFIRE);
    }
}
