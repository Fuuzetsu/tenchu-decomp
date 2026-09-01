#include "common.h"
#include "main.exe.h"
#include "infoview.h"
#include "item.h"
#include "misc.h"
#include "padcmd.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DoInfoViewProc(void);
 *     INFOVIEW.C:1413, 132 src lines, frame 344 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       long trig
 *     stack sp+32     struct TAdtSelect [12] Option
 *     reg   $v1       int i
 *     stack sp+128    struct TAdtSelect [4] Num
 *     stack sp+128    struct TAdtSelect [25] ItemName
 *     reg   $s0       int i
 *     stack sp+168    struct TAdtSelect [3] OkCancel
 *     stack sp+128    struct TAdtSelect [5] Option
 *     reg   $v1       int i
 *     stack sp+128    struct TAdtSelect [5] option
 *     reg   $v1       int c
 *     reg   $v1       int c
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern unsigned char fInitialize;
 *     extern enum TSystemFlag SystemFlag;
 *     extern short ItemCursor;
 *     extern long GameClock;
 *     extern unsigned char PutMapMode;
 * END PSX.SYM */

/*
 * DoInfoViewProc (0x8004ba5c) — per-frame in-game HUD/info processor: debug
 * menu dispatch (once the cheat sets `SYSFLAG_DEBUGMODE`, holding exactly
 * L2+R2 opens "select option"), item-cursor cycling on pad trig bits,
 * PauseProc gate,
 * item list / life bars / strain draw, and Select-held minimap.
 *
 * Matching notes (all verified against the original bytes; see
 * docs/matching-cookbook.md):
 *  - The debug-menu case bodies are STATIC INLINE HELPERS — see the comment
 *    at the helpers below; this is what makes the menu-buffer addresses
 *    rematerialize per call and the buffers overlap (temp-slot reuse).
 *  - The outer dispatch switches directly on AdtSelect's return value.
 *    Case 2/1's AdtSelect results still go to a separate variable (the
 *    helpers' `n`, $s0): carrying one result variable across the case-body
 *    calls promotes it to a callee-saved reg (one extra prologue save,
 *    +1 shift everywhere).
 *  - ItemLayoutMenu's inner dispatch is a nested 2-case switch: expand_case
 *    lays out tests-then-bodies (beqz / li 1 / beq / j end) where an
 *    if/else-if chain would put the first body on the fallthrough path and
 *    let cse reuse the compare's constant 1 for the AdtSelect mode arg
 *    (target rematerializes `li a2,1` in the branch-taken block).
 *  - Each item-cycle arm has PSX.SYM's block-local `c`: `i = CURR; c = i;
 *    do { i--; wrap; } while (item[i] == 0 && i != c);`. Reorg steals the
 *    top-of-loop decrement into the conditional backjump's delay slot,
 *    retargets the branch past it, and compensates (+1) on the fallthrough
 *    exit. Loading into `i` first and copying to `c` puts the lh in i's
 *    register (move a0,v1).
 *  - gp smalls of this TU: fInitialize,
 *    ItemCursor, PutMapMode (Build.hs maspsxGpExterns +
 *    permute.py). VISIBLE_ENEMIES_/GameClock/SystemFlag/str_opt are other
 *    TUs' — plain absolute externs.
 */

extern s16 VISIBLE_ENEMIES_;
/* gp-relative — defined by this (info-view) TU; Build.hs maspsxGpExterns */
extern u8 fInitialize;

extern char str_select_item[]; /* "select item" */
extern char str_number_of[]; /* "number of" */
extern char str_item_layout_option[]; /* "item layout option" */
extern char msg_clear_ok[]; /* "clear ok?" */
extern char str_select_option[]; /* "select option" */
extern char str_opt[]; /* "opt" — the effect-menu title */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern void InitializeInfoView(void);
extern void LayoutEnemyOption(void);
extern void AddItem2(void);
extern void ClearItemLayout(void);
extern void FileOption(void);
extern void PlayerOption(void);
extern void debug_menu_stage_option(void);
extern void PauseProc(void);
extern void PutItemList(void);
extern void PutLifeBar(s32 x, s32 y, s32 life, s32 lifemax, s32 mode);
extern int PutLifeBarS(void);
extern void PutStrain(s32 x, s32 y);
extern void PutMap(void);

/*
 * The three debug-menu case bodies are static inline helpers: each menu
 * buffer is the helper's own first local, so its address is the inlined
 * frame base itself (a plain register at expand time) and every AdtSelect
 * call re-materializes `addiu $a1,$sp,N` directly. Written as plain locals
 * of DoInfoViewProc, the same-valued addresses get forced into pseudos
 * (calls.c precompute) and CSE'd into a callee-saved temp — one extra
 * s-register and a +1-instruction prologue. Inline expansion also allocates
 * the helper frames as freed-and-reused temp slots, which is why the item
 * menu (0xC8 @ sp+0x78), the layout+confirm pair (0x28+0x18, reusing
 * sp+0x78/sp+0xA0) and the effect menu (0xF8, too big for the freed slot,
 * fresh at sp+0x140) overlap exactly as the original frame does.
 */
