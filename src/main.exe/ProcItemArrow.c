#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemArrow(struct tag_TItem *item);
 *     ITEM.C:3367, 118 src lines, frame 168 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     reg   $s4       struct ModelType * model
 *     reg   $s3       struct param_arrow * param
 *     stack sp+24     struct VECTOR v1
 *     stack sp+40     struct VECTOR v2
 *     stack sp+136    int rx
 *     stack sp+140    int ry
 *     reg   $a0       int cid
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s2       struct Humanoid * m
 *     reg   $s2       struct Humanoid * human
 *     stack sp+56     struct PARAM_ITEM_LAUNCH param
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s0       struct ModelType * model
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern long GameClock;
 * END PSX.SYM */

#include "item.h"

/*
 * MATCH.
 *
 * ProcItemArrow (0x80047b94) flies, arms, attaches, and renders the homing
 * arrow.  Before attachment it maintains a 300-unit conflict box and aims
 * from the previous to current position; a character hit either disposes
 * the arrow or attaches it to a random model object.  Its final two modes
 * blink the attached arrow before disposal.
 *
 * Matching notes:
 *  - `v1`, `v2`, `rx`, and `ry` use the stack slots recovered by PSX.SYM.
 *    GetVectorRotation writes the two full-word outputs at sp+0x88/sp+0x8c;
 *    their later stores to SVECTOR members naturally use only the low halves.
 *  - The dead `mode_index = ARROW_MODE_FLY` assignment is a zero-code CSE
 *    eviction. It forces expand_case to emit a fresh mode `lbu`; otherwise
 *    the entry
 *    guard's load is reused and the function is one instruction short.
 *  - Direct ITEM_MODE_DISPOSE operands still share the target's caller-saved
 *    value on the no-call mode-2 path; call-crossing disposal prefixes
 *    rematerialize it before the common indirect-call tail.
 *  - The payload is the `else` of the conflict-id test and uses inverse
 *    mode/kind guards, so a non-humanoid hit and both zero cases fall into
 *    the later aiming block without labels. Direct status tests let CSE keep
 *    the payload byte in v1 and `1` in v0 while still allowing the `li` to
 *    fill the payload-zero branch's delay slot.
 *  - `clock` prevents the halfword-load optimization on GameClock.  The
 *    original reads the declared long with `lw`, shifts it, then narrows at
 *    the model rotation store.
 *  - The target model-object cursor is a `ModelType **`: loop-free pointer
 *    adjustment after the guarded random remainder reproduces the single
 *    object-array load and the checked variable-division sequence.
 *  - `model->locate = item->locate->locate` intentionally remains a whole
 *    GsCOORDINATE2 assignment.  GCC emits the target five-iteration,
 *    16-byte block-copy loop before DrawModel.
 */
extern void MoveFly(TItem *item, param_fly *param);
extern short DrawModel(ModelType *objp);
extern s32 is_humanoid_on_stage_(Humanoid *human);
extern void ArrangeLocalMatrix(ModelType *model, MATRIX *t);

