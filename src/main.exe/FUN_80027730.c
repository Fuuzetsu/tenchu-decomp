#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * FUN_80027730 (0x80027730, 0xe8 bytes) — an animation-frame callback for a
 * fire/napalm-throwing attack: only fires while the currently-armed motion
 * trigger (dtM->count) is within [param_1, param_2], plays a sound on the
 * FIRST matching frame (count == param_1), then spawns a napalm item flying
 * from the wielded weapon's tip (Me_MOTION_C->model->object[2]) towards a
 * fixed-speed target point — near-twin of
 * handle_char_state_attacking_SEVEN_.c (same item-TU Humanoid/
 * ModelArchiveType, same dtM/dtR pair), but a frame RANGE instead of a
 * single frame, a plain literal move speed instead of a randomized one, and
 * the end point keeps `start`'s own Y (no target-height read).
 *
 * `dtM`/`dtR` proven fields carried over unchanged from that twin (only
 * dtM->count @ offset 2 and dtR's offset-2 s16 are proven; see its header
 * for why they're minimal stand-ins, Ghidra's guessed names unverified).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - Unlike the SEVEN twin (which re-reads `start_pos->vx`/`->vz` through
 *    the original pointer for `p.end.*`), THIS function's `p.end.*` must
 *    read back the already-stored `p.start.vx`/`.vy`/`.vz` (a stack reload,
 *    not the pointer) — confirmed by the raw asm, which reloads from the
 *    `sp+0x18/0x1c/0x20` slots, not through `$v0`. Re-deriving from
 *    `start_pos->` instead extends that pointer's live range across the
 *    GetMoveSpeed call, forcing a callee-saved register and an extra
 *    prologue/epilogue save/restore pair (8 bytes too long). Twins can
 *    genuinely differ in which of two equal-value expressions the original
 *    source used — don't assume the sibling's exact phrasing transfers.
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
extern void ReqItemUse(PARAM_ITEM_USE *p);

void FUN_80027730(s16 param_1, s16 param_2)
{
    VECTOR *start_pos;
    PARAM_ITEM_USE p;
    SVECTOR move;
    s16 count;

    count = dtM->count;
    if (param_1 <= count && count <= param_2)
    {
        if (count == param_1)
        {
            Sound(Me_MOTION_C, 0x28);
        }
        p.type = ITEM_KIND_2_KAEN;
        p.user = Me_MOTION_C;
        start_pos = GetAbsolutePosition(Me_MOTION_C->model->object[2], 0, -100, -300);
        p.start.vx = start_pos->vx;
        p.start.vy = start_pos->vy;
        p.start.vz = start_pos->vz;
        GetMoveSpeed(&move, dtR->unk2, 100, 0);
        p.end.vx = p.start.vx + move.vx;
        p.end.vy = p.start.vy;
        p.end.vz = p.start.vz + move.vz;
        ReqItemUse(&p);
    }
}
