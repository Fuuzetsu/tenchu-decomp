#include "common.h"
#include "mission.h"
#include "trial.exe.h"
#include "../main.exe/item.h"
#include "../main.exe/graphics.h"
#include <psxsdk/libgte.h>

/* Reconsider non-story actors once per second, and before mission entry.
 * TRIAL budgets three active actors. The eliminate-enemies objective also
 * projects their origins so one nearby offscreen actor can stay active. */
void ActivateTrialHumans(short initial)
{
    MATRIX matrix;
    SVECTOR screen;
    Humanoid *human;
    long dx, dz, distance, depth;
    short offscreen = 0;
    short i = 0;
    short active = 0;
    short visibility =
        (TrialMission.header.flags & TRIAL_OBJECTIVE_MASK) == TRIAL_ELIMINATE_ENEMIES;
    short range;

    for (; i < Humans; i++)
    {
        human = HumanGroup[i];
        if (human->type <= TONO)
            continue;

        dx = (human->locate->vx - StagePlayer->locate->vx) / 100;
        dz = (human->locate->vz - StagePlayer->locate->vz) / 100;
        distance = SquareRoot0(dx * dx + dz * dz);
        if (distance < 160)
            range = -1;
        else if (distance < 200)
            range = 1;
        else
            range = 0;

        if (visibility)
        {
            GsGetLs(&human->model->locate, &matrix);
            GsSetLsMatrix(&matrix);
            depth = RotTransPers(&UnitVector, (long *)&screen, NULL, NULL) >> 2;
            visibility = (__builtin_abs(screen.vx) > 320 || depth == 0) ? -1 : 1;
        }

        if ((u16)human->attribute & ATTR_SUSPEND)
        {
            if (human->status != STAT_DEAD && active < 3 &&
                (range == 1 || (range != 0 && visibility == -1 && offscreen <= 0)))
            {
                if (visibility == -1)
                    offscreen++;
                human->model->object[MODEL_PART_WAIST]->attribute =
                    (u16)human->model->object[MODEL_PART_WAIST]->attribute | MODEL_ATTR_COLLIDE;
                active++;
                human->attribute = (u16)human->attribute & ~ATTR_SUSPEND;
            }
        }
        else
        {
            if (visibility == -1 && human->life == 0)
            {
                offscreen--;
                range = 0;
            }
            if (range == 0 || ((range == 1 || initial) && active >= 3) ||
                (human->locate->vy >= 20000 && human->status == STAT_DEAD))
            {
                human->model->object[MODEL_PART_WAIST]->attribute =
                    (u16)human->model->object[MODEL_PART_WAIST]->attribute & ~MODEL_ATTR_COLLIDE;
                human->attribute = (u16)human->attribute | ATTR_SUSPEND;
            }
            else
                active++;
        }
    }
}
