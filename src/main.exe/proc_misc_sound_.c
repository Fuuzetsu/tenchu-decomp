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

    if (msg == MM_CREATE)
        goto reset;
    if (MM_DO <= msg)
        goto normal;
    return;

reset:
    tmp.min_delay = m->param.sound_init.min_delay;
    tmp.max_delay = m->param.sound_init.max_delay;
    tmp.sound_index = (u8)m->param.sound_init.sound;
    tmp.next = GameClock;
    *sched = tmp;
    m->mode = 0;
    return;

normal:
    if (m->mode != 0)
        return;
    if (sched->next > GameClock)
        return;

    {
        VECTOR pos = {m->x, m->y, m->z};

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
}
