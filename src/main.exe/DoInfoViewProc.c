#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * DoInfoViewProc (0x8004ba5c) — per-frame in-game HUD/info processor: debug
 * menu dispatch (once the cheat sets SystemFlag&2, holding exactly L2+R2 opens
 * "select option"), item-cursor cycling on pad trig bits, PauseProc gate,
 * item list / life bars / strain draw, and Select-held minimap.
 *
 * Matching notes (all verified against the original bytes; see
 * docs/matching-cookbook.md):
 *  - The debug-menu case bodies are STATIC INLINE HELPERS — see the comment
 *    at the helpers below; this is what makes the menu-buffer addresses
 *    rematerialize per call and the buffers overlap (temp-slot reuse).
 *  - The outer dispatch is a real switch on `sel` used ONLY for dispatch
 *    (caller-saved $v1). Case 2/1's AdtSelect results go to a separate
 *    variable (the helpers' `n`, $s0): reusing the switch variable across
 *    the case-body calls promotes it to a callee-saved reg (one extra
 *    prologue save, +1 shift everywhere).
 *  - ItemLayoutMenu's inner dispatch is a nested 2-case switch: expand_case
 *    lays out tests-then-bodies (beqz / li 1 / beq / j end) where an
 *    if/else-if chain would put the first body on the fallthrough path and
 *    let cse reuse the compare's constant 1 for the AdtSelect mode arg
 *    (target rematerializes `li a2,1` in the branch-taken block).
 *  - The item-cycle loops are `i = CURR; cur = i; do { i--; wrap; } while
 *    (item[i] == 0 && i != cur);` — reorg steals the top-of-loop decrement
 *    into the conditional backjump's delay slot, retargets the branch past
 *    it, and compensates (+1) on the fallthrough exit. Loading into `i`
 *    first and copying to `cur` puts the lh in i's register (move a0,v1).
 *  - `(s16)CamState.Owner->lifemax` forces the signed lh the original has —
 *    this TU's view disagrees with the item TU's u16 (ProcItemKusuri lhu's
 *    the same field).
 *  - gp smalls of this TU: fInitialize (fInitialize),
 *    CURRENTLY_SELECTED_ITEM_KIND_1_, PutMapMode (Build.hs maspsxGpExterns +
 *    permute.py). VISIBLE_ENEMIES_/GameClock/SystemFlag/D_80097C40 are other
 *    TUs' — plain absolute externs.
 */

typedef struct { debug_menu_choice e[11]; } MENU_MAIN_TBL;     /* 0x58 */
typedef struct { debug_menu_choice e[25]; } MENU_ITEM_TBL;     /* 0xC8 */
typedef struct { debug_menu_choice e[5];  } MENU_LAYOUT_TBL;   /* 0x28 */
typedef struct { debug_menu_choice e[4];  } MENU_COUNT_TBL;    /* 0x20 */
typedef struct { debug_menu_choice e[3];  } MENU_CONFIRM_TBL;  /* 0x18 */
typedef struct { debug_menu_choice e[31]; } MENU_EFFECT_TBL;   /* 0xF8 */


typedef struct
{
    VECTOR TargetVector;         /* 0x00 */
    Humanoid *Owner;             /* 0x10 */
    s32 Mode;                    /* 0x14 */
    s16 DirectionRX;             /* 0x18 */
    s16 DirectionRY;             /* 0x1A */
    s32 OldMode;                 /* 0x1C */
    u8 Valiation;                /* 0x20 */
} TCameraStatus;

extern TCameraStatus CamState;
extern u32 SystemFlag;
extern s32 GameClock;
extern s16 VISIBLE_ENEMIES_;
/* gp-relative — defined by this (info-view) TU; Build.hs maspsxGpExterns */
extern u8 fInitialize;                       /* fInitialize */
extern s16 CURRENTLY_SELECTED_ITEM_KIND_1_;
extern u8 PutMapMode;                       /* PutMap latch */

extern MENU_MAIN_TBL DEBUG_MENU_MAIN_SCREEN_OPTIONS;
extern MENU_ITEM_TBL DEBUG_MENU_ITEM_CHOICE_OPTIONS;
extern MENU_LAYOUT_TBL DEBUG_MENU_ITEM_LAYOUT_OPTIONS;
extern MENU_EFFECT_TBL DEBUG_MENU_HIDDEN_EFFECT_SPAWN_OPTIONS;
extern MENU_COUNT_TBL D_800124CC;
extern MENU_CONFIRM_TBL D_8001252C;
extern char D_800124C0[];                   /* "select item" */
extern char D_800124EC[];                   /* "number of" */
extern char D_80012544[];                   /* "item layout option" */
extern char D_80012558[];                   /* "clear ok?" */
extern char D_800125F0[];                   /* "select option" */
extern char D_80097C40[];                   /* effect-menu title buffer */

extern s32 AdtSelect(char *title, debug_menu_choice *menu, s32 mode);
extern s32 GetPad(s32 n);
extern void InitializeInfoView(void);
extern void LayoutEnemyOption(void);
extern void DebugMenuItemSet(void);
extern void ClearItemLayout(void);
extern void FileOption(void);
extern void PlayerOption(void);
extern void debug_menu_stage_option(void);
extern void AddMisc(s32 kind, s32 x, s32 y, s32 z, s32 ry, s32 n, s32 mode);
extern void PauseProc(void);
extern void PutItemList(void);
extern void PutLifeBar(s32 x, s32 y, s32 life, s32 lifemax, s32 mode);
extern void PutLifeBarS(void);
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
    u8 mi[0xC8];

    *(MENU_ITEM_TBL *)mi = DEBUG_MENU_ITEM_CHOICE_OPTIONS;
    n = AdtSelect(D_800124C0, (debug_menu_choice *)mi, 0);
    *(MENU_COUNT_TBL *)mi = D_800124CC;
    CamState.Owner->item[n] += AdtSelect(D_800124EC, (debug_menu_choice *)mi, 0);
}

