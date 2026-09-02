#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetCameraMode(enum TCameraMode mode);
 *     CAMERA.C:85, 9 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       enum TCameraMode mode
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct TCameraPos CamPosCriticalHit[3];
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

/* Scratchpad rotation at 0x1f800040 and matrix translation at 0x1f800094. */
extern SVECTOR scratch_rot_1f800040;
extern s32 scratch_trans_1f800094[2];

/* Retail declares this s16 here although the definition returns s32. */
extern short camera_terrain_pitch_(Humanoid *h);

void SetCameraMode(TCameraMode mode)
{
    VECTOR va;
    VECTOR vb;
    VECTOR vc;
    VECTOR vd;
    long flag;
    s32 rx;
    s32 ry;
    TCameraStatus *cs;
    TCameraPos *tbl;
    long *fp;
    VECTOR *pos;
    SVECTOR *rot;
    TCameraPos *camera;
    VECTOR *pv;
    s32 i;
    s32 hitf;

    switch (mode)
    {
    case CMODE_CRITICAL_HIT:
        CamState.OldMode = rand() % N_CRITICAL_CAMERA_POSITIONS;
        i = 0;
        cs = &CamState;
        tbl = CamPosCriticalHit;
        fp = &flag;
    loop:
        if (!(i < N_CRITICAL_CAMERA_POSITIONS))
            goto giveup;
        cs->OldMode++;
        if (cs->OldMode > MaxCriticalValiation)
            cs->OldMode = 0;
        camera = (TCameraPos *)(cs->OldMode * sizeof(*tbl) + (s32)tbl);
        pos = cs->Owner->locate;
        rot = cs->Owner->rotate;
        do
        {
            scratch_rot_1f800040.vx = rot->vx + camera_terrain_pitch_(cs->Owner);
            scratch_rot_1f800040.vy = rot->vy;
            scratch_rot_1f800040.vz = rot->vz;
            RotMatrixYXZ((SVECTOR *)TENCHU_SCRATCHPAD(0x40),
                         (MATRIX *)TENCHU_SCRATCHPAD(0x80));
        } while (0);
        scratch_trans_1f800094[0] = pos->vx;
        scratch_trans_1f800094[1] = pos->vy;
        scratch_trans_1f800094[2] = pos->vz;
        SetRotMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x80));
        SetTransMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x80));
        do
        {
            RotTrans(&camera->r1, &va, fp);
            RotTrans(&camera->r2, &vb, fp);
            RotTrans(&camera->p1, pv = &vc, fp);
            RotTrans(&camera->p2, pv = &vd, fp);
        } while (0);
        pv = 0;
        hitf = trace_ground_(&vc, &vd, 0, 0) > 0x7ff;
        i++;
        if (hitf)
            goto hit;
        goto loop;
    giveup:
        CamState.OldMode = 0;
        break;
    case CMODE_AIM:
        CamState.DirectionRX = 0;
        CamState.DirectionRY = 0;
        CamState.Mode = CMODE_DIRECTION;
        CamState.OldMode = mode;
        break;
    case CMODE_DIRECTION:
    case CMODE_SIGHT:
        if (CamState.Mode != CMODE_DIRECTION && CamState.Mode != CMODE_LOCK)
        {
            GetVectorRotation(CAMERA_VIEWPOINT(&ViewInfo),
                              CAMERA_REFERENCE(&ViewInfo), &rx, &ry);
            rx -= CamState.Owner->model->rotate.vx;
            ry -= CamState.Owner->model->rotate.vy;
            rx = (rx + 2 * ANGLE_FULL + ANGLE_HALF) % ANGLE_FULL - ANGLE_HALF;
            ry = (ry + 2 * ANGLE_FULL + ANGLE_HALF) % ANGLE_FULL - ANGLE_HALF;
            CamState.DirectionRX = 0;
            CamState.DirectionRY = ry;
        }
        CamState.Mode = CMODE_DIRECTION;
        CamState.OldMode = mode;
        break;
    case CMODE_NORMAL:
        if (CamState.Owner->pad.data & PADL1)
        {
            SetCameraMode(CMODE_DIRECTION);
            return;
        }
        CamState.Mode = mode;
        break;
    hit:
        cs->Mode = mode;
        cs->snap_pending = 1;
        break;
    default:
        CamState.Mode = mode;
        break;
    }
}
