#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SoundEx(struct VECTOR *locate, short seid);
 *     SEMNG.C:42, 15 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * locate
 *     param $a1       short seid
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern struct SoundEffect *StageSE;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

extern short PlaySE(SoundEffect *se, short pt, long dv);

short SoundEx(VECTOR *locate, short seid)
{
    VECTOR *pp;
    s32 dist, dx, dz;
    s32 maxvol;
    s32 dy;
    s32 angle;
    s32 vol;
    s32 raw;

    pp = StagePlayer->locate;
    if (locate == 0 || locate == pp)
    {
        return PlaySE(
            StageSE, seid,
            SOUND_SPATIAL(0, (seid == SE_RUN_STEP) ? 0x3f : SOUND_VOLUME_MAX));
    }

    dx = locate->vx - pp->vx;
    dz = locate->vz - pp->vz;
    dist = SquareRoot0(dx * dx + dz * dz);
    if (dist >= 18000)
    {
        return -1;
    }
    raw = locate->vy - pp->vy;
    dy = (raw >= 0) ? raw : -raw;
    if (dy >= 10000)
    {
        return -1;
    }
    maxvol = SOUND_VOLUME_MAX;
    if (dist < 2000 && dy < 2000)
    {
        angle = 0;
        dist = SOUND_VOLUME_MAX;
    }
    else
    {
        dist = maxvol - (dist << 7) / 18000;
        dist = (dist * (10000 - dy)) / 10000;
        raw = ratan2(-dx, -dz);
        angle = raw - StagePlayer->rotate->vy;
        if (CamState.Mode == CMODE_DIRECTION)
        {
            angle -= CamState.DirectionRY;
        }
        if (angle > ANGLE_HALF)
        {
            angle = ANGLE_FULL - angle;
        }
        else
        {
            if (angle < -0x7ff)
            {
                angle += ANGLE_FULL;
            }
        }
    }
    vol = SOUND_SPATIAL(angle, dist);
    return PlaySE(StageSE, seid, vol);
}
