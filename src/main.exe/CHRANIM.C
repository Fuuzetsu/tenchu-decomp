#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "timpack.h"
#include "graphics.h"
#include "adt.h"
#include "action.h"
#include "chranim.h"
#include "effect.h"
#include "font.h"
#include "sound.h"
#include "humanoid.h"
#include "item.h"
#include "model.h"
#include "vmemory.h"
#include <psxsdk/libgpu.h>

/*
 * Retail inserts the file-animation debug menu after CVAsetup. SetupTelop
 * moved elsewhere in the retail image; the manifest retains its place in the
 * demo source order.
 */

extern char *STAGE_ANIMATION_PREFICES[N_LANGUAGES];
extern char fmt_stage_cad[];       /* %sSTAGE%d%c.CAD */
extern char path_anim_tanka_tpd[]; /* K:\\WORK\\CDIMAGE\\ANIM\\tanka.tpd */
extern char fmt_num[];             /* %d */
extern u8 str_cancel[];            /* cancel */
extern char str_event_test[];      /* event test */
extern u8 TelopText[];
extern s16 CVAflag;
extern u8 ctype_tab[];             /* BSD _ctype_+1: &4 = digit */
extern u8 CHOSEN_CHARACTER;
extern Sprite3D *TANKA_SPRITES_[N_TANKA_SPRITES];

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CVAsetup(void);
 *     CHRANIM.C:64, 15 src lines, frame 88 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     unsigned char [50] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAdata;
 *     extern int StageID;
 *     extern struct POLY_F4 TelopbgP;
 * END PSX.SYM */

void CVAsetup(void)
{
    s16 i;
    u_long *adr;
    Sprite3D *sprite;
    Sprite3D *slot;
    int letter;
    u8 name[50];
    GsIMAGE image;

    if (CVAdata != 0)
    {
        vfree(CVAdata);
    }
    letter = 'A';
    if (PSTATE->CharType == RIKIMARU_0)
    {
        letter = 'R';
    }
    sprintf((char *)name, fmt_stage_cad,
            STAGE_ANIMATION_PREFICES[PSTATE->language],
            STAGE_NUMBER(StageID), letter);
    CVAdata = (CVAType *)FileRead(name);

    SetPolyF4(&TelopbgP);
    TelopbgP.b0 = 1;
    TelopbgP.g0 = 1;
    TelopbgP.r0 = 1;
    TelopbgP.x2 = -(SCREEN_W / 2);
    TelopbgP.x0 = -(SCREEN_W / 2);
    TelopbgP.x3 = SCREEN_W / 2;
    TelopbgP.x1 = SCREEN_W / 2;

    if (StageID == STAGE_ID_CORRUPT_MINISTER &&
        PSTATE->CharType == RIKIMARU_0)
    {
        adr = FileRead((u8 *)path_anim_tanka_tpd);
        for (i = 0; i < N_TANKA_SPRITES; i++)
        {
            GetTIMpackInfo(adr, &image, i);
            sprite = SetupSprite(0, &image);
            TANKA_SPRITES_[i] = sprite;
            sprite->attribute |= MODEL_ATTR_HIDDEN;
            TANKA_SPRITES_[i]->sprite.x = (2 - i) * 20 + 10;
            TANKA_SPRITES_[i]->sprite.y = (i % 3) * 8 - 4;
            slot = TANKA_SPRITES_[i];
            slot->sprite.b = 0;
            slot->sprite.g = 0;
            slot->sprite.r = 0;
        }
        TANKA_SPRITES_[N_TANKA_SPRITES - 1]->sprite.x -= 8;
        TANKA_SPRITES_[N_TANKA_SPRITES - 1]->sprite.y = 40;
        LoadTIMpackAndFree(adr);
    }
}

