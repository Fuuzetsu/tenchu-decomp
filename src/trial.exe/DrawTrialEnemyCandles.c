#include "common.h"
#include "mission_hud.h"
#include "mission.h"
#include "../main.exe/item.h"
#include "../main.exe/graphics.h"
#include <stdlib.h>

/* The eliminate-enemies objective shows one flickering candle per survivor.
 * Draw the previous count, then refresh it every ten mission-clock ticks. */
void DrawTrialEnemyCandles(void)
{
    short i, frame;
    Humanoid *human;

    if ((TrialMission.header.flags & TRIAL_OBJECTIVE_MASK) == TRIAL_ELIMINATE_ENEMIES)
    {
        for (i = 0; i < TrialRemainingEnemies; i++)
        {
            frame = rand() % 3;
            TrialEnemyCandles[frame].x = i * 10 - 60;
            GsSortSprite(&TrialEnemyCandles[frame], OTablePt, 1);
        }
        if (GameClock % 10 == 0)
        {
            TrialRemainingEnemies = 0;
            for (i = 1; i < Humans; i++)
            {
                human = HumanGroup[i];
                if (human->type > TONO && (human->type & PAGE_MASK) != PAGE_CIVILIAN &&
                    human->status != STAT_DEAD)
                    TrialRemainingEnemies++;
            }
        }
    }
}
