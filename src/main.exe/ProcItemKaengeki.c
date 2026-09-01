#include "common.h"
#include "main.exe.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemKaengeki(struct tag_TItem *item);
 *     ITEM.C:2287, 54 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_TItem * item
 *     reg   $s1       struct param_kaengeki * param
 *     reg   $s0       struct Humanoid * human
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+16     struct PARAM_ITEM_LAUNCH rp
 *
 * Globals it touches, as the original declared them:
 *     extern short ActionHalt;
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

#include "item.h"

/*
 * MATCH.
 *
 * ProcItemKaengeki (0x80043710) runs the fire-breath item.  It starts the
 * owner's use animation, drops the item if that animation is interrupted,
 * then spends 40 frames steering the owner and launching camera-relative
 * napalm requests.
 *
 * Matching notes:
 *  - `request`, `rx`, and `ry` are one contiguous sp+0x10..0x3f working
 *    window. The request union gives the shared 0x28-byte slot PSX.SYM's `p`
 *    view in mode 1 and `rp` view in mode 2; the latter also uses the
 *    trailing two words as camera rotation outputs.
 *  - `mode_index = 0` is a zero-byte CSE eviction.  Naming the entry mode
 *    load and dead-overwriting that local before the switch makes
 *    expand_case emit the target's fresh second `lbu`; a direct switch after
 *    the entry guard incorrectly reuses the first load.
 *  - `dispose_mode` is an s32 caller-saved local in a1.  It remains live from the entry
 *    compare to mode 2's no-call dispose path, while mode 1 rematerializes
 *    ITEM_MODE_DISPOSE as a fresh 0xff value after its calls. That difference
 *    keeps the two dispose prefixes separate while cross-jump merges them at
 *    the indirect call.
 *  - The completed request assigns user before type.  This prevents the
 *    type's `li 22` from filling the steering guard's delay slot and yields
 *    the target load/li/store ordering at the request head.
 *  - The steering writes use direct compound expressions through the owning
 *    `item->owner.human->model` path. A named model assignment instead colors the
 *    model into a0.
 *  - The staged vector statements are intentional: copy rotated end to
 *    start, scale start by 12, add the saved origin, double end, then add
 *    start.  They reproduce both rounds of stack stores in the target.
 */

extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);

void ProcItemKaengeki(TItem *item)
{
    enum
    {
        KAENGEKI_MODE_START = 0,
        KAENGEKI_MODE_WAIT = 1,
        KAENGEKI_MODE_FIRE = 2
    };
    param_kaengeki *param;
    union
    {
        PARAM_ITEM_LAUNCH p;
        PARAM_ITEM_LAUNCH rp;
    } request;
    void (*ppu)(TItem *);
    s32 rx;
    s32 ry;
    s32 dispose_mode;
    item_mode mode_index;

    param = &item->param.kaengeki;
    dispose_mode = ITEM_MODE_DISPOSE;
    mode_index = item->mode;
    if (mode_index == dispose_mode)
    {
        if (item->owner.human->motion->mid == MOT_ITEM_KAENGEKI)
        {
            NowReturnNormal(item->owner.human);
        }
        item->mode = KAENGEKI_MODE_START;
        return;
    }

    mode_index = KAENGEKI_MODE_START;
    switch (item->mode)
    {
    case KAENGEKI_MODE_START:
    {
        Humanoid *human;

        human = item->owner.human;
        if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
        {
            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_ITEM_KAENGEKI);
            human->status = STAT_ITEM;
            MoveHumanoid(human, human->motion->motion->orderspd,
                         human->motion->motion->sidespd);
        }
        Sound(item->owner.human, SE_ITEM_USE);
        item->mode++;
        return;
    }

    case KAENGEKI_MODE_WAIT:
    {
        if (item->owner.human->motion->count == 0 &&
            item->owner.human->motion->loop != 0)
        {
            SoundEx((VECTOR *)item->owner.human->model->locate.coord.t, SE_FIRE);
            item->mode++;
            param->count = KAENGEKI_DELAY;
        }
        if (item->owner.human->motion->mid == MOT_ITEM_KAENGEKI)
        {
            return;
        }
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner.human;
            itemID = item->type;
            memset(&request.p, 0, sizeof(PARAM_ITEM_LAUNCH));
            request.p.type = itemID;
            request.p.user.human = human;
            request.p.start.vx = pos->vx;
            request.p.start.vy = pos->vy;
            request.p.start.vz = pos->vz;
            request.p.end.vx = rand() % 200 - 100;
            request.p.end.vy = rand() % 100 - 200;
            request.p.end.vz = rand() % 200 - 100;
            ReqItemDrop(&request.p);
            ppu = item->proc;
            if (ppu == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
    }

    case KAENGEKI_MODE_FIRE:
    {
        ModelArchiveType *model;
        s32 rz;

        if (item->owner.human->motion->mid != MOT_ITEM_KAENGEKI)
        {
            goto dispose;
        }
        if (--param->count == 0)
        {
            goto dispose;
        }

        if ((item->owner.human->pad.data & PADLright) != 0)
        {
            item->owner.human->model->rotate.vy += 0x20;
        }
        else if ((item->owner.human->pad.data & PADLleft) != 0)
        {
            item->owner.human->model->rotate.vy -= 0x20;
        }

        request.rp.user.human = item->owner.human;
        request.rp.type = ITEM_NAPALM;
        request.rp.end.vx = param->end.vx;
        request.rp.end.vy = param->end.vy;
        request.rp.end.vz = param->end.vz;
        model = item->owner.human->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx,
                              &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(&request.rp.end, rx, ry, rz);

        request.rp.start.vx = request.rp.end.vx;
        request.rp.start.vy = request.rp.end.vy;
        request.rp.start.vz = request.rp.end.vz;
        request.rp.start.vx *= 12;
        request.rp.start.vy *= 12;
        request.rp.start.vz *= 12;
        request.rp.start.vx += param->start.vx;
        request.rp.start.vy += param->start.vy;
        request.rp.start.vz += param->start.vz;
        request.rp.end.vx *= 2;
        request.rp.end.vy *= 2;
        request.rp.end.vz *= 2;
        request.rp.end.vx += request.rp.start.vx;
        request.rp.end.vy += request.rp.start.vy;
        request.rp.end.vz += request.rp.start.vz;
        ReqItemUse(&request.rp);
        return;

    dispose:
        if (item->proc == 0)
        {
            return;
        }
        item->mode = dispose_mode;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != KAENGEKI_MODE_START)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner.human = 0;
        item->proc = 0;
        return;
    }
    }
}
