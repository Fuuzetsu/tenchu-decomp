#ifndef GAME_GLOBALS_H
#define GAME_GLOBALS_H

/* Shared globals recovered from the original game's debug symbols. */
struct Humanoid;

extern long GameClock;
extern int StageID;
extern unsigned long *GlobalAreaMap;
extern BattleType BattleDB[78];
extern struct Humanoid *StagePlayer;
extern short Humans;
extern short *StageAppearance[10];
extern StageCharType StageChar[18];

#endif
