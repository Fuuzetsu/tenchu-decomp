#include "common.h"
#include "tuning.h"
#include "sound.h"

#include "main.exe.h"

#include "action.h"
#include "appear.h"
#include "humanoid.h"
#include "item.h"
#include "padcmd.h"
#include "effect.h"
#include "afterimage.h"
#include "model.h"

/*
 * Retail reorganises the demo's MOTION.C and adds six helpers.
 * The translation-unit manifest retains the earlier source-line order.
 */

extern Humanoid *Me_MOTION_C;
extern MapVector map;
extern Humanoid *DeadHumanoid;
extern s32 StickonItem;

short SwimCheck(void);
short FallCheck(void);
short HangCheck(void);
void DamageControl(void);
short MotionAndMove(void);
void AttackCancelControl(s16 mode);
void bow_shoot_logic(s16 kind, VECTOR *start);
void ReturnNormal(void);
void ActNORMAL(void);
void ActACTION(void);
void ActMOVE(void);
void ActSWIM(void);
void ActKAGI(void);
void ActENGAGE(void);
void ActCHASE(void);
void ActATTACK(void);
void ActSTATE(void);
void ActJUMP(void);
void ActHANG(void);
void ActSQUAT(void);
void ActSTICKON(void);
void ActCEILHANG(void);
void ActSYURI(void);
void ActITEM(void);
void ActDAMAGE(void);
void ActDEAD(void);

static void (*ActionFunc[N_CHARACTER_STATUSES])(void) = {
    ActNORMAL,
    ActACTION,
    ActMOVE,
    ActSWIM,
    ActKAGI,
    ActENGAGE,
    ActCHASE,
    ActATTACK,
    ActSTATE,
    ActJUMP,
    ActHANG,
    ActSQUAT,
    ActSTICKON,
    ActCEILHANG,
    ActSYURI,
    ActITEM,
    ActDAMAGE,
    ActDEAD
};

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void HumanActionControl(struct Humanoid *human);
 *     MOTION.C:145, 21 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short dtPAD;
 *     extern short dtCMD;
 *     extern short motMODE;
 *     extern struct SVECTOR *dtV;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern void (*ActionFunc[18])();
 * END PSX.SYM */

void HumanActionControl(Humanoid *human)
{
    dtPAD = human->pad.data;
    Me_MOTION_C = human;
    dtCMD = GetCommand(&human->pad);
    motMODE = MOTION_MOVE_UNSET;
    dtV = &Me_MOTION_C->vector;
    dtL = Me_MOTION_C->locate;
    dtR = Me_MOTION_C->rotate;
    dtM = Me_MOTION_C->motion;
    motID = dtM->mid;
    if ((Me_MOTION_C->attribute & ATTR_HIT) != 0)
    {
        DamageControl();
    }
    else
    {
        if (FallCheck() != 0)
        {
            HangCheck();
        }
        else if ((Me_MOTION_C->map.attrib & MAP_WATER) != 0)
        {
            SwimCheck();
        }
    }
    if ((dtPAD & PADL1) != 0)
    {
        dtPAD = dtPAD & ~(PADLup | PADLright | PADLdown | PADLleft);
    }
    ActionFunc[Me_MOTION_C->status]();
    if (motMODE != MOTION_MOVE_UNSET)
    {
        MotionAndMove();
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SwimCheck(void);
 *     MOTION.C:219, 33 src lines, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct VECTOR vect
 *     reg   $a2       struct ModelArchiveType * mdl
 *     reg   $a1       short i
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct MotionManager *dtM;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 *     extern short ActionHalt;
 *     extern short motMODE;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

short SwimCheck(void)
{
    character_status status;
    short i;
    VECTOR vect;
    VECTOR *locate;
    int object_id;
    int r;
    u16 motion;
    long width;

    if (Me_MOTION_C->map.height <= 0)
    {
        if ((Me_MOTION_C->map.attrib & MAP_WATER) == 0)
        {
            return 0;
        }
        status = Me_MOTION_C->status;
        if (status == STAT_KAGI)
        {
            return 0;
        }
        if (status == STAT_SWIM)
        {
            goto return_one;
        }
        if (status == STAT_DEAD)
        {
            if (motID == MOT_DEAD_DROWN)
            {
                goto return_one;
            }
            if (dtM->loop == MOTION_LOOP_DISABLED)
            {
                return 0;
            }
        }

        if (Me_MOTION_C->status != STAT_SQUAT)
        {
            object_id = (*Me_MOTION_C->model->object)->id;
            if (object_id >= 0)
            {
                locate = dtL;
                dtL->vx = ConflictObject[object_id].position.vx;
                locate->vz = ConflictObject[object_id].position.vz;
            }
        }

        vect.vy = Me_MOTION_C->map.level;
        i = 0;
        do
        {
            r = rand();
            width = Me_MOTION_C->width;
            vect.vx = dtL->vx + (r % width) * 2 - width;
            r = rand();
            width = Me_MOTION_C->width;
            vect.vz = dtL->vz + (r % width) * 2 - width;
            SetSplash(&vect, (rand() & 7) << FIXED_SHIFT,
                      (rand() & 7) << FIXED_SHIFT, 6);
            i++;
        } while (i < 20);

        AttackCancelControl(ATTACK_CANCEL_ALL);
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_SWIM);
            PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_NONE);
        }
        ActionHalt = ACTION_HALT_NONE;
        set_model_hide_(Me_MOTION_C, 1);
        motion = GetMotionID(dtM, MOT_SWIM);
        if ((s16)motion < 0 || Me_MOTION_C->life == 0)
        {
            SET_MOTION(MOT_DEAD_DROWN, MOTION_MOVE_APPLY);
            Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
            Me_MOTION_C->life = 0;
            ReqLifeBar(Me_MOTION_C);
        }
        else
        {
            SET_MOTION(MOT_SWIM, MOTION_MOVE_APPLY);
        }

        SET_NOW_MOTION_UNLESS_CVA(goto motion_done);
    motion_done:
        Sound(Me_MOTION_C, SE_WATER_SPLASH);
        reset_alert_duration();
        goto return_one;
    }
    return 0;
return_one:
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short FallCheck(void);
 *     MOTION.C:256, 31 src lines, frame 40 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct SVECTOR vect
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct MotionManager *dtM;
 *     extern short RefrectMove[16][2];
 *     extern struct VECTOR *dtL;
 *     extern short motMODE;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

short FallCheck(void)
{
    if (motID == MOT_STATE_FALL)
    {
        return 1;
    }
    if (motID == MOT_ATTACK_DIVE)
    {
        return 0;
    }
    if (Me_MOTION_C->status == STAT_JUMP && Me_MOTION_C->map.height > 0)
    {
        return 1;
    }
    if (((u16)Me_MOTION_C->attribute & ATTR_FLOAT) != 0 ||
        Me_MOTION_C->map.height <= 1000)
    {
        return 0;
    }
    switch (Me_MOTION_C->status)
    {
    case STAT_SQUAT:
        if (dtM->loop != 0)
        {
            break;
        }
    case STAT_KAGI:
    case STAT_HANG:
    case STAT_CEILHANG:
    case STAT_DAMAGE:
    case STAT_DEAD:
        return 0;
    default:
        break;
    }

    dtM->mask = MOTION_MASK_ALL;
    dtL->vx += (Me_MOTION_C->width *
                RefrectMove[Me_MOTION_C->map.angleH][0]) >> 2;
    dtL->vz += (Me_MOTION_C->width *
                RefrectMove[Me_MOTION_C->map.angleH][1]) >> 2;
    motMODE = MOTION_MOVE_NONE;
    motID = MOT_STATE_FALL;
    SET_NOW_MOTION_UNLESS_CVA(goto found);
found:
    if (Me_MOTION_C->status == STAT_SQUAT)
    {
        dtM->count >>= 2;
    }
    AttackCancelControl(ATTACK_CANCEL_ALL);
    return -1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short HangCheck(void);
 *     MOTION.C:291, 51 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct SVECTOR vect
 *     reg   $s0       long yy
 *     reg   $s0       long y
 *     reg   $s1       long ry
 *     reg   $s4       long oy
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern unsigned long *GlobalAreaMap;
 *     extern short MotionUpdateMode;
 *     extern short motMODE;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

short HangCheck(void)
{
    SVECTOR vect;
    long yy;
    long y;
    long ry;
    long dy;
    long oy;
    long yc;
    short i;

    if ((Me_MOTION_C->type & PAGE_MASK) == PAGE_BEAST)
    {
        return 0;
    }
    if (Me_MOTION_C->map.height <= 0 || motID == MOT_JUMP_WALLKICK ||
        Me_MOTION_C->active_item == ACTIVE_ITEM_DISGUISE)
    {
        return 0;
    }
    yy = dtL->vy - Me_MOTION_C->height;
    GetMoveSpeed(&vect, dtR->vy, Me_MOTION_C->width >> 1, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, yy - 290,
                        dtL->vz + vect.vz, AREA_LEVEL_DEFAULT);
    dy = yy - 300;
    if (y < dtL->vy)
    {
        y = GetAreaMapLevel(GlobalAreaMap, dtL->vx - vect.vx, yy - 290,
                            dtL->vz - vect.vz, AREA_LEVEL_DEFAULT);
        if (y == (u32)LEVEL_NONE)
        {
            return 0;
        }
        if (dtL->vy < y)
        {
            dtL->vx -= (vect.vx >> 1);
            dtL->vz -= (vect.vz >> 1);
        }
        return 0;
    }
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx, dy, dtL->vz,
                        AREA_LEVEL_DEFAULT);
    if (y < dtL->vy - Me_MOTION_C->height)
    {
        return 0;
    }
    GetMoveSpeed(&vect, dtR->vy, (Me_MOTION_C->width >> 1) + 300, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx,
                        yy - LEDGE_PROBE_RISE, dtL->vz + vect.vz,
                        AREA_LEVEL_RETURN_DELTA);
    if (y == (u32)LEVEL_NONE || y > LEDGE_PROBE_RISE)
    {
        return 0;
    }
    dtL->vy -= (105 - y);
    if (Me_MOTION_C->status == STAT_HANG)
    {
        return 1;
    }
    ry = dtR->vy;
    oy = y;
    if (ry & 0xFF)
    {
        yc = ry & ANGLE_QUADRANT_MASK;
        if (ry & ANGLE_HALF_QUADRANT)
        {
            yc += ANGLE_QUADRANT;
        }
        ry = yc;
    }
    GetMoveSpeed(&vect, (s16)ry, (Me_MOTION_C->width >> 1) + 300, 0);
    dy = (dtL->vy - Me_MOTION_C->height) - LEDGE_PROBE_RISE;
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, dy,
                        dtL->vz + vect.vz, AREA_LEVEL_RETURN_DELTA);
    if (y == (u32)LEVEL_NONE || y > LEDGE_PROBE_RISE)
    {
        dtL->vy -= (oy - 5);
        return 0;
    }
    dtR->vy = ry;
    GetMoveSpeed(&vect, (s16)ry, (Me_MOTION_C->width >> 1) + 100, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, dy,
                        dtL->vz + vect.vz, AREA_LEVEL_RETURN_DELTA);
    if (y != (u32)LEVEL_NONE && y <= LEDGE_PROBE_RISE)
    {
        GetMoveSpeed(&vect, dtR->vy, -200, 0);
        dtL->vx += vect.vx;
        dtL->vz += vect.vz;
        dtL->vy -= (105 - y);
    }
    motID = MOT_HANG_CATCH;
    motMODE = MOTION_MOVE_APPLY;
    SET_NOW_MOTION_UNLESS_CVA(goto found);
found:
    Sound(Me_MOTION_C, SE_LEDGE_GRIP);
    if (StagePlayer != Me_MOTION_C)
    {
        return -1;
    }
    PadShockAR(PAD_PORT_1, RUMBLE_POWER_HALF, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_LONG);
    return -1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MapVector * StickonCheck(void);
 *     MOTION.C:346, 17 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v1       short rv
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct VECTOR *dtL;
 *     extern short RefrectVector[16];
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

