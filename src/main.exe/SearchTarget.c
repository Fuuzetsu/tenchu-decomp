#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SearchTarget(struct Humanoid *human, long *distance, short *degree);
 *     HUMAN.C:436, 46 src lines, frame 64 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s1       struct Humanoid * human
 *     param $s3       long * distance
 *     param $s4       short * degree
 *     reg   $s2       struct VECTOR * head
 *     stack sp+16     struct VECTOR vect
 *     stack sp+32     struct SVECTOR svect
 *     reg   $s0       short mode
 *     reg   $a0       long dx
 *     reg   $a1       long dz
 *     reg   $s0       short roty
 *     reg   $a1       short degree
 *     reg   $a3       short n
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct Humanoid *StagePlayer;
 *     extern long EmergencyNotice;
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * SearchTarget (0x80028b48) — the enemy perception test: how well
 * `human` can currently see human->target, reported as one of the SR_
 * codes. With no target it returns 0 at once. It always fills
 * *distance (SquareRoot0 of the 3D delta) and *degree (ratan2 bearing
 * minus the human's own rotate->vy, folded into +/-0x800 of the
 * 4096-unit circle), then throttles: unless ((GameClock +
 * model->object[0]->id) & 0x1f) is 0 it returns 0, so the real
 * evaluation runs one frame in 32, staggered per model id. mode picks
 * the searchsight row — 1 while the player is STAT_SQUAT or
 * STAT_STICKON, which shortens every range by about a quarter, else 0.
 * Vertical rejects come first: while the player is STAT_HANG a
 * non-negative vy reports SR_GONE, as does a vy under -3000 within
 * 4000 units; any |vy| >= 3000 is SR_GONE unless EmergencyNotice is
 * set, and even then SR_GONE inside 4000. Beyond far_distance is
 * SR_GONE. Outside a |*degree| of 900 (of 4096) it is SR_UNSEEN, as is
 * 450 or more while sneaking, or anything at or past sight_distance.
 * Otherwise it casts a ray: the eye is lifted to position.vy + 300 -
 * human->height, the delta re-based by the same amounts and lowered by
 * the player's height (half of it while squatting), then halved
 * (n <<= 1, components >>= 1) until every component is within limit —
 * 500, or 300 while sneaking — and walked through GlobalAreaMap by
 * GetAreaMapPassage over n steps. A blocked ray is SR_GONE; a clear
 * one is SR_SEEN inside clear_distance, else SR_GLIMPSE.
 */

/*
 * STATUS: MATCHING — exact 1128-byte / 282-instruction pure-C match.
 * The stack plan is frame 0x50, `vect` VECTOR at sp+0x10, position VECTOR at
 * sp+0x20, and passage SVECTOR at sp+0x30.
 *
 * The passage failure's SImode -2 is narrowed through `passage_pad`, crosses
 * a zero-code loop fence, and is widened through identical arms.  This strips
 * the literal equivalence until jump2 without emitting code, so retail's late
 * `j`/`li -2` island survives while the close-distance return folds directly
 * into its conditional branch.
 *
 * The three components of `vect` must be one stack VECTOR; `mode` must remain
 * s16 for the target's repeated promotion/copy chains; and the vertical
 * adjustment needs distinct `initial_delta_y`, updated `delta_y`, and
 * `base_y` identities.
 */

typedef struct
{
    s32 sight_distance; /* beyond this: SR_UNSEEN */
    s32 clear_distance; /* within this: SR_SEEN, else SR_GLIMPSE */
    s32 far_distance;   /* beyond this: SR_GONE (target lost) */
} SearchSight;

/* The two sight-range rows (retail data @ 0x80086b7c): row 0 for a
 * walking player {sight 16000, clear 10000, gone 20000}, row 1 when the
 * player sneaks {12000, 7000, 16000} — crouching or wall-pressing cuts
 * every enemy's perception ranges by roughly a quarter. */
extern SearchSight searchsight[];

