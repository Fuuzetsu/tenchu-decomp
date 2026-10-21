#ifndef TENCHU_ENDING_H
#define TENCHU_ENDING_H

#include "ending.exe.h"

enum
{
    ENDING_CHARACTER_SUBTITLES = 12,
    ENDING_CREDITS_SUBTITLES = 13,
    ENDING_CREDITS_MOVIE = 2,
    ENDING_CHARACTER_FRAMES = 627,
    ENDING_CREDITS_FRAMES = 5295,
    ENDING_CREDITS_SKIP_BUTTONS = PADstart | PADRright,
    ENDING_RESULTS_PRIORITY = 2,
    ENDING_RETRY_CURRENT_STAGE = 1,
    ENDING_UNLOCK_EXTRA_LANGUAGES = 0x20
};

extern SoundEffect *EndingVoices;
extern u_short EndingButtons;
extern u_short EndingPreviousButtons;

short ProcessEndingCommands(void);
void LoadEndingSubtitles(short stage);
short UpdateEndingSubtitles(int frame, SoundEffect *voices);
void PlayEndingSequence(u8 movie, u_short rgb24);
short RunEndingResults(void);
void DrawEndingTime(GsSPRITE *sprite, s32 time, s32 x, s32 y);
void DrawEndingNumber(GsSPRITE *sprite, s16 value, s16 x, s16 y);
void InitEndingScoreSprite(GsSPRITE *sprite, u_long *tim, int x, int y,
                           int brightness, u_long attributes);
void SaveEndingResults(void);
void GsInitGraph2(unsigned short width, unsigned short height,
                 unsigned short mode, unsigned short dither,
                 unsigned short rgb24);

#endif