MapVector *StickonCheck(void)
{
    facing_angle rv;

    if ((u16)Me_MOTION_C->type >= N_PLAYABLE_CHARACTERS)
    {
        return 0;
    }
    if ((Me_MOTION_C->map.attrib & (MAP_SLOPE_X | MAP_SLOPE_Z)) != 0)
    {
        return 0;
    }
    GetAreaMapVector(GlobalAreaMap, &map, dtL, Me_MOTION_C->width + 100,
                     AREA_LEVEL_STEP_DOWN | AREA_LEVEL_ALLOW_DEEP);
    if ((map.attrib & (MAP_SLOPE_X | MAP_SLOPE_Z)) == 0)
    {
        rv = RefrectVector[map.vector];
        /* The half-quadrant bit is set exactly on diagonal wall angles: a
         * stick already in progress may continue around a corner, but a new
         * one cannot start there. */
        if (Me_MOTION_C->status != STAT_STICKON &&
            (rv & ANGLE_HALF_QUADRANT) != 0)
        {
            return 0;
        }
        if (rv == ANGLE_NONE)
        {
            return 0;
        }
        if (Me_MOTION_C->status != STAT_STICKON)
        {
            SET_MOTION(MOT_STICKON, MOTION_MOVE_APPLY);
        }
        return &map;
    }
    return 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void JumpControl(void);
 *     MOTION.C:367, 37 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct VECTOR *dtL;
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR *dtV;
 *     extern short dtPAD;
 * END PSX.SYM */

void JumpControl(void)
{
    int id;

    spawn_smoke_burst_(dtL, 150, SMOKE_DRIFT_DIVISOR_DEFAULT, 8);
    if (GetMotionID(dtM, MOT_JUMP) < 0)
        return;

    if (motID == MOT_CHASE_DASH_FWD)
    {
        if (dtM->count < 11 &&
            GetMotionID(dtM, MOT_JUMP_RUN) >= 0)
        {
            SET_MOTION(MOT_JUMP_RUN, MOTION_MOVE_NONE);
            MoveHumanoid(Me_MOTION_C, RUN_JUMP_SPEED, 0);
            if (Me_MOTION_C == StagePlayer)
            {
                Sound(Me_MOTION_C, SE_JUMP_IMPACT);
            }
            Sound(Me_MOTION_C, SE_JUMP_MOVE);
        }
    }
    else
    {
        id = Me_MOTION_C->model->object[MODEL_PART_WAIST]->id;
        if (id >= 0)
        {
            dtL->vx = ConflictObject[id].position.vx;
            dtL->vz = ConflictObject[id].position.vz;
        }
        SET_MOTION(MOT_JUMP, MOTION_MOVE_NONE);
        dtV->vy = 0;
        if (dtPAD & PADLup)
        {
            if (GetMotionID(dtM, MOT_JUMP_FORWARD) >= 0)
            {
                SET_MOTION(MOT_JUMP_FORWARD, MOTION_MOVE_NONE);
            }
            MoveHumanoid(Me_MOTION_C, 100, 0);
        }
        else if (dtPAD & PADLdown)
        {
            if (GetMotionID(dtM, MOT_JUMP_BACK) >= 0)
            {
                SET_MOTION(MOT_JUMP_BACK, MOTION_MOVE_NONE);
            }
            MoveHumanoid(Me_MOTION_C, -100, 0);
        }
        else if (dtPAD & PADLright)
        {
            if (GetMotionID(dtM, MOT_JUMP_RIGHT) >= 0)
            {
                SET_MOTION(MOT_JUMP_RIGHT, MOTION_MOVE_NONE);
            }
            MoveHumanoid(Me_MOTION_C, 0, -100);
        }
        else if (dtPAD & PADLleft)
        {
            if (GetMotionID(dtM, MOT_JUMP_LEFT) >= 0)
            {
                SET_MOTION(MOT_JUMP_LEFT, MOTION_MOVE_NONE);
            }
            MoveHumanoid(Me_MOTION_C, 0, 100);
        }
        else
        {
            dtV->vz = 0;
            dtV->vx = 0;
        }
    }
}

/* MOTION.C's original severity-and-direction damage-animation table. */
static inline void RecordPlayerKill(Humanoid *victim)
{
    if ((victim->type & PAGE_MASK) == PAGE_CIVILIAN)
    {
        FriendHits++;
    }
    else if ((victim->attribute & (ATTR_WEAPON_DRAWN | PHASE_ALERT)) == 0)
    {
        Criticals++;
    }
    else
    {
        Murders++;
    }
}

static inline void RecoilAttacker(Humanoid *attacker, short damage,
                                  int rumble_power, int rumble_release)
{
    if (attacker->status == STAT_ATTACK)
    {
        attacker->motion->loop = damage / -3 - 1;
        attacker->vector.vz = 0;
        attacker->vector.vx = 0;
        if (StagePlayer == attacker)
        {
            PadShockAR(PAD_PORT_1, rumble_power, RUMBLE_ATTACK_NORMAL,
                       rumble_release);
        }
    }
}

static inline void RequestDamageFeedback(Humanoid *attacker,
                                         int rumble_release)
{
    Humanoid *who;

    if (StagePlayer == Me_MOTION_C)
    {
        PadShockAR(PAD_PORT_1, RUMBLE_POWER_HALF, RUMBLE_ATTACK_NORMAL,
                   rumble_release);
        who = attacker;
    }
    else
    {
        who = Me_MOTION_C;
    }
    ReqLifeBar(who);
}

static inline void PlayRandomHurtVoice(void)
{
    int random;
    short sound;

    random = rand();
    sound = CHAR_VOICE_HURT_ALT;
    if ((random & 1) != 0)
    {
        sound = CHAR_VOICE_HURT;
    }
    Sound(Me_MOTION_C, sound);
}

static inline void SnapToWaistConflict(void)
{
    int conflict_id;

    conflict_id = Me_MOTION_C->model->object[MODEL_PART_WAIST]->id;
    if (conflict_id >= 0)
    {
        dtL->vx = ConflictObject[conflict_id].position.vx;
        dtL->vz = ConflictObject[conflict_id].position.vz;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DamageControl(void);
 *     MOTION.C:408, 236 src lines, frame 72 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct VECTOR p
 *     reg   $s1       struct VECTOR * pp
 *     reg   $s2       short i
 *     reg   $s0       short id
 *     reg   $s4       short deg
 *     reg   $s1       short dmg
 *     reg   $s5       short did
 *     reg   $s3       struct Humanoid * enemy
 *     reg   $v1       short i
 *     stack sp+32     struct SVECTOR pv
 *     reg   $v1       short i
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short MotionUpdateMode;
 *     extern short motID;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct BattleType BattleDB[78];
 *     extern struct VECTOR *dtL;
 *     extern short motMODE;
 *     extern short Criticals;
 *     extern short Murders;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern short ActionHalt;
 *     extern struct MotionManager *dtM;
 *     extern struct SVECTOR ConflictDistance;
 *     extern struct SVECTOR *dtR;
 *     extern short FriendHits;
 *     extern unsigned long *GlobalAreaMap;
 *     extern unsigned char gNannido;
 *     extern short EngageLevel;
 *     extern struct SVECTOR *dtV;
 * END PSX.SYM */

void DamageControl(void)
{
    static motion_id damagemotion[N_DAMAGE_MOTIONS] = {
        MOT_DAMAGE,
        MOT_DAMAGE_FRONT_MID,
        MOT_DAMAGE_FRONT_HEAVY,
        MOT_DAMAGE_LAUNCH_BACK,
        MOT_DAMAGE_BACK_LIGHT,
        MOT_DAMAGE_BACK_LIGHT,
        MOT_DAMAGE_BACK_HEAVY,
        MOT_DAMAGE_LAUNCH_FORE
    };
    MotionManager *mmp;
    short did;
    short deg;
    short t;
    short newvy;
    int abs_direction;
    Humanoid *enemy;
    int id;
    short dmg;
    SVECTOR dir;
    VECTOR p;
    SVECTOR pv;

    id = (u16)Me_MOTION_C->vector.pad;
    dmg = 0;
    if (Me_MOTION_C->life < 1)
    {
        return;
    }
    if (MotionUpdateMode != 0)
    {
        return;
    }
    if (motID == MOT_SWIM_EXIT)
    {
        return;
    }
    if (Me_MOTION_C == StagePlayer)
    {
        SetCameraMode(CMODE_NORMAL);
    }
    if ((Me_MOTION_C->type & PAGE_MASK) == PAGE_BEAST)
    {
        enemy = ConflictObject[(short)id].common;
        if (enemy != (Humanoid *)CONFLICT_OWNER_ITEM)
        {
            Sound(enemy, CHAR_SE_IMPACT);
            DeleteConflict(ConflictObject[(short)id].model);
            deg = GetAttackDBID(enemy, enemy->motion->mid);
            {
                s16 hp;

                hp = (u16)Me_MOTION_C->life - (u16)BattleDB[deg].power;
                Me_MOTION_C->life = hp;
                if (hp < 0 || (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) == 0)
                {
                    Me_MOTION_C->life = 0;
                }
            }
            p.vx = dtL->vx;
            p.vy = dtL->vy - Me_MOTION_C->height / 2;
            p.vz = dtL->vz;
            SetImpact(&p, 6 * FIXED_ONE, IMPACT_SPRITE_HIT);
            if (StagePlayer == enemy)
            {
                PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX,
                           RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_SHORT);
            }
        }
        else
        {
            Me_MOTION_C->life = 0;
        }
        ReqLifeBar(Me_MOTION_C);
        if (Me_MOTION_C->life != 0)
        {
            SET_MOTION(MOT_DAMAGE, MOTION_MOVE_APPLY);
            Sound(Me_MOTION_C, CHAR_VOICE_HURT);
            reset_alert_duration();
        }
        else
        {
            SET_MOTION(MOT_DEAD, MOTION_MOVE_APPLY);
            if ((Me_MOTION_C->type != NINKEN) &&
                ((StagePlayer == enemy ||
                  enemy == (Humanoid *)CONFLICT_OWNER_ITEM)))
            {
                if ((Me_MOTION_C->attribute & (ATTR_WEAPON_DRAWN | PHASE_ALERT)) == 0)
                {
                    Criticals++;
                }
                else
                {
                    Murders++;
                }
            }
            if ((Me_MOTION_C->attribute & (ATTR_WEAPON_DRAWN | PHASE_ALERT)) != 0)
            {
                Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
                reset_alert_duration();
            }
        }
        SET_NOW_MOTION_UNLESS_CVA(goto attack_cancel);
    attack_cancel:
        AttackCancelControl(ATTACK_CANCEL_ALL);
        return;
    }
    if (motID < MOT_ATTACK_STEALTH_BACK)
    {
        goto resolve_hit;
    }
    if (motID <= MOT_ATTACK_STEALTH_SIDE_AYAME)
    {
        goto attack_break;
    }
    if (motID == MOT_DAMAGE_DOWNED)
    {
        return;
    }
    goto resolve_hit;
/* motID in the stealth-kill band (MOT_ATTACK_STEALTH_*): the hit
 * cancels the move — reset ActionHalt, pick recover/idle, nudge down */
attack_break:
    ActionHalt = ACTION_HALT_NONE;
    SELECT_RETURN_MOTION();
    dtL->vy--;
    return;
resolve_hit:
    dtM->mask = MOTION_MASK_ALL;
    AttackCancelControl(ATTACK_CANCEL_ALL);
    {
        Humanoid *conflict;

        t = id;
        conflict = ConflictObject[t].common;
        if (conflict == (Humanoid *)CONFLICT_OWNER_ITEM)
        {
            if (Me_MOTION_C->status == STAT_DAMAGE)
            {
                return;
            }
            GetConflictResult((ModelType *)Me_MOTION_C->model, t);
            t = GetItemType(t);
            switch (t)
            {
            case ITEM_MAKIBISHI:
                dmg = DMG_MAKIBISHI;
                SET_MOTION(MOT_DAMAGE_MAKIBISHI, MOTION_MOVE_APPLY);
                break;
            case ITEM_SHURIKEN:
                if (dmg == 0)
                {
                    dmg = DMG_SHURIKEN;
                }
                if ((Me_MOTION_C->type == NINJA_0) || (Me_MOTION_C->type == NINJA_1))
                {
                    Me_MOTION_C->item[ITEM_SHURIKEN]++;
                }
                /* fall through: the shared zero-damage test preserves the shuriken's DMG_SHURIKEN */
            case ITEM_HAPPOU:
                if (dmg == 0)
                {
                    dmg = DMG_HAPPOU;
                }
                /* fall through */
            case ITEM_GUN:
                if (dmg == 0)
                {
                    dmg = DMG_GUN;
                }
                /* fall through */
            case ITEM_ARROW:
                if (dmg == 0)
                {
                    dmg = DMG_ARROW;
                }
                p.vx = dtL->vx;
                motID = MOT_DAMAGE;
                p.vy = dtL->vy - Me_MOTION_C->height / 2;
                p.vz = dtL->vz;
                motMODE = MOTION_MOVE_APPLY;
                SetBlood(&p, 5, 90);
                break;
            case ITEM_NAPALM:
                dmg = DMG_NAPALM;
                if ((rand() & 1) == 0)
                {
                    motID = MOT_DAMAGE_BACK_LIGHT;
                }
                else
                {
                    motID = MOT_DAMAGE_FRONT_MID;
                }
                motMODE = MOTION_MOVE_APPLY;
                break;
            case ITEM_FIRE:
            case ITEM_JIRAI:
            case ITEM_LIGHTNINGBOLT:
                dmg = DMG_FIRE;
                if (t == ITEM_JIRAI)
                {
                    dmg = DMG_JIRAI;
                }
                if ((Me_MOTION_C->map.attrib & MAP_WATER) == 0)
                {
                    Me_MOTION_C->map.height = 1;
                }
                break;
            default:
                dmg = 0;
                break;
            }
            if (Me_MOTION_C->map.height > 0)
            {
                int abs_direction;

                did = GetDirection(ConflictDistance.vx, ConflictDistance.vz, dtR->vy);
                abs_direction = did;
                if (abs_direction < 0)
                {
                    abs_direction = -abs_direction;
                }
                if (abs_direction < ANGLE_QUADRANT)
                {
                    SET_MOTION(MOT_DAMAGE_LAUNCH_BACK, MOTION_MOVE_NONE);
                    dtR->vy += did;
                    MoveHumanoid(Me_MOTION_C, -DAMAGE_LAUNCH_SPEED, 0);
                }
                else
                {
                    SET_MOTION(MOT_DAMAGE_LAUNCH_FORE, MOTION_MOVE_NONE);
                    dtR->vy = (ANGLE_HALF + did) + dtR->vy;
                    MoveHumanoid(Me_MOTION_C, DAMAGE_LAUNCH_SPEED, 0);
                }
            }
            if ((Me_MOTION_C == StagePlayer) && (ARMOUR_EQUIPPED_ != 0))
            {
                dmg = (dmg * 7) / 10;
            }
            {
                s16 hp;

                hp = (u16)Me_MOTION_C->life - dmg;
                Me_MOTION_C->life = hp;
                if (hp <= 0)
                {
                    Me_MOTION_C->life = 0;
                    if (motID != MOT_DAMAGE_LAUNCH_BACK && motID != MOT_DAMAGE_LAUNCH_FORE)
                    {
                        SET_MOTION(MOT_DEAD, MOTION_MOVE_APPLY);
                    }
                    Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
                    {
                        TItemType item_type;

                        item_type = GetItemType((s16)id);
                        if ((item_type < ITEM_GUN) ||
                            (item_type > ITEM_ARROW && item_type != ITEM_LIGHTNINGBOLT))
                        {
                            RecordPlayerKill(Me_MOTION_C);
                        }
                    }
                }
                else
                {
                    PlayRandomHurtVoice();
                }
            }
            if (StagePlayer == Me_MOTION_C)
            {
                PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX,
                           RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_MEDIUM);
            }
            else
            {
                ReqLifeBar(Me_MOTION_C);
            }
            reset_alert_duration();
        }
        else
        {
            enemy = conflict;
            if (((Me_MOTION_C->type & PAGE_MASK) == PAGE_BOSS) &&
                (enemy != StagePlayer))
            {
                return;
            }
            t = 1;
            dir.vx = enemy->locate->vx - dtL->vx;
            dir.vy = enemy->locate->vy - dtL->vy;
            dir.vz = enemy->locate->vz - dtL->vz;
            while (__builtin_abs(dir.vx) > 100 || __builtin_abs(dir.vy) > 100 ||
                   __builtin_abs(dir.vz) > 100)
            {
                t = t << 1;
                dir.vx >>= 1;
                dir.vy >>= 1;
                dir.vz >>= 1;
            }
            p = *dtL;
            p.vy -= 1000;
            if (GetAreaMapPassage(GlobalAreaMap, &p, &dir, t) != 0)
            {
                return;
            }
            {
                int abs_direction;

                did = GetDirection(enemy->locate->vx - dtL->vx,
                                   enemy->locate->vz - dtL->vz,
                                   dtR->vy);
                deg = GetAttackDBID(enemy, enemy->motion->mid);
                if (Me_MOTION_C != StagePlayer &&
                    Me_MOTION_C->status != STAT_ATTACK &&
                    (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) != 0 &&
                    Me_MOTION_C->map.height == 0 &&
                    gNannido != DIFFICULTY_EASY)
                {
                    if (rand() % (EngageLevel + 1) == 0)
                    {
                        if (((Me_MOTION_C->type == NINJA_0) ||
                             (Me_MOTION_C->type == NINJA_1)) &&
                            (rand() & 1) != 0)
                        {
                            motID = MOT_CHASE_BACK;
                        }
                    }
                    else
                    {
                        motID = MOT_CHASE_BACK;
                    }
                }
                abs_direction = did;
                if (abs_direction < 0)
                {
                    abs_direction = -abs_direction;
                }
                if (abs_direction < 700)
                {
                    if (motID == MOT_CHASE_BACK)
                    {
                        goto counter_attack;
                    }
                    if (motID == MOT_ENGAGE)
                    {
                        return;
                    }
                }
                if (motID != MOT_DAMAGE_GETUP)
                {
                    goto take_damage;
                }
            counter_attack:
                /* Retail's own redundancy: unreachable here with
                 * MOT_ENGAGE (both entries guard on MOT_CHASE_BACK/MOT_DAMAGE_GETUP), yet
                 * the binary carries the duplicate test — reproduced
                 * faithfully. */
                if (motID == MOT_ENGAGE)
                {
                    return;
                }
                if (UpdateMotion(dtM, MOT_ENGAGE) != 0)
                {
                    /* the blood/impact spawn point */
                    VECTOR *pp;

                    SnapToWaistConflict();
                    mmp = dtM;
                    dtR->vy += did;
                    Me_MOTION_C->status = STAT_ENGAGE;
                    mmp->count = 0;
                    PlayMotion(mmp, 1);
                    dmg = (u16)BattleDB[deg].power;
                    dtM->loop = -dmg - 8;
                    MoveHumanoid(Me_MOTION_C, -((short)((dmg * 5) / 2) + 0x50), 0);
                    RecoilAttacker(enemy, dmg, RUMBLE_POWER_HALF,
                                   RUMBLE_RELEASE_NONE);
                    DeleteConflict(ConflictObject[(short)id].model);
                    pp = GetAbsolutePosition(
                        Me_MOTION_C->model->object[MODEL_PART_HEAD], 0,
                        (short)(dmg * 10 + 100), 0);
                    t = 0;
                    do
                    {
                        pv.vx = rand() % 100 - 50;
                        pv.vy = rand() % 100 - 50;
                        pv.vz = rand() % 100 - 50;
                        SetBleed(pp, &pv, rand() % 20 + 20, COLOR_YELLOW);
                        t++;
                    } while (t < 10);
                    RequestDamageFeedback(enemy, RUMBLE_RELEASE_NONE);
                    {
                        s16 r;

                        r = rand() % 360;
                        set_impact_ex_(pp, 0, 2 * FIXED_ONE, 6 * FIXED_ONE, RGB24(220, 220, 220), 0, r, 6, 9, IMPACT_SPRITE_FLASH);
                    }
                    if ((rand() & 1) != 0)
                    {
                        Sound(Me_MOTION_C, CHAR_VOICE_ACTION_B);
                    }
                    if ((enemy->type & PAGE_MASK) != PAGE_BEAST)
                    {
                        Sound(Me_MOTION_C, CHAR_SE_ATTACK_ALT);
                    }
                    return;
                }
            }
        take_damage:
            {
                SnapToWaistConflict();
            }
            dmg = (u16)BattleDB[deg].power;
            if (enemy != StagePlayer)
            {
                goto npc_attack;
            }
            if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) != 0)
            {
                goto difficulty_bonus;
            }
            Me_MOTION_C->life = 0;
            goto recheck_attacker;
        npc_attack:
            if (Me_MOTION_C != StagePlayer)
            {
                dmg = dmg / 3;
            }
        recheck_attacker:
            if (enemy != StagePlayer)
            {
                goto apply_multipliers;
            }
        difficulty_bonus:
            dmg -= ((u8)gNannido - DIFFICULTY_HARD);
        apply_multipliers:
            if (enemy->type == NINKEN)
            {
                dmg = dmg * 6;
            }
            if (Me_MOTION_C->active_item == ACTIVE_ITEM_PROTECTION)
            {
                dmg = dmg / 3;
            }
            if (enemy->active_item == ACTIVE_ITEM_PROTECTION)
            {
                dmg = (u32)(dmg << 0x10) >> 0xf;
            }
            {
                if ((Me_MOTION_C == StagePlayer) && (ARMOUR_EQUIPPED_ != 0))
                {
                    dmg = (dmg * 7) / 10;
                }
                deg = dmg >> 3;
                if (deg > DAMAGE_MOTION_LAUNCH_TIER)
                {
                    deg = DAMAGE_MOTION_LAUNCH_TIER;
                }
                if (Me_MOTION_C->map.height > 0)
                {
                    deg = DAMAGE_MOTION_LAUNCH_TIER;
                }
                t = dmg * 5 / 2 + 0x50;
                newvy = dtR->vy + did;
                abs_direction = __builtin_abs(did);
                dtR->vy = newvy;
                if (abs_direction < ANGLE_QUADRANT)
                {
                    t = -t;
                }
                else
                {
                    dtR->vy = newvy - ANGLE_HALF;
                }
                if (deg == DAMAGE_MOTION_LAUNCH_TIER)
                {
                    MoveHumanoid(Me_MOTION_C,
                                 (__builtin_abs(did) > ANGLE_QUADRANT)
                                     ? DAMAGE_LAUNCH_SPEED
                                     : -DAMAGE_LAUNCH_SPEED,
                                 0);
                }
                else
                {
                    MoveHumanoid(Me_MOTION_C, t, 0);
                }
            }
            {
                s16 hp;

                hp = (u16)Me_MOTION_C->life - dmg;
                Me_MOTION_C->life = hp;
                if (hp <= 0)
                {
                    Me_MOTION_C->life = 0;
                    DeadHumanoid = Me_MOTION_C;
                    if (deg != DAMAGE_MOTION_LAUNCH_TIER)
                    {
                        SET_MOTION(MOT_DEAD, MOTION_MOVE_APPLY);
                        SET_NOW_MOTION_UNLESS_CVA(goto death_motion_set);
                    death_motion_set:
                        if ((rand() & 1) != 0)
                        {
                            SET_MOTION(MOT_DEAD_ALT, MOTION_MOVE_APPLY);
                        }
                    }
                    else
                    {
                        int abs_direction;

                        abs_direction = did;
                        if (abs_direction < 0)
                        {
                            abs_direction = -abs_direction;
                        }
                        if (abs_direction > ANGLE_QUADRANT)
                        {
                            deg += DAMAGE_MOTION_FROM_BEHIND_OFFSET;
                        }
                        dtM->mid = MOTION_ID_NONE;
                        motID = damagemotion[deg];
                    }
                    if (enemy == StagePlayer)
                    {
                        RecordPlayerKill(Me_MOTION_C);
                    }
                    if ((Me_MOTION_C->attribute & (ATTR_WEAPON_DRAWN | PHASE_ALERT)) != 0)
                    {
                        Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
                        reset_alert_duration();
                    }
                }
                else
                {
                    int abs_direction;

                    abs_direction = did;
                    if (abs_direction < 0)
                    {
                        abs_direction = -abs_direction;
                    }
                    if (abs_direction > ANGLE_QUADRANT)
                    {
                        deg += DAMAGE_MOTION_FROM_BEHIND_OFFSET;
                    }
                    dtM->mid = MOTION_ID_NONE;
                    SET_MOTION(damagemotion[deg], MOTION_MOVE_NONE);
                    reset_alert_duration();
                }
            }
            RecoilAttacker(enemy, dmg, RUMBLE_POWER_MAX,
                           RUMBLE_RELEASE_SHORT);
            DeleteConflict(ConflictObject[(short)id].model);
            p.vx = dtL->vx;
            p.vy = dtL->vy - Me_MOTION_C->height / 2;
            p.vz = dtL->vz;
            SetBlood(&p, 5, 120);
            SetImpact(&p, 6 * FIXED_ONE, IMPACT_SPRITE_HIT);
            RequestDamageFeedback(enemy, RUMBLE_RELEASE_LONG);
            {
                int r;
                short sound_id;

                PlayRandomHurtVoice();
                r = rand();
                sound_id = CHAR_SE_IMPACT;
                if ((r & 1) == 0 && Me_MOTION_C->life == 0)
                {
                    sound_id = CHAR_SE_SPECIAL;
                }
                Sound(enemy, sound_id);
            }
        }
    }
    if ((Me_MOTION_C->life == 0) && (Me_MOTION_C->item[ITEM_KAWARIMI] != 0))
    {
        ReqItemDefault(Me_MOTION_C, ITEM_KAWARIMI);
        Me_MOTION_C->life = Me_MOTION_C->lifemax;
        if (motID != MOT_DAMAGE_LAUNCH_BACK && motID != MOT_DAMAGE_LAUNCH_FORE)
        {
            SET_MOTION(MOT_DAMAGE_FRONT_HEAVY, MOTION_MOVE_APPLY);
        }
    }
    Me_MOTION_C->pad.time = 0;
    if ((dtM->mid == MOT_SWIM) || (dtM->mid == MOT_SWIM_STROKE))
    {
        SVECTOR *v;

        v = dtV;
        v->vz = 0;
        v->vx = 0;
        if (Me_MOTION_C->life != 0)
        {
            motMODE = MOTION_MOVE_UNSET;
            return;
        }
        SET_MOTION(MOT_DEAD_DROWN, MOTION_MOVE_APPLY);
    }
    SET_NOW_MOTION_UNLESS_CVA(return);
    return;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackControl(void);
 *     MOTION.C:648, 74 src lines, frame 48 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct SVECTOR vect
 *     reg   $a1       short emid
 *     reg   $a2       short myid
 *     stack sp+26     short deg
 *     stack sp+24     short mydeg
 *     reg   $s0       struct Humanoid * enemy
 *     reg   $s0       struct Humanoid * human
 *     reg   $a1       short mid
 *     reg   $v0       struct Humanoid * enemy
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern short motID;
 *     extern short motMODE;
 *     extern short Criticals;
 *     extern short dtPAD;
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

