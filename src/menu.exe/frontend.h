#ifndef MENU_FRONTEND_H
#define MENU_FRONTEND_H

#include "menu.exe.h"

enum
{
    MENU_SKIP_BUTTONS = PADstart | PADRright | PADRdown | PADRleft,
    MENU_FADE_MAX = 128
};

extern short MenuWideMode;
extern Sprite3D *MenuFadeSprite;
extern void (*MenuDrawCallback)(int);

void DrawMenuBackground(short brightness);
BackGround *LoadMenuBackground(char *path);
void FadeMenuScreen(int step, int hold_frames);
void PlayOpeningMovie(char *path, int frames);
void ClearMenuPages(void);
void EmptyMenuCallback(int unused);

int open_stream(char *path, int x, int y, int read_mode, short rgb24);
void close_stream(void);
int DecodeMovieFrame(void);

#endif
