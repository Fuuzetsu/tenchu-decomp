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

extern char str_stage_option[]; /* stage option */  /* "stage option" (AdtSelect title) */
extern char fmt_layout_no[];  /* score message format string */
extern char str_by_rnd[]; /* "(by rnd.)" */
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
    TAdtSelect menu[11];
    s32 sel;
    ScoreStats stats;
    ScoreResult sr;

    __builtin_memcpy(menu, DEBUG_MENU_STAGE_OPTIONS, sizeof(menu));
    sel = AdtSelect(str_stage_option, menu, 0);
    switch (sel)
    {
    case 0:
        StageEndScreen();
        return;
    case 1:
        SelectStage(PSTATE);
        exec_process_(0x11);
        return;
    case 2:
        init_score_stats(&stats);
        sr = *calculate_score(&stats, CHOSEN_STAGE);
        AdtMessageBox(fmt_layout_no, STAGE_LAYOUT_NUMBER + 1,
                      (SystemFlag & SYSFLAG_RANDOM_LAYOUT)
                          ? str_by_rnd : EMPTY_STRING,
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
