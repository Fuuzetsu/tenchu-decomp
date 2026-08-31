#include "common.h"
#include "main.exe.h"
#include "infoview.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

/*
 * CheckCheatCodes (0x8004b354) — matches the just-entered button record
 * `rec` (n halfwords) against two hidden cheat sequences. If it equals the
 * first (CheatSeq), it plays a chime and opens the SAME debug item-cheat
 * menus DoInfoViewProc's ItemAddMenu uses (pick an item, then a count) and
 * adds the chosen count to the current character's carried-item stock. If it
 * equals the second (`ForbiddenCommand`), it sets `SYSFLAG_DEBUGMODE`. Either match
 * plays SoundEx(0, seid) at the end (seid = 0x4c for the item cheat, 10 for
 * the flag cheat); the item cheat ALSO plays SoundEx(0,10) up front.
 *
 * The two menu tables + the `CamState.Owner->item[]` `+=` idiom are verbatim
 * from DoInfoViewProc.c's ItemAddMenu (same TU family):
 * the fixed-size copy from DEBUG_MENU_ITEM_CHOICE_OPTIONS copies the 0xC8
 * table as one block move (emit_block_move 16-byte loop + 8-byte
 * tail), and the second AdtSelect's result is added to item[sel] where sel is
 * the first AdtSelect's result (captured into the callee-saved reg in the
 * second call's delay slot).
 *
 * Matching notes:
 *  - Both cheat paths end in SoundEx(0, seid) but written as TWO explicit
 *    per-branch calls (`SoundEx(0, 0x4c)` / `SoundEx(0, 10)`), NOT a shared
 *    `SoundEx(0, seid)` after the if/else. cross-jump merges only the common
 *    `jal SoundEx` tail (leaving the delay slot a nop), so each branch
 *    materialises its own `a0 = 0`; the shared-call form instead hoists a
 *    single `a0 = 0` into the merged jal's delay slot (17-byte diff).
 *  - The item pointer is reached through the recovered `CamState.Owner`
 *    field.  Its nonzero member access gives the target's separate base and
 *    destination registers without inventing a second object at +0x10.
 */
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

void CheckCheatCodes(s16 *rec, int n)
{
    s32 sel;
    union
    {
        TAdtSelect ItemName[25];
        TAdtSelect Num[4];
    } menu;

    if (memcmp(rec, CheatSeq, n << 1) == 0)
    {
        SoundEx(0, SE_MENU_CONFIRM);
        __builtin_memcpy(menu.ItemName, DEBUG_MENU_ITEM_CHOICE_OPTIONS,
                         sizeof(DEBUG_MENU_ITEM_CHOICE_OPTIONS));
        sel = AdtSelect(str_select_item, menu.ItemName, 0);
        __builtin_memcpy(menu.Num, sel_quantity, sizeof(sel_quantity));
        CamState.Owner->item[sel] +=
            AdtSelect(str_number_of, menu.Num, 0);
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
