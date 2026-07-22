#include "common.h"
#include "main.exe.h"
#define DeleteConflict DeleteConflict_prototype
#include "item.h"
#undef DeleteConflict
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActATTACK(void);
 *     MOTION.C:1306, 197 src lines, frame 128 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *    cleanup calls into one.  The item.h prototype is renamed while including
 *    the header so it does not erase this original call-site shape.
 *  - Both the retail and trial executables contain the otherwise dead
 *    Me_MOTION_C read immediately before the root-model coordinate clears.
 *    The volatile-qualified read records that real access explicitly.
 *  - cleanup_guard is a short.  Its HImode definitions make reorg duplicate
 *    the case-0 `li 3` into the switch branch delay slot, as in the target.
 *  - The reversed `direction > turn` comparisons only change fold/ref order;
 *    they give local allocation the target's $v1/$a1 assignment.
 *  - One-shot do loops around the MotionUpdateMode scans preserve the target's
 *    loop notes and load order without emitting control-flow instructions.
 */

extern MotionManager *dtM;
extern SVECTOR *dtR;
extern VECTOR *dtL;
extern SVECTOR *dtV;
extern s16 dtPAD;
extern s16 MotionUpdateMode;
extern s16 motID;
extern s16 motMODE;
extern s16 ActionHalt;
extern Humanoid *Me_MOTION_C;
extern Humanoid *StagePlayer;
extern BattleType BattleDB[78];
extern WeaponType WeaponDB[];
extern ConflictObjectType ConflictObject[];
extern HumanAnimType CVAhuman[];
extern TCameraStatus CamState;