static inline void ItemLayoutMenu(void)
{
    s32 n;
    u8 mi[0x40];

    *(MENU_LAYOUT_TBL *)mi = DEBUG_MENU_ITEM_LAYOUT_OPTIONS;
    *(MENU_CONFIRM_TBL *)(mi + 0x28) = D_8001252C;
    n = AdtSelect(D_80012544, (debug_menu_choice *)mi, 0);
    switch (n)
    {
    case 0:
        DebugMenuItemSet();
        break;
    case 1:
        if (AdtSelect(D_80012558, (debug_menu_choice *)(mi + 0x28), 1) == 1)
        {
            ClearItemLayout();
        }
        break;
    }
}

static inline void EffectSpawnMenu(void)
{
    MENU_EFFECT_TBL me;

    me = DEBUG_MENU_HIDDEN_EFFECT_SPAWN_OPTIONS;
    AddMisc(1, CamState.Owner->model->locate.coord.t[0],
            CamState.Owner->model->locate.coord.t[1],
            CamState.Owner->model->locate.coord.t[2],
            CamState.Owner->model->rotate.vy,
            AdtSelect(D_80097C40, (debug_menu_choice *)&me, 0), 0);
}

void DoInfoViewProc(void)
{
    u16 pad;
    u16 trig;
    s32 sel;
    s32 i;
    s32 cur;
    MENU_MAIN_TBL mm;

    pad = CamState.Owner->pad.data;
    trig = CamState.Owner->pad.trig;
    if (fInitialize == 0)
    {
        InitializeInfoView();
    }
    if ((SystemFlag & 2) && (u16)GetPad(0) == 3)
    {
        mm = DEBUG_MENU_MAIN_SCREEN_OPTIONS;
        VISIBLE_ENEMIES_ = 0;
        sel = AdtSelect(D_800125F0, &mm, 0);
        switch (sel)
        {
        case 0:
            LayoutEnemyOption();
            break;
        case 2:
            ItemAddMenu();
            break;
        case 1:
            ItemLayoutMenu();
            break;
        case 3:
            FileOption();
            break;
        case 4:
            PlayerOption();
            break;
        case 5:
            debug_menu_stage_option();
            break;
        case 0x63:
            EffectSpawnMenu();
            break;
        }
    }

    if ((pad & 0x10) == 0)
    {
        if ((trig & 2) != 0)
        {
            i = CURRENTLY_SELECTED_ITEM_KIND_1_;
            cur = i;
            do
            {
                i--;
                if (i < 0)
                    i = 0x19;
            } while (CamState.Owner->item[i] == 0 && i != cur);
        }
        else if ((trig & 1) != 0)
        {
            i = CURRENTLY_SELECTED_ITEM_KIND_1_;
            cur = i;
            do
            {
                i++;
                if (0x19 < i)
                    i = 0;
            } while (CamState.Owner->item[i] == 0 && i != cur);
        }
        else
        {
            goto nosel;
        }
        CURRENTLY_SELECTED_ITEM_KIND_1_ = i;
        SoundEx(0, 0xB);
    }
nosel:
    if (10 < GameClock)
    {
        PauseProc();
    }
    PutItemList();
    PutLifeBar(-0x94, 0x69, CamState.Owner->life, (s16)CamState.Owner->lifemax, 0);
    PutLifeBarS();
    PutStrain(-0x86, 0x5C);
    if ((GetPad(0) & 0x100) && (SystemFlag & 5) == 0)
    {
        PutMap();
    }
    else
    {
        PutMapMode = 0;
    }
}
