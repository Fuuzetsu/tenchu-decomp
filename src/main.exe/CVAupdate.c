#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVAupdate(void);
 *     CHRANIM.C:145, 89 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct SVECTOR vect
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       long i
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAnow;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct SVECTOR UnitVector;
 *     extern struct Humanoid *StagePlayer;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Humanoid *CameraTarget;
 *     extern short CameraPanMode;
 *     extern short CameraSpeed;
 *     extern struct CVAType *CVAdata;
 *     extern int StageID;
 * END PSX.SYM */

/*
 * CVAupdate (0x80050628) — interpret character-animation and camera events.
 *
 * Byte-matching. Jump-table function (switch on the event kind).
 *
 * Four source-shape constraints remain; each is a cc1-2.8.1 mechanism worth
 * knowing (see the commits for the RTL/pinned-source evidence):
 *   - the motion row's y position scaled by 1000: a dedicated `y` local made
 *     the last shift write a
 *     BLOCK-LOCAL temp whose copy to `y` sched1 sank past the GetAreaMapLevel
 *     call, so local-alloc's combine_regs tied the whole x1000 chain into one
 *     call-crossing quantity and forced it callee-saved. combine_regs refuses to
 *     tie into a pseudo that is not block-local, so reusing the long-lived `i`
 *     (PSX.SYM: `long i` in $s0, provably dead here) keeps the chain in $v0.
 *   - `cursor` keeps the command walk in one identity; reading `CVAnow`
 *     directly changes 10 canonical lines.
 *   - `model` keeps the object-loop base live; following `human->model`
 *     directly changes 14 canonical lines.
 *   - `pan_value` stages the default/override before the one CameraSpeed
 *     store; the direct ternary changes 15 canonical lines.
 *
 * PSX.SYM's three-locals record (vect, human, i) remains the through-line.
 * The complete indexed animation graph removes `anim_base`, `anim`, and
 * `slot`; on that graph the `packed` and `ch` carriers and the register-held
 * `invalid` constant also disappear exactly. Their earlier isolated failures
 * were allocation effects of the pointer-cursor graph, not source
 * requirements.
 */

extern s16 CVAflag; /* set by CVA camera/telop commands */
extern u8 TelopText[];
extern u8 ctype_tab[]; /* BSD _ctype_+1: &4 = digit */
extern u8 CHOSEN_CHARACTER;
extern Sprite3D *TANKA_SPRITES_[N_TANKA_SPRITES];

extern s16 PlayMotion(MotionManager *motion, s16 mode);
extern int ReqLifeBar(Humanoid *h);
extern void CdaStop(void);
extern void AVCameraSetup(void);
extern void SetBlood(VECTOR *pos, s16 n, s16 time);
extern void set_fade_(u8 arg0, u8 arg1, u8 arg2, long arg3);
extern char *strcpy(char *dst, const char *src);
extern void SetupTelop(u8 *telop, s16 line);