void AttackControl(void)
{
    s16 mydeg;
    s16 deg;

    {
        Humanoid *enemy;

        if ((u16)Me_MOTION_C->type < N_PLAYABLE_CHARACTERS)
        {
            enemy = GetNearestHumanoid(Me_MOTION_C, Me_MOTION_C->width + 1000);
            if (enemy != NULL)
            {
                u16 type;
                s32 group;

                /* Bosses, civilians, and beasts do not trade blows; on story page 0,
                 * only armed kerai types 7-9 do. */
                type = enemy->type;
                group = type & PAGE_MASK;
                if (group == PAGE_BOSS)
                    goto reject_enemy;
                if (group > PAGE_BOSS)
                    goto check_high_group;
                if (group == PAGE_PALACE)
                    goto check_low_group;
                goto enemy_type_ok;

            check_high_group:
                if (group == PAGE_CIVILIAN)
                    goto reject_enemy;
                if (group == PAGE_BEAST)
                    goto reject_enemy;
                goto enemy_type_ok;

            check_low_group:
                if ((u16)(type - 7) < 3)
                    goto enemy_type_ok;

            reject_enemy:
                enemy = NULL;

            enemy_type_ok:

                if (enemy != NULL &&
                    (enemy->attribute & (ATTR_WEAPON_DRAWN | ATTR_PHASE)) == 0 &&
                    enemy->status != STAT_ITEM && enemy->status != STAT_ACTION)
                {
                    GsCOORDINATE2 *target;

                    Me_MOTION_C->target = &enemy->model->locate;
                    GetTargetDistance(Me_MOTION_C, &mydeg);
                    target = enemy->target;
                    enemy->target = &StagePlayer->model->locate;
                    GetTargetDistance(enemy, &deg);
                    enemy->target = target;

                    if (dtL->vy == enemy->locate->vy &&
                        Me_MOTION_C->map.vector == 0 &&
                        Me_MOTION_C->map.angleL == 0 &&
                        Me_MOTION_C->map.angleH == 0)
                    {
                        s16 myid;
                        motion_id emid;

                        if (__builtin_abs(deg) > 1000 &&
                            __builtin_abs(mydeg) < 1000)
                        {
                            myid = MOT_ATTACK_STEALTH_BACK;
                            emid = MOT_DEAD_STEALTH_BACK;
                        }
                        else if (__builtin_abs(deg) < 1000 &&
                                 __builtin_abs(mydeg) < 1000)
                        {
                            myid = MOT_ATTACK_STEALTH_FRONT;
                            emid = MOT_DEAD_STEALTH_FRONT;
                        }
                        else
                        {
                            myid = MOT_ATTACK_STEALTH_SIDE;
                            emid = MOT_DEAD_STEALTH_SIDE;
                        }
                        if (Me_MOTION_C->type == AYAME_0)
                        {
                            myid += 3;
                            emid += 3;
                        }

                        enemy->rotate->vy = dtR->vy;
                        enemy->locate->vx = dtL->vx;
                        SET_MOTION(myid, MOTION_MOVE_APPLY);
                        enemy->locate->vz = dtL->vz;
                        enemy->life = 0;
                        if ((enemy->status != STAT_DEAD ||
                             enemy->motion->loop != MOTION_LOOP_DISABLED) &&
                            UpdateMotion(enemy->motion, emid) != 0)
                        {
                            enemy->status = (s8)MOTION_STATUS(emid);
                            MoveHumanoid(enemy, enemy->motion->motion->orderspd,
                                         enemy->motion->motion->sidespd);
                        }
                        DeleteConflict(enemy->model->object[MODEL_PART_WAIST]);
                        Criticals++;
                        return;
                    }
                }
            }
        }
    }

    if (dtPAD & PADLdown)
    {
        if (GetMotionID(dtM, MOT_ATTACK_BACK) < 0)
        {
            return;
        }
        SET_MOTION(MOT_ATTACK_BACK, MOTION_MOVE_APPLY);
    }
    else if (motID == MOT_SQUAT)
    {
        if (GetMotionID(dtM, MOT_ATTACK_CROUCH) < 0)
        {
            return;
        }
        SET_MOTION(MOT_ATTACK_CROUCH, MOTION_MOVE_APPLY);
    }
    else if (motID == MOT_CHASE_DASH_FWD)
    {
        if (dtM->count > 10)
        {
            return;
        }
        SET_MOTION(MOT_ATTACK, MOTION_MOVE_APPLY);
        if (GetMotionID(dtM, MOT_ATTACK_LUNGE) >= 0)
        {
            SET_MOTION(MOT_ATTACK_LUNGE, MOTION_MOVE_APPLY);
        }
    }
    else
    {
        if (dtPAD & PADLup)
        {
            motID = MOT_ATTACK;
        }
        else if (dtPAD & PADLright)
        {
            motID = MOT_ATTACK_RIGHT1;
        }
        else if (dtPAD & PADLleft)
        {
            motID = MOT_ATTACK_LEFT1;
        }
        else
        {
            motID = MOT_ATTACK;
        }
        motMODE = MOTION_MOVE_APPLY;
    }

    if (Me_MOTION_C == StagePlayer)
    {
        Humanoid *enemy;
        Humanoid *human;

        enemy = GetNearestHumanoid(Me_MOTION_C, 3000);
        human = Me_MOTION_C;
        if (enemy != NULL)
        {
            human->target = &enemy->model->locate;
        }
        else
        {
            human->target = NULL;
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short AttackContinuousCheck(struct BattleType *battle);
 *     MOTION.C:755, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct BattleType * battle
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

s16 AttackContinuousCheck(BattleType *battle)
{
    s16 wk;
    ModelType *model;
    s16 mode;

    if (dtM->count < battle->contfrm - 3)
    {
        return 0;
    }
    if (battle->contfrm + 3 < dtM->count)
    {
        return 0;
    }
    Me_MOTION_C->pad.time = 0;
    wk = Me_MOTION_C->wpatk;
    switch (wk)
    {
    case FIST:
        DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_0]);
        model = Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_1];
        break;
    case JAW:
        model = Me_MOTION_C->model->object[MODEL_PART_BEAST_HAND_0];
        break;
    case NO_WEAPON:
        mode = ATTACK_CANCEL_ALL;
        goto no_conflict;
    default:
        DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0]);
        model = Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1];
        break;
    }
    DeleteConflict(model);
    mode = ATTACK_CANCEL_ALL;
no_conflict:
    if ((mode & ATTACK_CANCEL_AFTERIMAGES) != 0)
    {
        if (Me_MOTION_C->illusion[WEAPON_HAND_0] != 0)
        {
            DisposeAfterimage(Me_MOTION_C->illusion[WEAPON_HAND_0]);
            Me_MOTION_C->illusion[WEAPON_HAND_0] = 0;
        }
        if (Me_MOTION_C->illusion[WEAPON_HAND_1] != 0)
        {
            DisposeAfterimage(Me_MOTION_C->illusion[WEAPON_HAND_1]);
            Me_MOTION_C->illusion[WEAPON_HAND_1] = 0;
        }
    }
    dtM->mask = MOTION_MASK_ALL;
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void WeaponHitWeapon(struct ModelType *hand);
 *     MOTION.C:771, 25 src lines, frame 56 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct ModelType * hand
 *     reg   $s2       struct VECTOR * p
 *     reg   $s4       short id
 *     reg   $s0       short i
 *     stack sp+16     struct SVECTOR pv
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct BattleType BattleDB[78];
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

void WeaponHitWeapon(ModelType *hand)
{
    SVECTOR pv;
    short i;
    short id;
    VECTOR *p;

    if ((hand->attribute & MODEL_ATTR_CONFLICT) != 0)
    {
        do
        {
            id = GetConflictResult(hand, CONFLICT_NONE);
            if (id < 0)
            {
                return;
            }
            if ((ConflictObject[id].size.pad &
                 CONFLICT_HIT) == 0)
            {
                continue;
            }
            if (ConflictObject[id].common == Me_MOTION_C)
            {
                continue;
            }

            MoveHumanoid(Me_MOTION_C, -30, 0);
            if (ConflictObject[id].common != (void *)CONFLICT_OWNER_ITEM)
            {
                MoveHumanoid(ConflictObject[id].common, -30, 0);
            }

            p = &ConflictObject[hand->id].position;
            for (i = 0; i < 10; i++)
            {
                pv.vx = rand() % 100 - 50;
                pv.vy = rand() % 100 - 50;
                pv.vz = rand() % 100 - 50;
                SetBleed(p, &pv, rand() % 20 + 20, RGB24(0, 127, 255));
            }

            hand->attribute = hand->attribute & ~MODEL_ATTR_COLLIDE;
            /* Retail indexes BattleDB with the conflict-pool slot, not either
             * fighter's attack id. */
            dtM->loop = BattleDB[id].power / -3 - 1;
            Sound(Me_MOTION_C, SE_WEAPON_CLASH);
            if (StagePlayer == Me_MOTION_C)
            {
                PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_NONE);
            }
            break;
        } while (1);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackBowControl(void);
 *     MOTION.C:800, 28 src lines, frame 72 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+56     struct SVECTOR vect
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

static inline const struct BowTimingEntry *BowTimingFromByteOffset(s32 byte_offset)
{
    return (const struct BowTimingEntry *)((const u8 *)BowTiming + byte_offset);
}

void AttackBowControl(s16 timing_window)
{
    s16 count;
    VECTOR *pos;
    PARAM_ITEM_LAUNCH item; /* Unused local recorded by PSX.SYM. */
    SVECTOR vect;           /* Unused local recorded by PSX.SYM. */
    s32 byte_offset;
    const struct BowTimingEntry *p;
    s32 byte_offset2;
    const struct BowTimingEntry *p2;

    count = dtM->count;
    if (count == 1)
    {
        Sound(Me_MOTION_C, CHAR_SE_ATTACK);
    }
    else
    {
        byte_offset = timing_window << 2;
        p = BowTimingFromByteOffset(byte_offset);
        if (p->min <= count && count < p->max)
        {
            UpdateOrnament(Me_MOTION_C->weapon[WEAPON_SLOT_INACTIVE_0], 0);
            DrawOrnament(Me_MOTION_C->weapon[WEAPON_SLOT_INACTIVE_0]);
        }
    }
    byte_offset2 = timing_window;
    byte_offset2 = (s16)byte_offset2 << 2;
    p2 = BowTimingFromByteOffset(byte_offset2);
    if (dtM->count == p2->max)
    {
        pos = GetAbsolutePosition(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0], 0, 0, 0);
        bow_shoot_logic(ITEM_ARROW, pos);
        Sound(Me_MOTION_C, CHAR_SE_ATTACK_ALT);
    }
}

void launch_lightning_bolt_(s16 frame)
{
    VECTOR *start_pos;
    PARAM_ITEM_LAUNCH p;
    SVECTOR move;

    if (dtM->count == frame)
    {
        Sound(Me_MOTION_C, CHAR_SE_SPECIAL);
        p.type = ITEM_LIGHTNINGBOLT;
        p.user = Me_MOTION_C;
        start_pos = GetAbsolutePosition(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0], 0, 0, -700);
        p.start.vx = start_pos->vx;
        p.start.vy = start_pos->vy;
        p.start.vz = start_pos->vz;
        GetMoveSpeed(&move, dtR->vy, ((rand() % 5) * 1000 + 4000), 0);
        p.end.vx = start_pos->vx + move.vx;
        p.end.vy = Me_MOTION_C->target->coord.t[1];
        p.end.vz = start_pos->vz + move.vz;
        ReqItemUse(&p);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActNORMAL(void);
 *     MOTION.C:916, 43 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct SVECTOR *dtR;
 *     extern short dtCMD;
 *     extern short SelectedItem;
 * END PSX.SYM */

void ActNORMAL(void)
{
    motion_id mid;

    mid = dtM->mid;
    switch (mid)
    {
    case MOT_NORMAL:
        if ((dtPAD & PADL2) && StagePlayer != Me_MOTION_C)
        {
            if (dtPAD & PADLdown)
            {
                SET_MOTION(MOT_ACTION_GESTURE, MOTION_MOVE_APPLY);
                return;
            }
            if (dtPAD & PADLright)
            {
                SET_MOTION(MOT_ACTION_LOOP, MOTION_MOVE_APPLY);
                return;
            }
            if (dtPAD & PADLleft)
            {
                SET_MOTION(MOT_ACTION_NOTICE, MOTION_MOVE_APPLY);
                return;
            }
            if (dtPAD & PADLup)
            {
                SET_MOTION(MOT_ACTION, MOTION_MOVE_APPLY);
            }
            return;
        }
        if (dtPAD & PADLright)
        {
            SET_MOTION(MOT_NORMAL_TURN_R, MOTION_MOVE_NONE);
            break;
        }
        if (dtPAD & PADLleft)
        {
            SET_MOTION(MOT_NORMAL_TURN_L, MOTION_MOVE_NONE);
            break;
        }
        if (dtM->count == 0 && rand() % 100 == 0)
        {
            motMODE = MOTION_MOVE_APPLY;
            motID = (rand() & 1) ? MOT_ACTION_FIDGET_A : MOT_ACTION_FIDGET_B;
        }
        break;

    case MOT_NORMAL_TURN_R:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_TURN_STEP);
        if (dtPAD & PADLright)
        {
            dtR->vy += Me_MOTION_C->turn;
        }
        else
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        }
        break;

    case MOT_NORMAL_TURN_L:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_TURN_STEP);
        if ((dtPAD & PADLleft) == 0)
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        }
        else
        {
            dtR->vy -= Me_MOTION_C->turn;
        }
        break;

    default:
        break;
    }
    if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
    {
        SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        return;
    }

    {
        pad_command command;

        command = dtCMD;
        if (command == CMD_NONE)
            goto command_0;
        if (command == CMD_DASH_BACKWARD)
            goto command_2;
        if (command < CMD_DASH_LEFT)
        {
            if (command == CMD_DASH_FORWARD)
                goto command_1;
            return;
        }
        if (command == CMD_DASH_LEFT)
            goto command_3;
        if (command == CMD_DASH_RIGHT)
            goto command_4;
        return;

    command_1:
        SET_MOTION(MOT_MOVE_DASH_FWD, MOTION_MOVE_APPLY);
        return;

    command_2:
        SET_MOTION(MOT_MOVE_DASH_BACK, MOTION_MOVE_APPLY);
        return;

    command_3:
        SET_MOTION(MOT_MOVE_DASH_LEFT, MOTION_MOVE_APPLY);
        return;

    command_0:
    {
        u16 trig;

        trig = Me_MOTION_C->pad.trig;
        if (trig & PADRdown)
        {
            JumpControl();
            return;
        }
        if (trig & PADRup)
        {
            SELECT_ITEM_USE_MOTION(item_sound, item_default);
            motMODE = MOTION_MOVE_APPLY;
            return;

        item_sound:
            SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
            return;

        item_default:
            ReqItemDefault(Me_MOTION_C,
                           SelectedItem);
            return;
        }
        if (dtPAD & PADRright)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            return;
        }
        if (dtPAD & PADLup)
        {
            SET_MOTION(MOT_MOVE, MOTION_MOVE_APPLY);
            return;
        }
        if (dtPAD & PADLdown)
        {
            SET_MOTION(MOT_MOVE_BACK, MOTION_MOVE_APPLY);
            return;
        }
        if (trig & PADRleft)
        {
            SET_MOTION(MOT_STATE_DRAW, MOTION_MOVE_APPLY);
        }
        return;
    }

    command_4:
        SET_MOTION(MOT_MOVE_DASH_RIGHT, MOTION_MOVE_APPLY);
        return;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActACTION(void);
 *     MOTION.C:963, 27 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v1       short i
 * END PSX.SYM */

void ActACTION(void)
{
    switch (dtM->mid)
    {
    case MOT_ACTION_LOOP:
        if (dtM->loop == 0)
            return;
        if (dtPAD == 0)
            return;
        SELECT_RETURN_MOTION();
        return;

    case MOT_ACTION_FIDGET_A:
    case MOT_ACTION_FIDGET_B:
        if (Me_MOTION_C->life != Me_MOTION_C->lifemax ||
            (Me_MOTION_C->attribute & PHASE_SUSPICIOUS))
        {
            SELECT_RETURN_MOTION();
        }
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, CHAR_VOICE_IDLE);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            SELECT_RETURN_MOTION();
        }
        break;

    case MOT_ACTION:
        if (dtM->count == 1)
        {
            s16 cleanup_guard;
            MotionManager *motion;
            Humanoid *human;
            OrnamentType **weapon;

            switch (Me_MOTION_C->wpatk)
            {
            case FIST:
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_0]);
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_1]);
                break;
            case JAW:
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_BEAST_HAND_0]);
                break;
            case NO_WEAPON:
                cleanup_guard = ATTACK_CANCEL_ALL;
                goto skip_afterimage_cleanup;
            default:
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0]);
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1]);
                break;
            }
            cleanup_guard = ATTACK_CANCEL_ALL;
        skip_afterimage_cleanup:
            if (cleanup_guard & ATTACK_CANCEL_AFTERIMAGES)
            {
                DISPOSE_WEAPON_AFTERIMAGE(Me_MOTION_C, WEAPON_HAND_0);
                DISPOSE_WEAPON_AFTERIMAGE(Me_MOTION_C, WEAPON_HAND_1);
            }
            motion = dtM;
            human = Me_MOTION_C;
            motion->mask = MOTION_MASK_ALL;
            /* Empty loop retained for code layout; its original source construct is unknown. */
            do
            {
            } while (0);
            weapon = human->weapon;
            if (human->wpatk == KATANAL && weapon[WEAPON_SLOT_INACTIVE_1] != 0)
            {
                weapon[WEAPON_SLOT_INACTIVE_0] = human->weapon[WEAPON_SLOT_ACTIVE_0];
                human->weapon[WEAPON_SLOT_ACTIVE_0] = weapon[WEAPON_SLOT_INACTIVE_1];
                weapon[WEAPON_SLOT_INACTIVE_1] = 0;
                Sound(human, CHAR_SE_WEAPON_CHANGE_B);
            }
        }
        {
            if (dtM->count == 0 && dtM->loop > 0)
            {
                dtM->count = dtM->motion->time - 1;
                PlayMotion(dtM, 1);
                dtM->loop = MOTION_LOOP_DISABLED;
                dtV->vz = 0;
                dtV->vx = 0;
            }
        }
        if (dtM->loop == MOTION_LOOP_DISABLED && dtPAD != 0)
        {
            SET_MOTION(MOT_DAMAGE_GETUP, MOTION_MOVE_APPLY);
            SET_NOW_MOTION_UNLESS_CVA(goto motion_ready);
        motion_ready:
            dtM->count = -0xf;
        }
        break;

    case MOT_ACTION_NOTICE:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, CHAR_VOICE_HURT);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_STATE_DRAW, MOTION_MOVE_APPLY);
        }
        break;

    case MOT_ACTION_GESTURE:
    case MOT_ACTION_VARIANT_3:
    default:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        SELECT_RETURN_MOTION();
        return;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActMOVE(void);
 *     MOTION.C:994, 32 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern short SelectedItem;
 * END PSX.SYM */

