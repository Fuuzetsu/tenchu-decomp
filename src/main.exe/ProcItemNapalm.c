#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ProcItemNapalm (0x800469c0) — expands the thrown fireball for twenty
 * frames, updates its two layered sprites, registers the damage conflict at
 * frame ten, emits a random attached frame effect on a collided character,
 * and disposes when it leaves the area map.  Modes 0/1/2 are initialise,
 * animate, and dispose; unknown modes still take the shared draw tail.
 *
 * Matching notes:
 *  - `ff` is live to mode 2 in $s6, while `switch (item->mode)` deliberately
 *    reloads the mode into $s4.  Literal `1` stores in case 1 reuse that switch
 *    index through cse's taken-edge equivalence; an explicit `mode` local adds
 *    $s7 and changes the frame.
 *  - The colour `t` is `u8` and is assigned in three statements.  Its narrow
 *    mode keeps rand()%25's quotient separate from the final remainder, giving
 *    the target's $v0/$v1 chain and the copy used by the third byte store.
 *    A single int expression is one instruction short and globally re-colours
 *    the chain.
 *  - The InsertConflict result is block-local `n`, distinct from the later
 *    query `cid`; sharing them rotates every register in the 0x78-byte index
 *    calculation.  Likewise, cache `proc` for the null check but invoke through
 *    `item->proc`: cse retains one load and allocates the target in $v0.
 *  - `random_pos` is zeroed and copied wholesale to `pos`; the two VECTOR
 *    declarations reproduce the sp+0x18/sp+0x28 stack objects and four-word
 *    block copy before SetFrame.
 *  - Both cleanup sequences are written out so jump2 cross-jumps from the map
 *    failure into mode 2's copy.  The two coordinate struct assignments at the
 *    end intentionally lower to the target's 0x50-byte copy loops.
 *  - This TU needs maspsx `--expand-div` for the model-count remainder guards
 *    and `--gp-extern sprNapalm2` for the six retail gp-relative loads; Build.hs
 *    and permute.py carry the mirrored per-function settings.
 */

extern s32 is_humanoid_on_stage_(Humanoid *human);
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemNapalm(struct tag_TItem *item);
 *     ITEM.C:3014, 71 src lines, frame 80 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     reg   $s5       struct Sprite3D * model
 *     reg   $s3       struct param_napalm * param
 *     reg   $s0       int ex
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       struct ModelType * model
 *     stack sp+16     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprNapalm2;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

void ProcItemNapalm(TItem *item)
{
    enum
    {
        MaxCount = 20
    };
    Sprite3D *model;
    param_napalm *param;
    void (*proc)(TItem *);
    u8 ff;
    u8 count;
    s32 ex;
    s32 cid;

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
        param->count = 0;
        item->mode++;
        return;

    case 1:
    {
        u8 t;

        ex = param->count;
        ex = ex * ex;
        item->locate->locate.coord.t[0] += param->vec.vx * ex / 100;
        item->locate->locate.coord.t[1] += param->vec.vy * ex / 100;
        item->locate->locate.coord.t[2] += param->vec.vz * ex / 100;

        t = rand() % 25;
        t = t - 26;
        t = t - param->count * 230 / MaxCount;
        model->sprite.r = t;
        model->sprite.g = model->sprite.r;
        model->sprite.b = model->sprite.r;
        model->sprite.rotate = (rand() % 360) << 12;
        model->scale = (ex << 12) / 50 + 0x1000;

        sprNapalm2->sprite.r = (ff - model->sprite.r) / 3;
        sprNapalm2->sprite.g = sprNapalm2->sprite.r;
        sprNapalm2->sprite.b = sprNapalm2->sprite.r;
        sprNapalm2->sprite.rotate = model->sprite.rotate;
        sprNapalm2->scale = model->scale;

        if (param->count == 10)
        {
            s32 n;

            DeleteConflict(item->locate);
            n = InsertConflict(item->locate);
            ConflictObject[n].offset.vx = 0;
            ConflictObject[n].offset.vz = 0;
            ConflictObject[n].offset.vy = 0;
            ConflictObject[n].size.vz = 500;
            ConflictObject[n].size.vy = 500;
            ConflictObject[n].size.vx = 500;
            ConflictObject[n].common = CONFLICT_OWNER_ITEM;
            ConflictObject[n].size.pad = CONFLICT_HIT;
            item->collision.size = 500;
            item->collision.ofsY = 0;
            item->collision.mode = 1;
            item->collision.pause = 0;
        }

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
        if (cid != -1)
        {
            Humanoid *human;

            human = (Humanoid *)ConflictObject[cid].common;
            if (is_humanoid_on_stage_(human) != 0)
            {
                ModelType **objects;
                ModelType *frame_model;
                VECTOR pos;
                VECTOR random_pos;

                objects = human->model->object;
                if (human->model->n > 0)
                {
                    objects += rand() % human->model->n;
                }
                frame_model = *objects;
                memset(&random_pos, 0, sizeof(VECTOR));
                random_pos.vx = rand() % 200 - 100;
                random_pos.vy = rand() % 200 - 100;
                random_pos.vz = rand() % 200 - 100;
                pos = random_pos;
                SetFrame(&pos, 0x3000, 60,
                         (GsCOORDINATE2 *)frame_model);
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

    case 2:
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
    sprNapalm2->locate = item->locate->locate;
    DrawSprite(sprNapalm2);
    model->locate = item->locate->locate;
    DrawSprite(model);
}
