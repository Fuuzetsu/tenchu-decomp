#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PlayerOption(void);
 *     INFOVIEW.C:1144, 25 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * frame-local buffer (same wrapper-struct-assignment idiom as
 * DoInfoViewProc's debug-menu helpers — see docs/matching-cookbook.md),
 * shows it with AdtSelect, then dispatches on the chosen index:
 *   0 reset position — warp Owner to StageConfig[]'s start coords and reset
 *                       the cached area-map node/index (also mirrored onto
 *                       two still-unnamed Humanoid fields at +0x30/+0x34)
 *   1 jump position  — debug_menu_player_jump()
 *   2 restart event  — leLayoutEnemy(1) + StartStageSequence()
 *   3 raise dead     — full-heal + clear ActionHalt + status
 * (table indices 4 "" and 5 "cancel" are inert: no case matches them, no
 * default).
 *
 * Matching notes (verified; see docs/matching-cookbook.md):
 *  - PSX.SYM's shared TStageConfig has px/py/pz/pr at
 *    0xC/0x10/0x14/0x18 after uid+name+path; the raw StageConfig[] bytes
 *    confirm all of those fields in retail.
 *  - Humanoid+0x30/+0x34 (Ghidra: field10_0x30/field14_0x34, inside item.h's
 *    still-opaque Humanoid.pad0) mirror FieldArea/FieldIndex per-character.
 *    Accessed here through raw offset casts rather than extending the
 *    shared item.h: this is the only function touching them so far and
 *    their real field names/types aren't proven.
 *  - The menu-title string ("player option") and the table itself sit in a
 *    shared debug-menu data blob far from this function's own code (like
 *    DoInfoViewProc's D_800125F0 etc.) — referenced by address, not
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
 *    and Humanoid+0x34 writes, not before them (Ghidra's statement order put
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
 *  - The final store folds the FieldArea assignment into the store
 *    expression (`*(...) = FieldArea = (AreaNodeType *)FieldIndex->index;`)
 *    rather than two statements: with two statements cc1 scheduled the
 *    independent Owner-reload (for the store's LHS) and the FieldArea load
 *    in the opposite order from the original. The chained form is
 *    semantically identical but flips that scheduling tie.
 */

typedef struct { TAdtSelect e[7]; } MENU_PLAYER_TBL;   /* 0x38 */


extern MENU_PLAYER_TBL DEBUG_MENU_PLAYER_CHOICE_OPTIONS;
extern char D_80014784[];                    /* "player option" */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern void debug_menu_player_jump(void);
extern void StartStageSequence(void);

void PlayerOption(void)
{
    s32 n;
    MENU_PLAYER_TBL mm;

    mm = DEBUG_MENU_PLAYER_CHOICE_OPTIONS;
    n = AdtSelect(D_80014784, &mm, 0);
    switch (n)
    {
    case 0:
        CamState.Owner->model->locate.coord.t[0] = StageConfig[StageID].px;
        CamState.Owner->model->locate.coord.t[1] = StageConfig[StageID].py - 10000;
        CamState.Owner->model->locate.coord.t[2] = StageConfig[StageID].pz;
        CamState.CriticalHit = 1;
        *(void **)((u8 *)CamState.Owner + 0x34) = GlobalAreaMap;
        FieldIndex = (NodeIndexType *)GlobalAreaMap;
        *(AreaNodeType **)((u8 *)CamState.Owner + 0x30) =
            FieldArea = (AreaNodeType *)FieldIndex->index;
        break;
    case 1:
        debug_menu_player_jump();
        break;
    case 3:
        CamState.Owner->life = CamState.Owner->lifemax;
        ActionHalt = 0;
        CamState.Owner->status = 0;
        break;
    case 2:
        leLayoutEnemy(1);
        StartStageSequence();
        break;
    }
}
