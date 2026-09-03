#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "effect.h"
#include "misc.h"

extern void DrawSpriteXYZ(GsSPRITE *sprt, s32 x, s32 y, s32 z, s32 scale);

void proc_misc_bonfire_(TMisc *m, TMiscMessage msg)
{
    SVECTOR direction[2];
    GsSPRITE *frame;

    direction[0] = (SVECTOR){
        .vx = 0,
        .vy = -60,
        .vz = 0
    };
    frame = &sprFrame[GameClock % MaxFrames];

    switch (msg)
    {
    case MM_CREATE:
        m->mode = 0;
        break;

    case MM_DESTROY:
    case MM_PAUSE:
    case MM_RESUME:
        break;

    default:
        if (m->mode != 0)
            break;

        frame->r = frame->g = frame->b = (u8)(rand() % 100 + 100);
        DrawSpriteXYZ(frame, m->x, m->y, m->z,
                      m->param.bonfire.scale);

        if ((GameClock & 0xF) == 0)
        {
            VECTOR bleed_pos = {
                .vx = m->x,
                .vy = m->y,
                .vz = m->z
            };

            SetBleedsDir(&bleed_pos, direction, 100, 10, 30,
                         RGB24(100, 100, 60));
        }

        if (GameClock % 79 == 0)
        {
            VECTOR pos = {
                .vx = m->x,
                .vy = m->y,
                .vz = m->z
            };

            SoundEx(&pos, SE_BONFIRE);
        }
        break;
    }
}
