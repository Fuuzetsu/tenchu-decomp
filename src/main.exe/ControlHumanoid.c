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

        GsGetLs(&model->locate, &mat);
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

    /* Retail reads only the low halfword here. */
    DrawModeSave[VISIBLE_ENEMIES_] = DrawTMDmode;
    VISIBLE_CHARACTERS_ON_STAGE_[VISIBLE_ENEMIES_] = human;
    VISIBLE_ENEMIES_++;
    if (ActionHalt != ACTION_HALT_NONE || human->life <= 0)
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
            rotation =
                human->motion->motion->rotate[MODEL_PART_HEAD];
            if (head->rotate.vx == rotation->x &&
                head->rotate.vy == rotation->y)
            {
                return;
            }
            head->rotate.vx = rotation->x;
            head->rotate.vy =
                human->motion->motion->rotate[MODEL_PART_HEAD]->y;
            UpdateCoordinate(head);
            return;
        }
        else
        {
            rotation_pair = human->model->object[MODEL_PART_WAIST]->rotate.vy +
                            human->model->object[MODEL_PART_TORSO]->rotate.vy;
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
        (human->target == &StagePlayer->model->locate ||
         human->motion->mid == MOT_ACTION))
    {
        return;
    }

    rotation_pair = human->model->object[MODEL_PART_WAIST]->rotate.vy +
                    human->model->object[MODEL_PART_TORSO]->rotate.vy +
                    human->rotate->vy;
    direction = GetDirection(
        human->target->coord.t[0] - human->locate->vx,
        human->target->coord.t[2] - human->locate->vz,
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

    direction = (human->target->coord.t[1] - human->locate->vy) / 2;
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
