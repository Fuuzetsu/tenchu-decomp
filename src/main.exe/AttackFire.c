#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackFire(short sfrm, short efrm);
 *     MOTION.C:876, 16 src lines, frame 72 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       short sfrm
 *     param $a1       short efrm
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     stack sp+56     struct SVECTOR vect
 * END PSX.SYM */

/*
 * AttackFire (0x80027730, 0xe8 bytes) — the animation-frame callback for a
 * fire/napalm-throwing attack: only fires while the currently-armed motion
 * trigger (dtM->count) is within [sfrm, efrm], plays a sound on the
 * FIRST matching frame (count == sfrm), then spawns a napalm item flying
 * from the wielded weapon's tip (Me_MOTION_C->model->object[2]) towards a
 * fixed-speed target point — near-twin of
 * handle_char_state_attacking_SEVEN_.c (same item-TU Humanoid/
 * ModelArchiveType, same dtM/dtR pair), but a frame RANGE instead of a
 * single frame, a plain literal move speed instead of a randomized one, and
 * the end point keeps `start`'s own Y (no target-height read).
 *
 * Naming evidence is independent and high-confidence: the demo symbol has
 * exactly two short parameters plus PARAM_ITEM_LAUNCH/SVECTOR stack locals,
 * and its assembly launches the same fire/napalm item over a frame range.
 * `callmatch --verify` also selected this function as the strictly better fit
 * after exposing the old one-frame lightning callback's fingerprint collision.
 *
 * PSX.SYM identifies the shared globals as MotionManager *dtM and
 * SVECTOR *dtR.
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

extern Humanoid *Me_MOTION_C;
extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);

void AttackFire(s16 sfrm, s16 efrm)
{
    VECTOR *start_pos;
    PARAM_ITEM_LAUNCH item;
    SVECTOR vect;
    s16 count;

    count = dtM->count;
    if (sfrm <= count && count <= efrm)
    {
        if (count == sfrm)
        {
            Sound(Me_MOTION_C, 0x28);
        }
        item.type = ITEM_NAPALM;
        item.user = Me_MOTION_C;
        start_pos = GetAbsolutePosition(Me_MOTION_C->model->object[2], 0, -100, -300);
        item.start.vx = start_pos->vx;
        item.start.vy = start_pos->vy;
        item.start.vz = start_pos->vz;
        GetMoveSpeed(&vect, dtR->vy, 100, 0);
        item.end.vx = item.start.vx + vect.vx;
        item.end.vy = item.start.vy;
        item.end.vz = item.start.vz + vect.vz;
        ReqItemUse(&item);
    }
}
