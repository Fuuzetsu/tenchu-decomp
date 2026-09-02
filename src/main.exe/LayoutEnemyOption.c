#include "common.h"
#include "main.exe.h"
#include "infoview.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void LayoutEnemyOption(void);
 *     INFOVIEW.C:849, 55 src lines, frame 192 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct TAdtSelect [11] ItemName
 *     stack sp+104    struct TAdtSelect [3] OkCancel
 *     stack sp+128    struct TAdtSelect [7] ItemName
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern int CurrentEnemyID;
 *     extern short Humans;
 * END PSX.SYM */

extern char str_enemy_layout_option[]; /* "enemy layout option" */
extern char msg_clear_ok_2[]; /* "clear ok?" */
extern char str_path_layout_option[]; /* "path layout option" */
extern char fmt_layout_enemies[]; /* "layout %d enemies" */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern void AddEnemy(void);
extern void leClearLayout(void);
extern void leAddPath(enemy_layout_index id, s32 x, s32 y, s32 z);
extern void leResetPath(enemy_layout_index id);
extern void SelectCameraOwnerOption(void); /* AdtMessageBox comes from item.h */

void LayoutEnemyOption(void)
{
    enum
    {
        ADD = 0,
        REMOVE = 1,
        RESET = 2,
        GO = 3,
        CLEAR = 4,
        SET_PATH = 5,
        REPORT = 6,
        CAMERA = 7
    };
    enum
    {
        ENEMY_PATH_SELECT = 0,
        ENEMY_PATH_ADD = 1,
        ENEMY_PATH_RESET = 2
    };
    s32 n;
    s32 k;
    TAdtSelect ItemName[11];
    TAdtSelect OkCancel[3];

    __builtin_memcpy(ItemName, DEBUG_MENU_ENEMY_LAYOUT_OPTIONS,
                     sizeof(ItemName));
    __builtin_memcpy(OkCancel, sel_okcancel, sizeof(OkCancel));
    n = AdtSelect(str_enemy_layout_option, ItemName, 0);
    if ((n & 0xFFFF) != 0xFFFF)
    {
        switch ((s16)n)
        {
        case ADD:
            AddEnemy();
            break;
        case REMOVE:
            leRemoveEnemy();
            break;
        case RESET:
            leLayoutEnemy(ENEMY_LAYOUT_EDIT);
            break;
        case GO:
            leLayoutEnemy(ENEMY_LAYOUT_GAMEPLAY);
            break;
        case CLEAR:
            if (AdtSelect(msg_clear_ok_2, OkCancel, 1) == 1)
            {
                leClearLayout();
            }
            break;
        case SET_PATH:
        {
            TAdtSelect ItemName[7];

            __builtin_memcpy(ItemName,
                             DEBUG_MENU_ENEMY_PATH_SETTING_OPTIONS,
                             sizeof(ItemName));
            k = (s16)AdtSelect(str_path_layout_option, ItemName, 0);
            if (k != ADT_SELECT_CANCEL)
            {
                switch (k)
                {
                case ENEMY_PATH_ADD:
                    leAddPath(CurrentEnemyID,
                              CamState.Owner->model->locate.coord.t[0],
                              CamState.Owner->model->locate.coord.t[1],
                              CamState.Owner->model->locate.coord.t[2]);
                    break;
                case ENEMY_PATH_RESET:
                    leResetPath(CurrentEnemyID);
                    break;
                case ENEMY_PATH_SELECT:
                    CurrentEnemyID = leFindEnemy();
                    break;
                }
            }
        }
        break;
        case REPORT:
            AdtMessageBox(fmt_layout_enemies, Humans);
            break;
        case CAMERA:
            SelectCameraOwnerOption();
            break;
        }
    }
}
