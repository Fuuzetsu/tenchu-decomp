#include "common.h"
#include "main.exe.h"

/*
 * ProcItemLaunch (0x80047048) — the launched/thrown item processor (grenade
 * family). Every frame: fly it (MoveFly), tick the arming countdown (at 0
 * register a 300-unit conflict box), spin it (rotate.vy = GameClock * 0x2AA),
 * mirror the coordinate into the shared model and draw + afterimage. If a
 * live character is in the conflict box: SetImpact + SoundEx and dispose.
 * Otherwise, once `param->fly.mode` is armed, dispatch on the rolling
 * overlay's `status`: KORO_WATER = plain dispose; KORO_WALL = detonate
 * (SetBleeds/SoundEx 0x31 + reset_alert_duration); KORO_GRAND/KORO_STAY =
 * drop a pickup (build a
 * PARAM_ITEM_LAUNCH at the model's position, dispose, ReqItemDrop).
 *
 * Matching notes (this is ProcItemHappou's skeleton — see that file for the
 * conflict-box and countdown conventions; all deltas verified):
 *  - The scratch model is `item->model` here (Happou draws into its gp-global
 *    HappouModel instead).
 *  - The spin block's three stores are inline (`item->locate->rotate.v* =`);
 *    each reloads item->locate (the in-struct stores invalidate the cached
 *    load). GameClock * 0x2AA is the plain literal multiply (shift/add
 *    chain), truncated by the sh.
 *  - `switch (param->fly.p.koro.status)` has bodies in source order
 *    KORO_WALL, KORO_GRAND/KORO_STAY, KORO_WATER; KORO_WATER is the
 *    shared `dispose:` label the impact path reaches by `goto` — its body
 *    (and the KORO_GRAND/KORO_STAY copy) are literal duplicates, NOT
 *    cross-jumped (they
 *    have different continuations).
 *  - The drop path builds `param` through a pointer (`p = &param;
 *    memset(p, ...)`) but stores fields DIRECT (`param.type = ...`, sp-folded),
 *    then copies with `rparam = *p;` — the *p spelling is what lets the
 *    0x28-byte block copy's source cursor coalesce with p ($s0) across the
 *    KORO_GRAND/KORO_STAY join label (a `rparam = param` spelling
 *    re-materializes the address). ReqItemDrop takes &rparam (the copy, not
 *    the original).
 *  - `rparam` is declared before `param` (slot order 0x18/0x40, matching
 *    PSX.SYM's rparam@24/param@72 declaration order).
 */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemLaunch(struct tag_TItem *item);
 *     ITEM.C:3089, 83 src lines, frame 128 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s0       struct tag_TItem * item
 *     reg   $s2       struct ModelType * model
 *     reg   $s1       struct param_launch * param
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     stack sp+24     struct PARAM_ITEM_STAY rparam
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct PARAM_ITEM_STAY * p
 *     stack sp+72     struct PARAM_ITEM_LAUNCH param
 *     reg   $s0       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s1       struct Humanoid * m
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern long GameClock;
 * END PSX.SYM */

#include "item.h"
#include "afterimage.h"

extern void MoveFly(TItem *item, param_fly *param);
extern short DrawModel(ModelType *objp);
extern s16 InsertConflict(ModelType *m);
extern s16 GetConflictResult(ModelType *m, s32 n);
extern s32 is_character_state_present_on_stage_(Humanoid *h);
extern void reset_alert_duration(void);

void ProcItemLaunch(TItem *item)
{
    ModelType *model;
    param_launch *param;
    u8 t;
    s32 cid;
    s32 n;
    PARAM_ITEM_LAUNCH *p;
    PARAM_ITEM_LAUNCH rparam;

    model = item->model;
    param = &item->param.launch;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        DisposeAfterimage(param->effect);
        item->mode = 0;
        return;
    }
    MoveFly(item, &param->fly);
    t = param->count - 1;
    param->count = t;
    if (t == 0)
    {
        DeleteConflict(item->locate);
        n = InsertConflict(item->locate);
        ConflictObject[n].offset.vx = 0;
        ConflictObject[n].offset.vz = 0;
        ConflictObject[n].offset.vy = 0;
        ConflictObject[n].size.vz = 300;
        ConflictObject[n].size.vy = 300;
        ConflictObject[n].size.vx = 300;
        ConflictObject[n].common = (void *)1;
        ConflictObject[n].size.pad = 1;
        item->collision.size = 300;
        item->collision.ofsY = 0;
        item->collision.mode = 1;
        item->collision.pause = 0;
    }
    item->locate->rotate.vx = 0;
    item->locate->rotate.vy = GameClock * 0x2aa;
    item->locate->rotate.vz = 0;
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawModel(model);
    DrawAfterimage(param->effect, 1);
    if ((item->locate->attribute & 0x8000) == 0)
        cid = -1;
    else
        cid = GetConflictResult(item->locate, -1);
    if (cid != -1 && is_character_state_present_on_stage_(ConflictObject[cid].common) != 0)
    {
        SetImpact((VECTOR *)item->locate->locate.coord.t, 0x4000, 2);
        SoundEx((VECTOR *)item->locate->locate.coord.t, 0x30);
        goto dispose;
    }
    if (param->fly.mode == 0)
        return;
    switch (param->fly.p.koro.status)
    {
    case KORO_WALL:
        SetBleeds((VECTOR *)item->locate->locate.coord.t, 0, 0x19, 0xa, 0xa, 0xffff00);
        SoundEx((VECTOR *)item->locate->locate.coord.t, 0x31);
        reset_alert_duration();
        return;

    case KORO_GRAND:
    case KORO_STAY:
    {
        PARAM_ITEM_LAUNCH param;

        p = &param;
        memset(p, 0, sizeof(PARAM_ITEM_LAUNCH));
        param.type = item->type;
        param.user = item->owner;
        param.start.vx = model->locate.coord.t[0];
        param.start.vy = model->locate.coord.t[1];
        param.start.vz = model->locate.coord.t[2];
        rparam = *p;
        if (item->proc != 0)
        {
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
        ReqItemDrop(&rparam);
        return;
    }

    case KORO_WATER:
    dispose:
        if (item->proc != 0)
        {
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
        return;
    }
}
