#include "common.h"
#include "mission_editor.h"
#include "card.h"
#include "../main.exe/font.h"
#include "../main.exe/images.h"
#include "../main.exe/sound.h"

static char *SavePrompts[] = {
    "\217\111\227\271\202\267\202\351\221\117\202\311\225\333\221\266\202\265\202\334\202\267\202\251", /* 終了する前に保存しますか */
    "\226\173\223\226\202\311\225\333\221\266\202\271\202\270\202\311\217\111\227\271\202\265\202\334\202\267\202\251", /* 本当に保存せずに終了しますか */
};

/* Copy the validated working map back before hashing or writing the mission.
 * Leaving offers Save first, then asks before discarding the changes. */
short SaveTrialEditorMission(short exiting)
{
    short brightness = 128;
    short phase = 0;
    short leave = 0;
    u32 checksum;
    u_long buttons, previous_buttons, pressed;
    char *prompt;

    ValidateTrialEditorMap();
    __builtin_memcpy(TrialMission.map, TrialEditorMap, sizeof(TrialEditorMap));
    checksum = CalculateTrialMissionChecksum();
    TrialEditorModified = TrialEditorChecksum != checksum ? -1 : 0;
    TrialEditorChecksum = checksum;
    /* The fade-in initializes button history before either prompt uses it. */
    while (phase >= 0)
    {
        StartDrawing();
        previous_buttons = buttons;
        buttons = GetRealPad(0);
        pressed = 0;
        if (!previous_buttons)
            pressed = buttons;
        DrawBG(TrialEditorBackground);
        switch (phase)
        {
        case 0:
            brightness -= 8;
            if (brightness <= 0)
            {
                phase = 2;
                brightness = 0;
            }
            else
                DrawTrialScreenFade(brightness, 320, 240, 0);
            break;
        case 1:
            if ((brightness += 8) >= 128)
            {
                phase = -1;
                brightness = 128;
                DrawTrialScreenFade(128, 320, 240, 0);
            }
            else
                DrawTrialScreenFade(brightness, 320, 240, 0);
            break;
        case 2:
            if (exiting)
            {
                phase = 3;
                exiting = 1;
            }
            else if (UpdateTrialCardSave(pressed, 0))
                phase = 1;
            break;
        case 3:
            prompt = SavePrompts[exiting - 1];
            SetupTelop(prompt, 0);
            draw_telop_line_(OTablePt->org, -(telop_text_width_(prompt) / 2), -8, prompt);
            GsSortSprite(&TrialEditorConfirmButtons, OTablePt, 0);
            if (pressed == PADRright || pressed == PADRdown)
            {
                SoundEx(NULL, TRIAL_SOUND_ACCEPT);
                switch (exiting)
                {
                case 1:
                    if (pressed == PADRright)
                    {
                        phase = 2;
                        leave = -1;
                        exiting = 0;
                    }
                    else
                        exiting = 2;
                    break;
                case 2:
                    phase = 1;
                    if (pressed == PADRright)
                        leave = -1;
                    break;
                }
            }
            break;
        }
        SkipFrame = 2;
        EndDrawing(-2);
    }
    return leave;
}
