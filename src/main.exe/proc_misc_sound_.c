#include "common.h"
#include "main.exe.h"
#include "misc.h"

extern s32 rand(void);

void proc_misc_sound_(TMisc *m, TMiscMessage msg)
{
    MiscSoundSchedule *sched;
    MiscSoundSchedule tmp;
    s32 lo;

    sched = &m->param.sound;

    switch (msg)
    {
    case MM_CREATE:
        tmp.min_delay = m->param.sound_init.min_delay;
        tmp.max_delay = m->param.sound_init.max_delay;
        tmp.sound_index = (u8)m->param.sound_init.sound;
        tmp.next = GameClock;
        *sched = tmp;
        m->mode = 0;
        break;

    case MM_DESTROY:
    case MM_PAUSE:
    case MM_RESUME:
        break;

    default:
        if (m->mode != 0)
            break;
        if (sched->next > GameClock)
            break;

        {
            VECTOR pos = {
                .vx = m->x,
                .vy = m->y,
                .vz = m->z
            };

            SoundEx(&pos, sched->sound_index + MISC_SOUND_ID_BASE);
        }

        if (sched->max_delay - sched->min_delay > 0)
        {
            lo = GameClock +
                 (rand() % (sched->max_delay - sched->min_delay) +
                  sched->min_delay);
        }
        else
        {
            lo = GameClock + sched->min_delay;
        }
        sched->next = lo;
        break;
    }
}