static inline void ItemAddMenu(void)
{
    s32 n;
    union
    {
        TAdtSelect ItemName[25];
        TAdtSelect Num[4];
    } menu;

    __builtin_memcpy(menu.ItemName, DEBUG_MENU_ITEM_CHOICE_OPTIONS,
                     sizeof(DEBUG_MENU_ITEM_CHOICE_OPTIONS));
    n = AdtSelect(str_select_item, menu.ItemName, 0);
    __builtin_memcpy(menu.Num, sel_quantity, sizeof(sel_quantity));
    CamState.Owner->item[n] += AdtSelect(str_number_of, menu.Num, 0);
}

static inline void ItemLayoutMenu(void)
{
    enum
    {
        ITEM_LAYOUT_SET = 0,
        ITEM_LAYOUT_CLEAR_ALL = 1
    };
    s32 n;
    TAdtSelect Option[5];
    TAdtSelect OkCancel[3];

    __builtin_memcpy(Option, DEBUG_MENU_ITEM_LAYOUT_OPTIONS, sizeof(Option));
    __builtin_memcpy(OkCancel, sel_okcancel2, sizeof(OkCancel));
    n = AdtSelect(str_item_layout_option, Option, 0);
    switch (n)
    {
    case ITEM_LAYOUT_SET:
        AddItem2();
        break;
    case ITEM_LAYOUT_CLEAR_ALL:
        if (AdtSelect(msg_clear_ok, OkCancel, 1) == 1)
        {
            ClearItemLayout();
        }
        break;
    }
}

static inline void EffectSpawnMenu(void)
{
    TAdtSelect Option[31];

    __builtin_memcpy(Option, DEBUG_MENU_HIDDEN_EFFECT_SPAWN_OPTIONS,
                     sizeof(Option));
    AddMisc(MISC_DOOR, CamState.Owner->model->locate.coord.t[0],
            CamState.Owner->model->locate.coord.t[1],
            CamState.Owner->model->locate.coord.t[2],
            CamState.Owner->model->rotate.vy,
            AdtSelect(str_opt, Option, 0), 0);
}

void DoInfoViewProc(void)
{
    enum
    {
        ENEMY = 0,
        ITEM = 1,
        CHARGE = 2,
        FILE = 3,
        PLAYER = 4,
        STAGE = 5,
        HIDDEN_EFFECT = 0x63
    };
    u16 pad;
    long trig;
    s32 i;
    TAdtSelect Option[11];

    pad = CamState.Owner->pad.data;
    trig = CamState.Owner->pad.trig;
    if (fInitialize == 0)
    {
        InitializeInfoView();
    }
    if ((SystemFlag & SYSFLAG_DEBUGMODE) && (u16)GetPad(0) == (PADL2 | PADR2))
    {
        __builtin_memcpy(Option, DEBUG_MENU_MAIN_SCREEN_OPTIONS,
                         sizeof(Option));
        VISIBLE_ENEMIES_ = 0;
        switch (AdtSelect(str_select_option, Option, 0))
        {
        case ENEMY:
            LayoutEnemyOption();
            break;
        case CHARGE:
            ItemAddMenu();
            break;
        case ITEM:
            ItemLayoutMenu();
            break;
        case FILE:
            FileOption();
            break;
        case PLAYER:
            PlayerOption();
            break;
        case STAGE:
            debug_menu_stage_option();
            break;
        case HIDDEN_EFFECT:
            EffectSpawnMenu();
            break;
        }
    }

    if ((pad & PADRup) == 0)
    {
        if ((trig & PADR2) != 0)
        {
            s32 c;

            i = ItemCursor;
            c = i;
            do
            {
                i--;
                if (i < 0)
                    i = ITEM_N;
            } while (CamState.Owner->item[i] == 0 && i != c);
        }
        else if ((trig & PADL2) != 0)
        {
            s32 c;

            i = ItemCursor;
            c = i;
            do
            {
                i++;
                if (i > ITEM_N)
                    i = 0;
            } while (CamState.Owner->item[i] == 0 && i != c);
        }
        else
        {
            goto nosel;
        }
        ItemCursor = i;
        SoundEx(0, SE_UI_CURSOR);
    }
nosel:
    if (GameClock > 10)
    {
        PauseProc();
    }
    PutItemList();
    PutLifeBar(-148, 105, CamState.Owner->life, CamState.Owner->lifemax, 0);
    PutLifeBarS();
    PutStrain(-134, 92);
    if ((GetPad(0) & PADselect) &&
        (SystemFlag & (SYSFLAG_DEBUGPRINT | SYSFLAG_PAUSE)) == 0)
    {
        PutMap();
    }
    else
    {
        PutMapMode = PUTMAP_OPEN;
    }
}
