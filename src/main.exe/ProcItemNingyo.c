#include "common.h"
#include "main.exe.h"
#include "item.h"

typedef struct
{
    VECTOR v;
    VECTOR pos;
} ProcItemNingyoVectors;

typedef union
{
    struct
    {
        SVECTOR sv;
        PARAM_ITEM_LAUNCH launch;
    } drop;
    ProcItemNingyoVectors vectors;
} ProcItemNingyoScratch;

extern SVECTOR svec_y_n25[]; /* {0,-25,0} */
extern u8 NingyoCount;

extern void MoveKorogari(TItem *item, param_korogari *param);
extern short DrawModel(ModelType *objp);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemNingyo(struct tag_TItem *item);
 *     ITEM.C:1882, 132 src lines, frame 112 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $s5       struct tag_TItem * item
 *     reg   $s4       struct param_ningyo * param
 *     reg   $a0       int cid
 *     reg   $s5       struct tag_TItem * item
 *     stack sp+24     struct SVECTOR sv
 *     stack sp+32     struct VECTOR v
 *     reg   $s5       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s2       int i
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       long len
 *     reg   $v0       struct Humanoid * human
 *     reg   $v0       struct Humanoid * human
 *     reg   $a1       int i
 *     reg   $s5       struct tag_TItem * item
 *     stack sp+48     struct VECTOR pos
 *     reg   $s4       struct param_korogari * param
 *     reg   $s4       struct param_korogari * param
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct TCameraStatus CamState;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern struct ModelType *NingyoModel;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR ConflictDistance;
 * END PSX.SYM */

/* MATCH (retail): the pure-C body has the exact 0x68 frame, 564 instructions,
 * exact 36/12/33/1 branch/jump/call/return inventory, and target
 * item/param/sentinel homes s3/s4/s5.  The short-lived loaded_model alias is
 * intentional: its single-set load receives scheduler promotion, then copy
 * coalescing erases the assignment into the destructively reused model and
 * preserves the target owner/type/model load order in s2/s1/s0.
 *
 * Clearing the short-lived launch pointer after memset breaks the stack-
 * address CSE that otherwise occupies s3.  Reusing the model pointer for its
 * embedded position then makes the derived-address and all three shared
 * modulus-constant sequences exact.  Separate base/result conflict pointers
 * and zero-trip fences around the call result and constants make the mode-1
 * address and constant ordering exact.  The otherwise-odd dispose one-shot
 * loops keep item in the narrow priority window between param and the later
 * bounce temporary and allow the indirect call's target delay slots.  They
 * emit no branch or loop instructions. */
