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

/*
 * PlayerOption (0x8005ce70) — debug menu case 4 (DoInfoViewProc.c): the
 * player-character submenu. Copies the fixed player-option menu table into a
 * frame-local array (the original TAdtSelect shape recovered from PSX.SYM),
 * shows it with AdtSelect, then dispatches on the chosen index:
 *   RESET            — warp Owner to StageConfig[]'s start coords and reset
 *                      the cached area-map node/index
 *   JUMP_POSITION    — debug_menu_player_jump()
 *   RESTART_EVENT    — gameplay enemy rebuild + StartStageSequence()
 *   RESURRECT        — full-heal + clear ActionHalt + status
 * (table indices 4 "" and 5 "cancel" are inert: no case matches them, no
 * default).
 *
 * Matching notes (verified; see docs/matching-cookbook.md):
 *  - PSX.SYM's shared TStageConfig has px/py/pz/pr at
 *    0xC/0x10/0x14/0x18 after uid+name+path; the raw StageConfig[] bytes
 *    confirm all of those fields in retail.
 *  - Humanoid+0x30/+0x34 are the retail additions `map.area` and
 *    `map.index`. GetAreaMapVector proves both members and the corresponding
 *    eight-byte growth from PSX.SYM's original MapVector.
 *  - The menu-title string ("player option") and the table itself sit in a
 *    shared debug-menu data blob far from this function's own code (like
 *    DoInfoViewProc's str_select_option etc.) — referenced by address, not
 *    embedded as a fresh C literal (a literal would land in this object's
 *    own .rodata, not the original's fixed address).
 *  - The dispatch is a real switch (dense case set 0..3, no default), case
 *    bodies laid out in SOURCE order 0,1,3,2 (not numeric 0,1,2,3) — case 2
 *    is written last so it falls through into the epilogue with no trailing
 *    jump, matching the target's instruction count exactly.
 *  - `StageConfig[StageID]` is read three times literally (.px/.py/.pz), no
 *    explicit index temp: cse merges the repeated array-address computation
 *    into one load of StageID + one multiply-by-0x1C, matching Ghidra's
 *    rendering (plain global for the first field, a synthesized `iVar1` for
 *    the other two — that's Ghidra naming the same cached value two ways,
 *    not two different source expressions).
 *  - `FieldIndex = (NodeIndexType *)GlobalAreaMap;` sits AFTER the OldMode
 *    and `Owner->map.index` writes, not before them (Ghidra's statement order put
 *    it first, textually adjacent to the comment describing it — a
 *    decompiler artifact, not the source's real position). Moving this one
 *    assignment two statements later fixed a whole-block register swap
 *    (CamState's address vs StageID's value fighting over $a0/$a1, from the
 *    very first coordinate store onward) even though FieldIndex itself has
 *    no data dependency on the coordinate writes — local-alloc's tie-break
 *    for a basic block depends on the WHOLE block's statement order, so a
 *    register tie manifesting early can require reordering something later
 *    in the same block. Found via decomp-permuter (a mid-search low-score
 *    candidate under output-30-1/ showed the exact reorder before the
 *    search converged to 0 on its own).
 *  - The final store folds the FieldArea assignment into the store expression
 *    (`CamState.Owner->map.area = FieldArea =
 *    FieldIndex->index.nodes;`)
 *    rather than two statements: with two statements cc1 scheduled the
 *    independent Owner-reload (for the store's LHS) and the FieldArea load
 *    in the opposite order from the original. The chained form is
 *    semantically identical but flips that scheduling tie.
 */

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
            FieldIndex->index.nodes;
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
