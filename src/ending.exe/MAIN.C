#include "common.h"
#include "ending.h"
#include "frontend.h"
#include "../main.exe/adt.h"
#include "../main.exe/font.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/sound.h"
#include "../main.exe/tuning.h"
#include <psxsdk/libcd.h>
#include <psxsdk/libgpu.h>
#include <psxsdk/libsnd.h>

extern char path_rikimaru_ending[];
extern char path_ayame_ending[];
extern char path_ending_credits[];
extern char fmt_ending_movie_path[];
extern char fmt_ending_voice_bank[];
extern char *EndingVoiceDirectories[N_LANGUAGES];

int main(void)
{
    /* Retail reserves these unused stack bytes before the clear rectangle.
     * No surviving access identifies the original local's type or purpose. */
    u8 unused[32];
    RECT clear_rect;
    int phase = 1;
    short selection;

    AdtPadRead = GetRealPad;
    ResetCallback();
    InitPadControl();
    InitFileSystem(READ_SOURCE_CDROM);
    CdInit();
    CdaStatus.flag = CDA_FLAG_ACTIVE;
    SetDispMask(0);
    InitSoundEffect();
    InitGraphicsSystem();
    setRECT(&clear_rect, 0, 0, 640, 512);
    ENDING_STATE->GameRetry |= ENDING_UNLOCK_EXTRA_LANGUAGES;
    ClearImage(&clear_rect, 0, 0, 0);
    DrawSync(0);
    ClearEndingPages();
    initialise_font();

    do
    {
        switch (phase)
        {
        case 1:
            LoadEndingSubtitles(ENDING_CHARACTER_SUBTITLES);
            PlayEndingSequence(ENDING_STATE->CharType, 0);
            phase = 2;
            break;
        case 2:
            LoadEndingSubtitles(ENDING_CREDITS_SUBTITLES);
            PlayEndingSequence(ENDING_CREDITS_MOVIE, 1);
            phase = 3;
            break;
        case 3:
            selection = RunEndingResults();
            phase = 4;
            break;
        }
    } while (phase != 4);

    GsInitGraph2(SCREEN_W, SCREEN_H, 4, 1, 0);
    GsInit3D();
    ClearEndingPages();
    SetDispMask(0);
    if (selection)
    {
        ENDING_STATE->GameRetry |= ENDING_RETRY_CURRENT_STAGE;
        exec_process_(PROCESS_MAIN);
    }
    else
    {
        ENDING_STATE->layout = 0xff;
        exec_process_(PROCESS_MENU);
    }
}

void PlayEndingSequence(u8 movie, u_short rgb24)
{
    RECT rect;
    char movie_path[64];
    char voice_path[64];
    int frames;
    u8 skippable;
    char character;
    int frame;

    SetDispMask(0);
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
    setRECT(&rect, 0, 0, 480, 480);
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    GsInitGraph2(SCREEN_W, SCREEN_H, 4, 0, rgb24);
    ClearEndingPages();

    /* The character epilogue must finish; the following credits may be
     * skipped with Start or Circle. The caller supplies only these ids. */
    switch (movie)
    {
    case RIKIMARU_0:
        frames = ENDING_CHARACTER_FRAMES;
        skippable = 0;
        sprintf(movie_path, fmt_ending_movie_path, path_rikimaru_ending);
        character = 'R';
        break;
    case AYAME_0:
        frames = ENDING_CHARACTER_FRAMES;
        skippable = 0;
        sprintf(movie_path, fmt_ending_movie_path, path_ayame_ending);
        character = 'A';
        break;
    case ENDING_CREDITS_MOVIE:
        frames = ENDING_CREDITS_FRAMES;
        skippable = 1;
        sprintf(movie_path, fmt_ending_movie_path, path_ending_credits);
        character = 'C';
        break;
    }
    sprintf(voice_path, fmt_ending_voice_bank,
            EndingVoiceDirectories[CHOSEN_LANGUAGE], character);
    EndingVoices = SetupSE(FileRead(voice_path));
    DrawSync(0);
    open_stream(movie_path, 0, 0, 0x1c0, rgb24);
    SetDispMask(1);
    GameClock = 0;
    do
    {
        StartDrawing();
        frame = DecodeMovieFrame();
        UpdateEndingSubtitles(frame, EndingVoices);
        VSync(0);
        GsDrawOt(OTablePt);
        DrawSync(0);
        GsSwapDispBuff();
    } while (frame < frames &&
             (!skippable ||
              !((EndingButtons = GetRealPad(0)) & ENDING_CREDITS_SKIP_BUTTONS)));
    SsSetMVol(0, 0);
    close_stream();
    DisposeSE(EndingVoices);
    EndingVoices = 0;
    GsInitGraph2(SCREEN_W, SCREEN_H, 4, 1, 0);
    GsInit3D();
    ClearEndingPages();
    SetDispMask(0);
}

/* ENDING string storage, in retail object order. */
char ending_voice_japanese[] = "K:\\WORK\\CDIMAGE\\SOUND\\";
char ending_voice_italian[] = "K:\\WORK\\CDIMAGE\\SOUND\\ITALIAN\\";
char ending_voice_french[] = "K:\\WORK\\CDIMAGE\\SOUND\\FRENCH\\";
char ending_voice_english[] = "K:\\WORK\\CDIMAGE\\SOUND\\ENGLISH\\";
char path_rikimaru_ending[] = "\\TENCHU\\MOVIE\\ENDRIKI.STR";
char path_ayame_ending[] = "\\TENCHU\\MOVIE\\ENDAYA.STR";
char path_ending_credits[] = "\\TENCHU\\MOVIE\\ENDING.STR";
char fmt_ending_voice_bank[] = "%sEND%c.VAB";