extern s16 GetAttackDBID(Humanoid *human, s16 mid);
extern s16 GetDirection(s32 dx, s32 dz, s32 roty);
extern s16 GetMotionID(MotionManager *mmp, s16 mid);
extern void handle_char_state_attacking_SEVEN_(s16 frame);
extern void AttackBowControl(s16 n);
extern s16 AttackContinuousCheck(BattleType *battle);
extern void bow_shoot_logic(s16 kind, VECTOR *start);
extern void GetMoveSpeed(SVECTOR *vect, s16 ry, s16 order, s16 side);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern void SetCameraMode(s32 mode);
extern void FUN_80033bc0(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern s16 InsertConflict(ModelType *model);
extern void PadShockAR(s32 port, s32 power, s32 attack, s32 release);
extern void DisposeAfterimage(AfterimageType *afi);
extern void WeaponHitWeapon(ModelType *model);
extern void ReturnNormal(void);
extern s16 UpdateMotion(MotionManager *mmp, s16 mid);
extern s16 PlayMotion(MotionManager *mmp, s16 mode);
extern s16 Sound(Humanoid *human, s16 id);
extern void DeleteConflict();

void ActATTACK(void)

{
  short *psVar1;
  bool bVar2;
  SVECTOR *pSVar5;
  MotionManager *pMVar6;
  short sVar7;
  short sVar8;
  short sVar9;
  short sVar10;
  short move_y;
  short updated;
  MotionDataType *pMVar11;
  VECTOR *pVVar12;
  int iVar13;
  AfterimageType *pAVar14;
  int iVar15;
  ModelType **ppMVar16;
  ModelType *pMVar17;
  short sVar18;
  BattleType *battle;
  ModelType *hand[2];
  SVECTOR local_68 [5];
  PARAM_ITEM_LAUNCH local_40;
  SVECTOR local_18;

  {
    Humanoid *human;

  if (dtM->count == 1) {
    short attack_id;
    short sound_id;

    if (Me_MOTION_C->life == 0) {
      motID = 0x1100;
      motMODE = 1;
      return;
    }
    attack_id = GetAttackDBID(Me_MOTION_C,motID);
    human = Me_MOTION_C;
    human->warid = attack_id;
    sound_id = 0xb;
    if ((motID != 0x713) && (sound_id = 10, (motID & 1U) != 0)) {
      sound_id = 9;
    }
    Sound(human,sound_id);
  }
  human = Me_MOTION_C;
  sVar7 = human->warid;
  battle = &BattleDB[sVar7];
  pMVar17 = human->target;
  }
  if (((pMVar17 != (ModelType *)0x0) && (dtM->count < battle->revise)) && (-1 < dtM->count))
  {
    Humanoid *human;
    short turn;
    short direction;

    direction = GetDirection((pMVar17->locate).coord.t[0] - dtL->vx,
                             (pMVar17->locate).coord.t[2] - dtL->vz,dtR->vy);
    human = Me_MOTION_C;
    turn = human->turn;
    if ((int)direction > (int)turn) {
      dtR->vy += 100;
    }
    else if (-(int)turn > (int)direction) {
      dtR->vy -= 100;
    }
    else {
      goto LAB_80021edc;
    }
    pMVar11 = human->motion->motion;
    MoveHumanoid(human,(u16)pMVar11->orderspd,(u16)pMVar11->sidespd);
  }
LAB_80021edc:
  switch ((short)(dtM->mid - 0x700)) {
  case 0:
    sVar8 = GetMotionID(dtM,0x700);
    switch (sVar8) {
    case 0xf1: {
      OrnamentType **weapon;

      weapon = Me_MOTION_C->weapon;
      if (dtM->count == 0x2a) {
        if (weapon[3] != (OrnamentType *)0x0) {
          weapon[2] = weapon[0];
          weapon[0] = weapon[3];
          weapon[3] = (OrnamentType *)0x0;
          Sound(Me_MOTION_C,1);
        }
      }
      else if ((dtM->count == 6) && (weapon[2] != (OrnamentType *)0x0)) {
        weapon[3] = weapon[0];
        weapon[0] = weapon[2];
        weapon[2] = (OrnamentType *)0x0;
        Sound(Me_MOTION_C,0);
      }
      break;
    }
    case 0xab: {
      VECTOR *pos;

      if (dtM->count == 0x14) {
        pos = GetAbsolutePosition(Me_MOTION_C->model->object[0xd],0,100,-100);
        bow_shoot_logic(0x14,pos);
        Sound(Me_MOTION_C,2);
      }
      break;
    }
    case 0xac: {
      VECTOR *pos;

      if (dtM->count == 0x16) {
        pos = GetAbsolutePosition(Me_MOTION_C->model->object[0xd],0,700,-100);
        bow_shoot_logic(0x14,pos);
        Sound(Me_MOTION_C,2);
      }
      break;
    }
    case 0xf5: {
      int last_frame;
      PARAM_ITEM_LAUNCH *request;
      short first_frame;

      first_frame = 0x24;
      sVar8 = dtM->count;
      request = &local_40;
      if (first_frame <= sVar8) {
        last_frame = 0x41;
        if (last_frame < sVar8) {
          break;
        }
        if (sVar8 == first_frame) {
          Sound(Me_MOTION_C,0x28);
        }
        local_40.type = ITEM_KIND_2_KAEN;
        local_40.user = Me_MOTION_C;
        pVVar12 = GetAbsolutePosition(Me_MOTION_C->model->object[2],0,-100,-300);
        local_40.start.vx = pVVar12->vx;
        local_40.start.vy = pVVar12->vy;
        local_40.start.vz = pVVar12->vz;
        GetMoveSpeed(&local_18,dtR->vy,100,0);
        local_40.end.vx = local_40.start.vx + local_18.vx;
        local_40.end.vy = local_40.start.vy;
        local_40.end.vz = local_40.start.vz + local_18.vz;
        ReqItemUse(request);
      }
      break;
    }
    case 0xe9:
      handle_char_state_attacking_SEVEN_(0xd);
      break;
    case 0xaa:
    case 0x1a4:
      AttackBowControl(0);
      break;
    }
    if ((((Me_MOTION_C->pad).trig & 0x80) != 0) &&
       (sVar8 = AttackContinuousCheck(battle), sVar8 != 0)) {
      short i;

      if ((dtPAD & 0x2000) != 0) {
        motID = 0x702;
      }
      else if ((dtPAD & 0x8000) != 0) {
        motID = 0x703;
      }
      else {
        motID = 0x701;
      }
      motMODE = 1;
      i = 0;
      do {
        if (MotionUpdateMode != 0) {
          for (; i < 5; i++) {
            if (CVAhuman[i].human == Me_MOTION_C) {
              goto LAB_800226f8;
            }
          }
        }
      } while (0);
      goto LAB_80022760;
    }
    break;
  case 1: {
    OrnamentType **weapon;

    sVar8 = Me_MOTION_C->weapon_kind;
    if (sVar8 == 0x2a) {
      weapon = Me_MOTION_C->weapon;
      if (dtM->count == 0x34) {
        if (weapon[3] != (OrnamentType *)0x0) {
          weapon[2] = weapon[0];
          weapon[0] = weapon[3];
          weapon[3] = (OrnamentType *)0x0;
          Sound(Me_MOTION_C,1);
        }
      }
      else if ((dtM->count == 1) && (weapon[2] != (OrnamentType *)0x0)) {
        weapon[3] = weapon[0];
        weapon[0] = weapon[2];
        weapon[2] = (OrnamentType *)0x0;
        Sound(Me_MOTION_C,0);
      }
    }
    else if (sVar8 == 0x29) {
      handle_char_state_attacking_SEVEN_(0xd);
    }
    else if (sVar8 == 0x35) {
      AttackBowControl(1);
    }
    if ((((Me_MOTION_C->pad).trig & 0x80) != 0) &&
       (sVar8 = AttackContinuousCheck(battle), sVar8 != 0)) {
      short i;

      motID = 0x704;
      motMODE = 1;
      if (MotionUpdateMode != 0) {
        for (i = 0; i < 5; i++) {
          if (CVAhuman[i].human == Me_MOTION_C) {
            goto LAB_800226f8;
          }
        }
      }
      goto LAB_80022760;
    }
    break;
  }
  case 4:
    if ((s16)Me_MOTION_C->weapon_kind == 0x29) {
      handle_char_state_attacking_SEVEN_(0xd);
    }
    else if ((s16)Me_MOTION_C->weapon_kind == 0x35) {
      AttackBowControl(1);
    }
    if ((((Me_MOTION_C->pad).trig & 0x80) != 0) &&
       (sVar8 = AttackContinuousCheck(battle), sVar8 != 0)) {
      short i;

      motID = 0x705;
      motMODE = 1;
      if (MotionUpdateMode != 0) {
        for (i = 0; i < 5; i++) {
          if (CVAhuman[i].human == Me_MOTION_C) {
            goto LAB_800226f8;
          }
        }
      }
      goto LAB_80022760;
    }
    break;
  case 5:
    if ((s16)Me_MOTION_C->weapon_kind == 0x29) {
      handle_char_state_attacking_SEVEN_(0xd);
    }
    break;
  case 6: {
    OrnamentType **weapon;

    if ((s16)Me_MOTION_C->weapon_kind == 0x2a) {
      weapon = Me_MOTION_C->weapon;
      if (dtM->count == 0x34) {
        if (weapon[3] != (OrnamentType *)0x0) {
          weapon[2] = weapon[0];
          weapon[0] = weapon[3];
          weapon[3] = (OrnamentType *)0x0;
          Sound(Me_MOTION_C,1);
        }
      }
      else if ((dtM->count == 0x10) && (weapon[2] != (OrnamentType *)0x0)) {
        weapon[3] = weapon[0];
        weapon[0] = weapon[2];
        weapon[2] = (OrnamentType *)0x0;
        Sound(Me_MOTION_C,0);
      }
    }
    if (((((Me_MOTION_C->pad).trig & 0x80) != 0) && (((int)(short)dtPAD & 0xa000U) != 0)) &&
       (sVar8 = AttackContinuousCheck(battle), sVar8 != 0)) {
      short i;

      if (((int)(short)dtPAD & 0x8000U) != 0) {
        motID = 0x708;
      }
      else {
        motID = 0x707;
      }
      motMODE = 1;
      i = 0;
      do {
        if (MotionUpdateMode != 0) {
          for (; i < 5; i++) {
            if (CVAhuman[i].human == Me_MOTION_C) {
              goto LAB_800226f8;
            }
          }
        }
      } while (0);
      goto LAB_80022760;
    }
    break;
  }
  case 9: {
    OrnamentType **weapon;

    if ((s16)Me_MOTION_C->weapon_kind == 0x2a) {
      weapon = Me_MOTION_C->weapon;
      if (dtM->count == 0x2b) {
        if (weapon[3] != (OrnamentType *)0x0) {
          weapon[2] = weapon[0];
          weapon[0] = weapon[3];
          weapon[3] = (OrnamentType *)0x0;
          Sound(Me_MOTION_C,1);
        }
      }
      else if ((dtM->count == 0xd) && (weapon[2] != (OrnamentType *)0x0)) {
        weapon[3] = weapon[0];
        weapon[0] = weapon[2];
        weapon[2] = (OrnamentType *)0x0;
        Sound(Me_MOTION_C,0);
      }
    }
    if (((((Me_MOTION_C->pad).trig & 0x80) != 0) && (((int)(short)dtPAD & 0xa000U) != 0)) &&
       (sVar8 = AttackContinuousCheck(battle), sVar8 != 0)) {
      short i;

      if ((dtPAD & 0x2000) == 0) {
        goto LAB_80022700;
      }
      motID = 0x70b;
      goto LAB_80022704;
LAB_800226f8:
      sVar8 = 0;
      goto LAB_80022780;
LAB_80022700:
      motID = 0x70a;
LAB_80022704:
      motMODE = 1;
      i = 0;
      do {
        if (MotionUpdateMode != 0) {
          for (; i < 5; i++) {
            if (CVAhuman[i].human == Me_MOTION_C) {
              goto LAB_800226f8;
            }
          }
        }
      } while (0);
LAB_80022760:
      sVar8 = SetNowMotion(Me_MOTION_C,motID,motMODE);
      motMODE = -1;
LAB_80022780:
      if (sVar8 != 0) {
        int conflict_id;

        conflict_id = (int)(*Me_MOTION_C->model->object)->id;
        if (-1 < conflict_id) {
          dtL->vx = ConflictObject[conflict_id].position.vx;
          dtL->vz = ConflictObject[conflict_id].position.vz;
        }
      }
      goto switchD_80021f18_caseD_2;
    }
    break;
  }
  case 0xf:
    if ((dtM->count == 1) && (3000 < (Me_MOTION_C->map).height)) {
      SetCameraMode(CMODE_FALL);
    }
    if (((int)(short)dtPAD & 0xf000U) != 0) {
      if ((dtPAD & 0x1000) != 0) {
        GetMoveSpeed(local_68,dtR->vy,10,0);
      }
      else if ((dtPAD & 0x4000) != 0) {
        GetMoveSpeed(local_68,dtR->vy,-10,0);
      }
      else if ((dtPAD & 0x2000) != 0) {
        GetMoveSpeed(local_68,dtR->vy,0,-10);
      }
      else {
        GetMoveSpeed(local_68,dtR->vy,0,10);
      }
      pSVar5 = dtV;
      local_68[0].vx = local_68[0].vx + dtV->vx;
      local_68[0].vz = local_68[0].vz + dtV->vz;
      if ((((local_68[0].vx >= 0) ? local_68[0].vx : -local_68[0].vx) < 0x65) &&
          (((local_68[0].vz >= 0) ? local_68[0].vz : -local_68[0].vz) < 0x65)) {
        dtV->vx = local_68[0].vx;
        pSVar5->vz = local_68[0].vz;
      }
    }
    if ((Me_MOTION_C->attribute & 0x800U) != 0) {
      motID = 0x710;
      motMODE = 0;
      Sound(Me_MOTION_C,0x1a);
      FUN_80033bc0(dtL,300,0xc,10);
    }
    if ((dtM->count == 0) && (dtM->loop == 1)) {
      dtM->loop = -1;
    }
    if (dtM->loop < 0) {
      dtM->loop--;
      if (dtM->loop < -0x1e) {
        motID = 0x803;
        motMODE = 0;
      }
    }
    if (motID != 0x70f) {
      short cleanup_guard;
      short kind;

      kind = Me_MOTION_C->weapon_kind;
      switch (kind) {
      case 2:
        DeleteConflict(Me_MOTION_C->model->object[8]);
        DeleteConflict(Me_MOTION_C->model->object[0xb]);
        cleanup_guard = 3;
        break;
      case 3:
        DeleteConflict(Me_MOTION_C->model->object[2]);
        cleanup_guard = 3;
        break;
      case 0:
        cleanup_guard = 3;
        break;
      default:
        DeleteConflict(Me_MOTION_C->model->object[0xd]);
        DeleteConflict(Me_MOTION_C->model->object[0xe]);
        cleanup_guard = 3;
        break;
      }
      if ((cleanup_guard & 2) != 0) {
        if ((AfterimageType *)Me_MOTION_C->illusion[0] != (AfterimageType *)0x0) {
          DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
          Me_MOTION_C->illusion[0] = (void *)0x0;
        }
        if ((AfterimageType *)Me_MOTION_C->illusion[1] != (AfterimageType *)0x0) {
          DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
          Me_MOTION_C->illusion[1] = (void *)0x0;
        }
      }
      dtM->mask = 0x7fff;
      SetCameraMode(CMODE_NORMAL);
      if (motID == 0x803) {
        return;
      }
    }
    break;
  case 0x10: {
    short cleanup_guard;
    short kind;

    if (dtM->loop < 0) {
      dtM->loop = 0;
    }
    if ((dtM->count == 0) && (dtM->loop != 0)) {
      kind = Me_MOTION_C->weapon_kind;
      switch (kind) {
      case 2:
        DeleteConflict(Me_MOTION_C->model->object[8]);
        DeleteConflict(Me_MOTION_C->model->object[0xb]);
        cleanup_guard = 3;
        break;
      case 3:
        DeleteConflict(Me_MOTION_C->model->object[2]);
        cleanup_guard = 3;
        break;
      case 0:
        cleanup_guard = 3;
        break;
      default:
        DeleteConflict(Me_MOTION_C->model->object[0xd]);
        DeleteConflict(Me_MOTION_C->model->object[0xe]);
        cleanup_guard = 3;
        break;
      }
      if ((cleanup_guard & 2) != 0) {
        if ((AfterimageType *)Me_MOTION_C->illusion[0] != (AfterimageType *)0x0) {
          DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
          Me_MOTION_C->illusion[0] = (void *)0x0;
        }
        if ((AfterimageType *)Me_MOTION_C->illusion[1] != (AfterimageType *)0x0) {
          DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
          Me_MOTION_C->illusion[1] = (void *)0x0;
        }
      }
      pMVar6 = dtM;
      motID = 0x501;
      motMODE = 1;
      goto LAB_80023740;
    }
    if (0 < (Me_MOTION_C->map).height) {
      return;
    }
    pSVar5 = dtV;
    dtV->vx = dtV->vx - (dtV->vx >> 2);
    pSVar5->vz = pSVar5->vz - (pSVar5->vz >> 2);
    return;
  }
  case 0x13:
    if (dtM->count != 0) {
      return;
    }
    if (dtM->loop == 0) {
      return;
    }
    motID = 0x501;
    motMODE = 1;
    return;
  case 0x14:
  case 0x15:
  case 0x16:
  case 0x17:
  case 0x18:
  case 0x19: {
    int conflict_id;
    Humanoid *human;
    short saved_mid;
    u32 shifted_mid;
    short motion_flag;
    short updated;

    if (dtM->count == 1) {
      ActionHalt = 1;
      SetCameraMode(CMODE_CRITICAL_HIT);
      CamState.CriticalHit = 1;
      return;
    }
    if ((dtM->loop == 0) && (dtL->vy == (Me_MOTION_C->target->locate).coord.t[1])) {
      return;
    }
    pMVar17 = *Me_MOTION_C->model->object;
    ActionHalt = 0;
    conflict_id = (int)(*Me_MOTION_C->model->object)->id;
    if (-1 < conflict_id) {
      dtL->vx = ConflictObject[conflict_id].position.vx;
      dtL->vz = ConflictObject[conflict_id].position.vz;
    }
    (void)*(Humanoid * volatile *)&Me_MOTION_C;
    (pMVar17->locate).coord.t[2] = 0;
    (pMVar17->locate).coord.t[0] = 0;
    ReturnNormal();
    saved_mid = motID;
    motion_flag = motMODE;
    human = Me_MOTION_C;
    if (((human->status != 0x11) || (human->motion->loop != -1)) &&
        ((shifted_mid = (u32)(u16)saved_mid << 16,
          updated = UpdateMotion(human->motion,(s16)(shifted_mid >> 16)), updated != 0) &&
         (human->status = (s8)(shifted_mid >> 24), motion_flag != 0))) {
      pMVar11 = human->motion->motion;
      MoveHumanoid(human,(u16)pMVar11->orderspd,(u16)pMVar11->sidespd);
    }
    dtM->count = 0;
    dtM->loop = 0;
    PlayMotion(dtM,1);
    motMODE = 0xffff;
    CamState.CriticalHit = 1;
    return;
  }
  }
switchD_80021f18_caseD_2:
  if (dtM->loop < 0) {
    dtM->loop++;
    if (dtM->loop != 0) {
      return;
    }
    pMVar11 = Me_MOTION_C->motion->motion;
    MoveHumanoid(Me_MOTION_C,(u16)pMVar11->orderspd,(u16)pMVar11->sidespd);
    return;
  }
  if ((dtM->count == 0) && (dtM->loop == 1)) {
    short cleanup_guard;
    short kind;
    short saved_mid;
    short i;

    saved_mid = motID;
    kind = Me_MOTION_C->weapon_kind;
    switch (kind) {
    case 2:
      DeleteConflict(Me_MOTION_C->model->object[8]);
      DeleteConflict(Me_MOTION_C->model->object[0xb]);
      cleanup_guard = 3;
      break;
    case 3:
      DeleteConflict(Me_MOTION_C->model->object[2]);
      cleanup_guard = 3;
      break;
    case 0:
      cleanup_guard = 3;
      break;
    default:
      DeleteConflict(Me_MOTION_C->model->object[0xd]);
      DeleteConflict(Me_MOTION_C->model->object[0xe]);
      cleanup_guard = 3;
      break;
    }
    if ((cleanup_guard & 2) != 0) {
      if ((AfterimageType *)Me_MOTION_C->illusion[0] != (AfterimageType *)0x0) {
        DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
        Me_MOTION_C->illusion[0] = (void *)0x0;
      }
      if ((AfterimageType *)Me_MOTION_C->illusion[1] != (AfterimageType *)0x0) {
        DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
        Me_MOTION_C->illusion[1] = (void *)0x0;
      }
    }
    motID = 0x501;
    motMODE = 1;
    dtM->mask = 0x7fff;
    if (MotionUpdateMode != 0) {
      for (i = 0; i < 5; i++) {
        if (CVAhuman[i].human == Me_MOTION_C) {
          goto LAB_8002315c;
        }
      }
    }
    SetNowMotion(Me_MOTION_C,motID,motMODE);
    motMODE = -1;
LAB_8002315c:
    dtR->vy = dtR->vy + (((*Me_MOTION_C->model->object)->rotate).vy - dtM->motion->rotate[0]->y);
    bVar2 = Me_MOTION_C == StagePlayer;
    ((*Me_MOTION_C->model->object)->rotate).vy = dtM->motion->rotate[0]->y;
    if ((bVar2) && (SetCameraMode(CMODE_NORMAL), saved_mid == 0x712)) {
      CamState.CriticalHit = 1;
    }
    (Me_MOTION_C->pad).time = 0;
    return;
  }
  {
    int hand_kind;

    if (battle->mid == 0) {
      return;
    }
    if (battle->atks < 1) {
      return;
    }
    ppMVar16 = Me_MOTION_C->model->object;
    hand_kind = (s16)Me_MOTION_C->weapon_kind;
    switch (hand_kind) {
    case 3:
      hand[0] = ppMVar16[2];
      hand[1] = ppMVar16[1];
      break;
    case 2:
      hand[0] = ppMVar16[8];
      hand[1] = ppMVar16[0xb];
      break;
    default:
      hand[0] = ppMVar16[0xd];
      hand[1] = ppMVar16[0xe];
      break;
    }
    if (dtM->count == battle->atks) {
      iVar13 = (int)Me_MOTION_C->wepid[0];
      if (-1 < iVar13) {
        Humanoid *owner;
        short conflict_size;

        sVar10 = InsertConflict(hand[0]);
        ConflictObject[sVar10].offset = WeaponDB[iVar13].confp;
        conflict_size = WeaponDB[iVar13].confp.pad;
        owner = Me_MOTION_C;
        ConflictObject[sVar10].size.pad = 1;
        ConflictObject[sVar10].size.vz = conflict_size;
        ConflictObject[sVar10].size.vy = conflict_size;
        ConflictObject[sVar10].size.vx = conflict_size;
        ConflictObject[sVar10].common = (void *)owner;
      }
      iVar13 = (int)Me_MOTION_C->wepid[1];
      if (-1 < iVar13) {
        Humanoid *owner;
        short conflict_size;

        sVar10 = InsertConflict(hand[1]);
        ConflictObject[sVar10].offset = WeaponDB[iVar13].confp;
        conflict_size = WeaponDB[iVar13].confp.pad;
        owner = Me_MOTION_C;
        ConflictObject[sVar10].size.pad = 1;
        ConflictObject[sVar10].size.vz = conflict_size;
        ConflictObject[sVar10].size.vy = conflict_size;
        ConflictObject[sVar10].size.vx = conflict_size;
        ConflictObject[sVar10].common = (void *)owner;
      }
      Sound(Me_MOTION_C,2);
      if (Me_MOTION_C == StagePlayer) {
        PadShockAR(0,0xff,5,0);
      }
    }
    else if (dtM->count == battle->atke) {
      short kind;

      kind = Me_MOTION_C->weapon_kind;
      switch (kind) {
      case 2:
        DeleteConflict(Me_MOTION_C->model->object[8], hand_kind);
        ((s16 (*)(ModelType *))DeleteConflict)(Me_MOTION_C->model->object[0xb]);
        break;
      case 3:
        DeleteConflict(Me_MOTION_C->model->object[2], hand_kind);
        break;
      case 0:
        break;
      default:
        DeleteConflict(Me_MOTION_C->model->object[0xd], hand_kind);
        DeleteConflict(Me_MOTION_C->model->object[0xe]);
        break;
      }
      dtM->mask = 0x7fff;
    }
    if ((dtM->count < battle->atke) && ((Me_MOTION_C->type & 0xf0U) != 0xa0)) {
      if (hand[0]->id != -1) {
        WeaponHitWeapon(hand[0]);
      }
      if (hand[1]->id != -1) {
        WeaponHitWeapon(hand[1]);
      }
    }
    if (battle->ilus < 1) {
      return;
    }
    if (dtM->count == battle->ilus) {
      iVar13 = (int)Me_MOTION_C->wepid[0];
      if (-1 < iVar13) {
        pAVar14 = SetupAfterimage(hand[0],10);
        pAVar14->vector1 = WeaponDB[iVar13].ilup0;
        pAVar14->vector2 = WeaponDB[iVar13].ilup1;
        Me_MOTION_C->illusion[0] = (void *)pAVar14;
      }
      iVar13 = (int)Me_MOTION_C->wepid[1];
      if (iVar13 < 0) {
        return;
      }
      pAVar14 = SetupAfterimage(hand[1],10);
      pAVar14->vector1 = WeaponDB[iVar13].ilup0;
      pAVar14->vector2 = WeaponDB[iVar13].ilup1;
      Me_MOTION_C->illusion[1] = (void *)pAVar14;
      return;
    }
    if (dtM->count != battle->ilue) {
      return;
    }
    if ((AfterimageType *)Me_MOTION_C->illusion[0] != (AfterimageType *)0x0) {
      DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
      Me_MOTION_C->illusion[0] = (void *)0x0;
    }
    if ((AfterimageType *)Me_MOTION_C->illusion[1] != (AfterimageType *)0x0) {
      DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
      Me_MOTION_C->illusion[1] = (void *)0x0;
    }
    pMVar6 = dtM;
LAB_80023740:
    pMVar6->mask = 0x7fff;
    return;
  }
}
