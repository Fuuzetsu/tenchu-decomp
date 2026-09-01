#ifndef GAME_GLOBALS_H
#define GAME_GLOBALS_H

#include "tuning.h"

/* Shared globals recovered from the original game's debug symbols. */
struct Humanoid;

extern long GameClock;
/* SkipFrame: EndDrawing drops a frame when VSync says the last one
 * overran, and the renderers check it to skip work they can afford to
 * miss. Every screen that has just finished a long load parks it in
 * AFTER_LOAD so the first frame back is presented without being judged
 * as an overrun. */
enum
{
    SKIPFRAME_NONE = 0,
    SKIPFRAME_SKIPPED = 1,
    SKIPFRAME_AFTER_LOAD = 2
};

extern short SkipFrame;
extern int StageID;
extern AreaMapType *GlobalAreaMap;
extern MapAttribute FieldAttrib;
extern game_difficulty gNannido;
extern unsigned char gSound;
extern unsigned char gSoundLevel;
extern unsigned char gSELevel;
extern unsigned char gfMemory;
/* Direct global view of TLinkInfo.GameRetry at persistent-state offset 0x48.
 * Bit 0 says the player is replaying this stage rather than reaching it
 * fresh: StageEndScreen's retry arm and the game-over screen set it, the
 * briefing and stage-end paths clear it, and the briefing cinematic is
 * skipped while it is up. No other bit of the byte is used. */
#define GAME_RETRY_REPLAY 1
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
extern HumanoidAttribute Attrib;
/* Raw flag-bit view for THINK.C sites whose retail loads are unsigned. */
/* SearchTarget result/state code (-2..2), not a distance. The value
 * names are invented from the return contexts: */
enum
{
    SR_GONE = -2,   /* out of range or unreachable (height gap, too far) */
    SR_UNSEEN = -1, /* outside the view cone / beyond near distance */
    SR_NONE = 0,    /* no target */
    SR_SEEN = 1,    /* clear sight (inside clear_distance) */
    SR_GLIMPSE = 2  /* perceived beyond clear distance (the "?" case) */
};
extern short SR;
extern PADtype *Pad;
/* Retail stores a leading NULL followed by one pointer per stage
 * configuration. stage1appearance through stage9appearance retain PSX.SYM's
 * names; stages 10 and 11 are retail additions named by that sequence. */
#define N_STAGE_APPEARANCE_TABLES (N_STAGE_CONFIGS + 1)
extern short *StageAppearance[N_STAGE_APPEARANCE_TABLES];
/* Retail's stage=-1 sentinel is entry 23; the demo table had 18 entries. */
extern StageCharType StageChar[24];

#endif
