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
 *    window. The same launch request is populated by the drop and fire
 *    modes; the latter also uses the trailing two words as camera rotation
 *    outputs.
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
 *    `item->owner->model` path. A named model assignment instead colors the
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
    PARAM_ITEM_LAUNCH request;
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
        if (item->owner->motion->mid == MOT_ITEM_KAENGEKI)
        {
            NowReturnNormal(item->owner);
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

        human = item->owner;
        if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
        {
            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_ITEM_KAENGEKI);
            human->status = STAT_ITEM;
            MoveHumanoid(human, human->motion->motion->orderspd,
                         human->motion->motion->sidespd);
        }
        Sound(item->owner, SE_ITEM_USE);
        item->mode++;
        return;
    }

    case KAENGEKI_MODE_WAIT:
    {
        if (item->owner->motion->count == 0 &&
            item->owner->motion->loop != 0)
        {
            SoundEx((VECTOR *)item->owner->model->locate.coord.t, SE_FIRE);
            item->mode++;
            param->count = KAENGEKI_DELAY;
        }
        if (item->owner->motion->mid == MOT_ITEM_KAENGEKI)
        {
            return;
        }
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            memset(&request, 0, sizeof(request));
            request.type = itemID;
            request.user = human;
            request.start.vx = pos->vx;
            request.start.vy = pos->vy;
            request.start.vz = pos->vz;
            request.end.vx = rand() % 200 - 100;
            request.end.vy = rand() % 100 - 200;
            request.end.vz = rand() % 200 - 100;
            ReqItemDrop(&request);
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

        if (item->owner->motion->mid != MOT_ITEM_KAENGEKI)
        {
            goto dispose;
        }
        if (--param->count == 0)
        {
            goto dispose;
        }

        if ((item->owner->pad.data & PADLright) != 0)
        {
            item->owner->model->rotate.vy += 0x20;
        }
        else if ((item->owner->pad.data & PADLleft) != 0)
        {
            item->owner->model->rotate.vy -= 0x20;
        }

        request.user = item->owner;
        request.type = ITEM_NAPALM;
        request.end.vx = param->end.vx;
        request.end.vy = param->end.vy;
        request.end.vz = param->end.vz;
        model = item->owner->model;
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
        RotateVector(&request.end, rx, ry, rz);

        request.start.vx = request.end.vx;
        request.start.vy = request.end.vy;
        request.start.vz = request.end.vz;
        request.start.vx *= 12;
        request.start.vy *= 12;
        request.start.vz *= 12;
        request.start.vx += param->start.vx;
        request.start.vy += param->start.vy;
        request.start.vz += param->start.vz;
        request.end.vx *= 2;
        request.end.vy *= 2;
        request.end.vz *= 2;
        request.end.vx += request.start.vx;
        request.end.vy += request.start.vy;
        request.end.vz += request.start.vz;
        ReqItemUse(&request);
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
        item->owner = 0;
        item->proc = 0;
        return;
    }
    }
}
