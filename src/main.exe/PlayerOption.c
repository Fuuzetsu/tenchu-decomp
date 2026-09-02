#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "infoview.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PlayerOption(void);
 *     INFOVIEW.C:1144, 25 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct TAdtSelect [5] option
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern int StageID;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct NodeIndexType *FieldIndex;
 *     extern struct AreaNodeType *FieldArea;
 *     extern short ActionHalt;
 * END PSX.SYM */

extern char str_player_option[]; /* player option */ /* "player option" */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern void debug_menu_player_jump(void);
extern void StartStageSequence(void);

void PlayerOption(void)
{
    enum
    {
        RESET = 0,
        JUMP_POSITION = 1,
        RESTART_EVENT = 2,
        RESURRECT = 3
    };
    TAdtSelect option[7];

    __builtin_memcpy(option, DEBUG_MENU_PLAYER_CHOICE_OPTIONS, sizeof(option));
    switch (AdtSelect(str_player_option, option, 0))
    {
    case RESET:
        CamState.Owner->model->locate.coord.t[0] = StageConfig[StageID].px;
        CamState.Owner->model->locate.coord.t[1] = StageConfig[StageID].py - 10000;
        CamState.Owner->model->locate.coord.t[2] = StageConfig[StageID].pz;
        CamState.snap_pending = 1;
        CamState.Owner->map.index = (NodeIndexType *)GlobalAreaMap;
        FieldIndex = (NodeIndexType *)GlobalAreaMap;
        CamState.Owner->map.area = FieldArea =
            (AreaNodeType *)FieldIndex->index;
        break;
    case JUMP_POSITION:
        debug_menu_player_jump();
        break;
    case RESURRECT:
        CamState.Owner->life = CamState.Owner->lifemax;
        ActionHalt = ACTION_HALT_NONE;
        CamState.Owner->status = STAT_NORMAL;
        break;
    case RESTART_EVENT:
        leLayoutEnemy(ENEMY_LAYOUT_GAMEPLAY);
        StartStageSequence();
        break;
    }
}