void ActMOVE(void)
{
    switch (dtM->mid)
    {
    case MOT_MOVE:
        if (dtM->count == 1 ||
            dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLup) == 0)
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
            break;
        }
        if (Me_MOTION_C->attribute & ATTR_WALL)
        {
            long y;
            long height;

            y = dtL->vy;
            height = Me_MOTION_C->map.height;
            dtL->vy -= LEDGE_PROBE_RISE;
            Me_MOTION_C->map.height = 1;
            if (HangCheck() == 0)
            {
                dtL->vy = y;
                Me_MOTION_C->map.height = height;
            }
            break;
        }
        if (dtPAD & (PADLleft | PADLright))
        {
            int current;
            int result;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & PADLright)
                result = current + Me_MOTION_C->turn;
            else
                result = current - Me_MOTION_C->turn;
            rotation->vy = result;
            MoveHumanoid(Me_MOTION_C,
                         Me_MOTION_C->motion->motion->orderspd,
                         Me_MOTION_C->motion->motion->sidespd);
        }
        break;

    case MOT_MOVE_BACK:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLdown) == 0)
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        }
        {
            int current;
            int result;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & PADLright)
                result = current + Me_MOTION_C->turn;
            else
                result = current - Me_MOTION_C->turn;
            rotation->vy = result;
            MoveHumanoid(Me_MOTION_C,
                         Me_MOTION_C->motion->motion->orderspd,
                         Me_MOTION_C->motion->motion->sidespd);
        }
        break;

    case MOT_MOVE_DASH_FWD:
    case MOT_MOVE_DASH_BACK:
    case MOT_MOVE_DASH_RIGHT:
    case MOT_MOVE_DASH_LEFT:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_ACROBATIC_MOVE);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        }
        break;

    default:
        break;
    }
    {
        if (Me_MOTION_C->pad.trig & PADRdown)
        {
            JumpControl();
            return;
        }
        if (Me_MOTION_C->pad.trig & PADRup)
        {
            SELECT_ITEM_USE_MOTION(item_sound, item_default);
            motMODE = MOTION_MOVE_APPLY;
            return;

        item_sound:
            SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
            return;

        item_default:
            ReqItemDefault(Me_MOTION_C, SelectedItem);
            return;
        }
        if (dtPAD & PADRright)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            return;
        }
        if (Me_MOTION_C->pad.trig & PADRleft)
        {
            SET_MOTION(MOT_STATE_DRAW, MOTION_MOVE_APPLY);
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSWIM(void);
 *     MOTION.C:1030, 57 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a2       struct ModelArchiveType * mdl
 *     reg   $a1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern short motMODE;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern struct SVECTOR *dtV;
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 *     extern short SelectedItem;
 * END PSX.SYM */

enum swim_steering_direction
{
    SWIM_STEER_FORWARD = 1,
    SWIM_STEER_REVERSE = -1
};

static inline void turn_swimmer(enum swim_steering_direction direction)
{
    s32 yaw;
    s32 turned_yaw;
    SVECTOR *rotation;

    rotation = dtR;
    yaw = rotation->vy;
    if ((dtPAD & PADLright) != 0)
        turned_yaw = yaw + direction * Me_MOTION_C->turn;
    else
        turned_yaw = yaw - direction * Me_MOTION_C->turn;
    rotation->vy = turned_yaw;
}

void ActSWIM(void)
{
    enum
    {
        SWIM_EXIT_MOVE_FRAME = 40,
        SWIM_EXIT_SPEED = 100
    };
    motion_id current_motion;
    int movement_speed;

    current_motion = dtM->mid;
    switch (current_motion)
    {
    case MOT_SWIM:
        if (SwimCheck() == 0)
        {
            SET_MOTION(MOT_SWIM_EXIT, MOTION_MOVE_APPLY);
            break;
        }
        if ((dtPAD & (PADLleft | PADLright)) != 0)
        {
            if (dtM->count == 1)
                Sound(Me_MOTION_C, SE_WATER_MOVE);
            turn_swimmer(SWIM_STEER_FORWARD);
            break;
        }
        if ((dtPAD & (PADLdown | PADLup)) == 0)
            break;
        SET_MOTION(MOT_SWIM_STROKE, MOTION_MOVE_NONE);
        movement_speed = SWIM_SPEED;
        if (dtPAD & PADLup)
        {
            MoveHumanoid(Me_MOTION_C, movement_speed, 0);
            break;
        }
        {
            Humanoid *backward_swimmer;

            movement_speed = -SWIM_SPEED;
            backward_swimmer = Me_MOTION_C;
            if (backward_swimmer->map.angleH != 0)
                break;
            MoveHumanoid(backward_swimmer, movement_speed, 0);
            break;
        }

    case MOT_SWIM_STROKE:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_WATER_MOVE);
        if (dtPAD & PADLup)
        {
            Humanoid *forward_swimmer;

            if (SwimCheck() == 0)
            {
                SET_MOTION(MOT_SWIM_EXIT, MOTION_MOVE_APPLY);
                break;
            }
            if ((dtPAD & (PADLleft | PADLright)) != 0)
                turn_swimmer(SWIM_STEER_FORWARD);
            movement_speed = SWIM_SPEED;
            forward_swimmer = Me_MOTION_C;
            MoveHumanoid(forward_swimmer, movement_speed, 0);
            break;
        }
        else if (dtPAD & PADLdown)
        {
            if (Me_MOTION_C->map.angleH != 0 || SwimCheck() == 0)
            {
                SVECTOR *blocked_velocity;
                VECTOR *position;

                blocked_velocity = dtV;
                position = dtL;
                position->vx -= blocked_velocity->vx;
                position->vz -= blocked_velocity->vz;
                blocked_velocity->vz = 0;
                blocked_velocity->vx = 0;
                break;
            }
            if ((dtPAD & (PADLleft | PADLright)) != 0)
                turn_swimmer(SWIM_STEER_REVERSE);
            movement_speed = -SWIM_SPEED;
            MoveHumanoid(Me_MOTION_C, movement_speed, 0);
            break;
        }
        else
        {
            SET_MOTION(MOT_SWIM, MOTION_MOVE_APPLY);
            break;
        }

    case MOT_SWIM_EXIT:
        if (dtM->count == 1)
        {
            ShowHumanoidBodyParts(Me_MOTION_C);
            Sound(Me_MOTION_C, SE_WATER_MOVE);
            return;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
                SetCameraMode(CMODE_NORMAL);
            if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
            {
                SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
                return;
            }
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
            return;
        }
        if (dtM->count <= SWIM_EXIT_MOVE_FRAME)
            return;
        MoveHumanoid(Me_MOTION_C, SWIM_EXIT_SPEED, 0);
        return;

    default:
        break;
    }

    {
        Humanoid *item_user;

        item_user = Me_MOTION_C;
        if ((item_user->pad.trig & PADRup) == 0)
            return;
        /* Every arm of the switch below except ITEM_KAGINAWA is dead past this
         * guard — retail's own code, kept as-is. */
        if (SelectedItem != ITEM_KAGINAWA)
            return;
        dtM->mask = MOTION_MASK_NOROOT;

        ShowHumanoidBodyParts(item_user);

        SELECT_ITEM_USE_MOTION(item_sound, item_default);
        motMODE = MOTION_MOVE_APPLY;
        return;

    item_sound:
        SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
        return;

    item_default:
        ReqItemDefault(Me_MOTION_C, SelectedItem);
        return;
    }
}

/* Per-axis limit on the grapple step: the approach is finished once every
 * axis is inside it, and the wire vector is halved until it fits. */
#define KAGI_STEP_MAX 400

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActKAGI(void);
 *     MOTION.C:1091, 86 src lines, frame 88 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct VECTOR v
 *     reg   $s0       long dist
 *     stack sp+40     struct PARAM_ITEM_LAUNCH item
 *     reg   $v1       short ry
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct VECTOR *dtL;
 *     extern struct TCameraStatus CamState;
 *     extern short motMODE;
 *     extern short motID;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct SVECTOR *dtR;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct SVECTOR *dtV;
 * END PSX.SYM */

void ActKAGI(void)
{
    VECTOR v;
    long dist;
    PARAM_ITEM_LAUNCH item;
    short ry;
    short i;

    switch (dtM->mid)
    {
    case MOT_KAGI:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }

        if (dtM->count == 1)
        {
            item.type = ITEM_KAGINAWA;
            item.user = Me_MOTION_C;
            item.start.vx = dtL->vx;
            item.start.vy = dtL->vy - Me_MOTION_C->height + 300;
            item.start.vz = dtL->vz;
            item.end = item.start;
            ReqItemUse(&item);
            Sound(Me_MOTION_C, SE_THROW_WEAPON);
        }
        else if (spare_item_slot_(SPARE_ITEM_SLOT_QUERY, Me_MOTION_C) == 0)
        {
            VECTOR *target;
            VECTOR *locate;
            s32 dx;
            s32 dz;

            motID = MOT_KAGI_FLY;
            locate = dtL;
            target = &CamState.TargetVector;
            dx = target->vx - locate->vx;
            v.vx = dx;
            motMODE = MOTION_MOVE_APPLY;
            dz = target->vz - locate->vz;
            v.vz = dz;
            if (dx == 0 && dz == 0)
            {
                SELECT_RETURN_MOTION();
            }
            else
            {
                ry = GetDirection(v.vx, v.vz, dtR->vy);
                dtR->vy += ry;
                Sound(Me_MOTION_C, SE_WEAPON_RECOVER);
            }
        }
        else if (Me_MOTION_C->pad.trig & (PADRleft | PADRdown | PADRright))
        {
            spare_item_slot_(SPARE_ITEM_SLOT_CLEAR, Me_MOTION_C);
            SELECT_RETURN_MOTION();
        }

        if ((Me_MOTION_C->map.attrib & MAP_WATER) &&
            (MOTION_STATUS(motID) != STAT_KAGI))
        {
            ModelArchiveType *model;

            model = Me_MOTION_C->model;
            if (model->n > MODEL_PART_BODY_LAST)
            {
                ry = MODEL_PART_BODY_LAST;
            }
            else
            {
                ry = model->n - 1;
            }
            HIDE_HUMANOID_BODY_PARTS(model, ry, i);
            SET_MOTION(MOT_SWIM, MOTION_MOVE_APPLY);
            dtM->mask = MOTION_MASK_ALL;
        }
        break;

    case MOT_KAGI_FLY:
    {
        MotionManager *mmp;
        Humanoid *human;
        u16 attrib;

        if (dtM->count == 0 && dtM->loop == 1)
        {
            VECTOR *p;

            p = GetAbsolutePosition(
                Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1], 0, 0, 0);
            dtM->loop = MOTION_LOOP_DISABLED;
            dtM->count = SetFlyWire(p, &CamState.TargetVector);
        }
        mmp = dtM;
        if (mmp->loop != MOTION_LOOP_DISABLED)
        {
            return;
        }
        if (--mmp->count >= 0)
        {
            return;
        }
        human = Me_MOTION_C;
        motID = MOT_KAGI_PULL;
        mmp->mask = MOTION_MASK_NOROOT;
        attrib = human->map.attrib;
        motMODE = MOTION_MOVE_APPLY;
        if (attrib & MAP_WATER)
        {
            Sound(human, SE_WATER_MOVE);
        }
        Sound(Me_MOTION_C, SE_GRAPPLE_PULL);
        break;
    }

    case MOT_KAGI_PULL:
        SetCameraMode(CMODE_AIM);
        v.vx = CamState.TargetVector.vx - dtL->vx;
        v.vy = CamState.TargetVector.vy - dtL->vy;
        v.vz = CamState.TargetVector.vz - dtL->vz;
        dist = SquareRoot0(v.vx * v.vx + v.vz * v.vz);
        ry = GetDirection(v.vx, v.vz, dtR->vy);
        Me_MOTION_C->model->object[MODEL_PART_WAIST]->rotate.vy = ry;
        Me_MOTION_C->model->object[MODEL_PART_WAIST]->rotate.vx = ratan2(dist, -v.vy);
        UpdateCoordinate(Me_MOTION_C->model->object[MODEL_PART_WAIST]);

        if (__builtin_abs(v.vx) < KAGI_STEP_MAX && __builtin_abs(v.vy) < KAGI_STEP_MAX &&
            __builtin_abs(v.vz) < KAGI_STEP_MAX)
        {
            Me_MOTION_C->attribute |= ATTR_WALL;
        }

        {
            Humanoid *human;
            SVECTOR *rotation;
            ModelType *root;
            ModelType *adjust_root;
            short old_ry;
            u16 sum;
            u32 quantized;

            human = Me_MOTION_C;
            if (human->attribute &
                (ATTR_PUSH | ATTR_HIT | ATTR_NOFLOOR | ATTR_WALL | ATTR_BUOYANT))
            {
                root = human->model->object[MODEL_PART_WAIST];
                rotation = dtR;
                old_ry = rotation->vy;
                sum = old_ry + root->rotate.vy;
                quantized = sum & ANGLE_QUADRANT_MASK;
                rotation->vy = sum;
                if (sum & ANGLE_HALF_QUADRANT)
                {
                    quantized += ANGLE_QUADRANT;
                }
                rotation->vy = quantized;
                /* Retail writes vy before immediately replacing it with the quantized value. */
                adjust_root = human->model->object[MODEL_PART_WAIST];
                motID = MOT_STATE_FALL;
                adjust_root->rotate.vy += old_ry - quantized;
                motMODE = MOTION_MOVE_NONE;
                dtM->mask = MOTION_MASK_ALL;
                SET_NOW_MOTION_UNLESS_CVA(goto motion_active);

            motion_active:
                dtM->count >>= 1;
                if (Me_MOTION_C->map.vector != MAP_PROBE_ALL)
                {
                    dtV->vz = 0;
                    dtV->vx = 0;
                }
                else
                {
                    dtV->vx >>= 1;
                    dtV->vz >>= 1;
                }
                dtV->vy = 0;
                return;
            }
        }

        while (__builtin_abs(v.vx) > KAGI_STEP_MAX || __builtin_abs(v.vy) > KAGI_STEP_MAX ||
               __builtin_abs(v.vz) > KAGI_STEP_MAX)
        {
            v.vx >>= 1;
            v.vy >>= 1;
            v.vz >>= 1;
        }
        setVector(dtV, v.vx, v.vy, v.vz);
        SetWire(GetAbsolutePosition(
                    Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1], 0, 0, 0),
                &CamState.TargetVector, 0, FIXED_ONE);
        break;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActENGAGE(void);
 *     MOTION.C:1181, 56 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short motID;
 *     extern short motMODE;
 *     extern short dtCMD;
 *     extern short ActionHalt;
 *     extern struct SVECTOR *dtR;
 *     extern struct SVECTOR *dtV;
 *     extern struct Humanoid *StagePlayer;
 *     extern long GameClock;
 *     extern struct VECTOR *dtL;
 *     extern short SelectedItem;
 * END PSX.SYM */

void ActENGAGE(void)
{
    short registered_id;
    short trig;

    switch (dtM->mid)
    {
    case MOT_ENGAGE_STANCE:
    {
        if (dtPAD & PADLright)
        {
            SET_MOTION(MOT_ENGAGE_TURN_R, MOTION_MOVE_NONE);
        }
        else if (dtPAD & PADLleft)
        {
            SET_MOTION(MOT_ENGAGE_TURN_L, MOTION_MOVE_NONE);
        }
        else if (dtCMD == CMD_LUNGE_BACK)
        {
            SET_MOTION(MOT_ATTACK_LUNGE_BACK, MOTION_MOVE_APPLY);
        }
        else if (dtCMD == CMD_FLIP)
        {
            SET_MOTION(MOT_JUMP_FLIP, MOTION_MOVE_NONE);
            MoveHumanoid(Me_MOTION_C, CHASE_WALK_SPEED, 0);
        }
        else if (dtM->count == 0 && rand() % 20 == 0)
        {
            SET_MOTION(MOT_ATTACK_TAUNT, MOTION_MOVE_APPLY);
        }
        if (ActionHalt == ACTION_HALT_STAGE_END && dtM->count == 0)
        {
            registered_id = GetMotionID(dtM, MOT_ENGAGE_SHEATHE);
            if (registered_id < 0)
            {
                SET_MOTION(MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            }
            else
            {
                SET_MOTION(MOT_ENGAGE_SHEATHE, MOTION_MOVE_APPLY);
            }
        }
        break;
    }

    case MOT_ENGAGE_TURN_R:
        dtR->vy += Me_MOTION_C->turn;
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_TURN_STEP);
        if ((dtPAD & PADLright) == 0)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        }
        break;

    case MOT_ENGAGE_TURN_L:
        dtR->vy -= Me_MOTION_C->turn;
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_TURN_STEP);
        if ((dtPAD & PADLleft) == 0)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        }
        break;

    case MOT_ENGAGE:
    {
        SVECTOR *velocity;
        int value;
        short count;

        velocity = dtV;
        value = velocity->vx;
        if (value != 0)
        {
            if (value > 0)
                value -= 4;
            else
                value += 4;
            velocity->vx = value;
        }
        velocity = dtV;
        value = velocity->vz;
        if (value != 0)
        {
            if (value > 0)
                value -= 4;
            else
                value += 4;
            velocity->vz = value;
        }
        count = --dtM->count;
        if (count < dtM->loop)
        {
            if ((dtPAD & PADLdown) != 0)
            {
                SET_MOTION(MOT_CHASE_BACK, MOTION_MOVE_APPLY);
            }
            else
            {
                SELECT_RETURN_MOTION();
            }
        }
        if ((GameClock & 3) != 0)
            return;
        spawn_smoke_burst_(dtL, 300, 10, 5);
        return;
    }

    case MOT_ENGAGE_SHEATHE:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        SET_MOTION(MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
        return;

    case MOT_ENGAGE_VARIANT_2:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        return;
    }

    if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) == 0)
    {
        SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        return;
    }
    else if (dtCMD != CMD_NONE)
    {
        switch (dtCMD)
        {
        case CMD_DASH_FORWARD:
            SET_MOTION(MOT_CHASE_DASH_FWD, MOTION_MOVE_APPLY);
            return;
        case CMD_LUNGE:
            SET_MOTION(MOT_ATTACK_LUNGE, MOTION_MOVE_APPLY);
            return;
        case CMD_DASH_BACKWARD:
            SET_MOTION(MOT_CHASE_DASH_BACK, MOTION_MOVE_APPLY);
            return;
        case CMD_DASH_RIGHT:
            SET_MOTION(MOT_CHASE_DASH_RIGHT, MOTION_MOVE_APPLY);
            return;
        case CMD_DASH_LEFT:
            SET_MOTION(MOT_CHASE_DASH_LEFT, MOTION_MOVE_APPLY);
            return;
        default:
            return;
        }
    }
    else
    {
        trig = Me_MOTION_C->pad.trig;
        if (trig & PADRdown)
        {
            JumpControl();
            return;
        }
        if (trig & PADRup)
        {
            switch (SelectedItem)
            {
            case ITEM_SHURIKEN:
                SET_MOTION(MOT_SYURI, MOTION_MOVE_APPLY);
                return;
            case ITEM_KAGINAWA:
                SET_MOTION(MOT_KAGI, MOTION_MOVE_APPLY);
                return;
            case ITEM_MAKIBISHI:
                SET_MOTION(MOT_ITEM, MOTION_MOVE_APPLY);
                return;
            case ITEM_SMOKE:
                SET_MOTION(MOT_ITEM_THROW, MOTION_MOVE_APPLY);
                return;
            case ITEM_FIRE:
                SET_MOTION(MOT_ITEM_THROW, MOTION_MOVE_APPLY);
                return;
            case ITEM_JIRAI:
                SET_MOTION(MOT_ITEM_PLANT, MOTION_MOVE_APPLY);
                return;
            case ITEM_NONE:
            case ITEM_KAWARIMI:
                SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
                return;
            default:
                ReqItemDefault(Me_MOTION_C, SelectedItem);
                return;
            }
        }
        else if (dtPAD & PADRright)
        {
            if (trig & PADRleft)
            {
                SET_MOTION(MOT_ATTACK_CROUCH, MOTION_MOVE_APPLY);
                return;
            }
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            return;
        }
        else
        {
            if (trig & PADRleft)
            {
                AttackControl();
                return;
            }
            if (dtPAD & PADLup)
            {
                SET_MOTION(MOT_CHASE, MOTION_MOVE_APPLY);
                return;
            }
            if ((dtPAD & PADLdown) == 0)
                return;
            SET_MOTION(MOT_CHASE_BACK, MOTION_MOVE_APPLY);
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActCHASE(void);
 *     MOTION.C:1241, 61 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       short turn
 *     reg   $v1       short i
 *     reg   $s0       long y
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short MotionUpdateMode;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern short dtCMD;
 *     extern short SelectedItem;
 * END PSX.SYM */

void ActCHASE(void)
{
    short turn;

    turn = Me_MOTION_C->turn / 2;
    switch (dtM->mid)
    {
    case MOT_CHASE:
    {
        if (dtM->count == 0 || dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C,
                  (Me_MOTION_C->map.attrib & MAP_WOOD)
                      ? SE_RUN_STEP_WOOD
                      : SE_RUN_STEP);
        }

        if (dtPAD & PADLup)
        {
            if (Me_MOTION_C->attribute & ATTR_LEDGE)
            {
                SET_MOTION(MOT_STATE_CLIMB, MOTION_MOVE_NONE);
                SET_NOW_MOTION_UNLESS_CVA(goto motion_ready);
            motion_ready:
                MoveHumanoid(Me_MOTION_C, 35, 0);
                if (dtM->mode & MOTION_MODE_CLIMB_ALTERNATE)
                {
                    dtM->mode &= ~MOTION_MODE_CLIMB_ALTERNATE;
                    dtM->count = 13;
                }
                else
                {
                    dtM->mode |= MOTION_MODE_CLIMB_ALTERNATE;
                }
                break;
            }

            if (Me_MOTION_C->attribute & ATTR_WALL)
            {
                long y;
                long height;

                y = dtL->vy;
                height = Me_MOTION_C->map.height;
                dtL->vy -= LEDGE_PROBE_RISE;
                Me_MOTION_C->map.height = 1;
                if (HangCheck() == 0)
                {
                    dtL->vy = y;
                    Me_MOTION_C->map.height = height;
                }
                break;
            }

            if (dtPAD & (PADLleft | PADLright))
            {
                int current;
                int result;
                SVECTOR *rotation;

                rotation = dtR;
                current = rotation->vy;
                if (dtPAD & PADLright)
                {
                    result = current + turn;
                }
                else
                {
                    result = current - turn;
                }
                rotation->vy = result;
                MoveHumanoid(Me_MOTION_C,
                             Me_MOTION_C->motion->motion->orderspd,
                             Me_MOTION_C->motion->motion->sidespd);
                break;
            }

            if ((dtPAD & PADRright) == 0)
            {
                break;
            }
            motID = MOT_SQUAT;
        }
        else
        {
            motID = MOT_ENGAGE_STANCE;
        }
        motMODE = MOTION_MOVE_APPLY;
        break;
    }

    case MOT_CHASE_BACK:
    {
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }

        if ((dtPAD & PADLdown) == 0)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        }
        else if (dtCMD == CMD_LUNGE_BACK)
        {
            SET_MOTION(MOT_ATTACK_LUNGE_BACK, MOTION_MOVE_APPLY);
        }
        else if (dtPAD & (PADLleft | PADLright))
        {
            int current;
            int result;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & PADLright)
            {
                result = current + turn * 4;
            }
            else
            {
                result = current - turn * 4;
            }
            rotation->vy = result;
            MoveHumanoid(Me_MOTION_C,
                         Me_MOTION_C->motion->motion->orderspd,
                         Me_MOTION_C->motion->motion->sidespd);
        }

        if ((dtPAD & PADRright) == 0)
        {
            break;
        }
        if (Me_MOTION_C->pad.trig & PADRleft)
        {
            SET_MOTION(MOT_ATTACK_CROUCH, MOTION_MOVE_APPLY);
            return;
        }
        SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
        break;
    }

    case MOT_CHASE_DASH_FWD:
        if (Me_MOTION_C->pad.trig & PADRleft)
        {
            AttackControl();
        }
        else if (Me_MOTION_C->pad.trig & PADRdown)
        {
            JumpControl();
        }
        /* fall through */
    case MOT_CHASE_DASH_BACK:
    case MOT_CHASE_DASH_RIGHT:
    case MOT_CHASE_DASH_LEFT:
    {
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_ACROBATIC_MOVE);
        }
        if (dtM->count < 7)
        {
            spawn_smoke_burst_(dtL, 150, SMOKE_DRIFT_DIVISOR_DEFAULT, 1);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        }
        return;
    }

    default:
        break;
    }

    if (dtCMD == CMD_FLIP)
    {
        SET_MOTION(MOT_JUMP_FLIP, MOTION_MOVE_NONE);
        MoveHumanoid(Me_MOTION_C, CHASE_WALK_SPEED, 0);
        return;
    }

    if (Me_MOTION_C->pad.trig & PADRdown)
    {
        JumpControl();
        return;
    }
    if (Me_MOTION_C->pad.trig & PADRup)
    {
        SELECT_ITEM_USE_MOTION(item_sound, item_default);
        motMODE = MOTION_MOVE_APPLY;
        return;

    item_sound:
        SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
        return;

    item_default:
        ReqItemDefault(Me_MOTION_C, SelectedItem);
        return;
    }
    if (Me_MOTION_C->pad.trig & PADRleft)
    {
        AttackControl();
    }
}

