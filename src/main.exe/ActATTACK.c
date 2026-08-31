#include "common.h"
#include "sound.h"
#define DeleteConflict DeleteConflict_prototype
#include "main.exe.h"
#undef DeleteConflict
#include "appear.h"
#include "humanoid.h"
#include "item.h"
#include "afterimage.h"

/* Twin-katana second-blade juggling: slot 0 is the blade in hand, slots
 * 2/3 park the other one. On the motion's draw frame the parked blade
 * moves into the hand (draw sound), on the stow frame it goes back
 * (sheathe sound). Retail copy-pastes this block into each dual-wield
 * attack case with different frame numbers; the macro is reconstruction
 * shorthand for that copy-paste (expands to the identical text). */
#define SWAP_TWIN_BLADE(draw_frame, stow_frame)                               \
    if (dtM->count == (draw_frame))                                           \
    {                                                                         \
        if (weapon[3] != 0)                                                   \
        {                                                                     \
            weapon[2] = weapon[0];                                            \
            weapon[0] = weapon[3];                                            \
            weapon[3] = 0;                                                    \
            Sound(Me_MOTION_C, 1);                                            \
        }                                                                     \
    }                                                                         \
    else if ((dtM->count == (stow_frame)) && (weapon[2] != 0))                \
    {                                                                         \
        weapon[3] = weapon[0];                                                \
        weapon[0] = weapon[2];                                                \
        weapon[2] = 0;                                                        \
        Sound(Me_MOTION_C, 0);                                                \
    }

/* End-of-attack weapon cleanup: drop the striking-limb conflict boxes
 * for the weapon class and dispose both afterimage trails. Retail
 * copy-pastes this block three times; the macro is reconstruction
 * shorthand (expands to the identical text; `kind` and `cleanup_guard`
 * are each site's locals). */
