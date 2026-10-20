#include "common.h"
#include "menu.exe.h"
#include "frontend.h"
#include "cardmenu.h"
#define MENU_DEFINE_PERSISTENT_RESET
#include "persistent.h"
#include "../main.exe/adt.h"
#include "../main.exe/font.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/item.h"
#include "../main.exe/sound.h"
#include "../main.exe/tuning.h"
#include "../main.exe/vmemory.h"
#include <psxsdk/libcd.h>
#include <psxsdk/libgpu.h>
#include <psxsdk/libsnd.h>

enum
{
    MENU_CANCEL = -1,
    MENU_NEW_GAME = 0,
    MENU_TRAINING = 1,
    MENU_MISSION_EDITOR = 2,
    MENU_OPTIONS = 3,
    OPENING_MOVIE_FRAMES = 1879,
    TRAINING_MOVIE_FRAMES = 247
};

/* Movie CAD rows use the same twelve-byte records as character animation,
 * but command 5 plays a voice instead of setting a camera pose. */
enum
{
    MOVIE_CMD_WAIT = 1,
    MOVIE_CMD_VOICE = 5,
    MOVIE_CMD_SUBTITLE = 8,
    MOVIE_SUBTITLE_CLEAR = -1,
    MOVIE_SUBTITLE_Y = 212
};

typedef struct MovieCommand
{
    s16 command;
    s16 value;
    s16 sound;
    s16 reserved[3];
} MovieCommand;

extern char path_opening_movie[];
extern char path_training_movie[];
extern u8 *MovieSubtitleData;
extern MovieCommand *MovieCommandCursor;
extern u8 MovieSubtitleText[];
extern char *MovieSubtitleDirectories[N_LANGUAGES];
extern char *MovieVoiceDirectories[N_LANGUAGES];
extern char fmt_movie_subtitles[];
extern char fmt_movie_voice_bank[];

void InitMenuPersistentState(void);
void DrawMenuBackground(short brightness);
void RunStartupCardCheck(void);
void PlayOpeningMovie(char *path, int frames);
short RunTitleMenu(void);
short SelectMenuCharacter(void);
short SelectMenuMission(void);
void LoadMovieSubtitles(short stage);
void PlaySubtitledMovie(char *path, int frames, u_short rgb24);
void RunOptionsMenu(void);
short ProcessMovieCommands(SoundEffect *voices);
void GsInitGraph2(unsigned short width, unsigned short height,
                 unsigned short mode, unsigned short dither,
                 unsigned short rgb24);

int main(void)
{
    short opening_count = -2;
    short selection;
    RECT clear_rect;

    if (MENU_STATE->GameRetry == 0 || MENU_STATE->magic != MENU_STATE_MAGIC)
        InitMenuPersistentState();

    AdtPadRead = GetRealPad;
    ResetCallback();
    InitPadControl();
    InitFileSystem(READ_SOURCE_CDROM);
    CdInit();
    CdaStatus.flag = CDA_FLAG_ACTIVE;
    InitSoundEffect();
    SetDispMask(0);
    InitGraphicsSystem();

    clear_rect.x = 0;
    clear_rect.y = 0;
    clear_rect.w = 640;
    clear_rect.h = 512;
    ClearImage(&clear_rect, 0, 0, 0);
    DrawSync(0);
    StartDrawing();
    DrawMenuBackground(128);
    EndDrawing(0);
    SetupSoundEffect(0, 9);

    if (MENU_STATE->GameRetry == 0)
    {
        RunStartupCardCheck();
        opening_count = -1;
    }
    MENU_STATE->GameRetry |= MENU_STATE_VISITED;

    for (;;)
    {
        VSync(2);
        if (++opening_count >= 0)
            PlayOpeningMovie(path_opening_movie, OPENING_MOVIE_FRAMES);

        while ((selection = RunTitleMenu()) != MENU_CANCEL)
        {
            if (selection == MENU_NEW_GAME)
            {
                while ((selection = SelectMenuCharacter()) != MENU_CANCEL)
                {
                    MENU_STATE->CharType = selection;
                    if (SelectMenuMission() != MENU_CANCEL)
                        exec_process_(PROCESS_MAIN);
                }
            }
            else if (selection == MENU_TRAINING)
            {
                selection = SelectMenuCharacter();
                if (selection != MENU_CANCEL)
                {
                    MENU_STATE->CharType = selection;
                    MENU_STATE->StageNo = StageOrder[0];
                    MENU_STATE->layout = 0;
                    LoadMovieSubtitles(14);
                    PlaySubtitledMovie(path_training_movie, TRAINING_MOVIE_FRAMES, 0);
                    exec_process_(PROCESS_MAIN);
                }
            }
            else if (selection == MENU_MISSION_EDITOR)
            {
                exec_process_(PROCESS_TRIAL);
            }
            else if (selection == MENU_OPTIONS)
            {
                RunOptionsMenu();
            }
        }
    }
}

