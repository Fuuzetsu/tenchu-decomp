#ifndef ENDING_FRONTEND_H
#define ENDING_FRONTEND_H

#include "ending.exe.h"

enum
{
    ENDING_SKIP_BUTTONS = PADRright | PADRdown | PADRleft,
    ENDING_FADE_MAX = 128
};

extern short EndingWideMode;
extern Sprite3D *EndingFadeSprite;
extern void (*EndingDrawCallback)(int);

void DrawEndingBackground(short brightness);
BackGround *LoadEndingBackground(char *path);
void FadeEndingScreen(int step, int hold_frames);
void PlayEndingMovie(char *path, int frames);
void ClearEndingPages(void);
void SyncMovieFrame(void);

int open_stream(char *path, int x, int y, int read_mode, short rgb24);
void close_stream(void);
int DecodeMovieFrame(void);

#endif
