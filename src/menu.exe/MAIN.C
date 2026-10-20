#include "common.h"
#include "menu.exe.h"
#include "../main.exe/adt.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/sound.h"
#include <psxsdk/libcd.h>
#include <psxsdk/libgpu.h>

enum
{
    MENU_CANCEL = -1,
    MENU_NEW_GAME = 0,
    MENU_TRAINING = 1,
    MENU_MISSION_EDITOR = 2,
    MENU_OPTIONS = 3,
    MENU_VISITED = 2,
    PERSISTENT_STATE_MAGIC = 0x19981110,
    OPENING_MOVIE_FRAMES = 1879,
    TRAINING_MOVIE_FRAMES = 247
};

extern char path_opening_movie[];
extern char path_training_movie[];

void InitMenuPersistentState(void);
void DrawMenuBackground(short brightness);
void RunStartupCardCheck(void);
void PlayOpeningMovie(char *path, int frames);
short RunTitleMenu(void);
short SelectMenuCharacter(void);
short SelectMenuMission(void);
void LoadMovieSubtitles(short stage);
void PlaySubtitledMovie(char *path, int frames, u_short interlace);
void RunOptionsMenu(void);

int main(void)
{
    short opening_count = -2;
    short selection;
    RECT clear_rect;

    if (MENU_STATE->GameRetry == 0 || MENU_STATE->magic != PERSISTENT_STATE_MAGIC)
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
    MENU_STATE->GameRetry |= MENU_VISITED;

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
