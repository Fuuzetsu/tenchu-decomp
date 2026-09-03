#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "chranim.h"
#include "images.h"
#include "score.h"
#include "infoview.h"

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern char str_stage_option[]; /* stage option */ /* "stage option" (AdtSelect title) */
extern char fmt_layout_no[]; /* layout number + kill/spot stats debug dump */
extern char str_by_rnd[];                          /* "(by rnd.)" */
extern char EMPTY_STRING[];

extern compact_stage_id CHOSEN_STAGE;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 CHOSEN_LANGUAGE;

extern void StageEndScreen(void);
extern void SelectStage(TLinkInfo *ps);

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
    ScoreStats stats;
    ScoreResult sr;

    __builtin_memcpy(menu, DEBUG_MENU_STAGE_OPTIONS, sizeof(menu));
    switch (AdtSelect(str_stage_option, menu, 0))
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