void ProcItemNingyo(TItem *item)
{
    param_ningyo *param;
    s32 cid;
    s32 ff;
    ProcItemNingyoScratch scratch;

    param = &item->param.ningyo;
    ff = ITEM_MODE_DISPOSE;
    if (item->mode == ff)
    {
        if (param->hp != NINGYO_HP)
        {
            s32 i;
            s32 n;
            Humanoid **humans;

            n = Humans;
            if (n > 0)
            {
                s32 limit;
                TCameraStatus *camera;

                do
                {
                    i = 0;
                } while (0);
                camera = &CamState;
                limit = n;
                humans = HumanGroup;
                do
                {
                    Humanoid *human;

                    human = *humans;
                    if (human->target == item->locate)
                    {
                        human->target = (ModelType *)camera->Owner->model;
                    }
                    i++;
                    humans++;
                } while (i < limit);
            }
            NingyoCount--;
        }
        item->mode = 0;
        return;
    }

    MoveKorogari(item, &param->koro);
    if (param->koro.status == KORO_WATER)
    {
        goto dispose;
    }

    switch (item->mode)
    {
    case 0:
    {
        s32 count;

        count = param->count - 1;
        param->count = count;
        if ((u8)count == 0)
        {
            param->count = 0;
            item->mode++;
            scratch.drop.sv = svec_y_n25[0];
            SetSmoke((VECTOR *)item->locate->locate.coord.t,
                     &scratch.drop.sv, 10, 6);
            SoundEx((VECTOR *)item->locate->locate.coord.t, 0x23);
            if (NingyoCount < 3)
            {
                param->hp = 3;
                NingyoCount++;
                goto draw_mode0;
            }
            else
            {
                Humanoid *owner;
                s32 type;
                ModelType *loaded_model;
                ModelType *model;
                PARAM_ITEM_LAUNCH *launchp;

                owner = item->owner;
                type = item->type;
                loaded_model = item->locate;
                model = loaded_model;
                launchp = &scratch.drop.launch;
                memset(launchp, 0, sizeof(PARAM_ITEM_LAUNCH));
                launchp = 0;
                scratch.drop.launch.type = type;
                scratch.drop.launch.user = owner;
                {
                    VECTOR *pos;

                    pos = (VECTOR *)model->locate.coord.t;
                    scratch.drop.launch.start.vx = pos->vx;
                    scratch.drop.launch.start.vy = pos->vy;
                    scratch.drop.launch.start.vz = pos->vz;
                }
                scratch.drop.launch.end.vx = rand() % 200 - 100;
                scratch.drop.launch.end.vy = rand() % 100 - 200;
                scratch.drop.launch.end.vz = rand() % 200 - 100;
                ReqItemDrop(&scratch.drop.launch);
            }
        }
        else
        {
            goto draw_mode0;
        }

    dispose:
        do
        {
            do
            {
                if (item->proc == 0)
                {
                    return;
                }
            } while (0);
            item->mode = ff;
            item->proc(item);
        } while (0);
        DeleteConflict(item->locate);
        if (item->mode != 0)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;

    draw_mode0:
        UpdateCoordinate(item->locate);
        item->model->locate = item->locate->locate;
        DrawSprite((Sprite3D *)item->model);
        return;
    }

    case 1:
    {
        s32 n;
        s32 size;
        s32 offset_y;
        s32 collision_mode;
        ConflictObjectType *conflicts;
        ConflictObjectType *conflict;

        param->count++;
        memset(&scratch.vectors.pos, 0, sizeof(VECTOR));
        scratch.vectors.pos.vx = param->count << 8;
        scratch.vectors.pos.vy = param->count << 8;
        scratch.vectors.pos.vz = param->count << 8;
        scratch.vectors.v = scratch.vectors.pos;
        RotMatrixYXZ(&item->locate->rotate, &item->locate->locate.coord);
        ScaleMatrix(&item->locate->locate.coord, &scratch.vectors.v);
        item->locate->locate.flg = 0;
        NingyoModel->locate = item->locate->locate;
        DrawModel(NingyoModel);
        if (param->count < 16)
        {
            return;
        }

        DeleteConflict(item->locate);
        do
        {
            n = InsertConflict(item->locate);
        } while (0);
        conflicts = ConflictObject;
        conflict = conflicts + n;
        do
        {
            offset_y = -250;
            size = 500;
        } while (0);
        conflict->common = CONFLICT_OWNER_ITEM;
        collision_mode = 12;
        conflict->offset.vx = 0;
        conflict->offset.vz = 0;
        conflict->offset.vy = offset_y;
        conflict->size.vz = size;
        conflict->size.vy = size;
        conflict->size.vx = size;
        conflict->size.pad = collision_mode;
        item->collision.mode = collision_mode;
        item->collision.size = size;
        item->collision.ofsY = offset_y;
        item->collision.pause = 0;
        param->count = 3;
        item->mode++;
        return;
    }

    case 2:
    {
        s32 count;

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            cid = -1;
        }
        else
        {
            cid = GetConflictResult(item->locate, -1);
        }

        count = param->count - 1;
        param->count = count;
        if ((u8)count == 0)
        {
            s32 i;
            Humanoid **humans;

            i = 0;
            humans = HumanGroup;
            while (1)
            {
                Humanoid *human;
                s32 len;

                if (i >= Humans)
                {
                    break;
                }
                human = *humans;
                len = GetVectorDistance(
                    (VECTOR *)item->locate->locate.coord.t,
                    human->locate);
                if (len < 10000 && human->target != 0 &&
                    len < GetVectorDistance(
                              (VECTOR *)human->target->locate.coord.t,
                              human->locate) &&
                    ((u16)human->type & 0xf0) != PAGE_BOSS)
                {
                    human->target = item->locate;
                }
                humans++;
                i++;
            }
            param->count = 30;
        }
        else if (cid != -1)
        {
            ConflictObjectType *conflict;
            ConflictObjectType *conflicts;
            s32 collision_mode;

            conflicts = ConflictObject;
            conflict = &conflicts[cid];
            collision_mode = conflict->size.pad;
            if (collision_mode == 1)
            {
                if (param->hp == 0)
                {
                    SetBleeds((VECTOR *)item->locate->locate.coord.t,
                              0, 30, 30, 30, 0xffff00);
                    SoundEx((VECTOR *)item->locate->locate.coord.t, 0x23);
                    if (item->proc != 0)
                    {
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
                    }
                    item->mode++;
                }
                else
                {
                    enum
                    {
                        R = 100
                    };
                    s32 vz;
                    s32 vx;
                    s32 shifted_vx;
                    u8 hp;

                    memset(&scratch.vectors.pos, 0, sizeof(VECTOR));
                    scratch.vectors.pos.vx = conflict->position.vx;
                    scratch.vectors.pos.vy = conflict->position.vy;
                    scratch.vectors.pos.vz = conflict->position.vz;
                    scratch.vectors.v = scratch.vectors.pos;
                    vx = -ConflictDistance.vx;
                    if (vx < 0)
                    {
                        vx += 15;
                    }
                    shifted_vx = vx >> 4;
                    vz = -ConflictDistance.vz;
                    if (vz < 0)
                    {
                        vz += 15;
                    }
                    hp = param->hp;
                    param->koro.vx = shifted_vx;
                    param->koro.vy = -R;
                    param->koro.vz = vz >> 4;
                    param->koro.hint = 0;
                    param->koro.status = KORO_NORMAL;
                    param->hp = hp - 1;
                    SoundEx((VECTOR *)item->locate->locate.coord.t, 0x30);
                }
            }
            else if (collision_mode != 8)
            {
                enum
                {
                    R = 100
                };
                s32 random_x;
                s32 random_z;
                s32 vx;
                s32 vy;
                s32 vz;
                s16 xbase;
                s16 xrem;

                random_x = rand();
                vx = -ConflictDistance.vx;
                if (vx < 0)
                {
                    vx += 7;
                }
                xbase = vx >> 3;
                xrem = random_x % 20;
                vy = 0;
                if (ConflictDistance.vy > -501)
                {
                    vy = -R;
                }
                random_z = rand();
                vz = -ConflictDistance.vz;
                if (vz < 0)
                {
                    vz += 7;
                }
                param->koro.vx = xbase + xrem - 10;
                param->koro.vy = vy;
                param->koro.hint = 0;
                param->koro.status = KORO_NORMAL;
                param->koro.vz = (vz >> 3) + random_z % 20 - 10;
            }
        }

        UpdateCoordinate(item->locate);
        NingyoModel->locate = item->locate->locate;
        DrawModel(NingyoModel);
        return;
    }
    }
    return;
}
