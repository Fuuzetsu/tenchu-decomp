#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemShinsoku(struct tag_TItem *item);
 *     ITEM.C:1324, 55 src lines, frame 104 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_TItem * item
 *     reg   $s1       struct param_shinsoku * param
 *     stack sp+24     struct VECTOR pos
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+40     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s0       struct VECTOR * apos
 *     stack sp+56     struct MapVector map
 *     stack sp+40     struct VECTOR pos
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

/*
 * MATCH.
 *
 * ProcItemShinsoku (0x8003f8a0, ITEM.C:1324) drives the rapid-movement item:
 * it starts/monitors motion 0xf05, drops itself if that motion is interrupted,
 * moves the owner's model with a floor query while steering from the pad, emits
 * a periodic effect, and restores the normal motion/camera before disposal.
 *
 * Matching notes:
 *  - The stack is two adjacent source objects: `pos` at sp+0x28 and
 *    `scratch` at sp+0x38. PSX.SYM records the latter as mode-1
 *    PARAM_ITEM_LAUNCH `p`, then mode-2 VECTOR `pos` followed by MapVector
 *    `map`; retail's larger MapVector makes both views exactly 0x28 bytes.
 *    This gives stackplan's exact 0x38-byte working window and 0x78 frame.
 *  - `launchp = 0` after memset is a zero-byte CSE eviction.  Without that dead
 *    reassignment, cse2 keeps `&scratch` in an extra callee-saved register
 *    through all three rand calls, adding an s5 save/restore and growing the
 *    frame.  Eviction makes both call sites re-materialize sp+0x38 like target.
 *  - The movement position is built through direct `pos` writes, then
 *    `apos = &pos` is assigned only for the query/level span.  The final
 *    model-coordinate copies must return to direct stack reads; using `apos`
 *    there emits s0-relative loads instead.
 *  - The validity flag is assigned at the END of both comparison arms.  reorg
 *    then places `valid=0`/`valid=1` in the two guard delay slots and global
 *    allocation reuses the comparison's v0, rather than the dead a1 map arg.
 *  - The camera high-base is assigned independently on the effect and skipped
 *    paths.  `(u32 *)0x80090000` plus the -0x6100 load offset produces the two
 *    target `lui v0,0x8009` definitions: one in the count guard's delay slot,
 *    and one after the effect call that clobbers v0.
 *  - The first mode-2 motion check dereferences `item->owner` directly; sharing
 *    the later pad-control `human` local changes a0 to a2 at all three loads.
 *  - The human-shaped `CamState.Owner` access emits two CamState HI16
 *    relocations around the effect call and one shared LO16 field load.  With
 *    CamState pinned to its retail address, those relocations resolve to the
 *    exact words formerly produced by the duplicated numeric-base scaffold.
 */

extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);
/* Retail's caller promotes these scalar arguments before the call. */
extern void set_impact_ex_(VECTOR *pos, GsCOORDINATE2 *super,
                           short start_size, short end_size,
                           long start_color, long end_color,
                           /* s32 tail vs the definition's u16s is measured
                            * byte-required: this caller passes negative
                            * rotates, and the signed view keeps them li -30
                            * (addiu) instead of ori 0xffe2. */
                           s32 rotate, s32 rotate_speed, s32 time, s32 type);

typedef union
{
    PARAM_ITEM_LAUNCH p;
    struct
    {
        VECTOR pos;
        MapVector map;
    } query;
} ProcItemShinsokuScratch;

