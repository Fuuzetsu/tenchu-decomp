#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "misc.h"

extern s32 rand(void);

void proc_misc_puff_(TMisc *m, TMiscMessage msg)
{
    VECTOR pos;
    SVECTOR dir;
    s32 x, z;

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

        x = m->x;
        pos.vx = (1 <= (m->param.puff.x_radius << 1))
                     ? x + (rand() % (m->param.puff.x_radius << 1) -
                            m->param.puff.x_radius)
                     : x - m->param.puff.x_radius;

        pos.vy = m->y;
        z = m->z;
        pos.vz = (1 <= (m->param.puff.z_radius << 1))
                     ? z + (rand() % (m->param.puff.z_radius << 1) -
                            m->param.puff.z_radius)
                     : z - m->param.puff.z_radius;

        if ((GameClock & 1) == 0)
        {
            dir.vx = 0;
            dir.vy = -400;
            dir.vz = 0;
            pos.vy += 2000;
            SetSmoke(&pos, &dir, 1, 1);
            pos.vy -= 2000;
            SetSplash(&pos, 4 * FIXED_ONE, 2 * FIXED_ONE, 10);
        }
        break;
    }
}
