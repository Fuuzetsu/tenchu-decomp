#include "common.h"
#include "main.exe.h"

/*
 * ProcItemGoshikimai (0x8004333c) — the goshikimai (five-colored rice charm)
 * item processor. mode 0: freeze the owner (dispose weapon, throw animation
 * 0xF03, status 8), nudge into place, advance to mode 1; mode 1: once the
 * throw animation reaches frame 0xF, build a PARAM_ITEM_LAUNCH from the owner's
 * held-object bone (object[0xd]) and the item's stashed end-velocity, restore
 * the owner's normal stance (NowReturnNormal), dispose the item, then spawn
 * the follow-up dropped/thrown item with ReqItemDrop.
 *
 * Matching notes (see also ProcItemKusuri.c/ReqItemGoshikimai.c for the
 * item-TU conventions):
 *  - `param = &item->param.goshikimai;` is the VERY FIRST statement
 *    (before even the entry mode==ITEM_MODE_DISPOSE test): the addiu fills
 *    the entry branch's delay slot, as in ReqItemGoshikimai.
 *  - `if (mode==ITEM_MODE_DISPOSE)` (a plain if, separate statement) and the `switch
 *    (item->mode)` right after each get their OWN fresh lbu of item->mode —
 *    two total reloads, matching the switch rule (expand_case always
 *    re-reads the discriminant). The switch has NO default: mode values
 *    other than 0/1 fall out of the switch with nothing after it (straight
 *    to the epilogue, mode untouched) — a real function-level fallthrough,
 *    not a case.
 *  - `item->mode = GOSHIKIMAI_MODE_START; return;` is written OUT TWICE —
 *    once as the entry `ITEM_MODE_DISPOSE`
 *    guard, once at the end of case 1 (when `mid != 0xf03`) — not shared via
 *    a goto/post-switch tail. GCC's cross-jump pass merges the two
 *    identical copies from the `sb`/`j` backwards (the cookbook's "shared
 *    tails" rule); a shared label would merge MORE than the original since
 *    the two call sites differ.
 *  - Case 0 mirrors ProcItemKusuri's case 0 exactly (dispose/animate/status/
 *    MoveHumanoid), just different animation id (0xf03) and status (8).
 *  - `param_goshikimai.vec` is read with the SAME fresh-vs-cached asymmetry
 *    ReqItemGoshikimai writes it with: vec.vx uses
 *    `item->param.goshikimai` directly, while vec.vy/vec.vz use `param`.
 *  - `item->owner->model->object[0xd]` is recomputed in full for EACH of the
 *    three GetAbsolutePosition calls (three separate jal's in the asm, no
 *    cached model/object pointer) — Ghidra's literal repetition is the
 *    source's real shape, not a decompiler artifact.
 *  - The dispose tail reuses `ITEM_MODE_DISPOSE` (the same ITEM_MODE_DISPOSE local tested
 *    at entry) for `item->mode = ITEM_MODE_DISPOSE`, like every other ProcItem*.
 */
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemGoshikimai(struct tag_TItem *item);
 *     ITEM.C:2223, 35 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     reg   $s0       struct param_goshikimai * param
 *     reg   $s0       struct Humanoid * human
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern short ActionHalt;
 * END PSX.SYM */

void ProcItemGoshikimai(TItem *item)
{
    enum
    {
        GOSHIKIMAI_MODE_START = 0,
        GOSHIKIMAI_MODE_THROW = 1
    };
    param_goshikimai *param;
    Humanoid *human;
    MotionDataType *md;
    MotionManager *mot;
    PARAM_ITEM_LAUNCH p;

    param = &item->param.goshikimai;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = GOSHIKIMAI_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case GOSHIKIMAI_MODE_START:
        human = item->owner;
        if (ActionHalt == 0 && human->life > 0)
        {
            dispose_weapon_data_of_char_(human, 3);
            UpdateMotion(human->motion, MOT_ITEM_PLANT);
            human->status = STAT_STATE;
            md = human->motion->motion;
            MoveHumanoid(human, md->orderspd, md->sidespd);
        }
        item->mode++;
        return;

    case GOSHIKIMAI_MODE_THROW:
        mot = item->owner->motion;
        if (mot->mid != MOT_ITEM_PLANT)
        {
            item->mode = GOSHIKIMAI_MODE_START;
            return;
        }
        if (mot->count != 15)
            return;
        p.type = ITEM_GOSHIKIMAI;
        p.user = item->owner;
        p.start.vx = GetAbsolutePosition(item->owner->model->object[MODEL_PART_WEAPON_HAND_0], 0, 0, 0)->vx;
        p.start.vy = GetAbsolutePosition(item->owner->model->object[MODEL_PART_WEAPON_HAND_0], 0, 0, 0)->vy;
        p.start.vz = GetAbsolutePosition(item->owner->model->object[MODEL_PART_WEAPON_HAND_0], 0, 0, 0)->vz;
        p.end.vx = param->vec.vx;
        p.end.vy = param->vec.vy;
        p.end.vz = param->vec.vz;
        NowReturnNormal(item->owner);
        if (item->proc != 0)
        {
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != GOSHIKIMAI_MODE_START)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
        ReqItemDrop(&p);
        return;
    }
}