void ProcItemShinsoku(TItem *item)
{
    param_shinsoku *param;
    u8 ff;
    VECTOR pos;
    ProcItemShinsokuScratch scratch;

    param = &item->param.shinsoku;
    ff = ITEM_MODE_DISPOSE;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }

    switch (item->mode)
    {
    case 0:
        SetNowMotion(item->owner, 0xf05, 1);
        Sound(item->owner, 0x4c);
        item->mode++;
        return;

    case 1:
    {
        MotionManager *motion;

        motion = item->owner->motion;
        if (motion->mid != 0xf05)
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;
            s32 rand_x;
            s32 rand_y;
            s32 rand_z;
            PARAM_ITEM_LAUNCH *launchp;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            launchp = &scratch.p;
            memset(launchp, 0, sizeof(PARAM_ITEM_LAUNCH));
            launchp = 0;
            scratch.p.type = itemID;
            scratch.p.user = human;
            scratch.p.start.vx = pos->vx;
            scratch.p.start.vy = pos->vy;
            scratch.p.start.vz = pos->vz;
            rand_x = rand();
            scratch.p.end.vx = rand_x % 200 - 100;
            rand_y = rand();
            scratch.p.end.vy = rand_y % 100 - 200;
            rand_z = rand();
            scratch.p.end.vz = rand_z % 200 - 100;
            ReqItemDrop(&scratch.p);
            if (item->proc == 0)
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
        if (motion->count != 0)
        {
            return;
        }
        if (motion->loop == 0)
        {
            return;
        }
        spawn_smoke_burst_(item->owner->locate, 0x96, 0xc, 8);
        param->count = 75;
        item->mode++;
        return;
    }

    case 2:
    {
        Humanoid *human;
        ModelArchiveType *model;
        s32 valid;
        u16 buttons;
        s32 rotate;
        VECTOR *apos;

        if (item->owner->motion->mid != 0xf05)
        {
            if (item->proc == 0)
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

        pos.vx = item->owner->model->locate.coord.t[0];
        pos.vy = item->owner->model->locate.coord.t[1];
        pos.vz = item->owner->model->locate.coord.t[2];
        pos.vx += param->vec.vx;
        pos.vy += param->vec.vy;
        pos.vz += param->vec.vz;
        apos = &pos;
        scratch.query.pos.vx = apos->vx;
        scratch.query.pos.vy = apos->vy;
        scratch.query.pos.vz = apos->vz;
        scratch.query.pos.vy -= 2000;
        GetAreaMapVector(GlobalAreaMap,
                         &scratch.query.map,
                         &scratch.query.pos, 500, 0);
        if (scratch.query.map.level >= apos->vy - 500)
        {
            if (scratch.query.map.level < apos->vy)
            {
                apos->vy = scratch.query.map.level;
            }
            valid = 1;
        }
        else
        {
            valid = 0;
        }
        if (valid != 0)
        {
            item->owner->model->locate.coord.t[0] = pos.vx;
            item->owner->model->locate.coord.t[1] = pos.vy;
            item->owner->model->locate.coord.t[2] = pos.vz;
        }

        if ((param->count & 3) == 0)
        {
            scratch.query.pos =
                *(VECTOR *)item->owner->model->locate.coord.t;
            scratch.query.pos.vy -= 300;
            set_impact_ex_(&scratch.query.pos, 0, 0x2000, 0x5000,
                           0x808080, 0, 0, -30, 0x10, 3);
        }
        if (CamState.Owner == item->owner)
        {
            SetCameraMode(CMODE_CROUCH);
        }

        human = item->owner;
        buttons = human->pad.data;
        if ((buttons & PADLright) != 0)
        {
            model = human->model;
            rotate = 0x40;
            model->rotate.vy += rotate;
            RotateVectorS(&param->vec, 0, rotate, 0);
        }
        else if ((buttons & PADLleft) != 0)
        {
            model = human->model;
            rotate = -0x40;
            model->rotate.vy += rotate;
            RotateVectorS(&param->vec, 0, rotate, 0);
        }

        param->count--;
        if (param->count != 0 && (item->owner->pad.trig & (PADRleft | PADRdown | PADRright | PADRup)) == 0)
        {
            return;
        }
        NowReturnNormal(item->owner);
        SetCameraMode(CMODE_NORMAL);
        if (item->proc == 0)
        {
            return;
        }
        item->mode = ITEM_MODE_DISPOSE;
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
    }
}
