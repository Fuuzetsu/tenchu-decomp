#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcSightShot(struct tag_TItem *item);
 *     ITEM.C:939, 83 src lines, frame 128 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     reg   $v1       struct param_launch * param
 *     reg   $s1       struct tag_TItem * item
 *     reg   $a1       struct ModelType * model
 *     stack sp+16     struct PARAM_ITEM_LAUNCH param
 *     stack sp+56     struct SVECTOR rot
 *     stack sp+64     int rx
 *     stack sp+68     int ry
 *     stack sp+72     struct PARAM_ITEM_LAUNCH param
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

#include "item.h"
#include <psxsdk/libgs.h>

/*
 * MATCH.
 *
 * ProcSightShot (0x8003ea84) owns the first-person aiming item.  Releasing
 * the item restores the inventory or drops it if the user's launch motion
 * was interrupted.  While the motion is active it counts down the sight,
 * draws the target sprite, searches either from ViewInfo or the user's model
 * rotation, disposes the sight item, and launches the projectile.
 *
 * Matching notes:
 *  - `launch = (param_launch *)item->param` is formed in the entry mode-test
 *    delay slot and kept in a0 until the aiming body.
 *    `ff` is s32 and therefore receives the target's long-lived s4.
 *  - The drop block and common disposal block deliberately precede the
 *    `sight_mode` label.  Source-ordering the normal sight path first gives
 *    equivalent behavior but reverses the target's physical basic blocks.
 *  - `param`, `rot`, and `view_rot` reproduce the exact sp+0x10..0x47 local
 *    window.  The rotation outputs are an eight-byte aggregate whose useful
 *    halfwords sit at sp+0x40 and sp+0x44.
 *  - Keep the user's model as a `ModelArchiveType *` and take
 *    `&model->rotate` only in SearchItemTarget2.  Precomputing an `SVECTOR *`
 *    creates a separate RTL pseudo: the owner/model chain moves to v1/v0 and
 *    gains a nop.  The aggregate pointer lets GCC coalesce the chain into a1
 *    and interleave the independent sight-count load exactly.
 *  - The three disposal sequences are intentionally separate.  Sharing them
 *    changes branch placement and whether SetCameraMode(13) precedes launch.
 */
typedef struct
{
    u16 rx;
    u16 pad0;
    u16 ry;
    u16 pad1;
} SightRotation;

extern GsRVIEW2 ViewInfo;
extern GsSPRITE TargetSprite;
extern GsOT *OTablePt;

extern void SetCameraMode(s32 mode);
extern void GetVectorRotation(VECTOR *from, VECTOR *to, u16 *rx, u16 *ry);
extern void SearchItemTarget2(Humanoid *user, SVECTOR *dir, VECTOR *from,
                              VECTOR *target);
extern void ReqItemLaunch(PARAM_ITEM_LAUNCH *param);

void ProcSightShot(tag_TItem *item)
{
    param_launch *launch;
    s32 ff;
    PARAM_ITEM_LAUNCH param;
    SVECTOR rot;
    SightRotation view_rot;
    Humanoid *human;

    launch = (param_launch *)item->param;
    ff = 0xff;
    if (item->mode == ff)
    {
        item->owner->item[0x19] = 0;
        item->mode = 0;
        return;
    }

    human = item->owner;
    if (human->item[0x19] == 0)
    {
        u8 item_count;

        item_count = human->item[item->type];
        if (item_count != ff)
        {
            human->item[item->type] = item_count + 1;
        }
        SetCameraMode(1);
        goto dispose;
    }

    if (human->motion->mid == 0xe00)
    {
        goto sight_mode;
    }

    {
        VECTOR *pos;
        Humanoid *drop_owner;
        s32 itemID;

        pos = GetAbsolutePosition(item->locate, 0, 0, 0);
        drop_owner = item->owner;
        itemID = item->type;
        memset(&param, 0, sizeof(PARAM_ITEM_LAUNCH));
        param.type = itemID;
        param.user = drop_owner;
        param.start.vx = pos->vx;
        param.start.vy = pos->vy;
        param.start.vz = pos->vz;
        param.end.vx = rand() % 200 - 100;
        param.end.vy = rand() % 100 - 200;
        param.end.vz = rand() % 200 - 100;
        ReqItemDrop(&param);
    }

dispose:
    if (item->proc != 0)
    {
        item->mode = ff;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
        {
            AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
    }
    return;

sight_mode:
    {
        u8 count;
        ModelArchiveType *model;

        count = launch->count;
        if (count != 0)
        {
            launch->count = count - 1;
        }
        if ((item->owner->pad.data & 0x10) != 0)
        {
            if (launch->count != 0)
            {
                return;
            }
            SetCameraMode(3);
            GsSortSprite(&TargetSprite, OTablePt, 0);
            return;
        }

        count = launch->count;
        model = item->owner->model;
        if (count == 0)
        {
            GsRVIEW2 *view;

            param.type = item->type;
            param.user = item->owner;
            param.start.vx = item->locate->locate.coord.t[0];
            param.start.vy = item->locate->locate.coord.t[1];
            param.start.vz = item->locate->locate.coord.t[2];
            view = &ViewInfo;
            GetVectorRotation((VECTOR *)view, (VECTOR *)&view->vrx,
                              &view_rot.rx, &view_rot.ry);
            rot.vz = 0;
            rot.vx = view_rot.rx;
            rot.vy = view_rot.ry;
            SearchItemTarget2(param.user, &rot, (VECTOR *)view, &param.end);
            if (item->proc != 0)
            {
                item->mode = ff;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != 0)
                {
                    AdtMessageBox(D_800121CC, item->type,
                                  (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
            }
            SetCameraMode(13);
        }
        else
        {
            param.type = item->type;
            param.user = item->owner;
            param.start.vx = item->locate->locate.coord.t[0];
            param.start.vy = item->locate->locate.coord.t[1];
            param.start.vz = item->locate->locate.coord.t[2];
            SearchItemTarget2(param.user, &model->rotate, &param.start,
                              &param.end);
            if (item->proc != 0)
            {
                item->mode = ff;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != 0)
                {
                    AdtMessageBox(D_800121CC, item->type,
                                  (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
            }
        }
        ReqItemLaunch(&param);
    }
}
