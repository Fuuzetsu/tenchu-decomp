#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcKaginawa(struct tag_TItem *item);
 *     ITEM.C:1026, 67 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     stack sp+16     int rx
 *     stack sp+20     int ry
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int dist
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

#include "item.h"

/*
 * ProcKaginawa (0x8003eea4) — the grappling-hook (kaginawa) item processor.
 * Three-way dispatch on the owner's hook state, all converging on the shared
 * ProcItem dispose tail:
 *  - owner->hookflag (Humanoid+0xCD) == 0: not yet thrown — enter DIRECTION
 *    camera mode and dispose.
 *  - owner is mid-throw but not in the hook-fly motion (motion->mid != MOT_KAGI):
 *    dispose.
 *  - owner IS in the hook-fly motion (mid == MOT_KAGI): aim the camera along the
 *    throw (GetVectorRotation off ViewInfo), and if the sight bit is held just
 *    re-sort the reticle sprite and bail; otherwise walk the camera target
 *    toward the hook tip (RotateVector a fixed offset, trace_ground_ a step,
 *    accumulate into CamState.TargetVector), snap onto the owner's model once
 *    close enough or the pitch turned upward, drop into LOCK camera mode,
 *    clear the hook flag, and dispose.
 *
 * Matching notes (see ProcItemTeleport.c / ProcItemKusuri.c for the item-TU
 * conventions this shares):
 *  - `dispose_mode = ITEM_MODE_DISPOSE` (0xff) is a callee-saved var ($s1),
 *    tested at entry and reused as `item->mode = dispose_mode` in the first
 *    two dispose blocks; in the big third block $s1 has been repurposed for
 *    the ViewInfo/CamState addresses, so the same `dispose_mode` variable is
 *    rematerialised as a fresh `li 0xff` there.
 *  - `owner = item->owner` is ONE load feeding both the hookflag test and the
 *    motion->mid test (caller-saved $v1, dies at the first call); the big
 *    block reloads item->owner for its pad-bit test and hookflag clear.
 *  - The dispose tail is written out in all three branches; jump2's
 *    cross-jump merges the identical `jalr`-onward suffix into one shared
 *    tail after the third branch (the mode-store instruction differs — $s1
 *    vs rematerialised $v1 — so the merge starts at the call, not earlier).
 *  - The hook flag lives at Humanoid+0xCD, i.e. `owner->item[ITEM_N]` one past
 *    the DoInfoViewProc-indexed slots (item.h sizes item[] to 0x1A to cover
 *    it); read `lbu`, written `sb 0`.
 *  - `w.vx = v.vx; …; w.vx += ViewInfo.vpx; …` is the two-phase raw-copy-
 *    then-add the target stores twice per field (a single `w.vx = v.vx +
 *    ViewInfo.vpx` would store once); v/w are separate VECTOR locals.
 *  - `v = vec_z_n20000;` is a plain extern VECTOR struct assignment — under
 *    -msplit-addresses the 16-byte (non-small) source address builds as a
 *    two-register lui/addiu pair and the four words copy through it.
 */
#include <psxsdk/libgs.h>

extern VECTOR vec_z_n20000; /* {0,0,-20000} */

void ProcKaginawa(TItem *item)
{
    void (*item_proc)(TItem *);
    Humanoid *owner;
    VECTOR v;
    VECTOR w;
    s32 rx, ry;
    s32 dist;
    s32 tx, ty, tz;
    u8 dispose_mode;

    dispose_mode = ITEM_MODE_DISPOSE;
    if (item->mode == dispose_mode)
    {
        item->mode = 0;
        return;
    }
    owner = item->owner;
    if (owner->item[ITEM_N] == 0)
    {
        SetCameraMode(CMODE_DIRECTION);
        item_proc = item->proc;
        if (item_proc == 0)
            return;
        item->mode = dispose_mode;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        item->owner = 0;
        item->proc = 0;
    }
    else if (owner->motion->mid != MOT_KAGI)
    {
        item_proc = item->proc;
        if (item_proc == 0)
            return;
        item->mode = dispose_mode;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        item->owner = 0;
        item->proc = 0;
    }
    else
    {
        GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
        if (item->owner->pad.data & PADRup)
        {
            if (rx < 0)
                GsSortSprite(TargetSprite, OTablePt, 0);
            return;
        }
        v = vec_z_n20000;
        RotateVector(&v, rx, ry, 0);
        w.vx = v.vx;
        w.vy = v.vy;
        w.vz = v.vz;
        w.vx += ViewInfo.vpx;
        w.vy += ViewInfo.vpy;
        w.vz += ViewInfo.vpz;
        trace_ground_((VECTOR *)&ViewInfo, &w, (VECTOR *)&CamState, 0);
        tx = v.vx;
        if (tx < 0)
            tx += 0xF;
        v.vx = tx >> 4;
        ty = v.vy;
        if (ty < 0)
            ty += 0xF;
        v.vy = ty >> 4;
        tz = v.vz;
        if (tz < 0)
            tz += 0xF;
        v.vz = tz >> 4;
        CamState.TargetVector.vx += v.vx;
        CamState.TargetVector.vy += v.vy;
        CamState.TargetVector.vz += v.vz;
        dist = GetVectorDistance((VECTOR *)CamState.Owner->model->locate.coord.t, &CamState.TargetVector);
        if (rx > 0 || dist > 15000)
        {
            CamState.TargetVector = *(VECTOR *)CamState.Owner->model->locate.coord.t;
        }
        SetCameraMode(CMODE_LOCK);
        item->owner->item[ITEM_N] = 0;
        item_proc = item->proc;
        if (item_proc == 0)
            return;
        item->mode = dispose_mode;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        item->owner = 0;
        item->proc = 0;
    }
}