/* Twin-katana second-blade juggling: slot 0 is the blade in hand, slots
 * 2/3 park the other one. On the motion's draw frame the parked blade
 * moves into the hand (draw sound), on the stow frame it goes back
 * (sheathe sound). Retail copy-pastes this block into each dual-wield
 * attack case with different frame numbers; the macro is reconstruction
 * shorthand for that copy-paste (expands to the identical text). */
#define SWAP_TWIN_BLADE(draw_frame, stow_frame)                               \
    if (dtM->count == (draw_frame))                                           \
    {                                                                         \
        if (weapon[WEAPON_SLOT_INACTIVE_1] != 0)                              \
        {                                                                     \
            weapon[WEAPON_SLOT_INACTIVE_0] =                                  \
                weapon[WEAPON_SLOT_ACTIVE_0];                                 \
            weapon[WEAPON_SLOT_ACTIVE_0] =                                    \
                weapon[WEAPON_SLOT_INACTIVE_1];                               \
            weapon[WEAPON_SLOT_INACTIVE_1] = 0;                               \
            Sound(Me_MOTION_C, CHAR_SE_WEAPON_CHANGE_B);                      \
        }                                                                     \
    }                                                                         \
    else if ((dtM->count == (stow_frame)) &&                                  \
             (weapon[WEAPON_SLOT_INACTIVE_0] != 0))                           \
    {                                                                         \
        weapon[WEAPON_SLOT_INACTIVE_1] =                                      \
            weapon[WEAPON_SLOT_ACTIVE_0];                                     \
        weapon[WEAPON_SLOT_ACTIVE_0] =                                        \
            weapon[WEAPON_SLOT_INACTIVE_0];                                   \
        weapon[WEAPON_SLOT_INACTIVE_0] = 0;                                   \
        Sound(Me_MOTION_C, CHAR_SE_WEAPON_CHANGE_A);                          \
    }

#define INSERT_WEAPON_CONFLICT(hand_)                                         \
    wid = (int)Me_MOTION_C->wepid[hand_];                                     \
    if (wid >= 0)                                                             \
    {                                                                         \
        Humanoid *owner;                                                      \
        short conflict_size;                                                  \
                                                                              \
        conflict_id = InsertConflict(hand[hand_]);                            \
        ConflictObject[conflict_id].offset =                           \
            WeaponDB[wid].confp;                                              \
        conflict_size = WeaponDB[wid].confp.pad;                              \
        owner = Me_MOTION_C;                                                  \
        ConflictObject[conflict_id].size.pad =             \
            CONFLICT_HIT;                                                     \
        ConflictObject[conflict_id].size.vz = conflict_size;        \
        ConflictObject[conflict_id].size.vy = conflict_size;        \
        ConflictObject[conflict_id].size.vx = conflict_size;        \
        ConflictObject[conflict_id].common = owner;                     \
    }

#define SETUP_WEAPON_AFTERIMAGE(hand_)                                        \
    ilu = SetupAfterimage(hand[hand_], 10);                                   \
    ilu->vector1 = WeaponDB[wid].ilup0;                                       \
    ilu->vector2 = WeaponDB[wid].ilup1;                                       \
    Me_MOTION_C->illusion[hand_] = ilu

#define FIRE_GUN_AT_FRAME(frame_, y_)                                         \
    if (dtM->count == frame_)                                                 \
    {                                                                         \
        pos = GetAbsolutePosition(                                            \
            Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0],             \
            0, y_, -100);                                                     \
        bow_shoot_logic(ITEM_GUN, pos);                                       \
        Sound(Me_MOTION_C, CHAR_SE_ATTACK);                                   \
    }

static inline void ClearAttackEffects(s16 mode)
{
    if (mode & ATTACK_CANCEL_CONFLICTS)
    {
        switch (Me_MOTION_C->wpatk)
        {
        case FIST:
            DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_0]);
            DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_1]);
            break;
        case JAW:
            DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_BEAST_HAND_0]);
            break;
        case NO_WEAPON:
            break;
        default:
            DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0]);
            DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1]);
            break;
        }
    }
    if (mode & ATTACK_CANCEL_AFTERIMAGES)
    {
        DISPOSE_WEAPON_AFTERIMAGE(Me_MOTION_C, WEAPON_HAND_0);
        DISPOSE_WEAPON_AFTERIMAGE(Me_MOTION_C, WEAPON_HAND_1);
    }
    dtM->mask = MOTION_MASK_ALL;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActATTACK(void);
 *     MOTION.C:1306, 197 src lines, frame 128 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       struct BattleType * battle
 *     reg   $v1       struct ModelType ** object
 *     stack sp+16     struct ModelType *[2] hand
 *     reg   $a2       struct OrnamentType ** weapon
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+24     struct PARAM_ITEM_LAUNCH item
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+24     struct PARAM_ITEM_LAUNCH item
 *     stack sp+104    struct SVECTOR vect
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+64     struct PARAM_ITEM_LAUNCH item
 *     reg   $v1       short i
 *     reg   $v1       short i
 *     reg   $v1       short i
 *     reg   $a2       struct OrnamentType ** weapon
 *     reg   $v1       short i
 *     reg   $a2       struct OrnamentType ** weapon
 *     reg   $v1       short i
 *     reg   $a3       struct ModelType * waist
 *     reg   $v1       short i
 *     reg   $t0       struct AfterimageType * ilu
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct BattleType BattleDB[78];
 *     extern struct SVECTOR *dtR;
 *     extern struct VECTOR *dtL;
 *     extern short dtPAD;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR *dtV;
 *     extern short ActionHalt;
 *     extern struct TCameraStatus CamState;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct WeaponType WeaponDB[28];
 * END PSX.SYM */

void ActATTACK(void)
{
    bool is_player;
    SVECTOR *v;
    MotionManager *mmp;
    short warid;
    short t;
    short conflict_id;
    MotionDataType *mot;
    VECTOR *pos;
    int wid;
    AfterimageType *ilu;
    ModelType **object;
    GsCOORDINATE2 *target;
    BattleType *battle;
    ModelType *hand[N_WEAPON_HANDS];
    union
    {
        PARAM_ITEM_LAUNCH item;
        SVECTOR fall_velocity;
    } scratch;
    PARAM_ITEM_LAUNCH item;
    SVECTOR vect;

    {
        Humanoid *human;

        if (dtM->count == 1)
        {
            short attack_id;
            short sound_id;

            if (Me_MOTION_C->life == 0)
            {
                SET_MOTION(MOT_DEAD, MOTION_MOVE_APPLY);
                return;
            }
            attack_id = GetAttackDBID(Me_MOTION_C, motID);
            human = Me_MOTION_C;
            human->warid = attack_id;
            sound_id = CHAR_VOICE_TAUNT;
            if (motID != MOT_ATTACK_TAUNT)
            {
                sound_id = CHAR_VOICE_ACTION_B;
                if (motID & 1)
                {
                    sound_id = CHAR_VOICE_ACTION_A;
                }
            }
            Sound(human, sound_id);
        }
        human = Me_MOTION_C;
        warid = human->warid;
        battle = &BattleDB[warid];
        target = human->target;
    }
    if (((target != 0) && (dtM->count < battle->revise)) && (dtM->count >= 0))
    {
        Humanoid *human;
        short turn;
        short direction;

        direction = GetDirection(target->coord.t[0] - dtL->vx,
                                 target->coord.t[2] - dtL->vz, dtR->vy);
        human = Me_MOTION_C;
        turn = human->turn;
        if ((int)direction > (int)turn)
        {
            dtR->vy += 100;
        }
        else if (-(int)turn > (int)direction)
        {
            dtR->vy -= 100;
        }
        else
        {
            goto dispatch;
        }
        mot = human->motion->motion;
        MoveHumanoid(human, (u16)mot->orderspd, (u16)mot->sidespd);
    }
dispatch:
    switch (dtM->mid)
    {
    case MOT_ATTACK:
        t = GetMotionID(dtM, MOT_ATTACK);
        switch (t)
        {
        case ATTACK_MOTID_KATANAL:
        {
            OrnamentType **weapon;

            weapon = Me_MOTION_C->weapon;
            SWAP_TWIN_BLADE(42, 6);
            break;
        }
        case ATTACK_MOTID_GUN:
        {
            VECTOR *pos;

            FIRE_GUN_AT_FRAME(20, 100);
            break;
        }
        case ATTACK_MOTID_TEPPO:
        {
            VECTOR *pos;

            FIRE_GUN_AT_FRAME(22, 700);
            break;
        }
        case ATTACK_MOTID_MANJI:
        {
            int last_frame;
            PARAM_ITEM_LAUNCH *request;
            short first_frame;

            first_frame = 36;
            t = dtM->count;
            request = &item;
            if (first_frame <= t)
            {
                last_frame = 65;
                if (last_frame < t)
                {
                    break;
                }
                if (t == first_frame)
                {
                    Sound(Me_MOTION_C, SE_FIRE);
                }
                item.type = ITEM_NAPALM;
                item.user = Me_MOTION_C;
                pos = GetAbsolutePosition(
                    Me_MOTION_C->model->object[MODEL_PART_HEAD], 0, -100,
                    -300);
                item.start.vx = pos->vx;
                item.start.vy = pos->vy;
                item.start.vz = pos->vz;
                GetMoveSpeed(&vect, dtR->vy, 100, 0);
                item.end.vx = item.start.vx + vect.vx;
                item.end.vy = item.start.vy;
                item.end.vz = item.start.vz + vect.vz;
                ReqItemUse(request);
            }
            break;
        }
        case ATTACK_MOTID_SEVEN:
            launch_lightning_bolt_(13);
            break;
        case ATTACK_MOTID_YUMI:
        case ATTACK_MOTID_KATAYUMI:
            AttackBowControl(BOW_TIMING_OPENING);
            break;
        }
        if (((Me_MOTION_C->pad.trig & PADRleft) != 0) &&
            AttackContinuousCheck(battle) != 0)
        {
            short i;

            if ((dtPAD & PADLright) != 0)
            {
                motID = MOT_ATTACK_SLASH2_RIGHT;
            }
            else if ((dtPAD & PADLleft) != 0)
            {
                motID = MOT_ATTACK_SLASH2_LEFT;
            }
            else
            {
                motID = MOT_ATTACK_SLASH2;
            }
            motMODE = MOTION_MOVE_APPLY;
            i = 0;
            /* Empty loop retained for code layout; its original source construct is unknown. */
            do
            {
            } while (0);
            if (MotionUpdateMode != 0)
            {
                for (; i < N_CVA_HUMANS; i++)
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto no_motion;
                    }
                }
            }
            goto set_motion;
        }
        break;
    case MOT_ATTACK_SLASH2:
    {
        OrnamentType **weapon;

        t = Me_MOTION_C->wpatk;
        if (t == KATANAL)
        {
            weapon = Me_MOTION_C->weapon;
            SWAP_TWIN_BLADE(52, 1);
        }
        else if (t == SEVEN)
        {
            launch_lightning_bolt_(13);
        }
        else if (t == KATAYUMI)
        {
            AttackBowControl(BOW_TIMING_COMBO);
        }
        if (((Me_MOTION_C->pad.trig & PADRleft) != 0) &&
            AttackContinuousCheck(battle) != 0)
        {
            short i;

            SET_MOTION(MOT_ATTACK_SLASH3, MOTION_MOVE_APPLY);
            if (MotionUpdateMode != 0)
            {
                for (i = 0; i < N_CVA_HUMANS; i++)
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto no_motion;
                    }
                }
            }
            goto set_motion;
        }
        break;
    }
    case MOT_ATTACK_SLASH3:
        if (Me_MOTION_C->wpatk == SEVEN)
        {
            launch_lightning_bolt_(13);
        }
        else if (Me_MOTION_C->wpatk == KATAYUMI)
        {
            AttackBowControl(BOW_TIMING_COMBO);
        }
        if (((Me_MOTION_C->pad.trig & PADRleft) != 0) &&
            AttackContinuousCheck(battle) != 0)
        {
            short i;

            SET_MOTION(MOT_ATTACK_SLASH4, MOTION_MOVE_APPLY);
            if (MotionUpdateMode != 0)
            {
                for (i = 0; i < N_CVA_HUMANS; i++)
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto no_motion;
                    }
                }
            }
            goto set_motion;
        }
        break;
    case MOT_ATTACK_SLASH4:
        if (Me_MOTION_C->wpatk == SEVEN)
        {
            launch_lightning_bolt_(13);
        }
        break;
    case MOT_ATTACK_RIGHT1:
    {
        OrnamentType **weapon;

        if (Me_MOTION_C->wpatk == KATANAL)
        {
            weapon = Me_MOTION_C->weapon;
            SWAP_TWIN_BLADE(52, 16);
        }
        if ((((Me_MOTION_C->pad.trig & PADRleft) != 0) && ((dtPAD & (PADLleft | PADLright)) != 0)) &&
            AttackContinuousCheck(battle) != 0)
        {
            short i;

            if ((dtPAD & PADLleft) != 0)
            {
                motID = MOT_ATTACK_RIGHT2_LEFT;
            }
            else
            {
                motID = MOT_ATTACK_RIGHT2;
            }
            motMODE = MOTION_MOVE_APPLY;
            i = 0;
            /* Empty loop retained for code layout; its original source construct is unknown. */
            do
            {
            } while (0);
            if (MotionUpdateMode != 0)
            {
                for (; i < N_CVA_HUMANS; i++)
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto no_motion;
                    }
                }
            }
            goto set_motion;
        }
        break;
    }
    case MOT_ATTACK_LEFT1:
    {
        OrnamentType **weapon;

        if (Me_MOTION_C->wpatk == KATANAL)
        {
            weapon = Me_MOTION_C->weapon;
            SWAP_TWIN_BLADE(43, 13);
        }
        if ((((Me_MOTION_C->pad.trig & PADRleft) != 0) && ((dtPAD & (PADLleft | PADLright)) != 0)) &&
            AttackContinuousCheck(battle) != 0)
        {
            short i;

            if ((dtPAD & PADLright) == 0)
            {
                goto combo_alt;
            }
            motID = MOT_ATTACK_LEFT2_RIGHT;
            goto set_combo;
        no_motion:
            t = 0;
            goto snap_origin;
        combo_alt:
            motID = MOT_ATTACK_LEFT2;
        set_combo:
            motMODE = MOTION_MOVE_APPLY;
            i = 0;
            /* Empty loop retained for code layout; its original source construct is unknown. */
            do
            {
            } while (0);
            if (MotionUpdateMode != 0)
            {
                for (; i < N_CVA_HUMANS; i++)
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto no_motion;
                    }
                }
            }
        set_motion:
            t = SetNowMotion(Me_MOTION_C, motID, motMODE);
            motMODE = MOTION_MOVE_UNSET;
        snap_origin:
            if (t != 0)
            {
                int conflict_id;

                conflict_id = (int)(*Me_MOTION_C->model->object)->id;
                if (conflict_id >= 0)
                {
                    dtL->vx = ConflictObject[conflict_id].position.vx;
                    dtL->vz = ConflictObject[conflict_id].position.vz;
                }
            }
            break;
        }
        break;
    }
    case MOT_ATTACK_DIVE:
        if ((dtM->count == 1) && (Me_MOTION_C->map.height > 3000))
        {
            SetCameraMode(CMODE_FALL);
        }
        if ((dtPAD & (PADLleft | PADLdown | PADLright | PADLup)) != 0)
        {
            if ((dtPAD & PADLup) != 0)
            {
                GetMoveSpeed(&scratch.fall_velocity, dtR->vy, 10, 0);
            }
            else if ((dtPAD & PADLdown) != 0)
            {
                GetMoveSpeed(&scratch.fall_velocity, dtR->vy, -10, 0);
            }
            else if ((dtPAD & PADLright) != 0)
            {
                GetMoveSpeed(&scratch.fall_velocity, dtR->vy, 0, -10);
            }
            else
            {
                GetMoveSpeed(&scratch.fall_velocity, dtR->vy, 0, 10);
            }
            v = dtV;
            scratch.fall_velocity.vx += dtV->vx;
            scratch.fall_velocity.vz += dtV->vz;
            if ((((scratch.fall_velocity.vx >= 0) ? scratch.fall_velocity.vx : -scratch.fall_velocity.vx) <= 100) &&
                (((scratch.fall_velocity.vz >= 0) ? scratch.fall_velocity.vz : -scratch.fall_velocity.vz) <= 100))
            {
                dtV->vx = scratch.fall_velocity.vx;
                v->vz = scratch.fall_velocity.vz;
            }
        }
        if ((Me_MOTION_C->attribute & ATTR_NOFLOOR) != 0)
        {
            SET_MOTION(MOT_ATTACK_DIVE_LAND, MOTION_MOVE_NONE);
            Sound(Me_MOTION_C, SE_LAND_HEAVY);
            spawn_smoke_burst_(dtL, 300, SMOKE_DRIFT_DIVISOR_DEFAULT, 10);
        }
        if ((dtM->count == 0) && (dtM->loop == 1))
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        if (dtM->loop < 0)
        {
            dtM->loop--;
            if (dtM->loop < -30)
            {
                SET_MOTION(MOT_STATE_FALL, MOTION_MOVE_NONE);
            }
        }
        if (motID != MOT_ATTACK_DIVE)
        {
            short cleanup_guard;
            short kind;

            kind = Me_MOTION_C->wpatk;
            CLEAR_WEAPON_ATTACK_EFFECTS(Me_MOTION_C, kind, cleanup_guard);
            dtM->mask = MOTION_MASK_ALL;
            SetCameraMode(CMODE_NORMAL);
            if (motID == MOT_STATE_FALL)
            {
                return;
            }
        }
        break;
    case MOT_ATTACK_DIVE_LAND:
    {
        short cleanup_guard;
        short kind;

        if (dtM->loop < 0)
        {
            dtM->loop = 0;
        }
        if ((dtM->count == 0) && (dtM->loop != 0))
        {
            kind = Me_MOTION_C->wpatk;
            CLEAR_WEAPON_ATTACK_EFFECTS(Me_MOTION_C, kind, cleanup_guard);
            mmp = dtM;
            SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
            mmp->mask = MOTION_MASK_ALL;
            return;
        }
        if (Me_MOTION_C->map.height > 0)
        {
            return;
        }
        v = dtV;
        dtV->vx -= (dtV->vx >> 2);
        v->vz -= (v->vz >> 2);
        return;
    }
    case MOT_ATTACK_TAUNT:
        if (dtM->count != 0)
        {
            return;
        }
        if (dtM->loop == 0)
        {
            return;
        }
        SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        return;
    case MOT_ATTACK_STEALTH_BACK:
    case MOT_ATTACK_STEALTH_FRONT:
    case MOT_ATTACK_STEALTH_SIDE:
    case MOT_ATTACK_STEALTH_BACK_AYAME:
    case MOT_ATTACK_STEALTH_FRONT_AYAME:
    case MOT_ATTACK_STEALTH_SIDE_AYAME:
    {
        int conflict_id;
        Humanoid *human;
        ModelType *waist;
        motion_id saved_mid;
        motion_move_mode apply_movement;

        if (dtM->count == 1)
        {
            ActionHalt = ACTION_HALT_ACTIVE;
            SetCameraMode(CMODE_CRITICAL_HIT);
            CamState.snap_pending = 1;
            return;
        }
        if ((dtM->loop == 0) && (dtL->vy == Me_MOTION_C->target->coord.t[1]))
        {
            return;
        }
        waist = *Me_MOTION_C->model->object;
        ActionHalt = ACTION_HALT_NONE;
        conflict_id = (int)(*Me_MOTION_C->model->object)->id;
        if (conflict_id >= 0)
        {
            dtL->vx = ConflictObject[conflict_id].position.vx;
            dtL->vz = ConflictObject[conflict_id].position.vz;
        }
        (void)*(Humanoid *volatile *)&Me_MOTION_C;
        waist->locate.coord.t[2] = 0;
        waist->locate.coord.t[0] = 0;
        ReturnNormal();
        saved_mid = motID;
        apply_movement = motMODE;
        human = Me_MOTION_C;
        if (human->status != STAT_DEAD ||
            human->motion->loop != MOTION_LOOP_DISABLED)
        {
            if (UpdateMotion(human->motion, saved_mid) != 0)
            {
                human->status = MOTION_STATUS(saved_mid);
                if (apply_movement != MOTION_MOVE_NONE)
                {
                    mot = human->motion->motion;
                    MoveHumanoid(human, (u16)mot->orderspd, (u16)mot->sidespd);
                }
            }
        }
        dtM->count = 0;
        dtM->loop = 0;
        PlayMotion(dtM, 1);
        motMODE = MOTION_MOVE_UNSET;
        CamState.snap_pending = 1;
        return;
    }
    }
    if (dtM->loop < 0)
    {
        dtM->loop++;
        if (dtM->loop != 0)
        {
            return;
        }
        mot = Me_MOTION_C->motion->motion;
        MoveHumanoid(Me_MOTION_C, (u16)mot->orderspd, (u16)mot->sidespd);
        return;
    }
    if ((dtM->count == 0) && (dtM->loop == 1))
    {
        short cleanup_guard;
        short kind;
        motion_id saved_mid;
        short i;

        saved_mid = motID;
        kind = Me_MOTION_C->wpatk;
        CLEAR_WEAPON_ATTACK_EFFECTS(Me_MOTION_C, kind, cleanup_guard);
        SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        dtM->mask = MOTION_MASK_ALL;
        SET_NOW_MOTION_UNLESS_CVA(goto align_rotation);
    align_rotation:
        dtR->vy += (((*Me_MOTION_C->model->object)->rotate).vy -
                    dtM->motion->rotate[MODEL_PART_WAIST]->y);
        is_player = Me_MOTION_C == StagePlayer;
        ((*Me_MOTION_C->model->object)->rotate).vy =
            dtM->motion->rotate[MODEL_PART_WAIST]->y;
        if (is_player)
        {
            SetCameraMode(CMODE_NORMAL);
            if (saved_mid == MOT_ATTACK_LUNGE_BACK)
            {
                CamState.snap_pending = 1;
            }
        }
        Me_MOTION_C->pad.time = 0;
        return;
    }
    {
        weapon_kind hand_kind;

        if (battle->mid == 0)
        {
            return;
        }
        if (battle->atks < 1)
        {
            return;
        }
        object = Me_MOTION_C->model->object;
        hand_kind = Me_MOTION_C->wpatk;
        switch (hand_kind)
        {
        case JAW:
            hand[WEAPON_HAND_0] = object[MODEL_PART_BEAST_HAND_0];
            hand[WEAPON_HAND_1] = object[MODEL_PART_BEAST_HAND_1];
            break;
        case FIST:
            hand[WEAPON_HAND_0] = object[MODEL_PART_ONININ_HAND_0];
            hand[WEAPON_HAND_1] = object[MODEL_PART_ONININ_HAND_1];
            break;
        default:
            hand[WEAPON_HAND_0] = object[MODEL_PART_WEAPON_HAND_0];
            hand[WEAPON_HAND_1] = object[MODEL_PART_WEAPON_HAND_1];
            break;
        }
        if (dtM->count == battle->atks)
        {
            INSERT_WEAPON_CONFLICT(WEAPON_HAND_0);
            INSERT_WEAPON_CONFLICT(WEAPON_HAND_1);
            Sound(Me_MOTION_C, CHAR_SE_ATTACK);
            if (Me_MOTION_C == StagePlayer)
            {
                PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_FAST, RUMBLE_RELEASE_NONE);
            }
        }
        else if (dtM->count == battle->atke)
        {
            ClearAttackEffects(ATTACK_CANCEL_CONFLICTS);
        }
        if ((dtM->count < battle->atke) && ((Me_MOTION_C->type & PAGE_MASK) != PAGE_BEAST))
        {
            if (hand[WEAPON_HAND_0]->id != CONFLICT_NONE)
            {
                WeaponHitWeapon(hand[WEAPON_HAND_0]);
            }
            if (hand[WEAPON_HAND_1]->id != CONFLICT_NONE)
            {
                WeaponHitWeapon(hand[WEAPON_HAND_1]);
            }
        }
        if (battle->ilus < 1)
        {
            return;
        }
        if (dtM->count == battle->ilus)
        {
            wid = (int)Me_MOTION_C->wepid[WEAPON_HAND_0];
            if (wid >= 0)
            {
                SETUP_WEAPON_AFTERIMAGE(WEAPON_HAND_0);
            }
            wid = (int)Me_MOTION_C->wepid[WEAPON_HAND_1];
            if (wid < 0)
            {
                return;
            }
            SETUP_WEAPON_AFTERIMAGE(WEAPON_HAND_1);
            return;
        }
        if (dtM->count != battle->ilue)
        {
            return;
        }
        DISPOSE_WEAPON_AFTERIMAGE(Me_MOTION_C, WEAPON_HAND_0);
        DISPOSE_WEAPON_AFTERIMAGE(Me_MOTION_C, WEAPON_HAND_1);
        mmp = dtM;
        mmp->mask = MOTION_MASK_ALL;
        return;
    }
}
#undef FIRE_GUN_AT_FRAME
#undef SETUP_WEAPON_AFTERIMAGE
#undef INSERT_WEAPON_CONFLICT

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSTATE(void);
 *     MOTION.C:1507, 79 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct SVECTOR *dtV;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern struct VECTOR *dtL;
 *     extern struct TCameraStatus CamState;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

