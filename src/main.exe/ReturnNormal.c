#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ReturnNormal(void);
 *     MOTION.C:210, 4 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

/*
 * ReturnNormal (0x800272a0) — pick the "return to normal" motion id + move
 * flag for the current motion-manager humanoid (Me_MOTION_C), writing them
 * into the globals motID / motMODE that NowReturnNormal.c (this TU's
 * caller, see its header) reloads right after calling this. If the current
 * character is the player (Me_MOTION_C == StagePlayer), also resets the
 * camera to normal mode (CMODE_NORMAL == 0, same literal PauseProc.c uses).
 */
extern Humanoid *Me_MOTION_C;

void ReturnNormal(void)
{
    if (Me_MOTION_C == StagePlayer)
    {
        SetCameraMode(CMODE_NORMAL);
    }
    if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
    {
        SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
    }
    else
    {
        SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
    }
}
