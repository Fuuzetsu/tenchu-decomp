#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

extern SVECTOR svec_y_n150[];

extern s32 is_humanoid_on_stage_(Humanoid *human);
extern s16 Think1sleep(void);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemNemuri(struct tag_TItem *item);
 *     ITEM.C:2738, 93 src lines, frame 112 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s3       struct param_napalm * param
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       struct VECTOR * pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s1       int env
 *     reg   $v0       int bright
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s3       struct Humanoid * human
 *     reg   $s3       struct Humanoid * human
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     stack sp+48     struct VECTOR pos
 *     reg   $s3       struct Humanoid * human
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * ProcItemNemuri (0x800458a8) — the sleeping-powder item. mode 0 starts
 * the owner's throw motion 0xf02 and plays sound 0x26. mode 1 waits for
 * that motion to reach count 3, then places the item at the owner's
 * model object[14] and installs a 1000-unit CONFLICT_SOFT gas cloud;
 * any other motion means the throw was interrupted and the item
 * disposes. mode 2 is the live cloud: it drifts along param->vec,
 * pulses its colour, scale and spin off rsin of its own frame counter,
 * sheds two grey (0x6e6e6e) bleed particles per frame, and expires past
 * 100 frames. A conflict against any on-stage humanoid other than the
 * owner puffs smoke, plays sound 0x23, and — for a living victim not
 * already in MOT_ACTION — disarms it (EquipWeapon 0), plays motion
 * 0x80f, installs Think1sleep and clears the phase bits unless it is a
 * PAGE_BOSS, then forces MOT_ACTION and Sound 6 before disposing.
 * Leaving the area map (GetAreaMapLevel returning LEVEL_NONE) and mode
 * 3 both dispose; every surviving frame redraws the sprite at the
 * item's coordinate.
 */

/*
 * Advances the sleeping-powder projectile, draws its pulsing sprite, puts a
 * collided humanoid to sleep, and disposes the item after impact, expiry, or
 * leaving the area map.
 *
 * Matching notes:
 *  - `eight` shares the collision-mode constant between a halfword and a word
 *    store; two literals produce an extra `li`.
 *  - The byte-identical `bleed_n` arms disappear in jump2, but their CFG keeps
 *    the call-count and colour pseudos out of the wave's register.  `bleed_n`
 *    is initialized, and is overwritten with its real value before the call.
 *  - The nested zero-trip loops emit no instructions.  Their loop notes weight
 *    `env` to 9 refs / 49 RTL insns (priority 5510), above `wave`'s 5 / 35
 *    (2857), selecting the target $t0/$t1 allocation.  Splitting the colour
 *    across the two statements then puts its `lui` in the branch delay slot
 *    and its `ori` at the join.
 *  - `bleed_range` is assigned after the duplicated X update so its $a1 copy
 *    fills that update's load delay.  The full-width `rotate_count` similarly
 *    schedules the count load before the scale store without an `andi 0xff`.
 *  - The cleanup paths cache `proc` for the null check but call through
 *    `item->proc`; jump2 cross-jumps them into the target shared tail.
 */