void ActSTATE(void)
{
    short cleanup_guard;
    long t;
    Humanoid *human;

    switch (dtM->mid)
    {
    case MOT_STATE_DRAW:
        if (dtM->count == 1)
        {
            CLEAR_WEAPON_ATTACK_EFFECTS(Me_MOTION_C, Me_MOTION_C->wpatk,
                                        cleanup_guard);
            dtM->mask = MOTION_MASK_ALL;
            if (Me_MOTION_C->type < KERAI_KATANA)
            {
                if (Me_MOTION_C->type > AYAME_1)
                {
                    SELECT_RETURN_MOTION();
                    return;
                }
            }
            if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) == 0)
            {
                return;
            }
            motID = MOT_ENGAGE_STANCE;
            motMODE = MOTION_MOVE_APPLY;
            return;

        }
        if (dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C, CHAR_SE_WEAPON_CHANGE_A);
            EquipWeapon(Me_MOTION_C, WEAPON_DRAWN);
            return;
        }
        if (dtM->count != 0)
        {
            return;
        }
        if (dtM->loop == 0)
        {
            return;
        }
        {
            human = Me_MOTION_C;
            if ((human->attribute & ATTR_PHASE) == 0)
            {
                human->attribute |= ATTR_SEARCH | PHASE_ALERT;
                human->chase[HUMANOID_CHASE_X] = StagePlayer->locate->vx;
                t = StagePlayer->locate->vz;
                human->actscnt = 1;
                human->chase[HUMANOID_CHASE_Z] = t;
            }
        }
        SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        return;

    case MOT_STATE_SHEATHE: /* stand down: sheathe (hitboxes and afterimage off),
                 * then back to idle unless still combat-ready */
        if (dtM->count == 1)
        {
            CLEAR_WEAPON_ATTACK_EFFECTS(Me_MOTION_C, Me_MOTION_C->wpatk,
                                        cleanup_guard);
            dtM->mask = MOTION_MASK_ALL;
            if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) != 0)
            {
                return;
            }
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
            return;

        }
        if (dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C, CHAR_SE_WEAPON_CHANGE_B);
            EquipWeapon(Me_MOTION_C, WEAPON_SHEATHED);
            return;
        }
        if (dtM->count != 0)
        {
            return;
        }
        if (dtM->loop == 0)
        {
            return;
        }
        SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        return;

    case MOT_STATE_FALL:
        if (dtM->count < -0x36 && dtV->vy > 200)
        {
            dtM->count = -0x1e;
        }
        if (dtV->vy > 0 && (Me_MOTION_C->pad.trig & PADRleft) != 0)
        {
            SET_MOTION(MOT_ATTACK_DIVE, MOTION_MOVE_NONE);
        }
        {
            if ((Me_MOTION_C->attribute & ATTR_NOFLOOR) != 0 || Me_MOTION_C->map.height <= 0)
            {
                if (dtM->count < -0x28)
                {
                    SELECT_RETURN_MOTION();
                    Sound(Me_MOTION_C, SE_LAND_LIGHT);
                    return;
                }
                if (dtM->count > -0x15)
                {
                    if ((Me_MOTION_C->type & PAGE_MASK) != PAGE_GUARD)
                    {
                        SET_MOTION(MOT_STATE_LAND_HEAVY, MOTION_MOVE_NONE);
                        return;
                    }
                }
                else
                {
                    SET_MOTION(MOT_STATE_LAND, MOTION_MOVE_NONE);
                    return;
                }

                {
                    motMODE = MOTION_MOVE_NONE;
                    motID = (rand() & 1) ? MOT_DAMAGE_SLAM_BACK : MOT_DAMAGE_SLAM_FORE;
                    Me_MOTION_C->life -= 10;
                    if (Me_MOTION_C->life < 0)
                    {
                        Me_MOTION_C->life = 0;
                    }
                    Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
                    ReqLifeBar(Me_MOTION_C);
                }
                return;
            }
        }

        if (dtM->count > -0x2e)
        {
            return;
        }
        dtV->vx >>= 1;
        dtV->vz >>= 1;
        return;

    case MOT_STATE_LAND:
    case MOT_STATE_LAND_HEAVY:
        if (dtM->count == 1 && Me_MOTION_C == StagePlayer)
        {
            PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_NONE);
            SetCameraMode(CMODE_NORMAL);
        }
        if (dtM->count < 5 && (dtPAD & PADRright) != 0 &&
            (Me_MOTION_C->pad.trig & PADRdown) != 0)
        {
            SET_MOTION(MOT_SQUAT_BACKFLIP, MOTION_MOVE_APPLY);
            dtR->vy += ANGLE_HALF;
        }
        /* fall through */
    case MOT_STATE_LAND_FLIP:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C,
                  motID == MOT_STATE_LAND ? SE_LAND_LIGHT : SE_LAND_HEAVY);
            spawn_smoke_burst_(dtL, 300, SMOKE_DRIFT_DIVISOR_DEFAULT, 10);
            if (StagePlayer == Me_MOTION_C)
            {
                if (motID == MOT_STATE_LAND_HEAVY)
                {
                    PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX,
                               RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_LONG);
                }
                else
                {
                    PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX,
                               RUMBLE_ATTACK_FAST, RUMBLE_RELEASE_NONE);
                }
            }
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (motID == MOT_STATE_LAND_FLIP)
            {
                CamState.snap_pending = 1;
            }
            SELECT_RETURN_MOTION();
            return;
        }
        dtV->vx -= dtV->vx >> 2;
        dtV->vz -= dtV->vz >> 2;
        return;

    case MOT_STATE_CLIMB:
        if (dtM->count != dtM->motion->time / 2)
        {
            if (dtM->count != 0)
            {
                return;
            }
            if (dtM->loop == 0)
            {
                return;
            }
        }
        SET_MOTION(MOT_CHASE, MOTION_MOVE_APPLY);
        SET_NOW_MOTION_UNLESS_CVA(goto motion_ready);
    motion_ready:
        Sound(Me_MOTION_C, SE_ACROBATIC_MOVE);
        return;

    case MOT_STATE_PICKUP:
        if (dtM->count != 0)
        {
            return;
        }
        if (dtM->loop == 0)
        {
            return;
        }
        SELECT_RETURN_MOTION();
        return;

    default:
    case MOT_STATE_VARIANT_2:
        return;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActJUMP(void);
 *     MOTION.C:1590, 69 src lines, frame 56 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct MapVector map
 *     reg   $a3       short ry
 *     reg   $v1       short i
 *     reg   $s0       short mid
 *     reg   $v1       short i
 *     stack sp+40     struct SVECTOR spd
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct VECTOR *dtL;
 *     extern struct MotionManager *dtM;
 *     extern short RefrectVector[16];
 *     extern struct SVECTOR *dtV;
 *     extern struct SVECTOR *dtR;
 *     extern short MotionUpdateMode;
 *     extern short motMODE;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 *     extern short dtPAD;
 * END PSX.SYM */

/* Jump-state motion, collision response, air steering, and landing control. */
void ActJUMP(void)
{
    motion_id mid; /* saved before SET_MOTION overwrites motID */
    u16 pad;
    MapVector map;
    SVECTOR spd;
    facing_angle ry;
    long level;
    long apex_offset;
    long scaled;
    SVECTOR *velocity;

    if ((Me_MOTION_C->pad.trig & PADRdown) != 0 && motID != MOT_JUMP_WALLKICK)
    {
        GetAreaMapVector(GlobalAreaMap, &map, dtL,
                         Me_MOTION_C->width + 300, AREA_LEVEL_DEFAULT);
        if (map.vector == 0)
        {
            return;
        }
        if (UpdateMotion(dtM, MOT_JUMP_WALLKICK) == 0)
        {
            return;
        }
        ry = RefrectVector[map.vector];
        dtL->vy -= 500;
        if (ry == ANGLE_NONE)
        {
            dtV->vx = -dtV->vx;
            dtV->vz = -dtV->vz;
        }
        else
        {
            dtR->vy = ry + ANGLE_HALF;
            MoveHumanoid(Me_MOTION_C, -100, 0);
        }
        Sound(Me_MOTION_C, SE_JUMP_IMPACT);
        return;
    }

    if ((Me_MOTION_C->attribute & (ATTR_NOFLOOR | ATTR_BUOYANT)) != 0 && dtM->count >= 2)
    {
        level = GetAreaMapLevel(GlobalAreaMap, dtL->vx, dtL->vy, dtL->vz,
                                AREA_LEVEL_DEFAULT);
        if (dtL->vy < level)
        {
            SET_MOTION(MOT_STATE_FALL, MOTION_MOVE_NONE);
            SET_NOW_MOTION_UNLESS_CVA(goto landed_motion_done);
        landed_motion_done:
            Sound(Me_MOTION_C, CHAR_VOICE_HURT);
            dtM->count >>= 2;
            if (Me_MOTION_C == StagePlayer)
            {
                PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_NONE);
            }
            return;
        }
        if (motID == MOT_JUMP_FLIP)
        {
            ModelType *object;

            dtR->vy += (*Me_MOTION_C->model->object)->rotate.vy;
            object = *Me_MOTION_C->model->object;
            SET_MOTION(MOT_STATE_LAND_FLIP, MOTION_MOVE_APPLY);
            object->rotate.vy = 0;
            return;
        }
        SET_MOTION(MOT_STATE_LAND, MOTION_MOVE_NONE);
        return;
    }
    else
    {
        if (dtM->count == 0 && dtM->loop != 0)
        {
            mid = (u16)motID;
            SET_MOTION(MOT_STATE_FALL, MOTION_MOVE_NONE);
            SET_NOW_MOTION_UNLESS_CVA(goto fall_motion_done);
        fall_motion_done:
            if (mid != MOT_JUMP_RUN)
            {
                if (mid != MOT_JUMP_FLIP)
                {
                    return;
                }
                dtR->vy += ANGLE_HALF;
                (*Me_MOTION_C->model->object)->rotate.vy = 0;
            }
            dtM->count >>= 1;
            return;
        }

        velocity = dtV;
        apex_offset = dtM->count - (dtM->motion->time >> 1);
        if (motID == MOT_JUMP_RUN)
        {
            scaled = apex_offset * 10;
        }
        else
        {
            scaled = apex_offset * 20;
        }
        velocity->vy = scaled;

        if ((dtPAD & (PADLleft | PADLdown | PADLright | PADLup)) != 0 && motID != MOT_JUMP_RUN)
        {
            pad = (u16)dtPAD;
            if ((pad & PADLup) != 0)
            {
                GetMoveSpeed(&spd, dtR->vy, 10, 0);
            }
            else if ((pad & PADLdown) != 0)
            {
                GetMoveSpeed(&spd, dtR->vy, -10, 0);
            }
            else if ((pad & PADLright) != 0)
            {
                GetMoveSpeed(&spd, dtR->vy, 0, -10);
            }
            else
            {
                GetMoveSpeed(&spd, dtR->vy, 0, 10);
            }
            spd.vx += dtV->vx;
            spd.vz += dtV->vz;
            if (__builtin_abs(spd.vx) <= 100)
            {
                if (__builtin_abs(spd.vz) <= 100)
                {
                    dtV->vx = spd.vx;
                    dtV->vz = spd.vz;
                }
            }
        }

        if ((dtPAD & PADRleft) == 0)
        {
            return;
        }
        if (motID == MOT_JUMP_FLIP)
        {
            return;
        }
        if (dtV->vy <= 0)
        {
            return;
        }
        if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) == 0)
        {
            return;
        }
        SET_MOTION(MOT_ATTACK_DIVE, MOTION_MOVE_NONE);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActHANG(void);
 *     MOTION.C:1663, 45 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       long y
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR *dtV;
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct VECTOR *dtL;
 *     extern short motID;
 *     extern short motMODE;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

