#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short HangCheck(void);
 *     MOTION.C:291, 51 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct SVECTOR vect
 *     reg   $s0       long yy
 *     reg   $s0       long y
 *     reg   $s1       long ry
 *     reg   $s4       long oy
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern unsigned long *GlobalAreaMap;
 *     extern short MotionUpdateMode;
 *     extern short motMODE;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * STATUS: MATCHING.
 *
 * HangCheck (0x8001ceb0, 0x4c4 bytes) — per-frame ledge-hang detector
 * (MOTION.C, called from ActCHASE/ActHANG/ActMOVE/HumanActionControl). Bails
 * immediately for a "type 0xA_" special character, or unless the player is
 * airborne with room below (map.height), not already recovering (motID !=
 * 0x901) and not while Henshin is active (itmctl != ITEM_HENSHIN). Casts a short forward
 * probe (GetMoveSpeed) both ways from the character's feet: if the ground is
 * closer than the character's own Y (a wall/ledge edge underfoot), nudges
 * `*dtL` back away from it and bails. Otherwise probes further down (300,
 * then 400 units) for a ledge to catch: once a catchable ledge is found,
 * snaps `*dtL` onto it, and — unless already in the "hang" status (10) —
 * snaps the character's own facing (`dtR->vy`) to the nearest 90-degree
 * quadrant and re-validates the ledge is still there at the snapped facing
 * (bailing back to the pre-snap height if not); on success installs the
 * "hang" motion (0xa01), guarding against clobbering a motion mid-update on
 * another Humanoid the same way MotionAndMove.c does (CVAhuman[] scan),
 * plays the catch sound, and rumbles the pad if this is the player's own
 * Humanoid.
 *
 * Matching constraints:
 *  - GetAreaMapLevel takes area, x, y, z, and mode; Ghidra under-counts these
 *    calls.
 *  - Leave width and the halved SVECTOR fields as plain signed fields. Their
 *    narrowing/shift uses produce lhu plus fused sign-extension/scale, while
 *    ordinary additions load them with lh. In particular, an explicit
 *    (u16)width cast changes the arithmetic shift to srl.
 *  - Keep every early bail as a literal guard-clause return. jump2 merges the
 *    islands into the last zero-return block; wrapping the body moves the
 *    shared zero materialization behind the success tail.
 *  - The wall nudge is if (dtL->vy < y) { stores } followed by a shared
 *    return. The y3 revert instead returns inside its store block. Those
 *    different block boundaries respectively prevent and permit sched1 to
 *    hoist the zero result into a load-delay hole.
 *  - Reuse y for every probe, then copy oy = y after the committed 400 probe.
 *    y is clobbered by revalidation while oy preserves the revert height; a
 *    direct assignment to oy removes the proving copy and is one instruction
 *    short.
 *  - dy is one variable assigned for both probe batches. Put dy = yy - 300
 *    before if (y < dtL->vy): reorg moves it into the wall branch's delay
 *    slot. A later placement folds it into the call argument.
 *  - Snap facing through yc, then assign ry = yc. Masking ry in place loses
 *    the target join copy and also changes which value the 0x200 test reads.
 *  - Preserve the fold-sensitive spellings dtL->vy - (105 - y) and
 *    dtL->vy - (oy - 5). Reassociated plus/minus forms emit the same value in
 *    a different instruction order.
 *  - Commit vx, then vz, then vy. Because vect is address-taken, moving the vy
 *    store earlier pins a later vect load below it and introduces a hazard nop.
 *  - Recompute the second (dtL->vy - height) - 400 base after the commit; the
 *    first batch still uses the original yy - 400.
 *  - Keep ry signed long; explicit `(s16)ry` at the two GetMoveSpeed calls
 *    retains the target narrowing without a separate carrier.
 *  - Retain MotionAndMove's short-index CVAhuman scan. Its found path jumps
 *    past SetNowMotion rather than returning because both paths must play the
 *    sound.
 *  - GlobalAreaMap and StagePlayer use absolute accesses in this TU; do not
 *    add them to the gp-relative extern list.
 */
extern Humanoid *Me_MOTION_C;

short HangCheck(void)
{
    SVECTOR vect;
    long yy;
    long y;
    long ry;
    long dy;
    long oy;
    long yc;
    short i;

    if ((Me_MOTION_C->type & PAGE_MASK) == PAGE_BEAST)
    {
        return 0;
    }
    if (Me_MOTION_C->map.height <= 0 || motID == MOT_JUMP_WALLKICK || Me_MOTION_C->itmctl == ITEM_HENSHIN)
    {
        return 0;
    }
    yy = dtL->vy - Me_MOTION_C->height;
    GetMoveSpeed(&vect, dtR->vy, Me_MOTION_C->width >> 1, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, yy - 290, dtL->vz + vect.vz, 0);
    dy = yy - 300;
    if (y < dtL->vy)
    {
        y = GetAreaMapLevel(GlobalAreaMap, dtL->vx - vect.vx, yy - 290, dtL->vz - vect.vz, 0);
        if (y == (u32)LEVEL_NONE)
        {
            return 0;
        }
        if (dtL->vy < y)
        {
            dtL->vx -= (vect.vx >> 1);
            dtL->vz -= (vect.vz >> 1);
        }
        return 0;
    }
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx, dy, dtL->vz, 0);
    if (y < dtL->vy - Me_MOTION_C->height)
    {
        return 0;
    }
    GetMoveSpeed(&vect, dtR->vy, (Me_MOTION_C->width >> 1) + 300, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx,
                        yy - LEDGE_PROBE_RISE, dtL->vz + vect.vz,
                        AREA_LEVEL_RETURN_DELTA);
    if (y == (u32)LEVEL_NONE || y > LEDGE_PROBE_RISE)
    {
        return 0;
    }
    dtL->vy -= (105 - y);
    if (Me_MOTION_C->status == STAT_HANG)
    {
        return 1;
    }
    ry = dtR->vy;
    oy = y;
    if (ry & 0xFF)
    {
        yc = ry & ANGLE_QUADRANT_MASK;
        if (ry & ANGLE_HALF_QUADRANT)
        {
            yc += ANGLE_QUADRANT;
        }
        ry = yc;
    }
    GetMoveSpeed(&vect, (s16)ry, (Me_MOTION_C->width >> 1) + 300, 0);
    dy = (dtL->vy - Me_MOTION_C->height) - LEDGE_PROBE_RISE;
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, dy,
                        dtL->vz + vect.vz, AREA_LEVEL_RETURN_DELTA);
    if (y == (u32)LEVEL_NONE || y > LEDGE_PROBE_RISE)
    {
        dtL->vy -= (oy - 5);
        return 0;
    }
    dtR->vy = ry;
    GetMoveSpeed(&vect, (s16)ry, (Me_MOTION_C->width >> 1) + 100, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, dy,
                        dtL->vz + vect.vz, AREA_LEVEL_RETURN_DELTA);
    if (y != (u32)LEVEL_NONE && y <= LEDGE_PROBE_RISE)
    {
        GetMoveSpeed(&vect, dtR->vy, -200, 0);
        dtL->vx += vect.vx;
        dtL->vz += vect.vz;
        dtL->vy -= (105 - y);
    }
    motID = MOT_HANG_CATCH;
    motMODE = 1;
    SET_NOW_MOTION_UNLESS_CVA(goto found);
found:
    Sound(Me_MOTION_C, SE_LEDGE_GRIP);
    if (StagePlayer != Me_MOTION_C)
    {
        return -1;
    }
    PadShockAR(0, RUMBLE_POWER_HALF, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_LONG);
    return -1;
}