short SearchTarget(Humanoid *human, long *distance, short *degree)
{
    VECTOR vect;
    VECTOR position;
    SVECTOR svect;
    s32 raw_degree;
    s32 roty;
    s16 mode;
    s32 limit;
    s32 absolute;
    s32 y;
    s32 base_y;
    s32 own_height;
    s32 initial_delta_y;
    s32 delta_y;
    s32 full_height;
    s32 half_height;
    s32 adjusted_y;
    u16 player_height;
    s16 signed_degree;
    s16 result_degree;
    s16 n;

    position = *human->locate;
    n = 1;
    if (human->target == 0)
    {
        return 0;
    }

    vect.vx = human->target->locate.coord.t[0] - position.vx;
    vect.vy = human->target->locate.coord.t[1] - position.vy;
    vect.vz = human->target->locate.coord.t[2] - position.vz;
    *distance = SquareRoot0(vect.vx * vect.vx + vect.vy * vect.vy +
                            vect.vz * vect.vz);

    roty = (u16)human->rotate->vy;
    raw_degree = ratan2(-vect.vx, -vect.vz) - roty;
    signed_degree = raw_degree;
    result_degree = raw_degree;
    if (signed_degree < 0x801)
    {
        goto degree_nested;
    }
    result_degree = 0x1000 - raw_degree;
    goto degree_done;
degree_nested:
    if (signed_degree < -0x7ff)
    {
        result_degree = raw_degree + 0x1000;
    }
degree_done:
    *degree = result_degree;

    if (((GameClock + human->model->object[0]->id) & 0x1f) != 0)
    {
        return 0;
    }

    /* Sneaking (STAT_SQUAT or STAT_STICKON) selects the short-range
     * sight row. */
    mode = (u16)(StagePlayer->status - 0xb) < 2;
    if (StagePlayer->status == STAT_HANG)
    {
        if (vect.vy >= 0)
        {
            goto passage_failure;
        }
        if (vect.vy < -3000)
        {
            if (*distance < 4000)
            {
                return SR_GONE;
            }
        }
    }

    if (__builtin_abs(vect.vy) >= 3000)
    {
        if (EmergencyNotice == 0)
        {
            return SR_GONE;
        }
        if (*distance < 4000)
        {
            goto passage_failure;
        }
    }

    if (*distance >= searchsight[mode].far_distance)
    {
        return SR_GONE;
    }

    absolute = __builtin_abs(*degree);
    if (absolute < 900)
    {
        if (absolute >= 450 && mode != 0)
        {
            return SR_UNSEEN;
        }
        if (*distance >= searchsight[mode].sight_distance)
        {
            return SR_UNSEEN;
        }

        limit = 500;
        if (mode != 0)
        {
            limit = 300;
        }
        y = position.vy;
        own_height = human->height;
        initial_delta_y = vect.vy;
        y += 300;
        y -= own_height;
        delta_y = initial_delta_y - 300;
        position.vy = y;
        base_y = delta_y + human->height;
        vect.vy = base_y;
        player_height = StagePlayer->height;
        full_height = (s16)player_height;
        if (StagePlayer->status == STAT_SQUAT)
        {
            half_height = (s16)player_height / 2;
            adjusted_y = base_y - half_height;
        }
        else
        {
            adjusted_y = base_y - full_height;
        }
        vect.vy = adjusted_y;

        while (limit < __builtin_abs(vect.vx) ||
               limit < __builtin_abs(vect.vy) ||
               limit < __builtin_abs(vect.vz))
        {
            n <<= 1;
            vect.vx >>= 1;
            vect.vy >>= 1;
            vect.vz >>= 1;
        }

        svect.vx = vect.vx;
        svect.vy = vect.vy;
        svect.vz = vect.vz;
        if (GetAreaMapPassage(GlobalAreaMap, &position, &svect, n) != 0)
        {
        passage_failure:
        {
            s32 passage_raw;
            s16 passage_pad;
            s32 passage_result;

            passage_raw = -2;
            passage_pad = passage_raw;
            do
            {
            } while (0);
            if (mode != 0)
            {
                passage_result = (s32)passage_pad;
            }
            else
            {
                passage_result = (s32)passage_pad;
            }
            return passage_result;
        }
        }
        return (*distance < searchsight[mode].clear_distance) ? SR_SEEN : SR_GLIMPSE;
    }
    return SR_UNSEEN;
}