void ActHANG(void)
{
    long y;

    dtV->vy = 0;
    switch (dtM->mid)
    {
    case MOT_HANG:
        if (dtPAD & PADLdown)
        {
            y = dtL->vy;
            do
            {
                y += 100;
                dtL->vy = y;
            } while (HangCheck() != 0);
            SET_MOTION(MOT_STATE_FALL, MOTION_MOVE_NONE);
        }
        else if (dtPAD & PADLright)
        {
            SET_MOTION(MOT_HANG_SHIMMY_RIGHT, MOTION_MOVE_APPLY);
        }
        else if (dtPAD & PADLleft)
        {
            SET_MOTION(MOT_HANG_SHIMMY_LEFT, MOTION_MOVE_APPLY);
        }
        else if ((dtPAD & PADLup) &&
                 GetAreaMapLevel(GlobalAreaMap, dtL->vx, dtL->vy - 2000,
                                 dtL->vz, AREA_LEVEL_STEP_DOWN) !=
                     (u32)LEVEL_NONE)
        {
            SET_MOTION(MOT_HANG_PULLUP, MOTION_MOVE_APPLY);
        }
        break;
    case MOT_HANG_SHIMMY_RIGHT:
    case MOT_HANG_SHIMMY_LEFT:
        if ((dtPAD & (PADLleft | PADLright)) == 0)
        {
            SET_MOTION(MOT_HANG, MOTION_MOVE_APPLY);
        }
        else if (HangCheck() == 0)
        {
            SET_MOTION(MOT_STATE_FALL, MOTION_MOVE_NONE);
        }
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_LEDGE_GRIP);
        }
        break;
    case MOT_HANG_PULLUP:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(CMODE_NORMAL);
            }
            if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
            {
                SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
                return;
            }
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
            return;
        }
        if (dtM->count >= 0)
        {
            dtV->vy = -35;
            if (dtM->count > 40)
            {
                MoveHumanoid(Me_MOTION_C, 100, 0);
            }
        }
        break;
    case MOT_HANG_CATCH:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_HANG, MOTION_MOVE_APPLY);
        }
        break;
    }
    if (Me_MOTION_C->attribute & ATTR_PUSH)
    {
        /* Shoved by a conflict object while hanging: knocked off. */
        MoveHumanoid(Me_MOTION_C, -10, 0);
        SET_MOTION(MOT_STATE_FALL, MOTION_MOVE_NONE);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSQUAT(void);
 *     MOTION.C:1712, 83 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short turn
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct SVECTOR *dtV;
 *     extern struct TCameraStatus CamState;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct VECTOR *dtL;
 *     extern short dtCMD;
 *     extern short SelectedItem;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

void ActSQUAT(void)
{
    short turn;

    turn = Me_MOTION_C->turn / 2;
    switch (dtM->mid)
    {
    case MOT_SQUAT:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        if (dtPAD & PADLup)
        {
            SET_MOTION(MOT_SQUAT_WALK_F, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & PADLdown)
        {
            SET_MOTION(MOT_SQUAT_WALK_B, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & PADLright)
        {
            SET_MOTION(MOT_SQUAT_WALK_R, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & PADLleft)
        {
            SET_MOTION(MOT_SQUAT_WALK_L, MOTION_MOVE_APPLY);
            break;
        }
        if (Me_MOTION_C->pad.trig & PADRdown)
        {
            SET_MOTION(MOT_SQUAT_BACKFLIP, MOTION_MOVE_APPLY);
            dtR->vy += ANGLE_HALF;
        }
        break;

    case MOT_SQUAT_WALK_F:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLup) == 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
        }
        else if (Me_MOTION_C->pad.trig & PADRdown)
        {
            SET_MOTION(MOT_SQUAT_BACKFLIP, MOTION_MOVE_APPLY);
            dtR->vy += ANGLE_HALF;
        }
        break;

    case MOT_SQUAT_WALK_B:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLdown) == 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & (PADLleft | PADLright))
        {
            int current;
            int result;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & PADLright)
            {
                result = current + turn;
            }
            else
            {
                result = current - turn;
            }
            rotation->vy = result;
            dtV->vz = 0;
            dtV->vx = 0;
            break;
        }
        goto move_if_stationary;

    case MOT_SQUAT_WALK_R:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLright) == 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & PADLdown)
        {
            dtR->vy += turn;
            dtV->vz = 0;
            dtV->vx = 0;
            break;
        }
        goto move_if_stationary;

    case MOT_SQUAT_WALK_L:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLleft) == 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & PADLdown)
        {
            dtR->vy -= turn;
            dtV->vz = 0;
            dtV->vx = 0;
            break;
        }
    move_if_stationary:
        if (dtV->vx == 0 && dtV->vz == 0)
        {
            MoveHumanoid(Me_MOTION_C,
                         Me_MOTION_C->motion->motion->orderspd,
                         Me_MOTION_C->motion->motion->sidespd);
        }
        break;

    case MOT_SQUAT_BACKFLIP:
        if (dtM->count == (dtM->motion->time >> 1))
        {
            Sound(Me_MOTION_C, SE_ACROBATIC_MOVE);
            CamState.snap_pending = 1;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
        }
        break;

    default:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_ACROBATIC_MOVE);
            return;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            return;
        }
        if (dtV->vx == 0 && dtV->vz == 0)
        {
            return;
        }
        if (GetAreaMapLevel(GlobalAreaMap,
                            dtL->vx + dtV->vx * 4,
                            dtL->vy,
                            dtL->vz + dtV->vz * 4,
                            AREA_LEVEL_STEP_DOWN |
                                AREA_LEVEL_RETURN_DELTA) < STEP_DROP_LIMIT)
        {
            dtL->vx -= dtV->vx;
            dtL->vz -= dtV->vz;
            dtV->vz = 0;
            dtV->vx = 0;
        }
        return;
    }
    if (motID == MOT_SQUAT_BACKFLIP)
    {
        return;
    }
    if (motID != MOT_SQUAT && (dtV->vx != 0 || dtV->vz != 0))
    {
        if (__builtin_abs(GetAreaMapLevel(GlobalAreaMap,
                                          dtL->vx + dtV->vx * 16,
                                          dtL->vy,
                                          dtL->vz + dtV->vz * 16,
                                          AREA_LEVEL_STEP_DOWN |
                                              AREA_LEVEL_RETURN_DELTA)) >= 500)
        {
            dtV->vz = 0;
            dtV->vx = 0;
        }
    }

    if (dtCMD != CMD_NONE)
    {
        switch (dtCMD)
        {
        case CMD_ROLL_FORWARD:
            SET_MOTION(MOT_SQUAT_ROLL_F, MOTION_MOVE_APPLY);
            break;
        case CMD_ROLL_BACKWARD:
            SET_MOTION(MOT_SQUAT_ROLL_B, MOTION_MOVE_APPLY);
            break;
        case CMD_ROLL_LEFT:
            SET_MOTION(MOT_SQUAT_ROLL_L, MOTION_MOVE_APPLY);
            break;
        case CMD_ROLL_RIGHT:
            SET_MOTION(MOT_SQUAT_ROLL_R, MOTION_MOVE_APPLY);
            break;
        }
        return;
    }

    if (Me_MOTION_C->pad.trig & PADRleft)
    {
        AttackControl();
        return;
    }
    if (Me_MOTION_C->pad.trig & PADRup)
    {
        SELECT_ITEM_USE_MOTION(item_sound, item_default);
        motMODE = MOTION_MOVE_APPLY;
        return;

    item_sound:
        SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
        return;

    item_default:
        ReqItemDefault(Me_MOTION_C, (TItemType)SelectedItem);
        return;
    }

    if ((dtPAD & PADRright) == 0)
    {
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_NORMAL);
        }
        if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
            return;
        }
        SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        return;
    }
    if (PlayerSSR != 0)
    {
        StickonCheck();
    }
    return;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSTICKON(void);
 *     MOTION.C:1799, 101 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v1       struct MapVector * map
 *     reg   $s0       struct ModelArchiveType * model
 *     reg   $a2       short y
 *     reg   $s2       short rv
 *     reg   $s1       short pd
 *     stack sp+16     struct SVECTOR vect
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct SVECTOR *dtR;
 *     extern short RefrectVector[16];
 *     extern short dtCMD;
 *     extern short motID;
 *     extern short motMODE;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtV;
 *     extern struct TCameraStatus CamState;
 *     extern short SelectedItem;
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

