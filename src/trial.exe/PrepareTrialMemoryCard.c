#include "common.h"
#include "mission_menu.h"
#include <psxsdk/libgpu.h>

/* Check the card before browsing missions for editing. The shared card
 * state machine draws any missing-card, format-status, or error prompts. */
short PrepareTrialMemoryCard(void)
{
    u_long previous_buttons;
    short phase = 1;
    short accepted;
    short result;

    ClearTrialDisplay(320, 480);
    ClearTrialPages();
    previous_buttons = GetRealPad(0);
    SetDispMask(1);
    DrawSync(0);
    VSync(0);
    do
    {
        StartDrawing();
        switch (phase)
        {
        case 1:
            result = UpdateTrialCardCheck(ReadTrialMenuPad(0, &previous_buttons, 0), 1);
            switch (result)
            {
            case -1:
                accepted = 1;
                phase++;
                break;
            case 1:
                accepted = 0;
                phase++;
                break;
            }
            break;
        default:
            phase = 0;
            break;
        }
        SkipFrame = 2;
        EndDrawing(-2);
    } while (phase);
    ClearTrialPages();
    SetDispMask(0);
    return accepted;
}
