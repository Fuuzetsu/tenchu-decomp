#include "common.h"
#include "traps.h"
#include "../main.exe/item.h"
#include <psxsdk/libgte.h>

/* Once per second, a newly occupied trigger fires the closest launcher.
 * Equally distant launchers are selected randomly during the grid scan. */
void UpdateTrialArrowTraps(void)
{
    PARAM_ITEM_LAUNCH request;
    SVECTOR velocity;
    Humanoid *human;
    short i, trigger_x, trigger_z;
    short x, z, best_x, best_z;
    int dx, dz, distance, best_distance, angle, launch_range;
    u8 packed, trap;

    if (GameClock % 30)
        return;
    for (i = 0; i < Humans; i++)
    {
        human = HumanGroup[i];
        if (human->status != STAT_DEAD && human->map.height == 0)
        {
            trigger_x = human->locate->vx / 2000;
            trigger_z = human->locate->vz / -2000;
            if (TrialTrapMap[trigger_x + trigger_z * TRIAL_MAP_WIDTH] == TRIAL_TRAP_TRIGGER)
                break;
        }
    }
    if (i >= Humans)
    {
        TrialLastTriggerX = TrialLastTriggerZ = -1;
        trigger_x = trigger_z = -1;
    }
    if (trigger_x != TrialLastTriggerX || trigger_z != TrialLastTriggerZ)
    {
        TrialLastTriggerX = trigger_x;
        TrialLastTriggerZ = trigger_z;
        SoundEx(human->locate, TRIAL_SE_TRAP_TRIGGER);
        best_x = best_z = TRIAL_MAP_WIDTH;
        best_distance = TRIAL_MAP_WIDTH * TRIAL_MAP_HEIGHT;
        z = x = 0;
        do
        {
            trap = TrialTrapMap[x + z * TRIAL_MAP_WIDTH];
            if (trap != TRIAL_TRAP_TRIGGER && trap != TRIAL_TRAP_EMPTY)
            {
                dx = x - trigger_x;
                dz = z - trigger_z;
                distance = SquareRoot0((dx * dx + dz * dz) * 100);
                if (distance < best_distance ||
                    (distance == best_distance && (rand() & 1)))
                {
                    best_x = x;
                    best_z = z;
                    best_distance = distance;
                }
            }
            z += ++x >= TRIAL_MAP_WIDTH ? (x = 0, 1) : 0;
        } while (z < TRIAL_MAP_HEIGHT);

        if (best_x != TRIAL_MAP_WIDTH || best_z != TRIAL_MAP_HEIGHT)
        {
            packed = TrialTrapMap[best_x + best_z * TRIAL_MAP_WIDTH];
            request.type = ITEM_ARROW;
            request.start.vx = best_x * 2000 + 1000;
            request.start.vz = best_z * -2000 - 1000;
            /* The scan coordinates now hold launch height and facing. */
            x = (packed >> 4) & 7;
            request.user = StagePlayer;
            request.start.vy = x * -1000 + 500;
            switch (z = packed & 3)
            {
            case 0: request.start.vz += 1100; break;
            case 1: request.start.vx += 1100; break;
            case 2: request.start.vz -= 1100; break;
            case 3: request.start.vx -= 1100; break;
            }
            angle = (short)(z * 1024 + (rand() & 0xff) - 128);
            launch_range = rand() % (x + 1) + 4;
            GetMoveSpeed(&velocity, angle, (x + launch_range) * 1000, 0);
            request.end.vy = 0;
            request.end.vx = request.start.vx - velocity.vx;
            request.end.vz = request.start.vz - velocity.vz;
            SoundEx(&request.start, TRIAL_SE_ARROW_LAUNCH);
            ReqItemUse(&request);
        }
    }
}
