#include "common.h"
#include "mission_hud.h"
#include "../main.exe/graphics.h"

/* Four bar.tim frames announce clock adjustments for mission events.
 * Negative commands reset the animation or advance/draw the current notice. */
int UpdateTrialTimeNotice(short event)
{
    GsSPRITE *sprite;

    if (event == TRIAL_TIME_NOTICE_RESET)
    {
        TrialTimeNoticeIndex = TrialTimeNoticeTicks = -1;
        return -1;
    }
    if (event != TRIAL_TIME_NOTICE_DRAW)
    {
        TrialTimeNoticeTicks = 0;
        TrialTimeNoticeIndex = event;
        return event;
    }
    if (TrialTimeNoticeTicks >= 0)
    {
        TrialTimeNoticeTicks++;
        sprite = &TrialTimeNotices[TrialTimeNoticeIndex];
        if (TrialTimeNoticeTicks > 90)
        {
            TrialTimeNoticeTicks = -1;
            return -1;
        }
        if (TrialTimeNoticeTicks < 15)
        {
            u8 brightness = TrialTimeNoticeTicks;
            sprite->r = sprite->g = sprite->b = (brightness + 1) * 8;
            sprite->scalex = sprite->scaley = (15 - TrialTimeNoticeTicks) * 500 + 5120;
        }
        else if (TrialTimeNoticeTicks < 18)
            SoundEx(NULL, 4);
        else if (TrialTimeNoticeTicks > 60)
            sprite->r = sprite->g = sprite->b = sprite->b - 4;
        GsSortSprite(sprite, OTablePt, 1);
    }
    return event;
}
