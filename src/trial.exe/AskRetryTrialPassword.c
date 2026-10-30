#include "common.h"
#include "mission_menu.h"
#include "../main.exe/images.h"
#include "../main.exe/sound.h"
#include <psxsdk/libgpu.h>

/* Circle returns to the keyboard; Start abandons password entry. */
short AskRetryTrialPassword(void)
{
    short running = 1;
    short retry = 0;
    u_long previous_buttons = 0;
    u_long pressed;
    Sprite3D *message;
    Sprite3D *controls;

    ClearTrialDisplay(320, 480);
    message = load_archive_sprite_(TrialImageArchive, 22);
    controls = load_archive_sprite_(TrialImageArchive, 23);
    controls->sprite.y = 95;
    SoundEx(NULL, TRIAL_SOUND_ERROR);
    previous_buttons = GetRealPad(0);
    SetDispMask(1);
    DrawSync(0);
    VSync(0);
    while (running)
    {
        StartDrawing();
        pressed = ReadTrialMenuPad(0, &previous_buttons, 0);
        if (pressed == PADRright)
        {
            running = 0;
            retry = 1;
            SoundEx(NULL, TRIAL_SOUND_ACCEPT);
        }
        else if (pressed == PADstart)
        {
            running = 0;
            retry = 0;
            SoundEx(NULL, TRIAL_SOUND_CANCEL);
        }
        GsSortSprite(&message->sprite, OTablePt, 10);
        GsSortSprite(&controls->sprite, OTablePt, 10);
        SkipFrame = 2;
        EndDrawing(-2);
    }
    SetDispMask(0);
    vfree(message);
    vfree(controls);
    return retry;
}
