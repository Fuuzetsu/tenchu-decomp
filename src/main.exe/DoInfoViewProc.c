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
extern void PutLifeBar(s32 x, s32 y, s32 life, s32 lifemax,
                       life_bar_style style);
extern int PutLifeBarS(void);
extern void PutStrain(s32 x, s32 y);
extern void PutMap(void);

static inline void ItemAddMenu(void)
{
    s32 n;
    TAdtSelect menu_options[ITEM_N];

    __builtin_memcpy(menu_options, DEBUG_MENU_ITEM_CHOICE_OPTIONS,
                     sizeof(DEBUG_MENU_ITEM_CHOICE_OPTIONS));
    n = AdtSelect(str_select_item, menu_options, 0);
    __builtin_memcpy(menu_options, sel_quantity, sizeof(sel_quantity));
    CamState.Owner->item[n] += AdtSelect(str_number_of, menu_options, 0);
}

static inline void ItemLayoutMenu(void)
{
    enum
    {
        ITEM_LAYOUT_SET = 0,
        ITEM_LAYOUT_CLEAR_ALL = 1
    };
    TAdtSelect Option[5];
    TAdtSelect OkCancel[3];

    __builtin_memcpy(Option, DEBUG_MENU_ITEM_LAYOUT_OPTIONS, sizeof(Option));
    __builtin_memcpy(OkCancel, sel_okcancel2, sizeof(OkCancel));
    switch (AdtSelect(str_item_layout_option, Option, 0))
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
    if ((SystemFlag & SYSFLAG_DEBUGMODE) && (u16)GetPad(PAD_CONTROLLER_1) == (PADL2 | PADR2))
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
    PutLifeBar(-148, 105, CamState.Owner->life, CamState.Owner->lifemax,
               LIFE_BAR_STYLE_PLAYER);
    PutLifeBarS();
    PutStrain(-134, 92);
    if ((GetPad(PAD_CONTROLLER_1) & PADselect) &&
        (SystemFlag & (SYSFLAG_DEBUGPRINT | SYSFLAG_PAUSE)) == 0)
    {
        PutMap();
    }
    else
    {
        PutMapMode = PUTMAP_OPEN;
    }
}
