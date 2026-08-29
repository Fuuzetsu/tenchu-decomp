#include "common.h"
#include "main.exe.h"
#include "item.h"

typedef union
{
    struct
    {
        SVECTOR vec;
        VECTOR pos;
        VECTOR pos_buf;
    } explosion;
    struct
    {
        VECTOR pos;
        VECTOR random_pos;
    } frame;
} ProcItemJiraiScratch;

extern SVECTOR svec_y_n25[]; /* {0,-25,0} */

extern s32 is_humanoid_on_stage_(Humanoid *human);
extern void reset_alert_duration(void);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemJirai(struct tag_TItem *item);
 *     ITEM.C:3527, 78 src lines, frame 104 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     param $s2       struct tag_TItem * item
 *     reg   $s6       struct Sprite3D * model
 *     reg   $v1       struct param_smoke * param
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s4       struct Humanoid * human
 *     reg   $s4       struct Humanoid * human
 *     reg   $s3       int i
 *     reg   $s0       struct ModelType * model
 *     stack sp+24     struct VECTOR pos
 *     stack sp+24     struct SVECTOR vec
 *     stack sp+32     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

/*
 * Advances the placed landmine through floor placement, collision arming,
 * detonation, and its ten-frame-effect burst before disposal.
 *
 * Matching notes:
 *  - The explosion and frame-effect aggregates are mutually exclusive and
 *    share ProcItemJiraiScratch.  This reproduces the target's exact
 *    sp+0x18..sp+0x3f working window and 0x60-byte frame.
 *  - `call_item` makes both disposal predecessors materialize the indirect
 *    call argument before entering their shared tail; calling `proc(item)`
 *    instead fills the jalr delay slot and removes one of those moves.
 *  - The zero-trip wrapper around `i = 0` keeps initialization after the
 *    character-state call, where it fills the following branch delay slot.
 *    That loop note initially gave `i` the allocator's preferred saved
 *    register, so the second zero-trip wrapper weights the three `% 200`
 *    expressions more heavily.  The generated division constant then takes
 *    $s1 and leaves the target $s3 for `i`, without emitting extra code.
 *  - This function needs maspsx `--expand-div` for the dynamic model-count
 *    remainder guard; Build.hs and permute.py carry the mirrored flag.
 */

void ProcItemJirai(TItem *item)
{
    Sprite3D *model;
    param_smoke *param;
    void (*proc)(TItem *);
    TItem *call_item;
    u8 ff;
    ProcItemJiraiScratch scratch;

    model = (Sprite3D *)item->model;
    param = &item->param.smoke;
    ff = ITEM_MODE_DISPOSE;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }

    switch (item->mode)
    {
    case 0:
    {
        s32 size;
        s32 collision_mode;
        s32 n;

        item->locate->locate.coord.t[1] =
            GetAreaMapLevel(GlobalAreaMap,
                            item->locate->locate.coord.t[0],
                            item->locate->locate.coord.t[1],
                            item->locate->locate.coord.t[2], 1);
        if (item->locate->locate.coord.t[1] == LEVEL_NONE ||
            ((u16)FieldArea->attribute & MAP_WATER) != 0)
        {
            u8 count;

            count = item->owner->item[item->type];
            if (count != ITEM_INFINITE)
            {
                item->owner->item[item->type] = count + 1;
            }
            proc = item->proc;
            if (proc == 0)
            {
                return;
            }
            call_item = item;
            item->mode = ff;
            goto dispose;
        }

        DeleteConflict(item->locate);
        n = InsertConflict(item->locate);
        size = 500;
        collision_mode = 8;
        ConflictObject[n].offset.vx = 0;
        ConflictObject[n].offset.vz = 0;
        ConflictObject[n].offset.vy = 0;
        ConflictObject[n].size.vz = size;
        ConflictObject[n].size.vy = size;
        ConflictObject[n].size.vx = size;
        ConflictObject[n].common = (void *)1;
        ConflictObject[n].size.pad = collision_mode;
        item->collision.size = size;
        item->collision.ofsY = 0;
        item->collision.mode = collision_mode;
        item->collision.pause = 0;
        item->mode++;
        break;
    }

    case 1:
    {
        s32 cid;

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            cid = -1;
        }
        else
        {
            cid = GetConflictResult(item->locate, -1);
        }
        if (cid != -1 &&
            is_humanoid_on_stage_(
                (Humanoid *)ConflictObject[cid].common) != 0)
        {
            s32 n;
            s32 size;
            s32 one;

            DeleteConflict(item->locate);
            n = InsertConflict(item->locate);
            size = 1500;
            one = 1;
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
            item->mode = item->mode + one;
        }
        break;
    }

    case 2:
        scratch.explosion.vec = svec_y_n25[0];
        memset(&scratch.explosion.pos_buf, 0, sizeof(VECTOR));
        scratch.explosion.pos_buf.vx = item->locate->locate.coord.t[0];
        scratch.explosion.pos_buf.vy = item->locate->locate.coord.t[1];
        scratch.explosion.pos_buf.vz = item->locate->locate.coord.t[2];
        scratch.explosion.pos = scratch.explosion.pos_buf;
        SetExplosion(&scratch.explosion.pos, &scratch.explosion.vec);
        scratch.explosion.vec.vx = 75;
        scratch.explosion.vec.vy = 200;
        scratch.explosion.vec.vz = 75;
        SetHinoko(&scratch.explosion.pos, &scratch.explosion.vec, 10);
        scratch.explosion.vec.vx = 0;
        scratch.explosion.vec.vy = -400;
        scratch.explosion.vec.vz = 0;
        SetSmoke(&scratch.explosion.pos, &scratch.explosion.vec, 20, 6);
        SoundEx(&scratch.explosion.pos, 0x25);
        item->mode++;
        param->count = 3;
        reset_alert_duration();
        break;

    case 3:
    {
        s32 cid;
        s32 count;

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
            s32 i;
            s32 present;

            human = (Humanoid *)ConflictObject[cid].common;
            present = is_humanoid_on_stage_(human);
            do
            {
                i = 0;
            } while (0);
            if (present != 0)
            {
                while (1)
                {
                    ModelType **objects;
                    ModelType *frame_model;

                    if (i >= 10)
                    {
                        break;
                    }
                    objects = human->model->object;
                    if (human->model->n > 0)
                    {
                        objects += rand() % human->model->n;
                    }
                    frame_model = *objects;
                    memset(&scratch.frame.random_pos, 0, sizeof(VECTOR));
                    i++;
                    do
                    {
                        scratch.frame.random_pos.vx = rand() % 200 - 100;
                        scratch.frame.random_pos.vy = rand() % 200 - 100;
                        scratch.frame.random_pos.vz = rand() % 200 - 100;
                    } while (0);
                    scratch.frame.pos = scratch.frame.random_pos;
                    SetFrame(&scratch.frame.pos, 0x3000,
                             rand() % 60 + 60,
                             (GsCOORDINATE2 *)frame_model);
                }
            }
        }

        count = param->count - 1;
        param->count = count;
        if ((u8)count != ITEM_INFINITE)
        {
            return;
        }
        proc = item->proc;
        if (proc == 0)
        {
            return;
        }
        call_item = item;
        item->mode = ITEM_MODE_DISPOSE;
    dispose:
        proc(call_item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;
    }
    }

    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);
}