void ActSTICKON(void)
{
    MapVector *map;
    ModelArchiveType *model;
    short y;
    short rv;
    short pd;
    short t;

    model = Me_MOTION_C->model;
    switch (dtM->mid)
    {
    case MOT_STICKON:
        if (dtM->count < 0)
        {
            MotionElementType *rotation;
            SVECTOR vect;
            u16 reflected_raw;
            s32 reflected;
            s32 wall_y;

            map = StickonCheck();
            if (map == 0)
            {
                SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
                dtM->mask = MOTION_MASK_ALL;
                return;
            }

            wall_y = dtR->vy & ANGLE_QUADRANT_MASK;
            if (dtR->vy & ANGLE_HALF_QUADRANT)
            {
                wall_y += ANGLE_QUADRANT;
            }
            reflected_raw = RefrectVector[map->vector] - wall_y;
            t = wall_y;
            rv = reflected_raw;
            reflected = (s16)reflected_raw;
            if (reflected == 0)
            {
                dtR->vy += ANGLE_HALF;
            }
            if (__builtin_abs(reflected) > ANGLE_HALF)
            {
                if (reflected > 0)
                {
                    reflected_raw = reflected - ANGLE_FULL;
                }
                else
                {
                    reflected_raw = reflected + ANGLE_FULL;
                }
                rv = reflected_raw;
            }

            dtR->vy += (t - dtR->vy) / -dtM->count;
            dtM->motion->rotate[MODEL_PART_WAIST]->y = rv;
            rotation = dtM->motion->rotate[MODEL_PART_HEAD];
            if (rv & ANGLE_QUADRANT)
            {
                rotation->y = -rv;
            }
            else
            {
                rotation->y = 0;
            }
            GetMoveSpeed(&vect, rv, -300, 0);
            dtM->motion->locate->x = vect.vx;
            dtM->motion->locate->z = vect.vz;
        }
        else if (dtM->loop > 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }

        if (dtCMD != CMD_NONE)
        {
            switch (dtCMD)
            {
            case CMD_ROLL_FORWARD:
                SET_MOTION(MOT_SQUAT_ROLL_F, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_BACKWARD:
                SET_MOTION(MOT_SQUAT_ROLL_B, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_LEFT:
                SET_MOTION(MOT_SQUAT_ROLL_L, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_RIGHT:
                SET_MOTION(MOT_SQUAT_ROLL_R, MOTION_MOVE_APPLY);
                break;
            }

            if ((s8)MOTION_STATUS(motID) == STAT_SQUAT)
            {
                dtM->mask = MOTION_MASK_ALL;
                SET_NOW_MOTION_UNLESS_CVA(goto stickon_motion_done);
            stickon_motion_done:
                dtM->count = -5;
                break;
            }
        }

        if (dtM->loop != MOTION_LOOP_DISABLED)
        {
            break;
        }

        {
            if ((dtPAD & (PADLleft | PADLdown | PADLright | PADLup)) != 0)
            {
                pd = 0;
                rv = model->object[MODEL_PART_WAIST]->rotate.vy >> 10 & 3;
                if (((dtPAD >> 12) & 1) == 0)
                {
                    do
                    {
                        pd++;
                    } while (((dtPAD >> (pd + 12)) & 1) == 0);
                }
                if (rv != ((pd + 2) & 3))
                {
                    MotionManager *update_motion;

                    update_motion = dtM;
                    y = MOT_STICKON_SLIDE_R;
                    if (rv == ((pd + 1) & 3))
                    {
                        y = MOT_STICKON_SLIDE_L;
                    }
                    UpdateMotion(update_motion, y);
                    Me_MOTION_C->status = STAT_STICKON;
                    dtV->vz = 0;
                    dtV->vx = 0;
                    dtM->mask = MOTION_MASK_NOROOT;
                    model->object[MODEL_PART_WAIST]->rotate.vx = -0x69;
                    UpdateCoordinate(model->object[MODEL_PART_WAIST]);
                }
                break;
            }
        }

        if ((Me_MOTION_C->pad.trig & PADRup) != 0)
        {
            rv = model->object[MODEL_PART_WAIST]->rotate.vy >> 10 & 3;
            pd = 0;
            switch ((u32)CamState.Mode)
            {
            case CMODE_STICK_L:
                if (rv == 2)
                {
                    pd = MOT_STICKON_THROW_L;
                }
                break;
            case CMODE_PEEP_L:
                if (rv == 3)
                {
                    pd = MOT_STICKON_THROW_L;
                }
                break;
            case CMODE_STICK_R:
                if (rv == 2)
                {
                    pd = MOT_STICKON_THROW_R;
                }
                break;
            case CMODE_PEEP_R:
                if (rv == 1)
                {
                    pd = MOT_STICKON_THROW_R;
                }
                break;
            }

            {
                s32 selected_item;
                s32 high_item;

                selected_item = SelectedItem;
                high_item = selected_item;
                StickonItem = selected_item;
                if (selected_item <= ITEM_SMOKE)
                {
                    if (selected_item < ITEM_FIRE && selected_item != ITEM_MAKIBISHI)
                    {
                        pd = 0;
                    }
                }
                else if (high_item != ITEM_DOKUDANGO)
                {
                    pd = 0;
                }

                if (pd != 0)
                {
                    motMODE = MOTION_MOVE_APPLY;
                    motID = pd;
                    dtM->mask = MOTION_MASK_NOROOT;
                }
                else
                {
                    SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
                }
            }
        }
        break;

    case MOT_STICKON_SLIDE_L:
    case MOT_STICKON_SLIDE_R:
    {
        if (dtCMD != CMD_NONE)
        {
            switch (dtCMD)
            {
            case CMD_ROLL_FORWARD:
                SET_MOTION(MOT_SQUAT_ROLL_F, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_BACKWARD:
                SET_MOTION(MOT_SQUAT_ROLL_B, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_LEFT:
                SET_MOTION(MOT_SQUAT_ROLL_L, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_RIGHT:
                SET_MOTION(MOT_SQUAT_ROLL_R, MOTION_MOVE_APPLY);
                break;
            }

            if ((s8)MOTION_STATUS(motID) == STAT_SQUAT)
            {
                dtM->mask = MOTION_MASK_ALL;
                SET_NOW_MOTION_UNLESS_CVA(goto slide_motion_done);
            slide_motion_done:
                dtM->count = -5;
                break;
            }
        }

        if (dtM->count < 0)
        {
            break;
        }

        if ((dtPAD & (PADLleft | PADLdown | PADLright | PADLup)) == 0)
        {
            goto slide_no_pad;
        }

        rv = model->object[MODEL_PART_WAIST]->rotate.vy >> 10 & 3;
        pd = 0;
        if (((dtPAD >> 12) & 1) == 0)
        {
            do
            {
                pd++;
            } while (((dtPAD >> (pd + 12)) & 1) == 0);
        }
        if (rv == ((pd + 2) & 3))
        {
            break;
        }
        t = MOT_STICKON_SLIDE_R;
        if (rv == ((pd + 1) & 3))
        {
            t = MOT_STICKON_SLIDE_L;
        }
        if (motID != t)
        {
            UpdateMotion(dtM, t);
        }

        if (dtPAD & PADLup)
        {
            MoveHumanoid(Me_MOTION_C, 30, 0);
        }
        else if (dtPAD & PADLdown)
        {
            MoveHumanoid(Me_MOTION_C, -30, 0);
        }
        else if (dtPAD & PADLleft)
        {
            MoveHumanoid(Me_MOTION_C, 0, 30);
        }
        else if (dtPAD & PADLright)
        {
            MoveHumanoid(Me_MOTION_C, 0, -30);
        }

        y = model->object[MODEL_PART_WAIST]->rotate.vy + dtR->vy;
        y &= ANGLE_MASK;
        dtL->vx += dtV->vx;
        dtL->vz += dtV->vz;
        map = StickonCheck();
        if (y != RefrectVector[map->vector])
        {
            if (rv == pd)
            {
                dtPAD = 0;
            }
            else
            {
                dtL->vx -= dtV->vx;
                dtL->vz -= dtV->vz;
                UpdateMotion(dtM, MOT_STICKON);
                dtM->loop = MOTION_LOOP_DISABLED;
                dtM->mask = MOTION_MASK_ALL;
            }
        }
        dtV->vz = 0;
        dtV->vx = 0;
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        break;

    slide_no_pad:
        SET_MOTION(MOT_STICKON, MOTION_MOVE_APPLY);
        dtM->mask = MOTION_MASK_ALL;
        break;
    }

    case MOT_STICKON_THROW_L:
    case MOT_STICKON_THROW_R:
    {
        VECTOR *position;
        PARAM_ITEM_LAUNCH item;
        s32 angle;

        if (dtM->count != 0 || dtM->loop == 0)
        {
            return;
        }

        pd = motID != MOT_STICKON_THROW_L;
        {
            s32 base_angle_value;

            /* Narrow only after selecting the base angle. */
            base_angle_value =
                (s16)(model->object[MODEL_PART_WAIST]->rotate.vy + dtR->vy);
            angle = (pd ? base_angle_value - ANGLE_QUADRANT
                        : base_angle_value + ANGLE_QUADRANT) &
                    0xF00;
        }
        item.user = Me_MOTION_C;
        item.type = StickonItem;
        Me_MOTION_C->item[StickonItem]--;
        position = GetAbsolutePosition(
            Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0 + pd],
            0, 0, 0);
        angle = (s16)angle;
        position->vx -= (rsin(angle) * 500) >> FIXED_SHIFT;
        position->vz -= (rcos(angle) * 500) >> FIXED_SHIFT;
        item.start.vx = position->vx;
        item.start.vy = position->vy;
        item.start.vz = position->vz;

        if (pd != 0)
        {
            angle -= ANGLE_HALF_QUADRANT;
        }
        else
        {
            angle += ANGLE_HALF_QUADRANT;
        }

        if (item.type == ITEM_MAKIBISHI)
        {
            s32 next_angle;

            for (t = 0; t < 5; t++)
            {
                next_angle = angle - 10;
                next_angle += rand() % 20;
                angle += next_angle - angle;
                /* Empty loop retained for code layout; its original source construct is unknown. */
                do
                {
                } while (0);
                y = next_angle;
                item.end.vx =
                    (rsin(y) * (-30 - rand() % 200)) >> FIXED_SHIFT;
                item.end.vy = rand();
                item.end.vy = -(item.end.vy % 30);
                item.end.vz =
                    (rcos(y) * (-30 - rand() % 200)) >> FIXED_SHIFT;
                ReqItemMakibishi((PARAM_ITEM_DROP *)&item);
            }
        }
        else
        {
            y = angle;
            item.end.vx = (rsin(y) * -120) >> FIXED_SHIFT;
            item.end.vy = 0;
            item.end.vz = (rcos(y) * -120) >> FIXED_SHIFT;
            switch (item.type)
            {
            case ITEM_FIRE:
                ReqItemFire(&item);
                break;
            case ITEM_SMOKE:
                ReqItemSmoke(&item);
                break;
            case ITEM_DOKUDANGO:
                ReqItemDokudango(&item);
                break;
            }
        }
        SET_MOTION(MOT_STICKON, MOTION_MOVE_APPLY);
        dtM->mask = MOTION_MASK_ALL;
        return;
    }

    default:
        break;
    }
    if ((dtPAD & PADRright) == 0)
    {
        dtM->mask = MOTION_MASK_ALL;
        SELECT_RETURN_MOTION();
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSYURI(void);
 *     MOTION.C:1908, 37 src lines, frame 64 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v0       struct VECTOR * p
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

void ActSYURI(void)
{
    VECTOR *p;
    PARAM_ITEM_LAUNCH item;

    switch (dtM->mid)
    {
    case MOT_SYURI:
        if (Me_MOTION_C != StagePlayer)
        {
            if (dtM->count != 0)
                return;
            if (dtM->loop == 0)
                return;
            SET_MOTION(MOT_SYURI_RECOVER, MOTION_MOVE_APPLY);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        if (dtM->count == 1)
        {
            item.type = ITEM_SHURIKEN;
            item.user = Me_MOTION_C;
            p = GetAbsolutePosition(
                Me_MOTION_C->model->object[MODEL_PART_HEAD], 0, 0, 0);
            item.start.vx = p->vx;
            item.start.vy = p->vy;
            item.start.vz = p->vz;
            item.end = item.start;
            ReqItemUse(&item);
            Sound(Me_MOTION_C, SE_THROW_WEAPON);
        }
        else if (spare_item_slot_(SPARE_ITEM_SLOT_QUERY, Me_MOTION_C) == 0)
        {
            SET_MOTION(MOT_SYURI_RECOVER, MOTION_MOVE_APPLY);
            Sound(Me_MOTION_C, SE_WEAPON_RECOVER);
        }
        else if (Me_MOTION_C->pad.trig & (PADRleft | PADRdown | PADRright))
        {
            spare_item_slot_(SPARE_ITEM_SLOT_CLEAR, 0);
            SELECT_RETURN_MOTION();
        }
        break;
    case MOT_SYURI_RECOVER:
        if (dtM->count == 1 && Me_MOTION_C != StagePlayer)
        {
            ReqItemDefault(Me_MOTION_C, ITEM_SHURIKEN);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SELECT_RETURN_MOTION();
        }
        break;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActITEM(void);
 *     MOTION.C:1949, 36 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     reg   $a1       short flag
 *     reg   $a0       short mode
 *     reg   $v0       struct VECTOR * p
 * END PSX.SYM */

void ActITEM(void)
{
    VECTOR *p;
    s16 mode;
    s16 flag;
    PARAM_ITEM_LAUNCH item;

    flag = 0;
    switch (dtM->mid)
    {
    case MOT_ITEM: /* scatter makibishi */
        if (dtM->count != 10)
            break;
        flag = 1;
        item.type = ITEM_MAKIBISHI;
        break;

    case MOT_ITEM_THROW: /* throw (fire/smoke/nemuri) */
        if (dtM->count != 5)
            break;
        mode = ITEM_FIRE;
        if (Me_MOTION_C == StagePlayer)
            mode = SelectedItem;
        if (mode == ITEM_FIRE)
        {
            flag = 1;
            item.type = mode;
        }
        else if (mode == ITEM_SMOKE)
        {
            flag = 1;
            item.type = mode;
        }
        break;

    case MOT_ITEM_PLANT: /* plant (jirai/goshikimai) */
        if (dtM->count != 5)
            break;
        flag = 1;
        item.type = ITEM_JIRAI;
        break;

    case MOT_ITEM_KAENGEKI: /* kaengeki flame */
    case MOT_ITEM_SHINSOKU: /* shinsoku cast */
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        dtM->loop = MOTION_LOOP_DISABLED;
        return;
    }

    if (flag)
    {
        item.user = Me_MOTION_C;
        p = GetAbsolutePosition(
            Me_MOTION_C->model->object[MODEL_PART_HEAD], 0, 0, 0);
        item.start.vx = p->vx;
        item.start.vy = p->vy;
        item.start.vz = p->vz;
        item.end = item.start;
        ReqItemUse(&item);
    }

    if (dtM->count == 0 && dtM->loop != 0)
    {
        SELECT_RETURN_MOTION();
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActDAMAGE(void);
 *     MOTION.C:1989, 53 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a2       struct OrnamentType ** weapon
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct SVECTOR *dtV;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

void ActDAMAGE(void)
{
    short done;

    done = false;
    switch (dtM->mid)
    {
    case MOT_DAMAGE_LAUNCH_BACK:
    {
        if (dtM->count == 1)
        {
            ShowHumanoidBodyParts(Me_MOTION_C);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        else
        {
            dtV->vy = (dtM->count - dtM->motion->time) * 6;
        }
        if ((Me_MOTION_C->attribute & ATTR_NOFLOOR) ||
            Me_MOTION_C->map.height < 0)
        {
            SET_MOTION(MOT_DAMAGE_SLAM_BACK, MOTION_MOVE_NONE);
        }
        if (dtM->count & 4)
            SetBlood(dtL, 1, 60);
        break;
    }

    case MOT_DAMAGE_LAUNCH_FORE:
    {
        if (dtM->count == 1)
        {
            ShowHumanoidBodyParts(Me_MOTION_C);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        else
        {
            dtV->vy = (dtM->count - dtM->motion->time) * 6;
        }
        if ((Me_MOTION_C->attribute & ATTR_NOFLOOR) ||
            Me_MOTION_C->map.height < 0)
        {
            SET_MOTION(MOT_DAMAGE_SLAM_FORE, MOTION_MOVE_NONE);
        }
        if (dtM->count & 4)
            SetBlood(dtL, 1, 60);
        break;
    }

    case MOT_DAMAGE_SLAM_BACK:
    case MOT_DAMAGE_SLAM_FORE:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_BODY_SLAM);
            spawn_smoke_burst_(dtL, 500, 30, 30);
            if (StagePlayer == Me_MOTION_C)
                PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_LONG);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_DAMAGE_DOWNED, MOTION_MOVE_APPLY);
            break;
        }
        dtV->vx -= (dtV->vx >> 2);
        dtV->vz -= (dtV->vz >> 2);
        break;

    case MOT_DAMAGE_DOWNED:
        if (Me_MOTION_C->life == 0)
        {
            dtM->loop = 0;
            dtM->count = 0;
            PlayMotion(dtM, 1);
            dtM->loop = MOTION_LOOP_FROZEN;
            Me_MOTION_C->status = STAT_DEAD;
            Me_MOTION_C->attribute &= ~ATTR_SEARCH;
            dtV->vx = dtV->vy = dtV->vz = 0;
            if (Me_MOTION_C == StagePlayer)
                return;
            DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WAIST]);
            TurnAroundAllItems(Me_MOTION_C);
            return;
        }
        dtM->loop--;
        if (Me_MOTION_C->life - Me_MOTION_C->lifemax >= dtM->loop)
        {
            SET_MOTION(MOT_DAMAGE_GETUP, MOTION_MOVE_APPLY);
        }
        break;

    case MOT_DAMAGE_MAKIBISHI:
    case MOT_DAMAGE_CHOKE:
    case MOT_DAMAGE_GETUP:
        if (dtM->count == 0 && dtM->loop != 0)
            done = true;
        break;

    default:
    {
        SVECTOR *velocity;
        int value;

        velocity = dtV;
        value = velocity->vx;
        if (value != 0)
        {
            if (value > 0)
                value -= 4;
            else
                value += 4;
            velocity->vx = value;
        }
        velocity = dtV;
        value = velocity->vz;
        if (value != 0)
        {
            if (value > 0)
                value -= 4;
            else
                value += 4;
            velocity->vz = value;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            OrnamentType **weapon;
            if (Me_MOTION_C->wpatk != KATANAL)
            {
                done = true;
                break;
            }
            done = true;
            weapon = Me_MOTION_C->weapon;
            if (weapon[WEAPON_SLOT_INACTIVE_1] != NULL)
            {
                weapon[WEAPON_SLOT_INACTIVE_0] = weapon[WEAPON_SLOT_ACTIVE_0];
                weapon[WEAPON_SLOT_ACTIVE_0] = weapon[WEAPON_SLOT_INACTIVE_1];
                weapon[WEAPON_SLOT_INACTIVE_1] = NULL;
                Sound(Me_MOTION_C, CHAR_SE_WEAPON_CHANGE_B);
            }
        }
        break;
    }
    }
    if (done)
    {
        if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
        {
            motID = MOT_ENGAGE_STANCE;
            motMODE = MOTION_MOVE_APPLY;
            Me_MOTION_C->attribute =
                (Me_MOTION_C->attribute & (u16)~ATTR_PHASE) | PHASE_ALERT;
        }
        else
        {
            SET_MOTION(MOT_STATE_DRAW, MOTION_MOVE_APPLY);
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActDEAD(void);
 *     MOTION.C:2046, 89 src lines, frame 64 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       struct VECTOR * pp
 *     stack sp+16     struct VECTOR p
 *     stack sp+32     struct SVECTOR v
 *     reg   $s2       short bldo
 *     reg   $s3       short blds
 *     reg   $s0       short blood
 * END PSX.SYM */

enum death_event_action
{
    DEATH_EVENT_SOUND_PLAYER = 0,
    DEATH_EVENT_SOUND_VICTIM = 1,
    DEATH_EVENT_RUMBLE = 2,
    DEATH_EVENT_GORE = 3,
    DEATH_EVENT_END = 4
};

/* Each death-script opcode gives the last two halfwords a different meaning.
 * DEATH_EVENT_END uses the gore payload too; a model_part of -1 makes it a
 * sentinel-only row. local_velocity packs Y in the low byte and Z in the
 * high byte and is unpacked into the SVECTOR below. */
typedef union
{
    struct
    {
        s16 sound_id;
        s16 unused;
    } sound;
    struct
    {
        s16 attack;
        s16 release;
    } rumble;
    struct
    {
        s16 model_part;
        u16 local_velocity;
    } gore;
} DeathEventPayload;

typedef struct
{
    s16 frame;
    s16 action; /* enum death_event_action in halfword table storage */
    DeathEventPayload payload;
} DeadEvent;

#define DEATH_GORE_VELOCITY_Y(velocity) ((velocity) & 0xff)
#define DEATH_GORE_VELOCITY_Z(velocity) ((velocity) >> 8)

extern DeadEvent *DeadEvents[N_STEALTH_DEATH_MOTIONS];
void ActDEAD(void)
{
    ModelArchiveType *model;
    short blood;
    short bldo;
    short blds;
    short i;
    motion_id mid;
    DeadEvent *pp;
    VECTOR p;
    SVECTOR v;

    model = Me_MOTION_C->model;
    blood = -1;
    if ((*model->object)->id < 0 && dtM->loop < 0)
        return;

    if (dtM->count != 0 || dtM->loop != 0)
    {
        if (dtM->count == 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        if (dtM->loop < 0 && dtV->vy == 0)
        {
            MotionManager *motion;
            Humanoid *human;
            SVECTOR *velocity;

            motion = dtM;
            motion->count = motion->motion->time;
            motion->loop = 0;
            PlayMotion(motion, 1);
            dtM->loop = MOTION_LOOP_DISABLED;
            if (motID != MOT_DEAD_DROWN)
            {
                Me_MOTION_C->attribute &= ~ATTR_SEARCH;
            }
            else
            {
                Me_MOTION_C->attribute |= ATTR_SEARCH; /* drowned */
                Me_MOTION_C->model->attribute |= MODEL_ATTR_HIDDEN;
            }

            velocity = dtV;
            human = Me_MOTION_C;
            velocity->vz = 0;
            velocity->vx = 0;
            if (human != StagePlayer)
            {
                DeleteConflict(*model->object);
                if ((Me_MOTION_C->type & PAGE_MASK) != PAGE_BOSS)
                    TurnAroundAllItems(Me_MOTION_C);
            }
            if (dtM->mid < MOT_DEAD_STEALTH_BACK)
                return;
            ActionHalt = ACTION_HALT_NONE;
            CamState.snap_pending = 1;
            return;
        }
    }

    if (dtM->mid > MOT_DEAD_DROWN && dtL->vy != StagePlayer->locate->vy)
    {
        dtL->vy--;
        motID = MOT_DEAD;
        ActionHalt = ACTION_HALT_NONE;
        motMODE = MOTION_MOVE_APPLY;
        if (dtM->count >= 10)
            return;
        PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_SHORT);
        Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
        Sound(StagePlayer, CHAR_SE_SPECIAL);
        return;
    }

    mid = dtM->mid;
    if (mid == MOT_DEAD_DROWN)
        goto splash_dead;
    if (mid < MOT_DEAD_DROWN)
        goto ordinary_dead;
    if (mid > MOT_DEAD_STEALTH_SIDE_AYAME)
        goto ordinary_dead;
    goto event_dead;

splash_dead:
{
    if (rand() % 20 == 0)
        Sound(Me_MOTION_C, SE_WATER_SPLASH);
    p.vy = Me_MOTION_C->map.level;
    if ((rand() & 5) == 0)
    {
        i = 0;
        do
        {
            long width;
            int r;

            r = rand();
            width = Me_MOTION_C->width;
            p.vx = dtL->vx + (r % width) * 2 - width;
            r = rand();
            width = Me_MOTION_C->width;
            p.vz = dtL->vz + (r % width) * 2 - width;
            SetSplash(&p, (rand() & 7) << FIXED_SHIFT,
                      (rand() & 7) << FIXED_SHIFT, 6);
            i++;
        } while (i < 5);
    }
    goto blood_effect;
}

event_dead:
{
    MotionManager *motion;
    int count;
    int stop;

    motion = dtM;
    pp = DeadEvents[motion->mid - MOT_DEAD_STEALTH_BACK];
    i = 0;
    if (pp[i].action == DEATH_EVENT_END)
        goto event_ready;
    count = motion->count;
    stop = DEATH_EVENT_END;
scan_event:
    if (pp[i].frame == count)
        goto event_ready;
    i++;
    if (pp[i].action != stop)
        goto scan_event;
event_ready:
    if (dtM->count < pp[i].frame)
        return;

    switch (pp[i].action)
    {
    case DEATH_EVENT_SOUND_PLAYER:
        Sound(StagePlayer, pp[i].payload.sound.sound_id);
        break;
    case DEATH_EVENT_SOUND_VICTIM:
        Sound(Me_MOTION_C, pp[i].payload.sound.sound_id);
        break;
    case DEATH_EVENT_RUMBLE:
        PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX,
                   pp[i].payload.rumble.attack,
                   pp[i].payload.rumble.release);
        break;
    case DEATH_EVENT_GORE:
    case DEATH_EVENT_END:
    {
        u16 packed;

        ReqLifeBar(Me_MOTION_C);
        blood = pp[i].payload.gore.model_part;
        packed = pp[i].payload.gore.local_velocity;
        bldo = DEATH_GORE_VELOCITY_Z(packed);
        blds = DEATH_GORE_VELOCITY_Y(packed);
        break;
    }
    }
    goto blood_effect;
}

#undef DEATH_GORE_VELOCITY_Z
#undef DEATH_GORE_VELOCITY_Y

ordinary_dead:
    if ((Me_MOTION_C->type & PAGE_MASK) != PAGE_BEAST)
    {
        if (dtM->count == 5 && DeadHumanoid == Me_MOTION_C)
        {
            Sound(DeadHumanoid, SE_DEATH);
            DeadHumanoid = 0;
        }
        blood = 1;
        bldo = 100;
        blds = 0;
    }

blood_effect:
    if ((dtM->count & 4) && blood != -1)
    {
        SVECTOR gore_position = {
            .vx = 0,
            .vy = -200,
            .vz = -240
        };
        SVECTOR gore_velocity = {
            .vx = 0,
            .vy = -blds,
            .vz = -bldo
        };

        if (blds == 0)
        {
            gore_position.vx = 0;
            gore_position.vy = -200;
            gore_position.vz = -240;
        }
        else
        {
            gore_position.vx = 0;
            gore_position.vy = -410;
            gore_position.vz = 0;
        }
        SetGore(&Me_MOTION_C->model->object[blood]->locate,
                &gore_position, &gore_velocity);
    }
}

/* An empty function kept at its original slot — likely a stubbed-out debug/screen hook; nothing references it. */
void nop_26f4c_(void)
{
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SetNowMotion(struct Humanoid *human, short mid, short move);
 *     MOTION.C:188, 7 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short mid
 *     param $a2       short move
 * END PSX.SYM */

short SetNowMotion(Humanoid *human, motion_id mid, motion_move_mode move)
{
    if (human->status == STAT_DEAD &&
        human->motion->loop == MOTION_LOOP_DISABLED)
    {
        return 0;
    }
    if (UpdateMotion(human->motion, mid) == 0)
    {
        return 0;
    }
    human->status = (s8)MOTION_STATUS(mid);
    if (move != MOTION_MOVE_NONE)
    {
        MoveHumanoid(human, human->motion->motion->orderspd,
                     human->motion->motion->sidespd);
    }
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short NowReturnNormal(struct Humanoid *human);
 *     MOTION.C:200, 6 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

short NowReturnNormal(Humanoid *human)
{
    Humanoid *current;
    MotionDataType *motion;
    motion_id next_motion;
    motion_move_mode apply_movement;

    Me_MOTION_C = human;
    ReturnNormal();
    current = Me_MOTION_C;
    next_motion = motID;
    apply_movement = motMODE;
    if (current->status == STAT_DEAD &&
        current->motion->loop == MOTION_LOOP_DISABLED)
    {
        return 0;
    }
    if (UpdateMotion(current->motion, next_motion) == 0)
    {
        return 0;
    }
    current->status = (s8)MOTION_STATUS(next_motion);
    if (apply_movement != MOTION_MOVE_NONE)
    {
        motion = current->motion->motion;
        MoveHumanoid(current, motion->orderspd, motion->sidespd);
    }
    return 1;
}

void dispose_weapon_data_of_char_(Humanoid *h, int mode)
{
    Me_MOTION_C = h;
    dtM = h->motion;
    AttackCancelControl(mode);
}

void set_model_hide_(Humanoid *human, s16 hide)
{
    ModelArchiveType *model;
    s16 last;
    s16 i;

    model = human->model;
    if (model->n > MODEL_PART_BODY_LAST)
    {
        last = MODEL_PART_BODY_LAST;
    }
    else
    {
        last = model->n - 1;
    }
    if (hide != 0)
    {
        HIDE_HUMANOID_BODY_PARTS(model, last, i);
        return;
    }
    SHOW_HUMANOID_BODY_PARTS(model, last, i);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short MotionAndMove(void);
 *     MOTION.C:173, 11 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

short MotionAndMove(void)
{
    short i;
    short result;

    if (MotionUpdateMode != 0)
    {
        i = 0;
        do
        {
            if (CVAhuman[i].human == Me_MOTION_C)
            {
                return 0;
            }
            i++;
        } while (i < N_CVA_HUMANS);
    }
    result = SetNowMotion(Me_MOTION_C, motID, motMODE);
    motMODE = MOTION_MOVE_UNSET;
    return result;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ReturnNormal(void);
 *     MOTION.C:210, 4 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

void ReturnNormal(void)
{
    SELECT_RETURN_MOTION();
}

void publish_ground_point_(void)
{
    s32 id;

    id = (*Me_MOTION_C->model->object)->id;
    if (id >= 0)
    {
        dtL->vx = ConflictObject[id].position.vx;
        dtL->vz = ConflictObject[id].position.vz;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackCancelControl(short mode);
 *     MOTION.C:726, 24 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short mode
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

void AttackCancelControl(s16 mode)
{
    ClearAttackEffects(mode);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackGunControl(short length, short frm);
 *     MOTION.C:832, 13 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short length
 *     param $a1       short frm
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 * END PSX.SYM */

void AttackGunControl(s16 length, s16 frm)
{
    PARAM_ITEM_LAUNCH item;

    if (dtM->count == frm)
    {
        bow_shoot_logic(
            ITEM_GUN,
            GetAbsolutePosition(
                Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0], 0,
                length, -100));
        Sound(Me_MOTION_C, CHAR_SE_ATTACK);
    }
}

void bow_shoot_logic(s16 kind, VECTOR *start)
{
    PARAM_ITEM_LAUNCH p;
    SVECTOR move;
    s16 dist;
    s32 rot;
    s16 speed;

    p.type = kind;
    p.user = Me_MOTION_C;
    p.start.vx = start->vx;
    p.start.vy = start->vy;
    p.start.vz = start->vz;
    dist = GetTargetDistance(Me_MOTION_C, 0);
    move.pad = dist;
    rot = dtR->vy;
    speed = dist;
    if (dist < 1000)
    {
        speed = 1000;
    }
    GetMoveSpeed(&move, rot, speed, 0);
    p.end.vx = p.start.vx + move.vx;
    p.end.vy = Me_MOTION_C->target->coord.t[1];
    p.end.vz = p.start.vz + move.vz;
    if (kind == ITEM_ARROW)
    {
        if (rand() % (EngageLevel + 1) != 0)
        {
            p.end.vy -= 1000;
        }
    }
    ReqItemUse(&p);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackPQD(short sfrm, short efrm);
 *     MOTION.C:849, 23 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short sfrm
 *     param $a1       short efrm
 * END PSX.SYM */

void AttackPQD(s16 sfrm, s16 efrm)
{
    Humanoid *human;
    s16 count;
    OrnamentType **weapons;
    OrnamentType *held;
    OrnamentType *stowed;
    s32 seid;

    human = Me_MOTION_C;
    count = dtM->count;
    weapons = human->weapon;
    if (count == efrm || efrm == MOTION_FRAME_ANY)
    {
        if (weapons[WEAPON_SLOT_INACTIVE_1] == 0)
            return;
        seid = CHAR_SE_WEAPON_CHANGE_B;
        held = (weapons[WEAPON_SLOT_INACTIVE_0] = human->weapon[WEAPON_SLOT_ACTIVE_0]);
        stowed = weapons[WEAPON_SLOT_INACTIVE_1];
        human->weapon[WEAPON_SLOT_ACTIVE_0] = stowed;
        weapons[WEAPON_SLOT_INACTIVE_1] = 0;
    }
    else
    {
        if (count != sfrm)
            return;
        if (weapons[WEAPON_SLOT_INACTIVE_0] == 0)
            return;
        seid = CHAR_SE_WEAPON_CHANGE_A;
        held = human->weapon[WEAPON_SLOT_ACTIVE_0];
        weapons[WEAPON_SLOT_INACTIVE_1] = held;
        human->weapon[WEAPON_SLOT_ACTIVE_0] = weapons[WEAPON_SLOT_INACTIVE_0];
        weapons[WEAPON_SLOT_INACTIVE_0] = 0;
    }
    Sound(human, seid);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackFire(short sfrm, short efrm);
 *     MOTION.C:876, 16 src lines, frame 72 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short sfrm
 *     param $a1       short efrm
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     stack sp+56     struct SVECTOR vect
 * END PSX.SYM */

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
            Sound(Me_MOTION_C, SE_FIRE);
        }
        item.type = ITEM_NAPALM;
        item.user = Me_MOTION_C;
        start_pos = GetAbsolutePosition(
            Me_MOTION_C->model->object[MODEL_PART_HEAD], 0, -100, -300);
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ItemControl(void);
 *     MOTION.C:896, 13 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 * END PSX.SYM */

void ItemControl(void)
{
    SELECT_ITEM_USE_MOTION(item_sound, item_default);
    motMODE = MOTION_MOVE_APPLY;
    return;

item_sound:
    SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
    return;

item_default:
    ReqItemDefault(Me_MOTION_C, SelectedItem);
}

/* The ceiling-hang state has no per-frame work; the handler slot exists so
 * the Act table stays fully populated. */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActCEILHANG(void);
 *     MOTION.C:1903, 1 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 * END PSX.SYM */

void ActCEILHANG(void)
{
}
