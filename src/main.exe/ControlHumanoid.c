#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "humanoid.h"
#include "item.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ControlHumanoid(struct Humanoid *human);
 *     HUMAN.C:107, 44 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct Humanoid * human
 *     reg   $s1       struct ModelArchiveType * model
 *     reg   $s2       long m
 *     stack sp+24     struct MATRIX mat
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern struct Humanoid *StagePlayer;
 *     extern short ActionHalt;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

/*
 * MATCHED.  394 instructions / 1576 bytes, 0 differing bytes.
 *
 * Three source facts are load-bearing here; all three were verified against the
 * pinned gcc-2.8.1 source (global.c) and the .lreg/.greg dumps.
 *
 * 1. GetDirection's third parameter is `short` (reference/psxsym-protos.h), and
 *    the three-term rotation sum must be narrowed through a VAR_DECL.
 *    convert_to_integer distributes an outer (s16) cast into a PLUS_EXPR's
 *    operands, making every halfword leaf narrow-use-only (`lhu`) and folding
 *    into the last operand's register; it cannot distribute into a VAR_DECL.
 *    Naming the full sum (`rotation_pair`) is the fix.
 * 2. The player-yaw sum and the enemy rotation sum are ONE reused variable
 *    (`rotation_pair`); retail keeps both in $v0.  Splitting them per branch
 *    drops both sums to 2 refs each, which hands them to local_alloc and
 *    perturbs the whole player block (measured: 68 bytes).
 * 3. The abs magnitude is THREE separate variables, one per site -- spelled as
 *    the PSX.SYM header describes ("a repeated name is a nested-block scope"),
 *    i.e. the two player sites shadow the enemy one.  Only the DISTINCT VAR_DECLs
 *    matter; the names do not.  This is what puts each magnitude in $a0, and it
 *    is pure global-alloc priority:
 *
 *      global.c:allocno_compare ranks allocnos by
 *          floor_log2(n_refs) * n_refs / live_length * 10000 * size
 *      (descending), and `.lreg` prints both inputs as
 *      "Register N used R times across L insns".
 *
 *    `direction` is 20 refs / 45 insns = 17777.  A SINGLE `magnitude` covering
 *    all three sites is 16 refs / 26 insns = 24615, so it is coloured FIRST.
 *    It has no hard-reg preference of its own (set_preference strips one level
 *    of the src expression and takes operand 0; for `(set mag (abs dir))` that
 *    is a global pseudo, and every other donating site yields $v0, which
 *    magnitude hard-conflicts with).  Meanwhile `direction` INHERITS $a0 from
 *    `rotation_pair`: expand_preferences unions hard-reg preferences BOTH WAYS
 *    between a single_set's global dest and any non-conflicting global carrying
 *    a REG_DEAD note on that insn, and `direction = CamState.DirectionRY -
 *    rotation_pair` kills rotation_pair.  rotation_pair prefers $a0 because its
 *    player-branch sum `(set rp (plus obj0_load obj1_load))` has operand 0 =
 *    the obj[0] load, which local_alloc colours $a0 (`lh $a0,0x52($v1)`).
 *    prune_preferences then puts $a0 into regs_someone_prefers[magnitude] (the
 *    union of LOWER-priority conflicting allocnos' preferences), so the single
 *    magnitude avoids $a0 and lands in $a1 -- 22 bytes, a pure 19-instruction
 *    a0->a1 rename.
 *
 *    Splitting per site drops each magnitude through a floor_log2 step and
 *    below `direction`, so `direction` is coloured first, takes its own $v1,
 *    and leaves regs_someone_prefers empty for each magnitude -- whose fallback
 *    scan then hits $a0 (it conflicts with hard $v0, and $v1 is taken):
 *        enemy magnitude   6 refs / 12 insns = 10000
 *        yaw   magnitude   5 refs /  7 insns = 14285
 *        pitch magnitude   5 refs /  7 insns = 14285
 *    all < direction's 17777.  The three sites are disjoint, so all three share
 *    $a0.  Each half still spans basic blocks (the abs itself branches), so
 *    unlike rotation_pair they remain GLOBAL allocnos -- which is why splitting
 *    magnitude wins where splitting rotation_pair loses.
 *
 * Dead ends, measured, so they are not retried: a `do{}while(0)` weighting
 * fence on the enemy-vertical block does add loop-depth-weighted refs to
 * `direction` (20 -> 24) and leaves live_length alone, but its LOOP_END note
 * lands next to the cross-jumped `UpdateCoordinate` tail and blocks the merge:
 * 282 bytes.  There is no free ref to remove from magnitude either -- its 16
 * refs correspond exactly to 19 emitted instructions (3 abs defs of 2 insns
 * each + 13 single-insn uses), so none is combine-folded.
 *
 * The player arm always returns, so the enemy head-tracking path needs no
 * trailing `else`. Its two calm-phase exclusions are one short-circuit return
 * guard. Keep the target-ordered vertical clamp nested: the flatter clamp
 * ladder differs by 26 lines.
 */

extern s16 VISIBLE_ENEMIES_;
extern s16 DrawModeSave[];
extern Humanoid *VISIBLE_CHARACTERS_ON_STAGE_[];
extern char fmt_dbg_pos[];  /* ~c800%02x~c888(%d,%d,%d)  */
extern char fmt_dbg_word[]; /* ~c880%04x=%02x  */
extern char fmt_dbg_pair[]; /* ~c080%02x/%d%d  */
extern char fmt_dbg_rot[];

extern void StateTransition(Humanoid *human);
extern void DrawShadow(Humanoid *human);
extern void register_character_death(Humanoid *human);
extern void spread_blood_pool_(Humanoid *human);
extern void HumanActionControl(Humanoid *human);
extern s32 DrawClip(ModelType *model, s32 *xy);
extern s16 PlayMotion(MotionManager *motion, s16 mode);

