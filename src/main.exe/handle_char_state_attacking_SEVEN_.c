#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * handle_char_state_attacking_SEVEN_ (0x8001f6b8, 0x12c bytes) — an
 * animation-frame callback (param is the frame/trigger id) for the SEVEN
 * weapon's attack: only fires when the currently-armed motion trigger
 * (dtM->count) matches the caller's id, then plays a sound and spawns a
 * lightning-bolt item flying from the wielded weapon's tip
 * (Me_MOTION_C->model->object[0xd], the same item-TU Humanoid/ModelArchiveType
 * used by FUN_80027304/NowReturnNormal/dispose_weapon_data_of_char_) towards
 * the target, landing at the target's actual height
 * (Me_MOTION_C->target->locate.coord.t[1] — the Y translation of its world
 * matrix) rather than a computed offset.
 *
 * This descriptive retail name is intentional. The historical `AttackFire`
 * assignment was a call-fingerprint collision: this callback has one short
 * parameter and launches a lightning bolt, while the demo `AttackFire` has two
 * shorts and launches fire over a frame range. That original name now belongs
 * to the matching callback at 0x80027730.
 *
 * PSX.SYM identifies the MOTION.C globals as MotionManager *dtM and
 * SVECTOR *dtR, including the `count` and `vy` fields used here. Same
 * original TU as Me_MOTION_C (gp-relative together, see gpsyms).
 *
 * The move-speed magnitude `(rand() % 5) * 1000 + 4000` is a plain C
 * expression — the magic-multiply (mod 5) and shift/add (*1000) sequences
 * are automatic cc1 constant-folding, not hand-derived.
 */

extern Humanoid *Me_MOTION_C;
extern short Sound(Humanoid *human, int seid);
extern void GetMoveSpeed(SVECTOR *out, s32 roty, s32 b, s32 width);

void handle_char_state_attacking_SEVEN_(s16 frame)
{
    VECTOR *start_pos;
    PARAM_ITEM_LAUNCH p;
    SVECTOR move;

    if (dtM->count == frame)
    {
        Sound(Me_MOTION_C, 5);
        p.type = ITEM_LIGHTNINGBOLT;
        p.user = Me_MOTION_C;
        start_pos = GetAbsolutePosition(Me_MOTION_C->model->object[0xD], 0, 0, -0x2BC);
        p.start.vx = start_pos->vx;
        p.start.vy = start_pos->vy;
        p.start.vz = start_pos->vz;
        GetMoveSpeed(&move, dtR->vy, (s16)((rand() % 5) * 1000 + 4000), 0);
        p.end.vx = start_pos->vx + move.vx;
        p.end.vy = Me_MOTION_C->target->locate.coord.t[1];
        p.end.vz = start_pos->vz + move.vz;
        ReqItemUse(&p);
    }
}
