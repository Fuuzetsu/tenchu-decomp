#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1ninja(void);
 *     THINK_1.C:99, 2 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * Think1ninja (0x8002c238, 0x164 bytes) — think-handler for a ninja-type
 * enemy: does nothing while jumping (status STAT_JUMP); otherwise runs
 * an actscnt-gated (every 31st call, matching Think1watch's/Think1trace's
 * old-actscnt idiom) random-action roll (Think1random) that, specifically
 * while the current motion is MOT_MOVE at frame zero,
 * checks whether stepping forward would change the character's area-map
 * level (an edge/stair/ledge check, same GetAreaMapLevel/GlobalAreaMap
 * idiom as turn_towards_player_.c's obstacle probe) and forces a "stop and
 * turn" result (0x1040) if so, or if the forward area-level probe itself is
 * far enough away (> 0x17D4); otherwise returns the random roll. ALL
 * BYTE-PROVEN except the residual below.
 *
 * `Me_THINK_C->map` (game_types.h's PSX.SYM-derived MapVector): the asm reads ALL 4
 * bytes at once (`lw`, not four `lbu`s) and compares directly against a
 * GetAreaMapLevel return value — this is Ghidra's own independently-built
 * Humanoid struct's MapVector at the exact same offset (right after
 * Humanoid's own PADtype pad).
 *
 * The adjacent `mid == MOT_MOVE && count == 0` field checks compile to the
 * target's single 32-bit load and compare; no packed-word cast is needed.
 *
 * `Think1random` takes ZERO arguments (not `Think1random(Me_THINK_C)` as
 * m2c's naive per-register liveness rendering suggests): $a0 still holds
 * Me_THINK_C at the call site (unclobbered since function entry), which
 * m2c mis-reads as an argument — the same m2c-over-counts-call-args tell as
 * Think1watch's `rand()`. Confirmed empirically: declaring it with an
 * argument compiled an extra, wrong `lw $a0,Me_THINK_C` reload right before
 * the `jal`.  Its original s16 return type is equally load-bearing: an s32
 * declaration creates a full-width return-copy pseudo that CSE carries to a
 * later return, displacing the real `result` from $s1.
 *
 * `field6_0xc` (already-named placeholder, offset 0xC) is also read here as
 * a move-speed magnitude input (`field6_0xc * 5`) — the SAME field
 * turn_towards_player_.c passes to GetMoveSpeed's `width` parameter at the
 * same offset (cross-function corroboration; not renamed here since
 * turn_towards_player_.c — outside this batch — already references it by
 * this name).
 *
 * The `abs_d2 < 500` branch jumps DIRECTLY past the `d2 <= 0x17D4` check to
 * the shared `result = 0x1040` body (a labeled shared-return body, per the
 * cookbook's "write the constant once, goto in" rule) — plain nested
 * if/else would re-test d2 and diverge from the asm.
 *
 * The nonnegative-first absolute-value ternary is also intentional.  It
 * reaches cc1's abssi2 expansion and emits the target's `bgez`, copy, and
 * self-negate sequence; the negative-first ternary instead folds the copy
 * away.  This shape and the s16 call prototype must be changed together.
 */
extern s16 Think1random(void);

s16 Think1ninja(void)
{
    u8 actscnt;
    s16 result;

    result = 0;
    if (Me_THINK_C->status == STAT_JUMP)
    {
        return 0;
    }
    actscnt = Me_THINK_C->actscnt;
    Me_THINK_C->actscnt++;
    if (actscnt > 30)
    {
        result = Think1random();
        if (Me_THINK_C->motion->mid == MOT_MOVE &&
            Me_THINK_C->motion->count == 0)
        {
            SVECTOR move;
            s32 d1;
            s32 d2;

            GetMoveSpeed(&move, Me_THINK_C->rotate->vy,
                         (s16)(Me_THINK_C->width * 5), 0);
            d1 = GetAreaMapLevel(GlobalAreaMap, Me_THINK_C->locate->vx,
                                 Me_THINK_C->locate->vy - EYE_HEIGHT,
                                 Me_THINK_C->locate->vz,
                                 AREA_LEVEL_STEP_DOWN | AREA_LEVEL_FIRST_HIT |
                                     AREA_LEVEL_REUSE_CACHED);
            d2 = GetAreaMapLevel(GlobalAreaMap, Me_THINK_C->locate->vx + move.vx,
                                 Me_THINK_C->locate->vy - EYE_HEIGHT,
                                 Me_THINK_C->locate->vz + move.vz,
                                 AREA_LEVEL_RETURN_DELTA | AREA_LEVEL_FIRST_HIT |
                                     AREA_LEVEL_REUSE_CACHED);
            if (d1 == Me_THINK_C->map.level)
            {
                s32 abs_d2;

                abs_d2 = (d2 >= 0) ? d2 : -d2;
                if (abs_d2 < 500)
                {
                    goto set_1040;
                }
            }
            if (d2 <= 6100)
            {
                return result;
            }
        set_1040:
            result = PADLup | PADRdown;
        }
    }
    return result;
}