#define DELETE_WEAPON_CONFLICTS_AND_AFTERIMAGES()                             \
    kind = Me_MOTION_C->wpatk;                                                \
    switch (kind)                                                             \
    {                                                                         \
    case WEP_ONININ:                                                          \
        DeleteConflict(Me_MOTION_C->model->object[8]);                        \
        DeleteConflict(Me_MOTION_C->model->object[0xb]);                      \
        cleanup_guard = 3;                                                    \
        break;                                                                \
    case WEP_BEAST:                                                           \
        DeleteConflict(Me_MOTION_C->model->object[2]);                        \
        cleanup_guard = 3;                                                    \
        break;                                                                \
    case WEP_NONE:                                                            \
        cleanup_guard = 3;                                                    \
        break;                                                                \
    default:                                                                  \
        DeleteConflict(Me_MOTION_C->model->object[0xd]);                      \
        DeleteConflict(Me_MOTION_C->model->object[0xe]);                      \
        cleanup_guard = 3;                                                    \
        break;                                                                \
    }                                                                         \
    if ((cleanup_guard & 2) != 0)                                             \
    {                                                                         \
        if (Me_MOTION_C->illusion[0] != 0)                                    \
        {                                                                     \
            DisposeAfterimage(Me_MOTION_C->illusion[0]);                      \
            Me_MOTION_C->illusion[0] = 0;                                     \
        }                                                                     \
        if (Me_MOTION_C->illusion[1] != 0)                                    \
        {                                                                     \
            DisposeAfterimage(Me_MOTION_C->illusion[1]);                      \
            Me_MOTION_C->illusion[1] = 0;                                     \
        }                                                                     \
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

/*
 * ActATTACK (0x80021d64) — the main humanoid attack-motion controller.  It
 * turns toward the current target, dispatches the active attack animation,
 * creates and removes weapon conflict boxes, drives projectiles and special
 * attacks, and manages weapon afterimages.
 *
 * Matching notes (6,648 bytes / 1,662 instructions):
 *  - MOTION.C's small globals need the ActATTACK gp-extern list in both the
 *    build and standalone matching tools.
 *  - DeleteConflict is intentionally old-style in this translation unit.
 *    The hand-selection value remains live in $a1 and is a harmless second
 *    argument on the first cleanup calls.  The typed case-2 call preserves a
 *    distinct HImode call result, stopping jump2 from folding two physical
 *    cleanup calls into one.  The conflict.h prototype is renamed while
 *    including the aggregate header so it does not erase this original
 *    call-site shape.
 *  - Both the retail and trial executables contain the otherwise dead
 *    Me_MOTION_C read immediately before the root-model coordinate clears.
 *    The volatile-qualified read records that real access explicitly.
 *  - cleanup_guard is a short.  Its HImode definitions make reorg duplicate
 *    the case-0 `li 3` into the switch branch delay slot, as in the target.
 *  - The reversed `direction > turn` comparisons only change fold/ref order;
 *    they give local allocation the target's $v1/$a1 assignment.
 *  - The Napalm request and its velocity scratch retain PSX.SYM's original
 *    block-local names, `item` and `vect`.
 *  - PSX.SYM records scoped PARAM_ITEM_LAUNCH locals named `item` at the
 *    earlier sp+0x18 slot, accounting for its 0x28-byte extent. Retail only
 *    uses the leading SVECTOR as fall-motion velocity, so an explicit union
 *    gives that inferred view a descriptive name without inventing an array.
 *  - One-shot do loops around the MotionUpdateMode scans preserve the target's
 *    loop notes and load order without emitting control-flow instructions.
 */

extern Humanoid *Me_MOTION_C;
extern void DeleteConflict();

extern void launch_lightning_bolt_(s16 frame);
extern void AttackBowControl(s16 n);
extern s16 AttackContinuousCheck(BattleType *battle);
extern void bow_shoot_logic(s16 kind, VECTOR *start);
extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern void WeaponHitWeapon(ModelType *model);
extern void ReturnNormal(void);
extern s16 UpdateMotion(MotionManager *mmp, s16 mid);
extern s16 PlayMotion(MotionManager *mmp, s16 mode);

void ActATTACK(void)
{
    bool is_player;
    SVECTOR *v;
    MotionManager *mmp;
    short warid;
    short t;
    short n;
    MotionDataType *mot;
    VECTOR *pos;
    int wid;
    AfterimageType *ilu;
    ModelType **object;
    ModelType *target;
    BattleType *battle;
    ModelType *hand[2];
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
                motID = MOT_DEAD;
                motMODE = 1;
                return;
            }
            attack_id = GetAttackDBID(Me_MOTION_C, motID);
            human = Me_MOTION_C;
            human->warid = attack_id;
            sound_id = 11;
            if (motID != MOT_ATTACK_TAUNT)
            {
                sound_id = 10;
                if (motID & 1)
                {
                    sound_id = 9;
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

        direction = GetDirection(target->locate.coord.t[0] - dtL->vx,
                                 target->locate.coord.t[2] - dtL->vz, dtR->vy);
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
        case 0xf1:
        {
            OrnamentType **weapon;

            weapon = Me_MOTION_C->weapon;
            SWAP_TWIN_BLADE(42, 6);
            break;
        }
        case 0xab:
        {
            VECTOR *pos;

            if (dtM->count == 20)
            {
                pos = GetAbsolutePosition(Me_MOTION_C->model->object[0xd], 0, 100, -100);
                bow_shoot_logic(ITEM_GUN, pos);
                Sound(Me_MOTION_C, 2);
            }
            break;
        }
        case 0xac:
        {
            VECTOR *pos;

            if (dtM->count == 22)
            {
                pos = GetAbsolutePosition(Me_MOTION_C->model->object[0xd], 0, 700, -100);
                bow_shoot_logic(ITEM_GUN, pos);
                Sound(Me_MOTION_C, 2);
            }
            break;
        }
        case 0xf5:
        {
            int last_frame;
            PARAM_ITEM_LAUNCH *request;
            short first_frame;

            /* The frame-window constants staged in first_frame/last_frame
             * are byte-required (direct literals recolor; measured). */
            first_frame = 0x24;
            t = dtM->count;
            request = &item;
            if (first_frame <= t)
            {
                last_frame = 0x41;
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
                pos = GetAbsolutePosition(Me_MOTION_C->model->object[2], 0, -100, -300);
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
        case 0xe9:
            launch_lightning_bolt_(0xd);
            break;
        case 0xaa:
        case 0x1a4:
            AttackBowControl(0);
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
            motMODE = 1;
            i = 0;
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            if (MotionUpdateMode != 0)
            {
                for (; i < 5; i++)
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
        if (t == WEP_TWIN_KATANA)
        {
            weapon = Me_MOTION_C->weapon;
            SWAP_TWIN_BLADE(52, 1);
        }
        else if (t == WEP_MEIOU)
        {
            launch_lightning_bolt_(0xd);
        }
        else if (t == WEP_KATAOKA)
        {
            AttackBowControl(1);
        }
        if (((Me_MOTION_C->pad.trig & PADRleft) != 0) &&
            AttackContinuousCheck(battle) != 0)
        {
            short i;

            motID = MOT_ATTACK_SLASH3;
            motMODE = 1;
            if (MotionUpdateMode != 0)
            {
                for (i = 0; i < 5; i++)
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
        if (Me_MOTION_C->wpatk == WEP_MEIOU)
        {
            launch_lightning_bolt_(0xd);
        }
        else if (Me_MOTION_C->wpatk == WEP_KATAOKA)
        {
            AttackBowControl(1);
        }
        if (((Me_MOTION_C->pad.trig & PADRleft) != 0) &&
            AttackContinuousCheck(battle) != 0)
        {
            short i;

            motID = MOT_ATTACK_SLASH4;
            motMODE = 1;
            if (MotionUpdateMode != 0)
            {
                for (i = 0; i < 5; i++)
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
        if (Me_MOTION_C->wpatk == WEP_MEIOU)
        {
            launch_lightning_bolt_(0xd);
        }
        break;
    case MOT_ATTACK_RIGHT1:
    {
        OrnamentType **weapon;

        if (Me_MOTION_C->wpatk == WEP_TWIN_KATANA)
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
            motMODE = 1;
            i = 0;
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            if (MotionUpdateMode != 0)
            {
                for (; i < 5; i++)
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

        if (Me_MOTION_C->wpatk == WEP_TWIN_KATANA)
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
            motMODE = 1;
            i = 0;
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            if (MotionUpdateMode != 0)
            {
                for (; i < 5; i++)
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto no_motion;
                    }
                }
            }
        set_motion:
            t = SetNowMotion(Me_MOTION_C, motID, motMODE);
            motMODE = -1;
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
            motID = MOT_ATTACK_DIVE_LAND;
            motMODE = 0;
            Sound(Me_MOTION_C, 0x1a);
            spawn_smoke_burst_(dtL, 300, 0xc, 10);
        }
        if ((dtM->count == 0) && (dtM->loop == 1))
        {
            dtM->loop = -1;
        }
        if (dtM->loop < 0)
        {
            dtM->loop--;
            if (dtM->loop < -30)
            {
                motID = MOT_STATE_FALL;
                motMODE = 0;
            }
        }
        if (motID != MOT_ATTACK_DIVE)
        {
            short cleanup_guard;
            short kind;

            DELETE_WEAPON_CONFLICTS_AND_AFTERIMAGES();
            dtM->mask = 0x7fff;
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
            DELETE_WEAPON_CONFLICTS_AND_AFTERIMAGES();
            mmp = dtM;
            motID = MOT_ENGAGE_STANCE;
            motMODE = 1;
            mmp->mask = 0x7fff;
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
        motID = MOT_ENGAGE_STANCE;
        motMODE = 1;
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
        short saved_mid;
        short motion_flag;

        if (dtM->count == 1)
        {
            ActionHalt = 1;
            SetCameraMode(CMODE_CRITICAL_HIT);
            CamState.snap_pending = 1;
            return;
        }
        if ((dtM->loop == 0) && (dtL->vy == Me_MOTION_C->target->locate.coord.t[1]))
        {
            return;
        }
        waist = *Me_MOTION_C->model->object;
        ActionHalt = 0;
        /* Re-walks the chain rather than reading waist->id: byte-required
         * (the second full deref is in the bytes; measured). */
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
        motion_flag = motMODE;
        human = Me_MOTION_C;
        if (human->status != STAT_DEAD || human->motion->loop != -1)
        {
            if (UpdateMotion(human->motion, saved_mid) != 0)
            {
                human->status = saved_mid >> 8;
                if (motion_flag != 0)
                {
                    mot = human->motion->motion;
                    MoveHumanoid(human, (u16)mot->orderspd, (u16)mot->sidespd);
                }
            }
        }
        dtM->count = 0;
        dtM->loop = 0;
        PlayMotion(dtM, 1);
        motMODE = -1;
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
        short saved_mid;
        short i;

        saved_mid = motID;
        DELETE_WEAPON_CONFLICTS_AND_AFTERIMAGES();
        motID = MOT_ENGAGE_STANCE;
        motMODE = 1;
        dtM->mask = 0x7fff;
        SET_NOW_MOTION_UNLESS_CVA(goto align_rotation);
    align_rotation:
        dtR->vy += (((*Me_MOTION_C->model->object)->rotate).vy - dtM->motion->rotate[0]->y);
        is_player = Me_MOTION_C == StagePlayer;
        ((*Me_MOTION_C->model->object)->rotate).vy = dtM->motion->rotate[0]->y;
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
        int hand_kind;

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
        case WEP_BEAST:
            hand[0] = object[2];
            hand[1] = object[1];
            break;
        case WEP_ONININ:
            hand[0] = object[8];
            hand[1] = object[0xb];
            break;
        default:
            hand[0] = object[0xd];
            hand[1] = object[0xe];
            break;
        }
        if (dtM->count == battle->atks)
        {
            wid = (int)Me_MOTION_C->wepid[0];
            if (wid >= 0)
            {
                Humanoid *owner;
                short conflict_size;

                n = InsertConflict(hand[0]);
                ConflictObject[n].offset = WeaponDB[wid].confp;
                conflict_size = WeaponDB[wid].confp.pad;
                owner = Me_MOTION_C;
                ConflictObject[n].size.pad = CONFLICT_HIT;
                ConflictObject[n].size.vz = conflict_size;
                ConflictObject[n].size.vy = conflict_size;
                ConflictObject[n].size.vx = conflict_size;
                ConflictObject[n].common = (void *)owner;
            }
            wid = (int)Me_MOTION_C->wepid[1];
            if (wid >= 0)
            {
                Humanoid *owner;
                short conflict_size;

                n = InsertConflict(hand[1]);
                ConflictObject[n].offset = WeaponDB[wid].confp;
                conflict_size = WeaponDB[wid].confp.pad;
                owner = Me_MOTION_C;
                ConflictObject[n].size.pad = CONFLICT_HIT;
                ConflictObject[n].size.vz = conflict_size;
                ConflictObject[n].size.vy = conflict_size;
                ConflictObject[n].size.vx = conflict_size;
                ConflictObject[n].common = (void *)owner;
            }
            Sound(Me_MOTION_C, 2);
            if (Me_MOTION_C == StagePlayer)
            {
                PadShockAR(0, 0xff, 5, 0);
            }
        }
        else if (dtM->count == battle->atke)
        {
            short kind;

            kind = Me_MOTION_C->wpatk;
            switch (kind)
            {
            case WEP_ONININ:
                DeleteConflict(Me_MOTION_C->model->object[8], hand_kind);
                /* The value-typed cast is load-bearing, and gcc 2.8.1's own
                 * jump.c proves it is the ONLY C-level escape: find_cross_jump
                 * compares CALL_INSN_FUNCTION_USAGE (the argument-register use
                 * list) plus the pattern code. This call has two potential
                 * merge partners — default's 1-arg call (the fallthrough
                 * before the join label; measured: plain 1-arg merges into it)
                 * and case 3's 2-arg call (measured: a hand_kind 2-arg
                 * spelling merges with that one instead). No argument shape
                 * differs from both at once; only the pattern code does, and
                 * value-typing this call (call_value vs call) is how. Retail
                 * emits a plain jal — an earlier note claiming jalr was
                 * wrong. */
                ((s16 (*)(ModelType *))DeleteConflict)(Me_MOTION_C->model->object[0xb]);
                break;
            case WEP_BEAST:
                DeleteConflict(Me_MOTION_C->model->object[2], hand_kind);
                break;
            case WEP_NONE:
                break;
            default:
                DeleteConflict(Me_MOTION_C->model->object[0xd], hand_kind);
                DeleteConflict(Me_MOTION_C->model->object[0xe]);
                break;
            }
            dtM->mask = 0x7fff;
        }
        if ((dtM->count < battle->atke) && ((Me_MOTION_C->type & PAGE_MASK) != PAGE_BEAST))
        {
            if (hand[0]->id != -1)
            {
                WeaponHitWeapon(hand[0]);
            }
            if (hand[1]->id != -1)
            {
                WeaponHitWeapon(hand[1]);
            }
        }
        if (battle->ilus < 1)
        {
            return;
        }
        if (dtM->count == battle->ilus)
        {
            wid = (int)Me_MOTION_C->wepid[0];
            if (wid >= 0)
            {
                ilu = SetupAfterimage(hand[0], 10);
                ilu->vector1 = WeaponDB[wid].ilup0;
                ilu->vector2 = WeaponDB[wid].ilup1;
                Me_MOTION_C->illusion[0] = (void *)ilu;
            }
            wid = (int)Me_MOTION_C->wepid[1];
            if (wid < 0)
            {
                return;
            }
            ilu = SetupAfterimage(hand[1], 10);
            ilu->vector1 = WeaponDB[wid].ilup0;
            ilu->vector2 = WeaponDB[wid].ilup1;
            Me_MOTION_C->illusion[1] = (void *)ilu;
            return;
        }
        if (dtM->count != battle->ilue)
        {
            return;
        }
        if (Me_MOTION_C->illusion[0] != 0)
        {
            DisposeAfterimage(Me_MOTION_C->illusion[0]);
            Me_MOTION_C->illusion[0] = 0;
        }
        if (Me_MOTION_C->illusion[1] != 0)
        {
            DisposeAfterimage(Me_MOTION_C->illusion[1]);
            Me_MOTION_C->illusion[1] = 0;
        }
        mmp = dtM;
        mmp->mask = 0x7fff;
        return;
    }
}
