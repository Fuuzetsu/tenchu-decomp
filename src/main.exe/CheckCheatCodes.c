#include "common.h"
#include "main.exe.h"
#include "infoview.h"
#include "item.h"
#include "sound.h"

extern char str_select_item[]; /* select item */
extern char str_number_of[];   /* number of */
/* Retail data: Left Right Left Right, Cross x2, Circle x2, Square x2,
 * Triangle x2 — the debug item-grant code. */
extern s16 CheatSeq[];
/* The retail command grew from the demo's original short [15] to 21
 * entries (20 buttons + the -1 terminator): Triangle Cross Square Circle, Cross x4, Triangle x4,
 * Square Circle Triangle Cross, Square x2, Circle x2 — the debug-mode
 * code. */
extern s16 ForbiddenCommand[21];

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern s32 memcmp(void *a, void *b, s32 n);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

void CheckCheatCodes(s16 *rec, int n)
{
    s32 sel;
    TAdtSelect menu_options[ITEM_N];

    if (memcmp(rec, CheatSeq, n << 1) == 0)
    {
        SoundEx(0, SE_MENU_CONFIRM);
        __builtin_memcpy(menu_options, DEBUG_MENU_ITEM_CHOICE_OPTIONS,
                         sizeof(DEBUG_MENU_ITEM_CHOICE_OPTIONS));
        sel = AdtSelect(str_select_item, menu_options, 0);
        __builtin_memcpy(menu_options, sel_quantity, sizeof(sel_quantity));
        CamState.Owner->item[sel] +=
            AdtSelect(str_number_of, menu_options, 0);
        SoundEx(0, SE_ITEM_USE);
    }
    else
    {
        if (memcmp(rec, ForbiddenCommand, n << 1) != 0)
        {
            return;
        }
        SystemFlag |= SYSFLAG_DEBUGMODE;
        SoundEx(0, SE_MENU_CONFIRM);
    }
}