void ProcItemNemuri(TItem *item)
{
    enum
    {
        MaxCount = 100
    };
    Sprite3D *model;
    param_napalm *param;
    void (*proc)(TItem *);
    u8 ff;
    u8 count;
    s32 rotate_count;

    model = (Sprite3D *)item->model;
    param = &item->param.napalm;
    ff = ITEM_MODE_DISPOSE;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }

    switch (item->mode)
    {
    case 0:
        SetNowMotion(item->owner, 0xf02, 1);
        SoundEx((VECTOR *)item->owner->model->locate.coord.t, 0x26);
        item->mode++;
        return;

    case 1:
        if (item->owner->motion->mid == 0xf02)
        {
            if (item->owner->motion->count != 3)
            {
                return;
            }
            {
                VECTOR *position;
                s32 n;
                s32 eight;

                position = GetAbsolutePosition(
                    item->owner->model->object[14], 0, 0, 0);
                param->count = 0;
                item->mode++;
                item->locate->locate.coord.t[0] = position->vx;
                item->locate->locate.coord.t[1] = position->vy;
                item->locate->locate.coord.t[2] = position->vz;
                DeleteConflict(item->locate);
                n = InsertConflict(item->locate);
                eight = 8;
                ConflictObject[n].offset.vx = 0;
                ConflictObject[n].offset.vz = 0;
                ConflictObject[n].offset.vy = 0;
                ConflictObject[n].size.vz = 1000;
                ConflictObject[n].size.vy = 1000;
                ConflictObject[n].size.vx = 1000;
                ConflictObject[n].common = (void *)1;
                ConflictObject[n].size.pad = eight;
                item->collision.size = 1000;
                item->collision.ofsY = 0;
                item->collision.mode = eight;
                item->collision.pause = 0;
                return;
            }
        }
        proc = item->proc;
        if (proc == 0)
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

    case 2:
    {
        s32 wave;
        s32 bright;
        s32 cid;
        s32 dead;
        s32 env;
        s32 bleed_n = 0;
        s32 bleed_range;
        Humanoid *human;

        wave = rsin(param->count * 0x88);
        if (wave < 0)
        {
            wave += 0x3f;
        }
        if (bleed_n != 0)
        {
            env = 0x6e0000;
            env |= 0x6e6e;
            item->locate->locate.coord.t[0] +=
                item->param.napalm.vec.vx;
            bleed_range = 300;
        }
        else
        {
            env = 0x6e0000;
            env |= 0x6e6e;
            item->locate->locate.coord.t[0] +=
                item->param.napalm.vec.vx;
            bleed_range = 300;
        }
        bleed_n = 2;
        item->locate->locate.coord.t[1] += param->vec.vy;
        item->locate->locate.coord.t[2] += param->vec.vz;
        bright = (wave >> 6) + 0x80;
        model->sprite.r = bright;
        model->sprite.g = bright;
        model->sprite.b = bright;
        rotate_count = param->count;
        model->scale = bright * 2 + 0x4000;
        model->sprite.rotate = rotate_count * 0x2d000;
        SetBleeds((VECTOR *)item->locate->locate.coord.t,
                  bleed_range, 10, bleed_n, 10, env);

        count = param->count + 1;
        param->count = count;
        if (count > MaxCount)
        {
            item->mode++;
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            cid = -1;
        }
        else
        {
            cid = GetConflictResult(item->locate, -1);
        }
        dead = -1;
        if (cid != dead)
        {
            human = (Humanoid *)ConflictObject[cid].common;
            if (is_humanoid_on_stage_(human) != 0 &&
                human != item->owner)
            {
                VECTOR random_pos;
                VECTOR random_buf;
                VECTOR smoke_pos;
                VECTOR human_buf;
                SVECTOR *vec;
                s16 life;

                if (human->model->n > 0)
                {
                    rand();
                }
                vec = (SVECTOR *)&random_buf;
                memset(&random_buf, 0, sizeof(VECTOR));
                random_buf.vx = rand() % 200 - 100;
                random_buf.vy = rand() % 200 - 100;
                random_buf.vz = rand() % 200 - 100;
                /* Dead copy, but retail's own: the 12-byte struct copy is in
                 * the shipped bytes (removal measures -32). The jittered
                 * position is computed and then never passed anywhere --
                 * SetSmoke below gets the body position instead. */
                random_pos = random_buf;
                SoundEx((VECTOR *)item->locate->locate.coord.t, 0x23);

                *vec = svec_y_n150[0];
                memset(&human_buf, 0, sizeof(VECTOR));
                human_buf.vx = human->model->locate.coord.t[0];
                human_buf.vy = human->model->locate.coord.t[1];
                human_buf.vz = human->model->locate.coord.t[2];
                smoke_pos = human_buf;
                SetSmoke(&smoke_pos, vec, 10, 30);

                life = human->life;
                if (life > 0 && human->motion->mid != MOT_ACTION)
                {
                    if ((human->type & 0xf0) != PAGE_BOSS && life != dead)
                    {
                        EquipWeapon(human, 0);
                        SetNowMotion(human, 0x80f, 1);
                        human->think[0] = Think1sleep;
                        human->attribute &= ~ATTR_PHASE;
                    }
                    SetNowMotion(human, 0x100, 1);
                    Sound(human, 6);
                }

                proc = item->proc;
                if (proc == 0)
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
        }

        if (GetAreaMapLevel(GlobalAreaMap,
                            item->locate->locate.coord.t[0],
                            item->locate->locate.coord.t[1],
                            item->locate->locate.coord.t[2], 0) ==
            (long)0x80000000)
        {
            proc = item->proc;
            if (proc == 0)
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
        break;
    }

    case 3:
        proc = item->proc;
        if (proc == 0)
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

    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);
}
