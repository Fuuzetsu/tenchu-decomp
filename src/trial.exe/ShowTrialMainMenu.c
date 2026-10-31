#include "common.h"
#include "trial.exe.h"
#include "../main.exe/sound.h"
#include <psxsdk/libgpu.h>

/* The fourth row exits; the other rows return the mode values consumed by
 * RunMissionEditor. Input is enabled only after the opening fade finishes. */
short ShowTrialMainMenu(void)
{
    short selection = TRIAL_MENU_PLAY;
    short brightness = 128;
    short phase = 1;
    u_long previous_buttons = 0;
    u_long pressed;

    ClearTrialDisplay(320, 480);
    InitTrialMenuGraphics();
    LoadTrialMenuGraphics();
    TrialSelectedMission.is_user_mission = 1;
    SetDispMask(1);
    DrawSync(0);
    VSync(0);
    do
    {
        StartDrawing();
        switch (phase)
        {
        case 1:
            brightness -= 12;
            if (brightness <= 0)
            {
                brightness = 0;
                phase++;
                previous_buttons = GetRealPad(0);
            }
            DrawTrialScreenFade(brightness, 320, 240, 0);
            break;
        case 2:
            pressed = ReadTrialMenuPad(0, &previous_buttons, 0);
            if (pressed == PADRright)
            {
                if (selection == 40)
                    selection = TRIAL_MENU_EXIT;
                SoundEx(0, 4);
                phase++;
            }
            else if (pressed == PADLup)
            {
                if (selection != TRIAL_MENU_PLAY)
                {
                    selection -= 10;
                    TrialMenuCursor->sprite.y -= 35;
                    SoundEx(0, 3);
                }
            }
            else if (pressed == PADLdown)
            {
                if (selection != 40)
                {
                    selection += 10;
                    TrialMenuCursor->sprite.y += 35;
                    SoundEx(0, 3);
                }
            }
            break;
        case 3:
            brightness += 12;
            if (brightness >= 128)
            {
                brightness = 128;
                phase++;
            }
            DrawTrialScreenFade(brightness, 320, 240, 0);
            break;
        default:
            phase = 0;
            break;
        }
        DrawBG(TrialMenuBackground);
        GsSortSprite(&TrialMenuCursor->sprite, OTablePt, 10);
        SkipFrame = 2;
        EndDrawing(-2);
    } while (phase != 0);
    SetDispMask(0);
    DisposeTrialMenuGraphics();
    return selection;
}