short ProcessMovieCommands(SoundEffect *voices)
{
    while (MovieCommandCursor->command != MOVIE_CMD_WAIT)
    {
        switch (MovieCommandCursor->command)
        {
        case CVA_CMD_SEQUENCE:
            break;
        case MOVIE_CMD_VOICE:
            PlaySEHigh(voices, MovieCommandCursor->sound, 127);
            break;
        case MOVIE_CMD_SUBTITLE:
            if (MovieCommandCursor->value == MOVIE_SUBTITLE_CLEAR)
            {
                MovieSubtitleText[0] = 0;
            }
            else
            {
                SetupTelop((u8 *)strcpy((char *)MovieSubtitleText,
                                      (char *)MovieSubtitleData +
                                          MovieCommandCursor->value), 0);
            }
            break;
        }
        MovieCommandCursor++;
    }
    return MovieCommandCursor->value != 0;
}

static inline short DrawMovieSubtitles(int frame, SoundEffect *voices)
{
    int x;
    GsOT_TAG *ot;

    if (!ProcessMovieCommands(voices))
        return 0;
    if (MovieSubtitleText[0])
    {
        int width = telop_text_width_(MovieSubtitleText);

        x = -(width / 2);
        x += SCREEN_W / 2;
        ot = OTablePt->org;
        draw_telop_line_(ot, x, MOVIE_SUBTITLE_Y, MovieSubtitleText);
    }
    if (frame >= MovieCommandCursor->value)
        MovieCommandCursor++;
    return 1;
}

void PlaySubtitledMovie(char *path, int frames, u_short rgb24)
{
    RECT rect;
    /* Retail reserves these unused stack bytes before the filename.
     * No surviving access identifies the original local's type or purpose. */
    u8 unused[64];
    char filename[96];
    SoundEffect *voices;
    int frame;

    SetDispMask(0);
    GsInitGraph2(SCREEN_W, SCREEN_H, 4, 0, rgb24);
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
    setRECT(&rect, 0, 0, 640, 512);
    ClearImage(&rect, 0, 0, 0);
    ClearMenuPages();
    initialise_font();
    SetupSoundEffect(0, -1);
    sprintf(filename, fmt_movie_voice_bank,
            MovieVoiceDirectories[CHOSEN_LANGUAGE]);
    voices = SetupSE(FileRead(filename));
    DrawSync(0);
    open_stream(path, 0, 0, 0x1c0, rgb24);
    GameClock = 0;
    SetDispMask(1);
    do
    {
        StartDrawing();
        frame = DecodeMovieFrame();
        DrawMovieSubtitles(frame, voices);
        VSync(0);
        GsDrawOt(OTablePt);
        DrawSync(0);
        GsSwapDispBuff();
    } while (frame < frames && !(GetRealPad(0) & MENU_SKIP_BUTTONS));
    SsSetMVol(0, 0);
    close_stream();
    DisposeSE(voices);
    ClearMenuPages();
    GsInitGraph2(SCREEN_W, SCREEN_H, 4, 1, 0);
    GsInit3D();
    ClearMenuPages();
    SetDispMask(0);
}

void RunStartupCardCheck(void)
{
    /* Retail leaves the first previous-button value uninitialised. */
    int buttons;
    int previous;
    int pressed;

    initialise_font();
    do
    {
        StartDrawing();
        previous = buttons;
        buttons = GetRealPad(0);
        pressed = 0;
        if (previous == 0 && buttons != 0)
            pressed = buttons;
        pressed = UpdateStartupCardCheck(pressed);
        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(2);
    } while (pressed == 0);
}

/* Draft: argument promotion and address loads are scheduled differently. */
#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/menu.exe/asm/nonmatchings/MAIN", LoadMovieSubtitles);
#else
void LoadMovieSubtitles(short stage)
{
    char filename[64];
    int stage_number;

    if (MovieSubtitleData)
        vfree(MovieSubtitleData);
    stage_number = stage;
    sprintf(filename, fmt_movie_subtitles,
            MovieSubtitleDirectories[CHOSEN_LANGUAGE], stage_number, 'R');
    MovieSubtitleData = FileRead(filename);
    MovieCommandCursor = (MovieCommand *)MovieSubtitleData;
    MovieSubtitleText[0] = 0;
}
#endif

short UpdateMovieSubtitles(int frame, SoundEffect *voices)
{
    return DrawMovieSubtitles(frame, voices);
}
