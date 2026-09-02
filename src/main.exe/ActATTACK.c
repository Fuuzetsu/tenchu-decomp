#include "common.h"
#include "tuning.h"
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
extern s16 UpdateMotion(MotionManager *mmp, motion_id mid);
extern s16 PlayMotion(MotionManager *mmp, s16 mode);

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
    ModelType *target;
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

            /* The frame-window constants staged in first_frame/last_frame
             * are byte-required (direct literals recolor; measured). */
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
            motMODE = MOTION_MOVE_APPLY;
            i = 0;
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
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
            AttackBowControl(1);
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
            AttackBowControl(1);
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
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
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
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
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
        if ((dtM->loop == 0) && (dtL->vy == Me_MOTION_C->target->locate.coord.t[1]))
        {
            return;
        }
        waist = *Me_MOTION_C->model->object;
        ActionHalt = ACTION_HALT_NONE;
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
            short kind;

            kind = Me_MOTION_C->wpatk;
            switch (kind)
            {
            case FIST:
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_0], hand_kind);
                /* NOT something anyone wrote. Both this cast and the
                 * bogus second argument on the sibling calls exist for one
                 * reason: to stop gcc 2.8.1's find_cross_jump merging these
                 * arms' identical `jal DeleteConflict; j <join>` tails.
                 * find_cross_jump compares CALL_INSN_FUNCTION_USAGE plus the
                 * pattern code, so a spurious extra argument (which emits
                 * NOTHING here -- the target sets only $a0) or value-typing
                 * the call (call_value vs call) makes two arms differ.
                 *
                 * What is actually wrong is upstream, and the file proves it:
                 * the SAME switch appears in
                 * DELETE_WEAPON_CONFLICTS_AND_AFTERIMAGES above with plain
                 * one-argument calls, and there the merge is CORRECT -- its
                 * five source calls emit three at each of its three sites
                 * (offsets 32/52/56; the ONININ_HAND_1 and BEAST_HAND_0 tails
                 * merge away). This switch's five all survive in retail
                 * (32/44/8/52/56), so the original had something here that
                 * blocked the merge, and it was not a cast.
                 *
                 * Measured while looking for it: plain one-argument calls
                 * throughout cost 35 lines (two arms merge); restoring the
                 * real prototype as well changes nothing further; moving the
                 * `dtM->mask` store from after the switch into each arm costs
                 * 54. The answer is a structural difference in this block we
                 * have not found -- exactly the situation ActSTATE was in
                 * until its nested humanoid aliases came out and its own
                 * SetCameraMode cast stopped being needed.
                 *
                 * Narrowed further since: the cast is uniquely required
                 * GIVEN this structure. Dropping just the cast while
                 * keeping the fake arguments costs 32; moving the fake
                 * argument onto this arm's second call instead of the
                 * cast, or onto default's, costs 56 either way. And the
                 * retail arms end in identical two-instruction
                 * `jal DeleteConflict; j <join>` tails that cc1 did NOT
                 * merge, while the macro switch above merges its
                 * three-instruction ones -- so retail's calls really did
                 * differ in RTL argument usage or pattern, and finding
                 * what source produced that is the open question. */
                ((s16 (*)(ModelType *))DeleteConflict)(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_1]);
                break;
            case JAW:
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_BEAST_HAND_0], hand_kind);
                break;
            case NO_WEAPON:
                break;
            default:
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0], hand_kind);
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1]);
                break;
            }
            dtM->mask = MOTION_MASK_ALL;
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
