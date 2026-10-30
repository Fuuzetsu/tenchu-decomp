#include "common.h"
#include "mission_menu.h"
#include "../main.exe/images.h"
#include "../main.exe/font.h"
#include "../main.exe/music.h"
#include "../main.exe/sound.h"
#include <psxsdk/libgpu.h>

/* Show the mission's objective and records before loading its stage. */
short ConfirmTrialMission(void)
{
    char names[TRIAL_PREVIEW_RECORDS][10];
    char times[TRIAL_PREVIEW_RECORDS][12];
    TrialObjectiveLabels objectives;
    u_long previous_buttons = 0;
    short accepted = 2;
    short phase = 1;
    short brightness = 128;
    short cue;
    u_long pressed;
    int i;
    int seconds;
    GsSPRITE *sprite;

    objectives = TrialObjectiveText;
    ClearTrialDisplay(320, 480);
    ResetTrialConfirmationGraphics();
    LoadTrialConfirmationGraphics();
    for (i = 0; i < TRIAL_PREVIEW_RECORDS; i++)
    {
        if (TrialMission.header.records[i].time < 0)
        {
            sprintf(names[i], TrialEmptyRecordName);
            sprintf(times[i], TrialEmptyRecordTime);
        }
        else
        {
            __builtin_memcpy(names[i], TrialMission.header.records[i].name.text, 10);
            seconds = TrialMission.header.records[i].time / 30;
            if (seconds / 60 < 60)
                sprintf(times[i], TrialRecordTimeFormat,
                        0x82, seconds / 600 + 0x4f,
                        0x82, seconds / 60 % 10 + 0x4f,
                        0x82, seconds % 60 / 10 + 0x4f,
                        0x82, seconds % 60 % 10 + 0x4f);
            else
                sprintf(times[i], TrialMaximumRecordTime);
        }
    }
    SetDispMask(1);
    DrawSync(0);
    VSync(0);
    while (phase != 0)
    {
        StartDrawing();
        switch (phase)
        {
        case 1:
            brightness -= 16;
            if (brightness <= 0)
            {
                brightness = 0;
                phase++;
                previous_buttons = GetRealPad(0);
            }
            DrawTrialScreenFade(brightness, 320, 240, 0);
            break;
        /* Settle for two updates before starting narration. */
        case 2:
        case 3:
            phase++;
            break;
        case 4:
            cue = GetTrialMissionNarrationCue();
            if (cue >= 0)
                _PlayMusic(cue, 0);
            phase++;
            break;
        case 5:
            pressed = ReadTrialMenuPad(0, &previous_buttons, 0);
            if (pressed == PADRdown)
            {
                accepted = 0;
                phase++;
                SoundEx(NULL, TRIAL_SOUND_CANCEL);
                CdaStop();
            }
            else if (pressed == PADRright)
            {
                accepted = 1;
                phase++;
                SoundEx(NULL, TRIAL_SOUND_ACCEPT);
                CdaStop();
                TrialConfirmationSprites[TRIAL_CONFIRM_CONTROLS]->sprite.attribute |= GS_ATTR_HIDDEN;
            }
            break;
        /* Keep drawing for seven updates after confirmation/cancellation. */
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            phase++;
            break;
        default:
            phase = 0;
            break;
        }
        DrawBG(TrialConfirmationBackground);
        for (i = 0; i < TRIAL_CONFIRM_STEALTH_LABEL; i++)
            GsSortSprite(&TrialConfirmationSprites[i]->sprite, OTablePt, 10);
        if (TrialMission.header.flags & TRIAL_REQUIRE_STEALTH)
        {
            sprite = &TrialConfirmationSprites[TRIAL_CONFIRM_STEALTH_LABEL]->sprite;
            sprite->r = sprite->g = sprite->b =
                128 + ((rcos(GameClock << 7) << 5) >> 12);
            GsSortSprite(&TrialConfirmationSprites[TRIAL_CONFIRM_STEALTH_LABEL]->sprite, OTablePt, 10);
        }
        SetupTelop(TrialMission.header.title, 0);
        draw_telop_line_(OTablePt->org, -88, -70, TrialMission.header.title);
        SetupTelop(TrialMission.header.author, 1);
        draw_telop_line_(OTablePt->org, -72, -45, TrialMission.header.author);
        SetupTelop(objectives.text[TrialMission.header.flags & TRIAL_OBJECTIVE_MASK], 2);
        draw_telop_line_(OTablePt->org, -72, -20,
                  objectives.text[TrialMission.header.flags & TRIAL_OBJECTIVE_MASK]);
        for (i = 0; i < TRIAL_PREVIEW_RECORDS; i++)
        {
            SetupTelop(names[i], i + 3);
            draw_telop_line_(OTablePt->org, -74, 15 + i * 20, names[i]);
            SetupTelop(times[i], i + 6);
            draw_telop_line_(OTablePt->org, 10, 15 + i * 20, times[i]);
        }
        SkipFrame = 2;
        EndDrawing(-2);
    }
    if (accepted)
        SetupTrialMission(TRIAL_STATE->CharType != 0, 1);
    FadeOutDirect(32, 2, 8, 8, 8);
    SetDispMask(0);
    DisposeTrialConfirmationGraphics();
    ClearTrialDisplay(320, 480);
    return accepted;
}
