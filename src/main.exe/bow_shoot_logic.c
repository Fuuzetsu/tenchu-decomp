#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * bow_shoot_logic (0x80027554, 0x134 bytes) — spawns a bow projectile
 * (`kind` is the PARAM_ITEM_LAUNCH.type — ITEM_ARROW for the arrow)
 * travelling from `*start` towards the target, landing at
 * the target's actual world-Y (Me_MOTION_C->target->locate.coord.t[1], same
 * field proven by handle_char_state_attacking_SEVEN_.c) with a small chance
 * (1 in EngageLevel+1) of landing 1000 units short specifically for arrows.
 *
 * `move.pad` is a real write, not a stray local: Ghidra's own struct-typed
 * decompilation resolves the GetTargetDistance store to local_10 (our
 * `move`)'s SVECTOR.pad field, and it's never reloaded from the stack
 * afterward (every later use reads the still-live register) — the original
 * really does park the raw distance in the move vector's otherwise-unused
 * pad slot before overwriting the whole vector via GetMoveSpeed.
 *
 * The `rand() % (EngageLevel + 1)` is a genuine runtime division (divisor is
 * a variable, not a constant) — real `div` + ASPSX's guarded-division
 * bnez/break 7/break 6 sequence, needing maspsx --expand-div for this file
 * (Build.hs maspsxGpExterns' `extra`, mirrored in tools/permute.py
 * MASPSX_EXTRA), same as GetAreaMapLevel.
 *
 * GetTargetDistance's return type must be declared `u32` (not `s16`) in
 * THIS caller: a 5-instruction block (store dist to move.pad, re-derive a
 * clean s16 for the `< 1000` compare, load dtR, do the compare) came out as
 * a pure reorder of identical instructions — the store landed after the
 * compare instead of before it — with every other lever (separate `dist`
 * local, hoisting the `dtR->vy` read, operand order) leaving it
 * unchanged. Found by one short decomp-permuter run; it's the caller-side
 * extern-return-type-is-an-extension-position-lever rule (see
 * Think1trace/BIS's GetRealPad in the cookbook) — declaring the callee
 * `u32` here defers the derived-s16 extension past the store, matching the
 * target exactly, whereas `s16` forces it immediately after the call.
 */

extern Humanoid *Me_MOTION_C;
extern s16 EngageLevel;

extern SVECTOR *dtR;
extern u32 GetTargetDistance(Humanoid *h, s16 *unused);
extern void GetMoveSpeed(SVECTOR *out, s32 roty, s32 b, s32 width);

void bow_shoot_logic(s16 kind, VECTOR *start)
{
    PARAM_ITEM_LAUNCH p;
    SVECTOR move;
    s16 dist;
    s32 rot;
    s16 speed;

    p.type = kind;
    p.user = Me_MOTION_C;
    p.start.vx = start->vx;
    p.start.vy = start->vy;
    p.start.vz = start->vz;
    dist = GetTargetDistance(Me_MOTION_C, 0);
    move.pad = dist;
    rot = dtR->vy;
    speed = dist;
    if (dist < 1000)
    {
        speed = 1000;
    }
    GetMoveSpeed(&move, rot, speed, 0);
    p.end.vx = p.start.vx + move.vx;
    p.end.vy = Me_MOTION_C->target->locate.coord.t[1];
    p.end.vz = p.start.vz + move.vz;
    if (kind == ITEM_ARROW)
    {
        if (rand() % (EngageLevel + 1) != 0)
        {
            p.end.vy -= 1000;
        }
    }
    ReqItemUse(&p);
}
