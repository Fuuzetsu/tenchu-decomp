#include "common.h"
#include "main.exe.h"
#include "score.h"
#include "infoview.h"

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

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern char str_stage_option[]; /* stage option */ /* "stage option" (AdtSelect title) */
extern char fmt_layout_no[]; /* layout number + kill/spot stats debug dump */
extern char str_by_rnd[];                          /* "(by rnd.)" */
extern char EMPTY_STRING[];

extern u8 CHOSEN_STAGE;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 CHOSEN_LANGUAGE;

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern void StageEndScreen(void);
extern void SelectStage(TLinkInfo *ps);
extern void exec_process_(s32 arg);
extern void AdtMessageBox(char *fmt, ...);
extern void CVAsetup(void);

void debug_menu_stage_option(void)
{
    /* The row values, with the labels DEBUG_MENU_STAGE_OPTIONS actually
     * ships (tools/gamedata.py DebugStageMenu). The blank rows and
     * "cancel" all carry 99 and fall through this switch untouched. */
    enum
    {
        DBGSTAGE_NEXT_STAGE = 0,
        DBGSTAGE_SELECT_STAGE = 1,
        DBGSTAGE_INFORMATION = 2,
        DBGSTAGE_ENGLISH = 3,
        DBGSTAGE_FRENCH = 4,
        DBGSTAGE_ITALIAN = 5,
        DBGSTAGE_JAPANESE = 6
    };
    TAdtSelect menu[11];
    s32 sel;
    ScoreStats stats;
    ScoreResult sr;

    __builtin_memcpy(menu, DEBUG_MENU_STAGE_OPTIONS, sizeof(menu));
    sel = AdtSelect(str_stage_option, menu, 0);
    switch (sel)
    {
    case DBGSTAGE_NEXT_STAGE:
        StageEndScreen();
        return;
    case DBGSTAGE_SELECT_STAGE:
        SelectStage(PSTATE);
        exec_process_(PROCESS_MAIN);
        return;
    case DBGSTAGE_INFORMATION:
        init_score_stats(&stats);
        sr = *calculate_score(&stats, CHOSEN_STAGE);
        AdtMessageBox(fmt_layout_no, STAGE_LAYOUT_NUMBER + 1,
                      (SystemFlag & SYSFLAG_RANDOM_LAYOUT)
                          ? str_by_rnd
                          : EMPTY_STRING,
                      stats.criticals, stats.murders, stats.findEnemies,
                      stats.friendHits, sr.score);
        return;
    case DBGSTAGE_ENGLISH:
        CHOSEN_LANGUAGE = LANG_ENGLISH;
        break;
    case DBGSTAGE_FRENCH:
        CHOSEN_LANGUAGE = LANG_FRENCH;
        break;
    case DBGSTAGE_ITALIAN:
        CHOSEN_LANGUAGE = LANG_ITALIAN;
        break;
    case DBGSTAGE_JAPANESE:
        CHOSEN_LANGUAGE = LANG_JAPANESE;
        break;
    default:
        return;
    }
    CVAsetup();
}