void ProcItemArrow(TItem *item)
{
    enum
    {
        ARROW_MODE_FLY = 0,
        ARROW_MODE_WAIT = 1,
        ARROW_MODE_BLINK = 2
    };
    ModelType *model;
    param_arrow *param;
    void (*ppu)(TItem *);
    item_mode mode_index;
    VECTOR v1;
    VECTOR v2;
    int rx;
    int ry;

    model = item->model.object;
    param = &item->param.arrow;
    mode_index = item->mode;
    if (mode_index == ITEM_MODE_DISPOSE)
    {
        item->mode = ARROW_MODE_FLY;
        return;
    }

    mode_index = ARROW_MODE_FLY;
    switch (item->mode)
    {
    case ARROW_MODE_FLY:
    {
        u8 count;
        s32 cid;

        v1.vx = item->locate->locate.coord.t[0];
        v1.vy = item->locate->locate.coord.t[1];
        v1.vz = item->locate->locate.coord.t[2];
        MoveFly(item, &param->fly);
        count = param->count - 1;
        param->count = count;
        if (count == 0)
        {
            s32 conflict_id;

            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            SET_ITEM_COLLISION(conflict_id, 300, CONFLICT_OWNER_ITEM,
                               CONFLICT_HIT);
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            cid = CONFLICT_NONE;
        }
        else
        {
            cid = GetConflictResult(item->locate, CONFLICT_NONE);
        }
        if (cid != CONFLICT_NONE)
        {
            Humanoid *human;

            human = ConflictObject[cid].common.human;
            if (is_humanoid_on_stage_(human) != 0)
            {
                if ((ConflictObject[cid].size.components.class_flags &
                     CONFLICT_HIT) != 0)
                {
                    ppu = item->proc;
                    if (ppu == 0)
                    {
                        return;
                    }
                    item->mode = ITEM_MODE_DISPOSE;
                    item->proc(item);
                    DeleteConflict(item->locate);
                    if (item->mode != ARROW_MODE_FLY)
                    {
                        AdtMessageBox(msg_item_dispose_fail, item->type,
                                      (u32)item->mode);
                    }
                    item->owner.human = 0;
                    item->proc = 0;
                    return;
                }
                else
                {
                    ModelType **models;
                    ModelType *model;

                    models = human->model->object;
                    if (human->model->n > 0)
                    {
                        models += rand() % human->model->n;
                    }
                    model = *models;
                    SetImpact(GetAbsolutePosition(model, 0, 0, 0),
                              6 * FIXED_ONE, IMPACT_SPRITE_HIT);
                    SoundEx(GetAbsolutePosition(model, 0, 0, 0), SE_PROJECTILE_HIT);
                    ArrangeLocalMatrix(model,
                                       &item->locate->locate.coord);
                    item->locate->locate.flg = 0;
                    item->locate->locate.super =
                        (GsCOORDINATE2 *)model;
                    item->locate->locate.coord.t[0] = 0;
                    item->locate->locate.coord.t[1] = 0;
                    item->locate->locate.coord.t[2] = 0;
                    param->count = 120;
                    item->mode++;
                    DeleteConflict(item->locate);
                    break;
                }
            }
        }
        else
        {
            if (param->fly.mode != FLY_MODE_ARC)
            {
                if (param->fly.p.koro.status != KORO_NORMAL)
                {
                    if (param->fly.p.koro.status == KORO_WATER)
                    {
                        ppu = item->proc;
                        if (ppu == 0)
                        {
                            return;
                        }
                        item->mode = ITEM_MODE_DISPOSE;
                        item->proc(item);
                        DeleteConflict(item->locate);
                        if (item->mode != ARROW_MODE_FLY)
                        {
                            AdtMessageBox(msg_item_dispose_fail, item->type,
                                          (u32)item->mode);
                        }
                        item->owner.human = 0;
                        item->proc = 0;
                        return;
                    }
                    SoundEx((VECTOR *)item->locate->locate.coord.t, SE_PROJECTILE_IMPACT);
                    SetBleeds((VECTOR *)item->locate->locate.coord.t,
                              0, 25, 30, 30, COLOR_YELLOW);
                    param->count = 30;
                    item->mode++;
                    DeleteConflict(item->locate);
                    return;
                }
            }
        }

        v2.vx = item->locate->locate.coord.t[0];
        v2.vy = item->locate->locate.coord.t[1];
        v2.vz = item->locate->locate.coord.t[2];
        GetVectorRotation(&v1, &v2, &rx, &ry);
        item->locate->rotate.vx = rx;
        item->locate->rotate.vy = ry;
        {
            s32 clock;

            clock = GameClock;
            item->locate->rotate.vz = clock << 8;
        }
        UpdateCoordinate(item->locate);
        break;
    }

    case ARROW_MODE_WAIT:
    {
        u8 count;

        count = param->count - 1;
        param->count = count;
        if (count != 0)
        {
            break;
        }
        param->count = 15;
        item->mode++;
        break;
    }

    case ARROW_MODE_BLINK:
    {
        u8 count;

        count = param->count - 1;
        param->count = count;
        if (count == 0)
        {
            ppu = item->proc;
            if (ppu == 0)
            {
                return;
            }
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != ARROW_MODE_FLY)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner.human = 0;
            item->proc = 0;
            return;
        }
        if ((count & 1) != 0)
        {
            return;
        }
        break;
    }
    }

    model->locate = item->locate->locate;
    DrawModel(model);
}
