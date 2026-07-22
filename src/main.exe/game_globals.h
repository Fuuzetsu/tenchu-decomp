#ifndef GAME_GLOBALS_H
#define GAME_GLOBALS_H

/* Shared globals recovered from the original game's debug symbols. */
struct Humanoid;

extern long GameClock;
extern short SkipFrame;
extern int StageID;
extern unsigned long *GlobalAreaMap;
extern unsigned char gNannido;
extern unsigned char gSound;
extern unsigned char gSoundLevel;
extern unsigned char gSELevel;
extern unsigned char gfMemory;
extern BattleType BattleDB[78];
extern struct Humanoid *StagePlayer;
extern short Humans;
extern short ActionHalt;
extern short EngageLevel;
/* SearchTarget result/state code (-2..2), not a distance. */
extern short SR;
extern PADtype *Pad;
/* Retail stores 12 pointers; the demo PSX.SYM table had 10. */
extern short *StageAppearance[12];
extern StageCharType StageChar[18];

#endif
