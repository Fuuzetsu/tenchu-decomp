#include "common.h"
#include "main.exe.h"

/*
 * debug_menu_stage_option (0x8005c9fc, 448 bytes) — debug menu case 5
 * ("select option" -> "stage option"): DoInfoViewProc.c's case 5 dispatches
 * here.
 *
 * MATCHED (112/112 instructions, byte-identical, first pass). Same
 * menu-copy/AdtSelect/switch family as DoInfoViewProc.c's helpers (see that
 * file's header) but a plain top-level function, not an inlined helper (its
 * menu buffer and case-2 locals never overlap: they're simply sequential
 * frame slots in ONE flat frame, not an inlined-static-helper temp-slot
 * reuse situation — no address rematerializes across calls here).
 *
 * A genuine table-switch (7 cases, default falls straight to the epilogue;
 * cases 3-6 cross-jump into a shared `sb $vN,CHOSEN_LANGUAGE` + CVAsetup()
 * tail) — see docs/matching-cookbook.md's "Split functions" section. gp
 * smalls of this TU: SystemFlag (Build.hs maspsxGpExterns + permute.py),
 * shared with the neighbouring switchD_8005bbf0/switchD_8005c6cc jump
 * tables in the same .rodata pool (0x800147xx).
 */

typedef struct { TAdtSelect e[11]; } MENU_STAGE_TBL;   /* 0x58 */

/* init_score_stats' scratch/output (byte fields + a capped-GameClock word). */
typedef struct
{
    u8 stageBosses;   /* 0x0 */
    u8 stageEnemies;  /* 0x1 */
    u8 findEnemies;   /* 0x2 */
    u8 murders;       /* 0x3 */
    u8 criticals;     /* 0x4 */
    u8 friendHits;    /* 0x5 */
    u8 pad[2];        /* 0x6 */
    s32 clock;        /* 0x8 */
} ScoreStats;          /* 0xC */

/* calculate_score's static result (all u16/s16 -> align 2, not 4). */
typedef struct
{
    u16 field0;   /* 0x0 */
    u16 field2;   /* 0x2 */
    u16 field4;   /* 0x4 */
    u16 field6;   /* 0x6 */
    s16 score;    /* 0x8 */
    u16 grade;    /* 0xA */
} ScoreResult;    /* 0xC */

#define PSTATE ((PersistentState *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern MENU_STAGE_TBL DEBUG_MENU_STAGE_OPTIONS;
extern char D_80014648[];  /* "stage option" (AdtSelect title) */
extern char D_80014658[];  /* score message format string */
extern char D_800146D0[];  /* "(by rnd.)" */
extern char EMPTY_STRING[];

extern u8 CHOSEN_STAGE;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 CHOSEN_LANGUAGE;
extern u32 SystemFlag; /* gp-relative — defined by this TU */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern void StageEndScreen(void);
extern void SelectStage(PersistentState *ps);
extern void FUN_8004f6c0(s32 arg);
extern void init_score_stats(ScoreStats *out);
extern ScoreResult *calculate_score(ScoreStats *stats, s32 stage);
extern void AdtMessageBox(char *fmt, ...);
extern void CVAsetup(void);

void debug_menu_stage_option(void)
{
    MENU_STAGE_TBL menu;
    s32 sel;
    ScoreStats stats;
    ScoreResult sr;

    menu = DEBUG_MENU_STAGE_OPTIONS;
    sel = AdtSelect(D_80014648, &menu, 0);
    switch (sel)
    {
    case 0:
        StageEndScreen();
        return;
    case 1:
        SelectStage(PSTATE);
        FUN_8004f6c0(0x11);
        return;
    case 2:
        init_score_stats(&stats);
        sr = *calculate_score(&stats, CHOSEN_STAGE);
        AdtMessageBox(D_80014658, STAGE_LAYOUT_NUMBER + 1,
                      (SystemFlag & 8) ? D_800146D0 : EMPTY_STRING,
                      stats.criticals, stats.murders, stats.findEnemies,
                      stats.friendHits, sr.score);
        return;
    case 3:
        CHOSEN_LANGUAGE = 0;
        break;
    case 4:
        CHOSEN_LANGUAGE = 1;
        break;
    case 5:
        CHOSEN_LANGUAGE = 2;
        break;
    case 6:
        CHOSEN_LANGUAGE = 3;
        break;
    default:
        return;
    }
    CVAsetup();
}
