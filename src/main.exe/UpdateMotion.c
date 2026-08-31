#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short UpdateMotion(struct MotionManager *mmp, short mid);
 *     ACTION.C:182, 34 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct MotionManager * mmp
 *     param $t0       short mid
 *     reg   $a0       struct MotionRegistType * mrp
 *     reg   $a2       short i
 *     reg   $a1       short j
 *     reg   $a3       short * xyz
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionRegistType MOTcommon[41];
 * END PSX.SYM */

/*
 * STATUS: MATCHING.
 *
 * UpdateMotion (0x8001b65c, 0x278 bytes) — switch mmp's active motion to
 * `mid`, unless it's already the current motion (returns -1, a no-op).
 * Search mmp's own registered-motion table (mmp->motreg, sentinel
 * mid == -1) for a row matching `mid`; if none has a non-NULL `motion`
 * there, fall back to searching the global common table MOTcommon the same
 * way. If neither table has a usable row, returns 0 (failure). Otherwise
 * installs the found MotionDataType* as mmp->motion, latches mmp->mid,
 * derives mmp->count (a signed byte, motion->sweep sign-extended) and
 * mmp->loop = 0, clamps mmp->n to min(mmp->model->n, mmp->motion->n), calls
 * SetupSpline(mmp) to reseed the interpolation state, then re-normalizes
 * every bone's rotation (mmp->model->object[i]->rotate, cast to a short[3]
 * array of vx/vy/vz) into canonical range: first snap a value that
 * overshot by nearly a half turn back a full turn (abs > 0x800 -> +-0x1000),
 * then reduce it mod 0x1000, keeping the sign (a truncating divide). Returns
 * 1 on success.
 *
 * Matching constraints:
 *  - Both table searches test mid != requested at entry/bottom and test the
 *    -1 sentinel inside the body. This priority is the reverse of GetMotionID.
 *    Reuse mrp for both tables and i for both searches, the min, and the bone
 *    loop; the PSX.SYM local set has no separate copies.
 *  - Compute each rotation base directly as
 *    (s16 *)&mmp->model->object[i]->rotate. Each component fixup is one
 *    self-referencing assignment, giving one load and one store per pass.
 *  - Keep the min as the ternary motion->n < model->n ? motion->n : model->n.
 *    It loads both operands in SI mode and moves the chosen value. A staged
 *    if reloads the u8 field, causes a 13-instruction coloring cascade, and
 *    accounted for 22 of the former 26 differing bytes.
 *  - Spell the absolute-value test inline as (t < 0) ? -t : t. In this
 *    comparison context it becomes one opaque abssi2 pattern; assigning the
 *    ternary to a temporary exposes a different copy/branch/negate sequence.
 *  - Keep the full-turn snap as one assignment whose condition re-reads
 *    xyz[j] and whose arms update t in place. Expanding the destination before
 *    the branch creates the target address copy in the delay slot and stores
 *    through that copy; testing t instead loses it.
 */
extern void SetupSpline(MotionManager *mmp);

s16 UpdateMotion(MotionManager *mmp, s16 mid)
{
    MotionRegistType *mrp;
    MotionDataType *md;
    s16 i;
    s16 j;
    s16 *xyz;
    s32 t;
    s16 sweep;

    if (mid == mmp->mid)
        return -1;

    mrp = mmp->motreg;
    i = 0;
    while (mrp[i].mid != mid)
    {
        if (mrp[i].mid == -1)
            break;
        i++;
    }
    if (mrp[i].motion == 0)
    {
        mrp = MOTcommon;
        i = 0;
        while (mrp[i].mid != mid)
        {
            if (mrp[i].mid == -1)
                break;
            i++;
        }
        if (mrp[i].motion == 0)
            return 0;
    }

    md = mrp[i].motion;
    mmp->mid = mid;
    mmp->motion = md;
    sweep = md->sweep;
    mmp->count = sweep;
    if (sweep & 0x80)
        mmp->count = sweep - 0x100;
    mmp->loop = 0;
    i = (mmp->motion->n < mmp->model->n) ? mmp->motion->n : mmp->model->n;
    mmp->n = i;
    SetupSpline(mmp);

    for (i = 0; i < mmp->model->n; i++)
    {
        xyz = (s16 *)&mmp->model->object[i]->rotate;
        for (j = 0; j < 3; j++)
        {
            t = xyz[j];
            if (((t < 0) ? -t : t) > ANGLE_HALF)
                xyz[j] = (xyz[j] < 0) ? (t += ANGLE_FULL) : (t -= ANGLE_FULL);
            xyz[j] = xyz[j] % ANGLE_FULL;
        }
    }
    return 1;
}
