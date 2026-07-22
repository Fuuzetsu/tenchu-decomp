#ifndef GAME_GLOBALS_H
#define GAME_GLOBALS_H

/* Shared globals recovered from the original game's debug symbols. */
struct Humanoid;

extern long GameClock;
extern short SkipFrame;
extern int StageID;
extern AreaMapType *GlobalAreaMap;
extern short FieldAttrib;
extern unsigned char gNannido;
extern unsigned char gSound;
extern unsigned char gSoundLevel;
extern unsigned char gSELevel;
extern unsigned char gfMemory;
/* Direct global view of TLinkInfo.GameRetry at persistent-state offset 0x48. */
extern unsigned char GameRetry;
extern TSystemFlag SystemFlag;
/* Retail's mid=-1 sentinel is entry 104; the demo table had 78 entries. */
extern BattleType BattleDB[105];
extern struct Humanoid *StagePlayer;
extern short Humans;
extern short ActionHalt;
extern short EngageLevel;
extern short Criticals;
extern short Findenemies;
extern short Murders;
extern short FriendHits;
extern short StageEnemies;
extern short StageCitizens;
/* Retail adds this halfword counter beside the PSX.SYM-recorded set. */
extern short StageBosses;
extern long Distance;
extern short Degree;
extern short Attrib;
/* Raw flag-bit view for THINK.C sites whose retail loads are unsigned. */
#define ATTRIB_BITS (*(unsigned short *)&Attrib)
/* SearchTarget result/state code (-2..2), not a distance. */
extern short SR;
extern PADtype *Pad;
/* Retail stores 12 pointers. Slot 0 is NULL; stage1appearance through
 * stage9appearance retain PSX.SYM's names, while stages 10 and 11 are retail
 * additions named by the established sequence. */
extern short *StageAppearance[12];
/* Retail's stage=-1 sentinel is entry 23; the demo table had 18 entries. */
extern StageCharType StageChar[24];

#endif
