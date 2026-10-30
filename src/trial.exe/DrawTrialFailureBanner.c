#include "common.h"
#include "mission_hud.h"
#include "../main.exe/graphics.h"
#include "../main.exe/sound.h"

/* SetupTrialMission loads fail.tim into this sprite with zero brightness.
 * Fade it up to 128, then blink on the mission clock's 32-frame cycle. */
void DrawTrialFailureBanner(void)
{
    GsSPRITE *sprite = &TrialFailureBanner;

    if ((signed char)sprite->b >= 0)
    {
        if (sprite->b >= 104)
            SoundEx(NULL, 4);
        sprite->r = sprite->g = sprite->b = sprite->b + 8;
    }
    else if (!(GameClock & 0x18))
        return;

    GsSortSprite(&TrialFailureBanner, OTablePt, 1);
}