void debug_menu_file_animation_test(void)
{
    u8 text[0x100];
    TAdtSelect menu[64];
    CVAType *event;
    u8 *buffer;
    s32 count;
    s32 selection;

    buffer = text;
    event = CVAdata;
    count = 0;
    while (event->mode != CVA_CMD_END)
    {
        if (event->mode == CVA_CMD_SEQUENCE)
        {
            sprintf((char *)buffer, fmt_num, event->payload.sequence.id);
            menu[count].name = buffer;
            menu[count].value = event->payload.sequence.id;
            count++;
            buffer += strlen((char *)buffer) + 1;
        }
        event++;
    }
    menu[count].name = str_cancel;
    menu[count].value = ADT_SELECT_CANCEL;
    count++;
    menu[count].name = NULL;

    selection = AdtSelect(str_event_test, menu, 0);
    if (selection != ADT_SELECT_CANCEL)
    {
        CVAsequence((s16)selection);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVAsequence(short sid);
 *     CHRANIM.C:83, 58 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short sid
 *     reg   $s3       struct SoundEffect * vab
 *     reg   $a0       struct Humanoid * human
 *     reg   $s0       short sound
 *     reg   $s0       short i
 *     reg   $a1       short j
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAdata;
 *     extern struct CVAType *CVAnow;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 *     extern struct Humanoid *CameraTarget;
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short ActionHalt;
 *     extern short MotionUpdateMode;
 *     extern struct TCdaStatus CdaStatus;
 *     extern short CVAtime;
 *     extern short VoiceMode;
 * END PSX.SYM */

s16 CVAsequence(s16 sid)
{
    CVAType *event;
    CVAType *cursor;
    Humanoid *human;
    s16 sound;
    s16 i;
    s16 motion;
    s32 wanted;
    s32 end_mode;
    s32 type_class;
    HumanAnimType *anim_base;

    CVAnow = CVAdata;
    if (CVAdata->mode == CVA_CMD_END)
        return 0;

    wanted = sid;
    end_mode = CVA_CMD_END;
scan_event:
    event = CVAnow;
    if (event->mode == CVA_CMD_SEQUENCE &&
        event->payload.sequence.id == wanted)
        goto event_found;
    CVAnow = event + 1;
    if (event[1].mode != end_mode)
        goto scan_event;

event_found:

    if (CVAnow->mode == CVA_CMD_END)
        return 0;

    memset(CVAhuman, 0, sizeof(CVAhuman));
    cursor = CVAnow;
    sound = cursor->payload.sequence.music;
    i = 0;
    CameraTarget = StagePlayer;
    cursor++;
    CVAnow = cursor;
    TelopText[0] = 0;
    for (; i < Humans; i++)
    {
        human = HumanGroup[i];
        if (human->status != STAT_DEAD &&
            (human->attribute & ATTR_SUSPEND) == 0)
        {
            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            NowReturnNormal(HumanGroup[i]);
            HumanGroup[i]->pad.data = 0;
        }
    }

    CVAflag = 0;
    if (CVAupdate() == 0)
        return 0;

    if (ActionHalt != ACTION_HALT_STAGE_END)
        ActionHalt = ACTION_HALT_ACTIVE;
    MotionUpdateMode = 1;
    StagePlayer->target = 0;
    PadShockAR(PAD_PORT_1, RUMBLE_POWER_OFF, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_NONE);
    PadShock(PAD_PORT_1, 0, 0);
    PadProc();

    if (sound > 0)
    {
        PlayMusicFormID(sound);
        while (CdaStatus.status != CDA_STATUS_IDLE)
        {
            if (CdaGetCurrentLength() > 0)
                break;
        }
    }

    CVAtime = 0;
    VoiceMode = 1;
    do
    {
    } while (CVArun() != 0);
    VoiceMode = 0;
    if (ActionHalt != ACTION_HALT_STAGE_END)
        ActionHalt = ACTION_HALT_NONE;
    MotionUpdateMode = 0;
    SetCameraMode(CMODE_NORMAL);

    i = 0;
    anim_base = CVAhuman;
    type_class = PAGE_BOSS;
    for (; i < N_CVA_HUMANS; i++)
    {
        human = anim_base[i].human;
        if (human != 0 && human->status != STAT_DEAD)
        {
            motion = MOT_ENGAGE_STANCE;
            if ((human->attribute & ATTR_WEAPON_DRAWN) == 0 &&
                (motion = 0, (human->type & PAGE_MASK) == type_class))
                motion = MOT_STATE_DRAW;
            SetNowMotion(human, motion, MOTION_MOVE_APPLY);
        }
    }

    if (sound > 0)
        VSync(60);
    CdaStop();
    PadShockAR(PAD_PORT_1, RUMBLE_POWER_OFF, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_NONE);
    PadShock(PAD_PORT_1, 0, 0);
    PadProc();
    PadProc();
    return 1;
}

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
                    SetNowMotion(human, MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);

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

                /* ACTOR commands pack two signed bytes into the x field. */
                if (CVAnow->payload.actor.motion == CVA_ACTOR_DESPAWN)
                {
                    human->life = HUMANOID_LIFE_INACTIVE;
                    human->attribute = (human->attribute | ATTR_SUSPEND | PHASE_ALERT) & ~ATTR_CUSTOMAI;
                    human->motion->mid = MOTION_ID_NONE;
                    SetNowMotion(human, MOT_NORMAL, MOTION_MOVE_APPLY);
                    PlayMotion(human->motion, 1);
                    human->motion->count--;
                }
                else
                {
                    i = MOTION_STATUS(CVAnow->payload.actor.motion);
                    if (human->status == STAT_DEAD &&
                        (u32)(i - MOTION_STATUS(MOT_DAMAGE)) > 1)
                        return 0;
                    if (human->life > 0)
                    {
                        if (i == MOTION_STATUS(MOT_DEAD))
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
                    SetNowMotion(human, CVAnow->payload.actor.motion, MOTION_MOVE_APPLY);
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
                    if (StageID != STAGE_ID_CORRUPT_MINISTER ||
                        CHOSEN_CHARACTER != RIKIMARU_0)
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
                        TANKA_SPRITES_[i - 1]->attribute &=
                            ~MODEL_ATTR_HIDDEN;
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVArun(void);
 *     CHRANIM.C:238, 47 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a2       struct MotionManager * mmp
 *     reg   $s1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern int StageID;
 *     extern struct GsOT *OTablePt;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern short CVAtime;
 *     extern struct CVAType *CVAnow;
 * END PSX.SYM */

short CVArun(void)
{
    s16 i;
    Sprite3D *e;
    MotionManager *mmp;
    Humanoid *human;
    Humanoid *reload;
    motion_id motid;

    ComputeAllConflict();
    StartDrawing();
    AVCameraControl();
    ControlAllHumanoid();
    DrawConstruction();
    DrawEffect();
    DoItemProc();
    DoMiscProc();
    DrawTelop();
    draw_visible_characters_();

    if (StageID == STAGE_ID_CORRUPT_MINISTER && CHOSEN_CHARACTER == RIKIMARU_0)
    {
        for (i = 0; i < N_TANKA_SPRITES; i++)
        {
            e = TANKA_SPRITES_[i];
            if ((e->attribute & MODEL_ATTR_HIDDEN) == 0)
            {
                if ((s8)e->sprite.r >= 0)
                {
                    e->sprite.r = e->sprite.g = e->sprite.b =
                        e->sprite.b + 8;
                }
                GsSortSprite(&TANKA_SPRITES_[i]->sprite, OTablePt, 0);
            }
        }
    }

    EndDrawing(-2);

    for (i = 0; i < N_CVA_HUMANS; i++)
    {
        human = CVAhuman[i].human;
        if (human != 0 && CVAhuman[i].loop <= human->motion->loop)
        {
            mmp = human->motion;
            motid = CVAhuman[i].motid;
            if (motid == MOTION_ID_NONE)
            {
                mmp->loop = MOTION_LOOP_DISABLED;
                reload = CVAhuman[i].human;
                reload->vector.vz = 0;
                reload->vector.vx = 0;
            }
            else if (human->status != STAT_DEAD)
            {
                SetNowMotion(human, motid, MOTION_MOVE_APPLY);
                CVAhuman[i].human = 0;
            }
        }
    }

    CVAtime++;
    if (CVAtime >= CVAnow->payload.wait.frames)
    {
        CVAnow++;
        return CVAupdate();
    }
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AVCameraSetup(void);
 *     CHRANIM.C:289, 29 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct SVECTOR vect
 *     reg   $a1       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAnow;
 *     extern struct Humanoid *CameraTarget;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

void AVCameraSetup(void)
{
    CVAType *event;
    Humanoid *human;
    SVECTOR vect;
    s32 ry;

    event = CVAnow;
    switch (event->payload.camera_cut.kind)
    {
    case CVA_CAMERA_CUT_TARGET_RELATIVE_BASE:
    case CVA_CAMERA_CUT_TARGET_RELATIVE_QUARTER_TURN:
    case CVA_CAMERA_CUT_TARGET_RELATIVE_HALF_TURN:
    case CVA_CAMERA_CUT_TARGET_RELATIVE_THREE_QUARTER_TURN:
        ry = (u16)CameraTarget->rotate->vy +
             event->payload.camera_cut.kind * ANGLE_QUADRANT;
        vect.pad = (s16)ry;
        GetMoveSpeed(&vect, (s16)ry,
                     (event->payload.camera_cut.parameters.orbit.distance != 0)
                         ? event->payload.camera_cut.parameters.orbit.distance
                         : CVA_CAMERA_DEFAULT_ORBIT_DISTANCE,
                     0);
        ViewInfo.vpx = CameraTarget->locate->vx + vect.vx;
        ViewInfo.vpy = (CameraTarget->locate->vy - CameraTarget->height) +
                       CVA_CAMERA_TARGET_HEIGHT_OFFSET;
        ViewInfo.vpz = CameraTarget->locate->vz + vect.vz;
        break;

    case CVA_CAMERA_CUT_FIXED_POSITION:
        ViewInfo.vpx = event->payload.camera_cut.parameters.fixed.position.x *
                       CVA_CAMERA_POSITION_SCALE;
        ViewInfo.vpy = event->payload.camera_cut.parameters.fixed.position.y *
                       CVA_CAMERA_POSITION_SCALE;
        ViewInfo.vpz = event->payload.camera_cut.parameters.fixed.position.z *
                       CVA_CAMERA_POSITION_SCALE;
        break;

    case CVA_CAMERA_CUT_HUMANOID_POSITION:
        human = GetHumanoid(
            event->payload.camera_cut.parameters.humanoid.actor);
        if (human == 0)
        {
            return;
        }
        ViewInfo.vpx = human->locate->vx;
        ViewInfo.vpy = (human->locate->vy - human->height) +
                       CVA_CAMERA_TARGET_HEIGHT_OFFSET;
        ViewInfo.vpz = human->locate->vz;
        CameraTarget = human;
        break;
    }

    GsSetRefView2(&ViewInfo);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AVCameraControl(void);
 *     CHRANIM.C:322, 44 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct SVECTOR vect
 *     reg   $s1       long xx
 *     reg   $s0       long zz
 *     reg   $s2       long len
 *     reg   $a1       short ry
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern short CameraPanMode;
 *     extern short CameraSpeed;
 *     extern struct Humanoid *CameraTarget;
 * END PSX.SYM */

void AVCameraControl(void)
{
    SVECTOR vect;
    long xx;
    long zz;
    long len;
    short ry;
    long base_angle;
    long speed;

    xx = ViewInfo.vpx - ViewInfo.vrx;
    zz = ViewInfo.vpz - ViewInfo.vrz;
    len = SquareRoot0(xx * xx + zz * zz);
    ry = GetDirection(xx, zz, 0);

    switch (CameraPanMode)
    {
    case CAMERA_PAN_DISABLED:
        return;
    case CAMERA_PAN_NORMAL_CAMERA:
        Camera();
        return;
    case CAMERA_PAN_ORBIT_ANGLE_INCREASE:
    case CAMERA_PAN_ORBIT_ANGLE_DECREASE:
        base_angle = ry;
        if (CameraPanMode == CAMERA_PAN_ORBIT_ANGLE_INCREASE)
        {
            speed = CameraSpeed;
            ry = base_angle + speed;
        }
        else
        {
            speed = CameraSpeed;
            ry = base_angle - speed;
        }
        GetMoveSpeed(&vect, ry, len, 0);
        ViewInfo.vpx = ViewInfo.vrx + vect.vx;
        ViewInfo.vpz = ViewInfo.vrz + vect.vz;
        break;
    case CAMERA_PAN_UP:
    case CAMERA_PAN_DOWN:
        ViewInfo.vpy += (CameraPanMode == CAMERA_PAN_UP)
                            ? -CameraSpeed
                            : CameraSpeed;
        break;
    case CAMERA_PAN_ZOOM_IN:
    case CAMERA_PAN_ZOOM_OUT:
        if (CameraPanMode == CAMERA_PAN_ZOOM_IN)
        {
            len -= CameraSpeed;
        }
        else
        {
            len += CameraSpeed;
        }
        GetMoveSpeed(&vect, ry, len, 0);
        ViewInfo.vpx = ViewInfo.vrx + vect.vx;
        ViewInfo.vpz = ViewInfo.vrz + vect.vz;
        break;
    case CAMERA_PAN_TRACK_TARGET:
        ViewInfo.vrx = CameraTarget->locate->vx;
        ViewInfo.vry = CameraTarget->locate->vy - CameraTarget->height + 300;
        ViewInfo.vrz = CameraTarget->locate->vz;
        break;
    }

    GsSetRefView2(&ViewInfo);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTelop(void);
 *     CHRANIM.C:420, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_F4 TelopbgP;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

void DrawTelop(void)
{
    enum
    {
        TELOP_INNER_Y = 90
    };
    s32 w;

    TelopbgP.y1 = TELOP_INNER_Y;
    TelopbgP.y0 = TELOP_INNER_Y;
    TelopbgP.y3 = SCREEN_H / 2;
    TelopbgP.y2 = SCREEN_H / 2;
    GsSortPoly(&TelopbgP, OTablePt, 1);
    TelopbgP.y1 = -(SCREEN_H / 2);
    TelopbgP.y0 = -(SCREEN_H / 2);
    TelopbgP.y3 = -TELOP_INNER_Y;
    TelopbgP.y2 = -TELOP_INNER_Y;
    GsSortPoly(&TelopbgP, OTablePt, 1);
    w = telop_text_width_(TelopText);
    draw_telop_line_(OTablePt->org, -(w / 2), 92, TelopText);
}
