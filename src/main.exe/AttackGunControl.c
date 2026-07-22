#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackGunControl(short length, short frm);
 *     MOTION.C:832, 13 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       short length
 *     param $a1       short frm
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 * END PSX.SYM */

/*
 * AttackGunControl (0x800274e8, 0x6c bytes) - an animation-frame callback (this
 * TU's dtM/Me_MOTION_C family, see handle_char_state_attacking_SEVEN_.c /
 * AttackFire.c, the real near-twins - not leRestoreEnemyLayout, which was
 * just an assembly-shape false positive): fires only when the currently-armed
 * motion trigger (dtM->count) matches the caller's frame id, then spawns a
 * projectile via bow_shoot_logic (ITEM_GUN, not the ITEM_ARROW case) from
 * the wielded weapon's tip (Me_MOTION_C->model->object[0xd], same item-TU
 * Humanoid/ModelArchiveType as the siblings) and plays a sound.
 *
 * The recovered name is backed by the demo's two-short `length, frm`
 * prototype and PARAM_ITEM_LAUNCH local, plus the retail gun request kind and
 * weapon-object-0xd role. It is not a positional-only PSX.SYM transplant.
 *
 * PSX.SYM identifies `dtM` as the shared MotionManager used by its siblings.
 *
 * Matching notes: the frame was 40 bytes (exactly sizeof(PARAM_ITEM_LAUNCH))
 * short until adding an UNUSED local `PARAM_ITEM_LAUNCH p;` - never read or
 * written, just declared. cc1 reserves stack space for every declared
 * automatic aggregate regardless of use, so a dead local of the sibling
 * functions' own PARAM_ITEM_LAUNCH size reproduces the target's frame exactly;
 * plausible as leftover from refactoring this callback (which used to build
 * the item params inline like AttackFire.c/handle_char_state_attacking_
 * SEVEN_.c do) into a thin bow_shoot_logic(kind, pos) wrapper.
 */
extern Humanoid *Me_MOTION_C;
extern void bow_shoot_logic(s16 kind, VECTOR *start);
extern short Sound(Humanoid *human, int seid);

void AttackGunControl(s16 length, s16 frm)
{
    PARAM_ITEM_LAUNCH p;

    if (dtM->count == frm) {
        bow_shoot_logic(ITEM_GUN, GetAbsolutePosition(Me_MOTION_C->model->object[0xD], 0, length, -100));
        Sound(Me_MOTION_C, 2);
    }
}
