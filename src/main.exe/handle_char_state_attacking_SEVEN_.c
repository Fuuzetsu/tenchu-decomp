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
 * `dtM`/`dtR` are new globals (this is the first function to touch them):
 * only ONE s16 field (at byte offset 2) is proven for each, so both are
 * typed as minimal 2-field stand-ins reaching just that offset — nothing
 * cross-checks Ghidra's own guessed field names ("count" / "vy"), so they're
 * not asserted here (see the cookbook's "zero other usages -> it's a guess"
 * rule). Same original TU as Me_MOTION_C (gp-relative together, see gpsyms).
 *
 * The move-speed magnitude `(rand() % 5) * 1000 + 4000` is a plain C
 * expression — the magic-multiply (mod 5) and shift/add (*1000) sequences
 * are automatic cc1 constant-folding, not hand-derived.
 */

typedef struct
{
    u8 pad0[2];
    s16 count; /* Ghidra's guessed name, unverified */
} dtM_type;

typedef struct
{
    u8 pad0[2];
    s16 unk2; /* Ghidra guessed "vy" (SVECTOR), unverified */
} dtR_type;

extern dtM_type *dtM;
extern dtR_type *dtR;
extern Humanoid *Me_MOTION_C;
extern void Sound(Humanoid *h, int id);
extern void GetMoveSpeed(SVECTOR *out, s32 roty, s32 b, s32 width);

void handle_char_state_attacking_SEVEN_(s16 frame)
{
    VECTOR *start_pos;
    PARAM_ITEM_USE p;
    SVECTOR move;

    if (dtM->count == frame)
    {
        Sound(Me_MOTION_C, 5);
        p.type = ITEM_KIND_2_LIGHTNING_BOLT;
        p.user = Me_MOTION_C;
        start_pos = GetAbsolutePosition(Me_MOTION_C->model->object[0xD], 0, 0, -0x2BC);
        p.start.vx = start_pos->vx;
        p.start.vy = start_pos->vy;
        p.start.vz = start_pos->vz;
        GetMoveSpeed(&move, dtR->unk2, (s16)((rand() % 5) * 1000 + 4000), 0);
        p.end.vx = start_pos->vx + move.vx;
        p.end.vy = Me_MOTION_C->target->locate.coord.t[1];
        p.end.vz = start_pos->vz + move.vz;
        ReqItemUse(&p);
    }
}
