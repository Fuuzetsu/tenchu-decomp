#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void HumanActionControl(struct Humanoid *human);
 *     MOTION.C:145, 21 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short dtPAD;
 *     extern short dtCMD;
 *     extern short motMODE;
 *     extern struct SVECTOR *dtV;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern void (*ActionFunc[18])();
 * END PSX.SYM */

/*
 * HumanActionControl (0x8001c80c, 0x124 bytes) — the per-humanoid, per-frame
 * top-level driver: snapshots the pad, latches the "d-globals" (dtL/dtR/dtV/
 * dtM/motID) other MOTION.C functions read, runs whichever of
 * DamageControl/FallCheck-then-{HangCheck,SwimCheck} applies, drops the
 * D-pad directions from dtPAD while L1 (camera-center) is held, dispatches through `ActionFunc[human->status]`
 * (an indirect call through the per-status function-pointer table), and
 * finally runs MotionAndMove() unless the dispatched handler left
 * `motMODE` at MOTION_MOVE_UNSET.
 *
 * Matching note: populate the shared d-globals directly in this order. Reading
 * `mid` back through the freshly assigned `dtM` keeps the motion pointer live
 * and gives the retail unsigned halfword transfer without a cast-only struct
 * view or any invented locals.
 */

extern Humanoid *Me_MOTION_C;
extern void (*ActionFunc[N_CHARACTER_STATUSES])(void);
extern s16 FallCheck(void);
extern short HangCheck(void);
extern short SwimCheck(void);
extern void DamageControl(void);
extern short MotionAndMove(void);

void HumanActionControl(Humanoid *human)
{
    dtPAD = human->pad.data;
    Me_MOTION_C = human;
    dtCMD = GetCommand(&human->pad);
    motMODE = MOTION_MOVE_UNSET;
    dtV = &Me_MOTION_C->vector;
    dtL = Me_MOTION_C->locate;
    dtR = Me_MOTION_C->rotate;
    dtM = Me_MOTION_C->motion;
    motID = dtM->mid;
    if ((Me_MOTION_C->attribute & ATTR_HIT) != 0)
    {
        DamageControl();
    }
    else
    {
        if (FallCheck() != 0)
        {
            HangCheck();
        }
        else if ((Me_MOTION_C->map.attrib & MAP_WATER) != 0)
        {
            SwimCheck();
        }
    }
    if ((dtPAD & PADL1) != 0)
    {
        dtPAD = dtPAD & ~(PADLup | PADLright | PADLdown | PADLleft);
    }
    ActionFunc[Me_MOTION_C->status]();
    if (motMODE != MOTION_MOVE_UNSET)
    {
        MotionAndMove();
    }
}
