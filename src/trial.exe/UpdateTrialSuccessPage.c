#include "common.h"
#include "results.h"
#include "telop.h"
#include "../main.exe/font.h"
#include <psxsdk/libgte.h>

const TrialResultStatFields TrialResultStats = {
    {&Criticals, &Murders, &Findenemies, &FriendHits}
};
const char path_trial_success[] = "K:\\WORK\\CDIMAGE\\TRIAL\\image\\success.arc";
const u8 TrialResultsEmptyName[] = "\x81|\x81|\x81|\x81|";

/* Static statistics draw throughout the fade. Title and leaderboard text
 * begin on the second interactive frame, with the inserted rank pulsing. */
short UpdateTrialSuccessPage(short operation)
{
    short result = 0;
    TrialResultStatFields stats = TrialResultStats;
    int i;
    u_long buttons;

    switch (operation)
    {
    case TRIAL_RESULT_PAGE_INIT:
        TrialSuccessArchive = FileRead(path_trial_success);
        TrialSuccessPanel = valloc(sizeof(TrialImagePanel));
        InitTrialImagePanel(TrialSuccessPanel);
        SetupTrialImagePanel(TrialSuccessPanel, TrialSuccessArchive, 0);
        TrialSuccessDigits[0] = CreateTrialSprite(TrialSuccessArchive, 1);
        TrialSuccessDigits[0]->h = 16;
        TrialSuccessDigits[1] = CreateTrialSprite(TrialSuccessArchive, 2);
        TrialSuccessDigits[1]->h = 16;
        TrialSuccessTitleX = -(strlen(TrialMission.header.title) * 4);
        if (Findenemies >= 999)
            Findenemies = 999;
        if (TrialRecordRank >= 0 && !TrialMission.header.records[TrialRecordRank].name.text[0])
            sprintf(TrialMission.header.records[TrialRecordRank].name.text, "%s",
                TRIAL_STATE->CharType ? "\x8d\xca\x8f\x97" : "\x97\xcd\x8a\xdb");
        _PlayMusic(MUSIC_TRACK_COMPLETE, 1);
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
        if (++TrialSuccessTextFrames > 1)
        {
            SetupTelop((u8 *)TrialMission.header.title, 0);
            draw_telop_line_(OTablePt->org, TrialSuccessTitleX, -80, (u8 *)TrialMission.header.title);
            for (i = 0; i < TRIAL_MISSION_RECORD_COUNT; i++)
            {
                if (TrialRecordRank == i)
                    TrialSuccessDigits[0]->g = TrialSuccessDigits[0]->b =
                        (rcos(GameClock * 192) * 56 >> 12) + 64;
                else
                    TrialSuccessDigits[0]->r = TrialSuccessDigits[0]->g = TrialSuccessDigits[0]->b = 128;
                DrawTrialTime(TrialSuccessDigits[0], TrialMission.header.records[i].time,
                    220, i * 16 - 52);
                if (TrialMission.header.records[i].time >= 0)
                {
                    SetupTrialColoredTelop((u8 *)TrialMission.header.records[i].name.text, i + 1, TRIAL_TELOP_WHITE);
                    draw_telop_line_(OTablePt->org, 44, i * 16 - 52,
                        (u8 *)TrialMission.header.records[i].name.text);
                }
                else
                {
                    SetupTrialColoredTelop((u8 *)TrialResultsEmptyName, i + 1, TRIAL_TELOP_WHITE);
                    draw_telop_line_(OTablePt->org, 44, i * 16 - 52, (u8 *)TrialResultsEmptyName);
                }
            }
        }
        break;
    case TRIAL_RESULT_PAGE_DRAW:
        TrialSuccessDigits[0]->r = TrialSuccessDigits[0]->g = TrialSuccessDigits[0]->b = 128;
        for (i = 0; i < 4; i++)
            DrawTrialNumber(TrialSuccessDigits[0], *stats.values[i], -106, i * 24 - 57);
        DrawTrialTime(TrialSuccessDigits[1], TrialResultTime, -96, 60);
        DrawTrialImagePanel(TrialSuccessPanel, 100);
        break;
    case TRIAL_RESULT_PAGE_DISPOSE:
        CdaStop();
        DisposeTrialImagePanel(TrialSuccessPanel);
        for (i = 0; i < 2; i++)
            vfree(TrialSuccessDigits[i]);
        vfree(TrialSuccessArchive);
        TrialSuccessTextFrames = 0;
        break;
    }
    return result;
}
