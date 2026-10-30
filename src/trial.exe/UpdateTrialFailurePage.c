#include "common.h"
#include "results.h"
#include "telop.h"
#include "../main.exe/font.h"

/* The failure layout uses a two-by-two statistic grid and five leaderboard
 * rows. Unlike the success page, disposal does not reset its text counter. */
short UpdateTrialFailurePage(short operation)
{
    short result = 0;
    TrialResultStatFields stats = TrialResultStats;
    int i;
    u_long buttons;

    switch (operation)
    {
    case TRIAL_RESULT_PAGE_INIT:
        TrialFailureArchive = FileRead("K:\\WORK\\CDIMAGE\\TRIAL\\image\\fail.arc");
        TrialFailurePanel = valloc(sizeof(TrialImagePanel));
        InitTrialImagePanel(TrialFailurePanel);
        SetupTrialImagePanel(TrialFailurePanel, TrialFailureArchive, 0);
        TrialFailureDigits = CreateTrialSprite(TrialFailureArchive, 1);
        TrialFailureDigits->h = 16;
        TrialFailureTitleX = -(strlen(TrialMission.header.title) * 4);
        if (Findenemies >= 999)
            Findenemies = 999;
        _PlayMusic(MUSIC_TRACK_GAMEOVER, 0);
        break;
    case TRIAL_RESULT_PAGE_INPUT:
        buttons = TrialResultsButtons;
        TrialResultsButtons = GetRealPad(0);
        TrialResultsNewButtons = buttons = (buttons ^ TrialResultsButtons) & TrialResultsButtons;
        if (buttons == PADRright)
        {
            result = 1;
            SoundEx(NULL, TRIAL_SOUND_ACCEPT);
        }
        else if (buttons == PADstart)
        {
            result = -1;
            SoundEx(NULL, TRIAL_SOUND_CANCEL);
        }
        if (++TrialFailureTextFrames > 1)
        {
            SetupTelop((u8 *)TrialMission.header.title, 0);
            draw_telop_line_(OTablePt->org, TrialFailureTitleX, -83, (u8 *)TrialMission.header.title);
            for (i = 0; i < 5; i++)
            {
                DrawTrialTime(TrialFailureDigits, TrialMission.header.records[i].time, 10, i * 16 + 27);
                if (TrialMission.header.records[i].time >= 0)
                {
                    SetupTrialColoredTelop((u8 *)TrialMission.header.records[i].name.text, i + 1, TRIAL_TELOP_WHITE);
                    draw_telop_line_(OTablePt->org, -166, i * 16 + 27,
                        (u8 *)TrialMission.header.records[i].name.text);
                }
                else
                {
                    SetupTrialColoredTelop((u8 *)TrialResultsEmptyName, i + 1, TRIAL_TELOP_WHITE);
                    draw_telop_line_(OTablePt->org, -166, i * 16 + 27, (u8 *)TrialResultsEmptyName);
                }
            }
        }
        break;
    case TRIAL_RESULT_PAGE_DRAW:
        for (i = 0; i < 4; i++)
            DrawTrialNumber(TrialFailureDigits, *stats.values[i], (i % 2) * 226 - 86,
                i >= 2 ? -12 : -35);
        DrawTrialTime(TrialFailureDigits, TrialResultTime, 49, -55);
        DrawTrialImagePanel(TrialFailurePanel, 100);
        break;
    case TRIAL_RESULT_PAGE_DISPOSE:
        CdaStop();
        DisposeTrialImagePanel(TrialFailurePanel);
        vfree(TrialFailureDigits);
        vfree(TrialFailureArchive);
        break;
    }
    return result;
}