void ControlHumanoid(Humanoid *human)
{
    ModelArchiveType *model;
    s32 m;
    MATRIX mat;
    ModelType *head;
    s32 direction;
    s32 magnitude;
    s32 rotation_pair;

    model = human->model;
    m = 1;
    if (model->object[MODEL_PART_WAIST]->id >= 0)
    {
        DefaultActionHumanoid(human);
        StateTransition(human);
        DrawShadow(human);
    }
    else if (human->status == STAT_DEAD)
    {
        register_character_death(human);
        spread_blood_pool_(human);
    }
    HumanActionControl(human);
    if ((SystemFlag & SYSFLAG_DEBUGMODE) != 0)
    {
        if (SkipFrame != 0)
        {
            goto skip_draw;
        }
        if (human == StagePlayer)
        {
            FntPrint(fmt_dbg_pos, human->type,
                     human->locate->vx / 1000,
                     human->locate->vy / 1000,
                     human->locate->vz / 1000);
            FntPrint(fmt_dbg_word, (u16)human->attribute, (u8)human->status);
            FntPrint(fmt_dbg_pair, (u8)human->motion->mid,
                     human->motion->loop, human->motion->count);
            FntPrint(fmt_dbg_rot, human->rotate->vy,
                     human->model->object[MODEL_PART_WAIST]->id);
        }
    }

    if (SkipFrame == 0)
    {
        goto do_draw;
    }
skip_draw:
    m = 0;
    goto draw_done;
do_draw:
    if (human != StagePlayer)
    {
        s32 clip;

        GsGetLs((GsCOORDINATE2 *)model, &mat);
        GsSetLsMatrix(&mat);
        clip = DrawClip((ModelType *)model, 0);
        m = 0;
        if (clip >= 0)
        {
            m = -1;
        }
    }
draw_done:

    PlayMotion(human->motion, human->status == STAT_ATTACK ? -1 : m);
    human->slocate = *human->locate;
    human->locate->vx += human->vector.vx;
    human->locate->vz += human->vector.vz;
    human->locate->vy += human->vector.vy;
    UpdateCoordinate((ModelType *)model);

    if (m == 0)
    {
        return;
    }

    /* The u16 view makes this an lhu (a plain read is lw): byte-required
     * (verified against the .s). */
    DrawModeSave[VISIBLE_ENEMIES_] = DrawTMDmode;
    VISIBLE_CHARACTERS_ON_STAGE_[VISIBLE_ENEMIES_] = human;
    VISIBLE_ENEMIES_++;
    if (ActionHalt != 0 || human->life <= 0)
    {
        return;
    }

    if (human == StagePlayer)
    {
        if (human->status == STAT_STICKON)
        {
            return;
        }
        head = human->model->object[MODEL_PART_HEAD];
        if (CamState.Mode != CMODE_DIRECTION && CamState.Mode != CMODE_SIGHT)
        {
            MotionElementType *rotation;

            if (human->motion->loop != MOTION_LOOP_DISABLED)
            {
                return;
            }
            rotation = human->motion->motion->rotate[2].keyframes;
            if (head->rotate.vx == rotation->x &&
                head->rotate.vy == rotation->y)
            {
                return;
            }
            head->rotate.vx = rotation->x;
            /* The re-walked chain (not rotation->y) is byte-required (the
             * fresh loads are in the bytes; measured). */
            head->rotate.vy =
                human->motion->motion->rotate[2].keyframes->y;
            UpdateCoordinate(head);
            return;
        }
        else
        {
            rotation_pair = human->model->object[MODEL_PART_WAIST]->rotate.vy +
                            human->model->object[1]->rotate.vy;
            {
                s32 magnitude;

                direction = CamState.DirectionRY - rotation_pair;
                magnitude = direction >= 0 ? direction : -direction;
                if (magnitude > 900)
                {
                    head->rotate.vy = magnitude * 900 / direction;
                }
                else
                {
                    head->rotate.vy = direction;
                }
            }
            {
                s32 magnitude;

                direction = CamState.DirectionRX;
                magnitude = direction >= 0 ? direction : -direction;
                if (magnitude > 500)
                {
                    head->rotate.vx = magnitude * 500 / direction;
                }
                else
                {
                    head->rotate.vx = direction;
                }
            }
        }
        UpdateCoordinate(head);
        return;
    }
    if ((human->attribute & ATTR_PHASE) != PHASE_ALERT &&
        (human->target.archive == StagePlayer->model ||
         human->motion->mid == MOT_ACTION))
    {
        return;
    }

    rotation_pair = human->model->object[MODEL_PART_WAIST]->rotate.vy +
                    human->model->object[1]->rotate.vy +
                    human->rotate->vy;
    direction = GetDirection(
        human->target.model->locate.coord.t[0] - human->locate->vx,
        human->target.model->locate.coord.t[2] - human->locate->vz,
        (s16)rotation_pair);
    magnitude = direction >= 0 ? direction : -direction;
    if (magnitude >= 1800)
    {
        return;
    }

    head = human->model->object[MODEL_PART_HEAD];
    if (magnitude > 900)
    {
        head->rotate.vy = magnitude * 900 / direction;
    }
    else
    {
        head->rotate.vy = direction;
    }

    direction = (human->target.model->locate.coord.t[1] - human->locate->vy) / 2;
    if (direction != 0)
    {
        if (direction >= -500)
        {
            if (direction <= 100)
            {
                head->rotate.vx = direction;
            }
            else
            {
                head->rotate.vx = 100;
            }
        }
        else
        {
            head->rotate.vx = -500;
        }
    }
    UpdateCoordinate(head);
}
