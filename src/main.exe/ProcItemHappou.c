#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "effect.h"

/*
 * ProcItemHappou (0x8004488c) — the happou (fire cracker / bouncing bomb)
 * item processor. Every frame: fly it (MoveFly), tick its countdown; at 0 register
 * a 300-unit conflict box. Always redraw (into the shared scratch model
 * HappouModel rather than item->model — the item's own visual is elsewhere)
 * and re-draw its afterimage trail. If the flight `mode` and rolling-overlay
 * `status` are armed, detonate (SetBleeds/SoundEx, dispose). Separately, if a
 * live character sits in the conflict box, SetImpact+SoundEx+DeleteConflict
 * (no dispose — this is the "someone touched it" hit, independent of the
 * detonation-armed path above).
 *
 * Matching notes (see also ProcItemMakibishi.c for the collision-box
 * conventions, ProcItemManebue.c for the countdown idiom):
 *  - `model = HappouModel; param = &item->param.launch;` are both
 *    computed before the entry mode==ITEM_MODE_DISPOSE test (model
 *    sequential, param's
 *    addiu fills the entry branch's delay slot).
 *  - The countdown is `t = param->count - 1;` (real `addiu -1`, sign-extended)
 *    tested on the NEW/post-decrement value — Manebue's idiom, NOT
 *    LightningBolt's `+0xff`-on-the-OLD-value idiom (verified against the
 *    raw immediate bytes in both places; per-function, not a fixed rule).
 *  - No entry `ff` local this time: ITEM_MODE_DISPOSE is materialized for
 *    the entry test in a transient register, reused for unrelated things
 *    immediately after — nothing carries it to the later
 *    `item->mode = ITEM_MODE_DISPOSE` dispose
 *    (a fresh literal there), so a plain literal at entry matches too.
 *  - The 300/1 collision-box constants are plain literals (no shared
 *    variable): unlike Makibishi's `item->mode += one`, nothing here adds
 *    them to anything (only stores), so there's no register-form-add tell
 *    forcing a variable.
 *  - `conflict_id = InsertConflict(...)` is `s32` (the same scheduling-tie
 *    fix as Makibishi/LightningBolt: extend right at the assignment).
 *  - The redundant-looking `mode != FLY_MODE_ARC && mode == FLY_MODE_ROLL &&
 *    param->fly.p.koro.status != KORO_NORMAL` is written
 *    exactly that way (three separate tests, matching Ghidra) — the asm
 *    shows two distinct branches on `mode` even though `== 1` implies `!= 0`.
 *  - The two dispose-shaped tails (detonate-and-dispose; the separate
 *    hit-conflict SetImpact path) are NOT the same code — no cross-jump
 *    merge here, they're independent, sequential ifs.
 */
#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemHappou(struct tag_TItem *item);
 *     ITEM.C:2486, 52 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s2       struct ModelType * model
 *     reg   $s1       struct param_launch * param
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s0       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s1       struct Humanoid * m
 *     reg   $s1       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType *HappouModel;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

extern void MoveFly(TItem *item, param_fly *param);
extern short DrawModel(ModelType *objp);
extern s32 is_humanoid_on_stage_(Humanoid *h);
/* Ghidra/m2c call this D_80097F54; bound here under a fresh name via
 * config/symbols.main.exe.txt because the data.s-internal `glabel
 * D_80097F54` (and its neighbor SyurikenModel, also referenced by
 * ReqItemLaunch) resolves 8 bytes low — a pre-existing accumulated-offset
 * drift in that data section, upstream of this function. Symbols bound
 * via config/symbols.main.exe.txt (BLOOD_POOL_MODEL_/NINKEN_CHARACTER_PTR,
 * right next to the drifted ones) resolve correctly regardless, which is
 * how this was diagnosed and is the same mechanism used to route around it. */

void ProcItemHappou(TItem *item)
{
    ModelType *model;
    param_launch *param;
    u8 t;
    fly_mode mode;
    s32 i;
    s32 conflict_id;

    model = HappouModel;
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
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawModel(model);
    DrawAfterimage(param->effect, 1);
    mode = param->fly.mode;
    if (mode != FLY_MODE_ARC)
    {
        if (mode == FLY_MODE_ROLL &&
            param->fly.p.koro.status != KORO_NORMAL)
        {
            SetBleeds((VECTOR *)item->locate->locate.coord.t, 0, 25, 10, 10, COLOR_YELLOW);
            SoundEx((VECTOR *)item->locate->locate.coord.t, SE_PROJECTILE_IMPACT);
            if (item->proc != 0)
            {
                DISPOSE_ITEM(item);
            }
        }
    }
    if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        i = CONFLICT_NONE;
    else
        i = GetConflictResult(item->locate, CONFLICT_NONE);
    if (i != CONFLICT_NONE &&
        is_humanoid_on_stage_(ConflictObject[i].common.human) != 0)
    {
        SetImpact((VECTOR *)item->locate->locate.coord.t, 4 * FIXED_ONE,
                  IMPACT_SPRITE_HIT);
        SoundEx((VECTOR *)item->locate->locate.coord.t, SE_PROJECTILE_HIT);
        DeleteConflict(item->locate);
    }
}
