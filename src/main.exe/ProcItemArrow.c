#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemArrow(struct tag_TItem *item);
 *     ITEM.C:3367, 118 src lines, frame 168 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *  - The dead `mode_index = 0` assignment is a zero-code CSE eviction.  It
 *    forces expand_case to emit a fresh mode `lbu`; otherwise the entry
 *    guard's load is reused and the function is one instruction short.
 *  - `ff` is an s32 caller-saved value in a1.  It survives only the no-call
 *    mode-2 path; call-crossing mode-0 disposal prefixes rematerialize
 *    ITEM_MODE_DISPOSE (0xff)
 *    before all copies merge at the common indirect-call tail.
 *  - The payload guard is explicit control flow so both zero tests target
 *    the later aiming block.  `water` is deliberately s16: autorules
 *    found that narrowing this comparison-only constant colors the payload
 *    byte into v1 and `1` into v0, while still allowing the `li` to fill the
 *    payload-zero branch's delay slot.  s32 swaps those two registers.
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
extern s32 is_character_state_present_on_stage_(Humanoid *human);
extern void ArrangeLocalMatrix(ModelType *model, MATRIX *t);

void ProcItemArrow(TItem *item)
{
    ModelType *model;
    param_arrow *param;
    void (*ppu)(TItem *);
    s32 ff;
    u8 mode_index;
    VECTOR v1;
    VECTOR v2;
    int rx;
    int ry;

    model = item->model;
    param = &item->param.arrow;
    ff = ITEM_MODE_DISPOSE;
    mode_index = item->mode;
    if (mode_index == ff)
    {
        item->mode = 0;
        return;
    }

    mode_index = 0;
    switch (item->mode)
    {
    case 0:
    {
        u8 count;
        s32 cid;
        s32 one;

        v1.vx = item->locate->locate.coord.t[0];
        v1.vy = item->locate->locate.coord.t[1];
        v1.vz = item->locate->locate.coord.t[2];
        MoveFly(item, &param->fly);
        count = param->count - 1;
        param->count = count;
        one = 1;
        if (count == 0)
        {
            s32 n;
            s32 size;

            DeleteConflict(item->locate);
            n = InsertConflict(item->locate);
            size = 300;
            ConflictObject[n].offset.vx = 0;
            ConflictObject[n].offset.vz = 0;
            ConflictObject[n].offset.vy = 0;
            ConflictObject[n].size.vz = size;
            ConflictObject[n].size.vy = size;
            ConflictObject[n].size.vx = size;
            ConflictObject[n].common = (void *)one;
            ConflictObject[n].size.pad = one;
            item->collision.size = size;
            item->collision.ofsY = 0;
            item->collision.mode = one;
            item->collision.pause = 0;
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            cid = -1;
        }
        else
        {
            cid = GetConflictResult(item->locate, -1);
        }
        if (cid != -1)
        {
            Humanoid *human;

            human = (Humanoid *)ConflictObject[cid].common;
            if (is_character_state_present_on_stage_(human) != 0)
            {
                if ((ConflictObject[cid].size.pad & 1) != 0)
                {
                    ppu = item->proc;
                    if (ppu == 0)
                    {
                        return;
                    }
                    item->mode = ITEM_MODE_DISPOSE;
                    item->proc(item);
                    DeleteConflict(item->locate);
                    if (item->mode != 0)
                    {
                        AdtMessageBox(msg_item_dispose_fail, item->type,
                                      (u32)item->mode);
                    }
                    item->owner = 0;
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
                              0x6000, 2);
                    SoundEx(GetAbsolutePosition(model, 0, 0, 0), 0x30);
                    ArrangeLocalMatrix(model,
                                       &item->locate->locate.coord);
                    item->locate->locate.flg = 0;
                    item->locate->locate.super =
                        (GsCOORDINATE2 *)model;
                    item->locate->locate.coord.t[0] = 0;
                    item->locate->locate.coord.t[1] = 0;
                    item->locate->locate.coord.t[2] = 0;
                    param->count = 0x78;
                    item->mode++;
                    DeleteConflict(item->locate);
                    break;
                }
            }
            else
            {
                goto aim;
            }
        }

        {
            s16 water;
            u8 kind;

            if (param->fly.mode == 0)
            {
                goto aim;
            }
            water = KORO_WATER;
            kind = param->fly.p.koro.status;
            if (kind == KORO_NORMAL)
            {
                goto aim;
            }
            if (kind == water)
            {
                ppu = item->proc;
                if (ppu == 0)
                {
                    return;
                }
                item->mode = ITEM_MODE_DISPOSE;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != 0)
                {
                    AdtMessageBox(msg_item_dispose_fail, item->type,
                                  (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
                return;
            }
            SoundEx((VECTOR *)item->locate->locate.coord.t, 0x31);
            SetBleeds((VECTOR *)item->locate->locate.coord.t,
                      0, 0x19, 0x1e, 0x1e, 0xffff00);
            param->count = 0x1e;
            item->mode++;
            DeleteConflict(item->locate);
            return;
        }

    aim:
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

    case 1:
    {
        u8 count;

        count = param->count - 1;
        param->count = count;
        if (count != 0)
        {
            break;
        }
        param->count = 0xf;
        item->mode++;
        break;
    }

    case 2:
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
            item->mode = ff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner = 0;
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

