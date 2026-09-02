#include "common.h"
#include "tuning.h"
#include "main.exe.h"

#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemTeleport(struct tag_TItem *item);
 *     ITEM.C:1097, 39 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

extern void SnapCameraTargetVector(void);

void ProcItemTeleport(TItem *item)
{
    void (*ppu)(TItem *);
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = ITEM_MODE_START;
        return;
    }
    if ((item->owner->pad.data & PADRup) != 0)
    {
        SetCameraMode(CMODE_SIGHT);
        GsSortSprite(TargetSprite, OTablePt, 0);
        return;
    }
    SnapCameraTargetVector();
    if (GetVectorDistance(MODEL_POSITION(CamState.Owner->model), &CamState.TargetVector) < 20000)
    {
        CamState.Owner->model->locate.coord.t[0] = CamState.TargetVector.vx;
        CamState.Owner->model->locate.coord.t[1] = CamState.TargetVector.vy;
        CamState.Owner->model->locate.coord.t[2] = CamState.TargetVector.vz;
        SetBleeds(&CamState.TargetVector, 1000, 20, 50, 60, COLOR_WHITE);
    }
    SetCameraMode(CMODE_NORMAL);
    ppu = item->proc;
    if (ppu == 0)
        return;
    DISPOSE_ITEM(item);
}
