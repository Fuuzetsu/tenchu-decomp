#include "common.h"
#include "mission_hud.h"
#include "../main.exe/graphics.h"

/* The live clock uses five glyphs (M M : S S) at 30 ticks per second.
 * Hide the leading zero; at 59:59, blink in half-second intervals. */
void DrawTrialMissionTimer(int ticks)
{
    short i, digit;
    short x = -24;

    if (ticks >= 107970 && (GameClock / 15 & 1))
        return;

    for (i = 0; i < 5; i++, x += 12)
    {
        switch (i)
        {
        case 0: digit = (ticks / 18000) % 6; break;
        case 1: digit = (ticks / 1800) % 10; break;
        case 2: digit = 10; break;
        case 3: digit = (ticks / 300) % 6; break;
        case 4: digit = (ticks / 30) % 10; break;
        }
        if (i || digit)
        {
            TrialTimerDigits[digit].x = x;
            TrialTimerDigits[digit].y = -76;
            GsSortSprite(&TrialTimerDigits[digit], OTablePt, 2);
        }
    }
}
