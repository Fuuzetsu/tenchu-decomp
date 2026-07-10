#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

/*
 * FUN_800274e8 (0x800274e8, 0x6c bytes) - an animation-frame callback (this
 * TU's dtM/Me_MOTION_C family, see AttackFire.c /
 * FUN_80027730.c, the real near-twins - not leRestoreEnemyLayout, which was
 * just an assembly-shape false positive): fires only when the currently-armed
 * motion trigger (dtM->count) matches the caller's frame id, then spawns a
 * projectile via bow_shoot_logic (kind 0x14 - not the 0x15 arrow case) from
 * the wielded weapon's tip (Me_MOTION_C->model->object[0xd], same item-TU
 * Humanoid/ModelArchiveType as the siblings) and plays a sound.
 *
 * `dtM` kept as the same minimal 2-field stand-in as its siblings (only the
 * offset-2 `count` field is proven; Ghidra's own field name is an unverified
 * guess - see the cookbook's "zero other usages -> it's a guess" rule).
 *
 * Matching notes: the frame was 40 bytes (exactly sizeof(PARAM_ITEM_USE))
 * short until adding an UNUSED local `PARAM_ITEM_USE p;` - never read or
 * written, just declared. cc1 reserves stack space for every declared
 * automatic aggregate regardless of use, so a dead local of the sibling
 * functions' own PARAM_ITEM_USE size reproduces the target's frame exactly;
 * plausible as leftover from refactoring this callback (which used to build
 * the item params inline like FUN_80027730.c/handle_char_state_attacking_
 * SEVEN_.c do) into a thin bow_shoot_logic(kind, pos) wrapper.
 */
typedef struct
{
    u8 pad0[2];
    s16 count; /* Ghidra's guessed name, unverified */
} dtM_type;

extern dtM_type *dtM;
extern Humanoid *Me_MOTION_C;
extern void bow_shoot_logic(s16 kind, VECTOR *start);
extern void Sound(Humanoid *h, int id);

void FUN_800274e8(s16 param_1, s16 param_2)
{
    PARAM_ITEM_USE p;

    if (dtM->count == param_2) {
        bow_shoot_logic(0x14, GetAbsolutePosition(Me_MOTION_C->model->object[0xD], 0, param_1, -100));
        Sound(Me_MOTION_C, 2);
    }
}