s16 CVAupdate(void)
{
    Humanoid *human;
    ModelArchiveType *model;
    CVAType *cursor;
    VECTOR vect;
    s32 i;
    s32 pan_value;

    cursor = CVAnow;
    if (cursor->mode != CVA_CMD_WAIT)
    {
        do
        {
            switch (cursor->mode)
            {
            case CVA_CMD_SEQUENCE:
                /* A chained header mid-stream: music re-selects the CD
                 * track, and CVA_MUSIC_STOP silences it. */
                if (CVAnow->payload.sequence.music == CVA_MUSIC_STOP)
                    CdaStop();
                break;

            case CVA_CMD_MOTION:
                human = GetHumanoid(CVAnow->payload.motion.actor);
                if (human == 0)
                    return 0;
                i = 0;

                human->attribute &= ~ATTR_SUSPEND;
                human->motion->mask = MOTION_MASK_ALL;
                while (1)
                {
                    if (CVAhuman[i].human == 0)
                        break;
                    i++;
                    if (i >= N_CVA_HUMANS)
                        break;
                }
                if (i == N_CVA_HUMANS)
                    SetNowMotion(human, MOT_ENGAGE_STANCE, 1);

                human->vector = UnitVector;
                human->model->object[MODEL_PART_WAIST]->attribute |= MODEL_ATTR_COLLIDE;
                model = human->model;
                i = 0;
                if (model->n > 0)
                {
                    do
                    {
                        model->object[i]->attribute &= ~MODEL_ATTR_HIDDEN;
                        i++;
                    } while (i < model->n);
                }

                if (StagePlayer != human &&
                    human->life == HUMANOID_LIFE_INACTIVE)
                {
                    human->attribute |= ATTR_CUSTOMAI;
                    human->life = human->lifemax;
                }

                if (CVAnow->payload.motion.facing !=
                    CVA_MOTION_NO_REPOSITION)
                {
                    human->locate->vx = human->point[HUMANOID_HOME_X] =
                        CVAnow->payload.motion.position.x *
                        CVA_WORLD_POSITION_SCALE;
                    human->locate->vz = human->point[HUMANOID_HOME_Z] =
                        CVAnow->payload.motion.position.z *
                        CVA_WORLD_POSITION_SCALE;
                    i = CVAnow->payload.motion.position.y *
                        CVA_WORLD_POSITION_SCALE;
                    human->locate->vy = GetAreaMapLevel(
                        GlobalAreaMap, human->locate->vx,
                        i - CVA_WORLD_POSITION_SCALE,
                        human->locate->vz, AREA_LEVEL_DEFAULT);
                    if (i < human->locate->vy ||
                        human->locate->vy == LEVEL_NONE)
                        human->locate->vy = i;
                    human->rotate->vy = CVAnow->payload.motion.facing;
                    UpdateCoordinate((ModelType *)human->model);
                    if (__builtin_abs(human->locate->vy -
                                      StagePlayer->locate->vy) > 20000)
                        human->attribute |= ATTR_SUSPEND;
                }
                break;

            case CVA_CMD_ACTOR:
                human = GetHumanoid(CVAnow->payload.actor.actor);
                if (human == 0)
                    return 0;

                /* For ACTOR commands the signed x slot packs two bytes.  The
                 * direct invalid test and arithmetic >>8 still share the
                 * target's one shift after the animation scans are indexed. */
                if (CVAnow->payload.actor.motion == CVA_ACTOR_DESPAWN)
                {
                    human->life = HUMANOID_LIFE_INACTIVE;
                    human->attribute = (human->attribute | ATTR_SUSPEND | PHASE_ALERT) & ~ATTR_CUSTOMAI;
                    human->motion->mid = MOTION_ID_NONE;
                    SetNowMotion(human, 0, 1);
                    PlayMotion(human->motion, 1);
                    human->motion->count--;
                }
                else
                {
                    i = CVAnow->payload.actor.motion >> 8;
                    if (human->status == STAT_DEAD && (u32)(i - (MOT_DAMAGE >> 8)) > 1)
                        return 0;
                    if (human->life > 0)
                    {
                        if (i == (MOT_DEAD >> 8))
                        {
                            human->life = 0;
                            ReqLifeBar(human);
                        }
                    }
                    i = 0;

                    while (1)
                    {
                        if (CVAhuman[i].human == human)
                            break;
                        i++;
                        if (i >= N_CVA_HUMANS)
                            break;
                    }
                    if (i == N_CVA_HUMANS)
                    {
                        i = 0;
                        while (1)
                        {
                            if (CVAhuman[i].human == 0)
                                break;
                            i++;
                            if (i >= N_CVA_HUMANS)
                                break;
                        }
                        if (i == N_CVA_HUMANS)
                            return 0;
                    }

                    human->motion->mid = MOTION_ID_NONE;
                    SetNowMotion(human, CVAnow->payload.actor.motion, 1);
                    PlayMotion(human->motion, 1);
                    human->motion->count--;
                    CVAhuman[i].human = human;

                    CVAhuman[i].loop =
                        CVAnow->payload.actor.loop < 1
                            ? MOTION_LOOP_FOREVER
                            : CVAnow->payload.actor.loop;
                    CVAhuman[i].motid =
                        CVAnow->payload.actor.next_motion == 0
                            ? MOT_ENGAGE_STANCE
                            : CVAnow->payload.actor.next_motion;

                    if (human->type == S2 &&
                        CVAnow->payload.actor.motion == MOT_DEAD)
                        SoundEx(0, SE_CUTSCENE_DEATH);
                }
                break;

            case CVA_CMD_CAMERA_CUT:
                AVCameraSetup();
                CVAflag = 1;
                break;

            case CVA_CMD_CAMERA_POSE:
                if (CVAnow->payload.camera_pose.kind ==
                    CVA_CAMERA_POSE_FIXED_REFERENCE)
                {
                    ViewInfo.vrx =
                        CVAnow->payload.camera_pose.reference.fixed.position.x *
                        CVA_CAMERA_POSITION_SCALE;
                    ViewInfo.vry =
                        CVAnow->payload.camera_pose.reference.fixed.position.y *
                        CVA_CAMERA_POSITION_SCALE;
                    ViewInfo.vrz =
                        CVAnow->payload.camera_pose.reference.fixed.position.z *
                        CVA_CAMERA_POSITION_SCALE;
                }
                else
                {
                    human = GetHumanoid(
                        CVAnow->payload.camera_pose.reference.humanoid.actor);
                    if (human == 0)
                        return 0;
                    ViewInfo.vrx = human->locate->vx;
                    ViewInfo.vry = human->locate->vy - human->height +
                                   CVA_CAMERA_TARGET_HEIGHT_OFFSET;
                    ViewInfo.vrz = human->locate->vz;
                    CameraTarget = human;
                }
                GsSetRefView2(&ViewInfo);
                break;

            case CVA_CMD_CAMERA_PAN:
                CameraPanMode = CVAnow->payload.camera_pan.mode;
                pan_value = CVA_CAMERA_DEFAULT_PAN_SPEED;
                if (CVAnow->payload.camera_pan.speed != 0)
                    pan_value = CVAnow->payload.camera_pan.speed;
                CameraSpeed = pan_value;
                break;

            case CVA_CMD_EFFECT:
                vect.vx = CVAnow->payload.effect.parameters.raw.x *
                          CVA_EFFECT_POSITION_SCALE;
                vect.vy = CVAnow->payload.effect.parameters.raw.y *
                          CVA_EFFECT_POSITION_SCALE;
                vect.vz = CVAnow->payload.effect.parameters.raw.z *
                          CVA_EFFECT_POSITION_SCALE;
                switch (CVAnow->payload.effect.kind)
                {
                case CVA_EFFECT_BLOOD:
                    SetBlood(&vect,
                             CVAnow->payload.effect.parameters.blood.count,
                             CVA_BLOOD_DURATION);
                    break;
                case CVA_EFFECT_FADE:
                    set_fade_(
                        (u8)CVAnow->payload.effect.parameters.fade.red,
                        (u8)CVAnow->payload.effect.parameters.fade.green,
                        (u8)CVAnow->payload.effect.parameters.fade.blue,
                        CVAnow->payload.effect.parameters.fade.frames);
                    break;
                }
                break;

            case CVA_CMD_TELOP:
                if (CVAnow->payload.telop.text_offset != CVA_TELOP_CLEAR)
                {
                    SetupTelop((u8 *)strcpy((char *)TelopText,
                                            (char *)CVAdata +
                                                CVAnow->payload.telop.text_offset),
                               0);
                    CVAflag = 1;
                    if (StageID != STAGE_FREE_PRINCESS || CHOSEN_CHARACTER != RIKIMARU_0)
                        break;

                    if ((ctype_tab[TelopText[0]] & 4) == 0)
                        break;

                    i = TelopText[0] - '0';
                    if (i == 0)
                    {
                        do
                        {
                            TANKA_SPRITES_[i]->attribute |= MODEL_ATTR_HIDDEN;
                            i++;
                        } while (i < N_TANKA_SPRITES);
                    }
                    else
                    {
                        *(u16 *)&TANKA_SPRITES_[i - 1]->attribute &= ~MODEL_ATTR_HIDDEN;
                    }
                }
                TelopText[0] = 0;
                break;
            }

            CVAnow++;
            cursor = CVAnow;
        } while (cursor->mode != CVA_CMD_WAIT);
    }

    return CVAnow->payload.wait.frames != 0;
}
