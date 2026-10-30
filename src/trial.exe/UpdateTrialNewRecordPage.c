#include "common.h"
#include "results.h"
#include "../main.exe/images.h"
#include "../main.exe/graphics.h"
#include <psxsdk/libgte.h>

extern ArcFile *TrialNewRecordArchive;
extern BackGround *TrialNewRecordBackground;
extern GsSPRITE *TrialNewRecordDigits;

/* Circle accepts name entry; Start keeps the default name. The selected
 * protagonist's backdrop determines the number and time positions. */
short UpdateTrialNewRecordPage(short operation)
{
    short result = 0;
    char path[128];
    const char *backgrounds[] = {"image\\newriki.tim", "image\\newaya.tim"};
    u_long *tim;
    u_long buttons;

    switch (operation)
    {
    case TRIAL_RESULT_PAGE_INIT:
        sprintf(path, "%s%s", "K:\\WORK\\CDIMAGE\\TRIAL\\", backgrounds[TRIAL_STATE->CharType]);
        tim = FileRead(path);
        TrialNewRecordBackground = load_background_(tim);
        vfree(tim);
        TrialNewRecordArchive = FileRead("K:\\WORK\\CDIMAGE\\TRIAL\\image\\numfont.arc");
        TrialNewRecordDigits = CreateTrialSprite(TrialNewRecordArchive, 0);
        TrialNewRecordDigits->h = 16;
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
        break;
    case TRIAL_RESULT_PAGE_DRAW:
        TrialNewRecordDigits->r = TrialNewRecordDigits->g = TrialNewRecordDigits->b = 128;
        DrawTrialNumber(TrialNewRecordDigits, TrialRecordRank + 1,
            TRIAL_STATE->CharType ? -102 : -120, TRIAL_STATE->CharType ? -15 : -30);
        TrialNewRecordDigits->g = TrialNewRecordDigits->b =
            (rcos(GameClock << 7) * 56 >> 12) + 64;
        DrawTrialTime(TrialNewRecordDigits, TrialResultTime,
            TRIAL_STATE->CharType ? -22 : -40, TRIAL_STATE->CharType ? 10 : -4);
        DrawBG(TrialNewRecordBackground);
        break;
    case TRIAL_RESULT_PAGE_DISPOSE:
        DisposeBG(TrialNewRecordBackground);
        vfree(TrialNewRecordDigits);
        vfree(TrialNewRecordArchive);
        break;
    }
    return result;
}
