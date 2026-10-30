#include "common.h"
#include "traps.h"
#include "../main.exe/item.h"

/* A cover sinks while an active character stands on it. The collision
 * node's height counts the collapse; clearing its division removes support. */
void UpdateTrialCollapsingFloors(void)
{
    short i, j;
    Humanoid *human;
    TrialTerrainOrnament *ornament;
    AreaNodeType *node;
    int x, z;

    if (GameClock & 1)
        return;
    for (i = 0; i < Humans; i++)
    {
        human = HumanGroup[i];
        if (!((u16)human->attribute & ATTR_SUSPEND) &&
            (human->map.attrib & TRIAL_MAP_COLLAPSING) && human->map.height == 0)
        {
            x = (human->locate->vx / 2000) * 2000 + 1000;
            z = (human->locate->vz / -2000) * -2000 - 1000;
            for (j = 0; j < TrialTerrainOrnamentCount; j++)
            {
                ornament = &TrialTerrainOrnaments[j];
                if (ornament->x == x && ornament->z == z)
                {
                    node = human->map.area;
                    if ((short)(node->y % 30) == 0)
                        SoundEx((VECTOR *)ornament, TRIAL_SE_FLOOR_SINK);
                    node->y += 5;
                    if (node->y > 100)
                    {
                        node->y = 0;
                        node->division = 0;
                        SoundEx((VECTOR *)ornament, TRIAL_SE_TRAP_TRIGGER);
                        ornament->x = ornament->z = 0x7fffffff;
                    }
                    else
                        ornament->y += 50;
                    break;
                }
            }
        }
    }
}
