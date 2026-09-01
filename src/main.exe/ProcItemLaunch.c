#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "effect.h"

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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
extern s32 is_humanoid_on_stage_(Humanoid *h);
extern void reset_alert_duration(void);

void ProcItemLaunch(TItem *item)
{
    ModelType *model;
    param_launch *param;
    u8 t;
    s32 cid;
    s32 conflict_id;
    PARAM_ITEM_LAUNCH *p;
    PARAM_ITEM_LAUNCH rparam;

    model = item->model.object;
    param = &item->param.launch;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        DisposeAfterimage(param->effect);
        item->mode = ITEM_MODE_START;
        return;
    }
    MoveFly(item, &param->fly);
    t = param->count - 1;
    param->count = t;
    if (t == 0)
    {
        DeleteConflict(item->locate);
        conflict_id = InsertConflict(item->locate);
        SET_ITEM_COLLISION(conflict_id, 300, CONFLICT_OWNER_ITEM,
                           CONFLICT_HIT);
    }
    item->locate->rotate.vx = 0;
    item->locate->rotate.vy = GameClock * 0x2aa;
    item->locate->rotate.vz = 0;
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawModel(model);
    DrawAfterimage(param->effect, 1);
    if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        cid = CONFLICT_NONE;
    else
        cid = GetConflictResult(item->locate, CONFLICT_NONE);
    if (cid != CONFLICT_NONE &&
        is_humanoid_on_stage_(ConflictObject[cid].common.human) != 0)
    {
        SetImpact((VECTOR *)item->locate->locate.coord.t, 4 * FIXED_ONE,
                  IMPACT_SPRITE_HIT);
        SoundEx((VECTOR *)item->locate->locate.coord.t, SE_PROJECTILE_HIT);
        goto dispose;
    }
    if (param->fly.mode == FLY_MODE_ARC)
        return;
    switch (param->fly.p.koro.status)
    {
    case KORO_WALL:
        SetBleeds((VECTOR *)item->locate->locate.coord.t, 0, 25, 10, 10, COLOR_YELLOW);
        SoundEx((VECTOR *)item->locate->locate.coord.t, SE_PROJECTILE_IMPACT);
        reset_alert_duration();
        return;

    case KORO_GRAND:
    case KORO_STAY:
    {
        PARAM_ITEM_LAUNCH param;

        p = &param;
        memset(p, 0, sizeof(PARAM_ITEM_LAUNCH));
        param.type = item->type;
        param.user.human = item->owner.human;
        param.start.vx = model->locate.coord.t[0];
        param.start.vy = model->locate.coord.t[1];
        param.start.vz = model->locate.coord.t[2];
        rparam = *p;
        if (item->proc != 0)
        {
            DISPOSE_ITEM(item);
        }
        ReqItemDrop(&rparam);
        return;
    }

    case KORO_WATER:
        dispose:
            if (item->proc != 0)
            {
                DISPOSE_ITEM(item);
            }
            return;
        }
    }
