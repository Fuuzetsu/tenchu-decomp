#include "common.h"
#include "main.exe.h"
#include "item.h"

extern Humanoid *Me_MOTION_C;
extern MotionManager *dtM;
extern VECTOR *dtL;
extern SVECTOR *dtR;
extern SVECTOR *dtV;
extern s16 MotionUpdateMode;
extern s16 motID;
extern s16 motMODE;
extern s16 ActionHalt;
extern s16 EngageLevel;
extern s16 Criticals;
extern s16 Murders;
extern s16 FriendHits;
extern s16 PLAYER_REDUCE_DAMAGE_DUE_TO_ARMOUR;
extern Humanoid *D_8009770C;
extern u8 gNannido;
extern ConflictObjectType ConflictObject[80];
extern BattleType BattleDB[78];
extern HumanAnimType CVAhuman[5];
extern SVECTOR ConflictDistance;
extern u16 D_80086B6C[8];

extern void SetCameraMode(s32 mode);
extern s16 Sound(Humanoid *human, s16 id);
extern s16 GetAttackDBID(Humanoid *human, s16 mid);
extern void ReqLifeBar(Humanoid *human);
extern void DeleteConflict(ModelType *model);
extern void SetImpact(VECTOR *pos, s16 scale, s16 type);
extern void PadShockAR(s32 port, s32 power, s32 attack, s32 release);
extern void reset_alert_duration(void);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 mode);
extern void AttackCancelControl(s16 mode);
extern VECTOR *GetAreaMapPassage(u32 *area, VECTOR *pos, SVECTOR *vect, s16 n);
extern s16 GetDirection(s32 dx, s32 dz, s32 roty);
extern s16 PlayMotion(MotionManager *mmp, s16 mode);
extern VECTOR *GetAbsolutePosition(ModelType *model, s32 x, s32 y, s32 z);
extern void FUN_8003944c(long *pos, long pz, short time, short vx,
                         long scale, long rotate, s16 vy, u16 vz,
                         u16 unk22, u16 mode);
extern s16 GetConflictResult(ModelType *model, s16 index);
extern void SetBlood(VECTOR *pos, s16 n, s16 time);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DamageControl(void);
 *     MOTION.C:408, 236 src lines, frame 72 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 * END PSX.SYM */

/*
 * DamageControl (0x8001d6bc) — resolves item and humanoid collisions into
 * damage, knockback, animation, blood, score, and player-feedback effects.
 *
 * STATUS: MATCH — 5812 bytes / 1453 instructions, byte-exact including the
 * compiled switch's own .rodata jump table (was 1320 -> 112 across the first
 * Fable escalation, 112 -> 90 -> 6 -> 0 in the continuation; the first pass
 * re-derived the source decomposition from the target RTL shapes and landed
 * the s16 hp life-decrement tie; the continuation recovered the id/TVar8/deg
 * identity and the assigned-abs knockback shape, see below).  Fence-free.
 *
 * What this session PROVED (the prior "below the C level / HARD-CONFLICT"
 * verdict was an artifact of the old decomposition, not a cc1 limit):
 *  - The dominant $a1->$a0 family was caused by the function-spanning cached
 *    pointer locals (pHVar14/pHVar17/pSVar1).  The original uses NO caches:
 *    each region loads Me_MOTION_C / StagePlayer / dtR / dtV fresh, and cc1's
 *    per-region CSE temps then land in $a0/$v1 exactly like retail.  Killing
 *    the caches dissolved every "hard conflict" rtlguide reported.
 *  - The do-while(0)+goto guard scaffolding around the 0x602 engage block is
 *    not original.  The real shape is a plain nested if with an else arm
 *    (`if (rand() % (EngageLevel+1) == 0) { type checks; if (rand()&1)
 *    motID=0x602; } else { motID=0x602; }`); cc1's cross-jump + eager delay
 *    fill reproduce the shared `sh motID` tail with per-predecessor `li 0x602`
 *    delay slots automatically.  Same for case 0x15: a plain
 *    `if ((rand()&1)==0) motID=0x1003; else motID=0x1001;` (no next_mot temp,
 *    no fence) and case 1 storing motID/motMODE directly; the shared store is
 *    cross-jumped, giving `j DC08 / li v0,0x100A` for case 1.
 *  - `-(x/3) - 1` must be spelled `x / -3 - 1`: cc1 folds -x-1 into nor, but a
 *    NEGATIVE divisor makes expmed emit the reversed magic-division subtract
 *    (subu sign,hi) with a plain addiu -1 — the retail shape at both sites.
 *  - `dmg <<= 1` under enemy->active_item==0xc is `(u32)(dmg << 0x10) >> 0xf`
 *    (sll 16 / srl 15).  The old `(dmg<<16); dmg>>=0xf` truncated to zero via
 *    the short lvalue (real behavior bug, compiled to `move s1,zero`).
 *  - The armour block computes deg BEFORE the knockback: `deg=(short)dmg>>3;
 *    clamp; clamp; TVar8=(short)dmg*5/2+0x50;` — no cached `(u16)dmg<<16`
 *    temp; every read re-extends dmg so the sll is shared/rematerialized.
 *  - Both ReqLifeBar sites are if/else (`who=enemy` in the taken arm, else
 *    `who=Me_MOTION_C`), which lets who coalesce with the Me load in $a0 and
 *    compile the else arm to zero code.
 *  - GetAbsolutePosition's third arg is (short)-converted at the call site
 *    (sll/sra interleaved into the pointer chain); FUN_8003944c's rot arg is
 *    an s16 param (sll/sra, not andi — prototype changed in this TU), and its
 *    `rand() % 0x168` is precomputed into a temp so the 0xB60B60B7 magic pair
 *    forms before the 0xDCDCDC pair.
 *  - The exit block loads dtV INSIDE the mid==0x300/0x302 arm; the pad.time
 *    identical-arm fence was scaffolding and is gone.
 *  - PSX.SYM roles that survive: dmg=$s1, enemy=$s3, did=$s4, id=$s5.
 *
 * What the CONTINUATION proved (112 -> 90):
 *  - `id` is an INT loaded via `(u16)vector.pad` (lhu s5) with `(short)id`
 *    casts at every signed use (sll/sra 0x10).  The prior `s8 id` was
 *    BALLAST: its lbu/sll24 bytes were wrong, but the QI->HI conversion kept
 *    2 extra flow-time refs on TVar8 that held the deg/TVar8 allocation
 *    order.  With the correct width those refs belong to the switch-head
 *    extension TEMP (both sides read $s0=temp: `addu a1,s0` args), so the
 *    order had to come from somewhere real:
 *  - The passage halving loop is a REAL `while` loop, not the Ghidra
 *    if+goto.  cc1 duplicates the 3-way abs entry test at -O2 (identical
 *    bytes), and the NOTE_INSN_LOOP notes make flow2 count the body's
 *    `TVar8 <<= 1` refs at loop weight: p84 14->16 refs = priority
 *    2413->3678, restoring TVar8 > deg > enemy = s0/s2/s3 (dmg stays s1).
 *    19 single-register rows (incl. the %100 magic in s2 and the blood
 *    counter in s0) fell together.  regalloc.py's `--between 82 87 84`
 *    window plus the .flow dump's `Register N used M times` lines are the
 *    measurement loop for this class.
 *
 * What the 90 -> 0 step proved (the 0x8001e7d8-e858 knockback family):
 *  - The knockback abs is the ASSIGNED-abs statement
 *    `iVar13 = __builtin_abs((int)(short)did);` — cc1's mips abssi2 is ONE
 *    type-"multi" insn whose template emits `bgez %1,1f%# / subu %0,$0,%0 /
 *    1:` internally (same-register form; identical bytes to bgez/nop/negu).
 *    Because the branch lives INSIDE the template, reorg never sees an
 *    unfilled bgez: nothing can steal the `move s0,a1` TVar8 copy out of
 *    the lhu load-delay slot (the explicit `if (ad < 0) ad = -ad;` spelling
 *    exposes a real branch whose backward scan ALWAYS steals that copy —
 *    provably, from reorg.c's fill_simple_delay_slots), and the missing
 *    block boundary lets the whole surrounding schedule and allocation
 *    (dtR=$v1, vy=$v0, sVar12=$a0 fresh) fall out with NO fence.  The
 *    unconditional dtR->vy store sits between the abs and the <0x400 test,
 *    where reorg lands it in the beqz delay.
 *  - The deg==3 arm is `MoveHumanoid(Me, (0x400 < __builtin_abs(
 *    (int)(short)did)) ? 0x46 : -0x46, 0)` — the abs INSIDE the call's
 *    ternary arg: a0=Me evaluates first (lw at the arm top, e830), the
 *    ±0x46 branches jump straight to the call point (no move_speed
 *    variable, no extra j/lw pair; the default-then-override spelling cost
 *    +4 length and 18 bytes).
 */


/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void DamageControl(void)

{
  MotionManager *pMVar2;
  short did;
  short deg;
  short sVar6;
  short TVar8;
  short sVar12;
  int iVar13;
  Humanoid *enemy;
  int id;
  short dmg;
  SVECTOR local_40;
  VECTOR local_38;
  SVECTOR local_28;

  id = (u16)(Me_MOTION_C->vector).pad;
  dmg = 0;
  if (Me_MOTION_C->life < 1) {
    return;
  }
  if (MotionUpdateMode != 0) {
    return;
  }
  if (motID == 0x301) {
    return;
  }
  if (Me_MOTION_C == StagePlayer) {
    SetCameraMode(CMODE_NORMAL);
  }
  if ((Me_MOTION_C->type & 0xf0U) == 0xa0) {
    enemy = (Humanoid *)ConflictObject[(short)id].common;
    if (enemy != (Humanoid *)1) {
      int iVar11;

      Sound(enemy,4);
      DeleteConflict(ConflictObject[(short)id].model);
      deg = GetAttackDBID(enemy,enemy->motion->mid);
      {
        s16 hp;

        hp = (u16)Me_MOTION_C->life - (u16)BattleDB[deg].power;
        Me_MOTION_C->life = hp;
        if ((hp * 0x10000 < 0) || ((Me_MOTION_C->attribute & 0x40U) == 0)) {
          Me_MOTION_C->life = 0;
        }
      }
      local_38.vx = dtL->vx;
      iVar11 = (u32)(u16)Me_MOTION_C->height << 0x10;
      local_38.vy = dtL->vy -
                    (((iVar11 >> 0x10) + (int)((u32)iVar11 >> 0x1f)) >> 1);
      local_38.vz = dtL->vz;
      SetImpact(&local_38,0x6000,2);
      if (StagePlayer == enemy) {
        PadShockAR(0,0xff,10,10);
      }
    }
    else {
      Me_MOTION_C->life = 0;
    }
    ReqLifeBar(Me_MOTION_C);
    if (Me_MOTION_C->life != 0) {
      motID = 0x1000;
      motMODE = 1;
      Sound(Me_MOTION_C,6);
      reset_alert_duration();
    }
    else {
      motID = 0x1100;
      motMODE = 1;
      if ((Me_MOTION_C->type != 0xa9) &&
         ((StagePlayer == enemy || (enemy == (Humanoid *)1)))) {
        if ((Me_MOTION_C->attribute & 0x42U) == 0) {
          Criticals = Criticals + 1;
        }
        else {
          Murders = Murders + 1;
        }
      }
      if ((Me_MOTION_C->attribute & 0x42U) != 0) {
        Sound(Me_MOTION_C,8);
        reset_alert_duration();
      }
    }
LAB_8001d954:
  {
    short i;

    if (MotionUpdateMode != 0) {
      for (i = 0; i < 5; i++) {
        if (CVAhuman[i].human == Me_MOTION_C) {
          goto LAB_8001d9c0;
        }
      }
    }
    SetNowMotion(Me_MOTION_C,motID,motMODE);
    motMODE = -1;
  }
LAB_8001d9c0:
    AttackCancelControl(3);
    return;
  }
  if (motID < 0x714) {
    goto LAB_8001da70;
  }
  if (motID < 0x71a) {
    goto LAB_8001da00;
  }
  if (motID == 0x1009) {
    return;
  }
  goto LAB_8001da70;
LAB_8001da00:
  ActionHalt = 0;
  if (Me_MOTION_C == StagePlayer) {
    SetCameraMode(CMODE_NORMAL);
  }
  if ((Me_MOTION_C->attribute & 0x40U) != 0) {
    motID = 0x501;
    motMODE = 1;
  }
  else {
    motID = 0;
    motMODE = 1;
  }
  dtL->vy = dtL->vy + -1;
  return;
LAB_8001da70:
  dtM->mask = 0x7fff;
  AttackCancelControl(3);
  {
  Humanoid *conflict;

  TVar8 = id;
  conflict = (Humanoid *)ConflictObject[TVar8].common;
  if (conflict == (Humanoid *)1) {
    if (Me_MOTION_C->status == 0x10) {
      return;
    }
    GetConflictResult((ModelType *)Me_MOTION_C->model,TVar8);
    TVar8 = GetItemType((int)TVar8);
    switch((short)(TVar8 - ITEM_SHURIKEN)) {
    case 1:
      dmg = 3;
      motID = 0x100a;
      motMODE = 1;
      break;
    case 0:
      if ((short)dmg == 0) {
        dmg = 0x14;
      }
      if ((Me_MOTION_C->type == 0x87) || (Me_MOTION_C->type == 0x8a)) {
        Me_MOTION_C->item[1] = Me_MOTION_C->item[1] + '\x01';
      }
      /* fall through: the shared zero-damage test preserves an existing 20 */
    case 0xe:
  switchD_8001db0c_caseD_e:
      if ((short)dmg == 0) {
        dmg = 0x1e;
      }
  switchD_8001db0c_caseD_13:
      if ((short)dmg == 0) {
        dmg = 0x14;
      }
  switchD_8001db0c_caseD_14:
      if ((short)dmg == 0) {
        dmg = 10;
      }
    {
      int iVar11;

      local_38.vx = dtL->vx;
      motID = 0x1000;
      iVar11 = (u32)(u16)Me_MOTION_C->height << 0x10;
      local_38.vy = dtL->vy -
                    (((iVar11 >> 0x10) + (int)((u32)iVar11 >> 0x1f)) >> 1);
      local_38.vz = dtL->vz;
    }
      motMODE = 1;
      SetBlood(&local_38,5,0x5a);
      break;
    case 0x13:
      goto switchD_8001db0c_caseD_13;
    case 0x14:
      goto switchD_8001db0c_caseD_14;
    case 0x15:
      dmg = 0x19;
      if ((rand() & 1) == 0) {
        motID = 0x1003;
      }
      else {
        motID = 0x1001;
      }
      motMODE = 1;
      break;
    case 3:
    case 5:
    case 0x16:
      dmg = 0x1e;
      if (TVar8 == 6) {
        dmg = 0x2d;
      }
      if ((Me_MOTION_C->map.attrib & 4U) == 0) {
        (Me_MOTION_C->map).height = 1;
      }
      break;
    default:
      dmg = 0;
      break;
    }
    if (0 < (Me_MOTION_C->map).height) {
      int iVar11;

      did = GetDirection((int)ConflictDistance.vx,(int)ConflictDistance.vz,dtR->vy);
      iVar11 = (int)did;
      if (iVar11 < 0) {
        iVar11 = -iVar11;
      }
      if (iVar11 < 0x400) {
        motID = 0x1005;
        motMODE = 0;
        dtR->vy = dtR->vy + did;
        MoveHumanoid(Me_MOTION_C,-0x46,0);
      }
      else {
        motID = 0x1006;
        motMODE = 0;
        dtR->vy = (0x800 + did) + dtR->vy;
        MoveHumanoid(Me_MOTION_C,0x46,0);
      }
    }
    if ((Me_MOTION_C == StagePlayer) && (PLAYER_REDUCE_DAMAGE_DUE_TO_ARMOUR != 0)) {
      dmg = ((short)dmg * 7) / 10;
    }
   {
    s16 iVar11;

    iVar11 = (u16)Me_MOTION_C->life - dmg;
    Me_MOTION_C->life = (short)iVar11;
    if (iVar11 * 0x10000 < 1) {
      Me_MOTION_C->life = 0;
      if (1 < (u32)(u16)motID - 0x1005) {
        motID = 0x1100;
        motMODE = 1;
      }
      Sound(Me_MOTION_C,8);
      {
      TItemType item_type;

      item_type = GetItemType((s16)id);
      if ((item_type < ITEM_GUN) ||
          ((ITEM_ARROW < item_type && (item_type != ITEM_LIGHTNINGBOLT)))) {
        if ((Me_MOTION_C->type & 0xf0U) == 0x90) {
          FriendHits = FriendHits + 1;
        }
        else if ((Me_MOTION_C->attribute & 0x42U) == 0) {
          Criticals = Criticals + 1;
        }
        else {
          Murders = Murders + 1;
        }
      }
      }
    }
    else {
      int r;
      short sound_id;

      r = rand();
      sound_id = 7;
      if ((r & 1) != 0) {
        sound_id = 6;
      }
      Sound(Me_MOTION_C,sound_id);
    }
   }
    if (StagePlayer == Me_MOTION_C) {
      PadShockAR(0,0xff,10,0x14);
    }
    else {
      ReqLifeBar(Me_MOTION_C);
    }
    reset_alert_duration();
  }
  else {
    enemy = conflict;
    if (((Me_MOTION_C->type & 0xf0U) == 0x80) && (enemy != StagePlayer)) {
      return;
  }
  {
    TVar8 = 1;
    local_40.vx = enemy->locate->vx - dtL->vx;
    local_40.vy = enemy->locate->vy - dtL->vy;
    local_40.vz = enemy->locate->vz - dtL->vz;
    while ((100 < __builtin_abs((int)local_40.vx)) ||
           (100 < __builtin_abs((int)local_40.vy)) ||
           (100 < __builtin_abs((int)local_40.vz))) {
      TVar8 = TVar8 << 1;
      /* Retail uses direct arithmetic halves for all three components. */
      local_40.vx >>= 1;
      local_40.vy >>= 1;
      local_40.vz >>= 1;
    }
    local_38 = *dtL;
    local_38.vy = local_38.vy + -1000;
    if (GetAreaMapPassage(GlobalAreaMap,&local_38,&local_40,TVar8) != (VECTOR *)0x0) {
      return;
    }
  }
  {
    int iVar11;

    did = GetDirection(enemy->locate->vx - dtL->vx,
                       enemy->locate->vz - dtL->vz,dtR->vy);
    deg = GetAttackDBID(enemy,enemy->motion->mid);
    if (Me_MOTION_C != StagePlayer) {
      if ((((Me_MOTION_C->status != 7) &&
           ((Me_MOTION_C->attribute & 0x40U) != 0)) &&
          ((Me_MOTION_C->map).height == 0)) &&
         (gNannido != DIFFICULTY_EASY)) {
        if (rand() % (EngageLevel + 1) == 0) {
          if ((Me_MOTION_C->type == 0x87) || (Me_MOTION_C->type == 0x8a)) {
            if ((rand() & 1) != 0) {
              motID = 0x602;
            }
          }
        }
        else {
          motID = 0x602;
        }
      }
    }
    iVar11 = did;
    if (iVar11 < 0) {
      iVar11 = -iVar11;
    }
    if (iVar11 < 700) {
      if (motID == 0x602) {
        goto LAB_8001e1e8;
      }
      if (motID == 0x500) {
        return;
      }
    }
    if (motID != 0x100c) {
      goto LAB_8001e5d8;
    }
LAB_8001e1e8:
    if (motID == 0x500) {
      return;
    }
    sVar6 = UpdateMotion(dtM,0x500);
    if (sVar6 != 0) {
        int conflict_id;
        VECTOR *blood_pos;

        conflict_id = (int)(*Me_MOTION_C->model->object)->id;
        if (-1 < conflict_id) {
          dtL->vx = ConflictObject[conflict_id].position.vx;
          dtL->vz = ConflictObject[conflict_id].position.vz;
        }
        pMVar2 = dtM;
        dtR->vy = dtR->vy + did;
        Me_MOTION_C->status = 5;
        pMVar2->count = 0;
        PlayMotion(pMVar2,1);
        dmg = (u16)BattleDB[deg].power;
        dtM->loop = -dmg - 8;
        MoveHumanoid(Me_MOTION_C,-((short)((dmg * 5) / 2) + 0x50),0);
        if (enemy->status == 7) {
          enemy->motion->loop = dmg / -3 - 1;
          (enemy->vector).vz = 0;
          (enemy->vector).vx = 0;
          if (StagePlayer == enemy) {
            PadShockAR(0,0x7f,10,0);
          }
        }
        DeleteConflict(ConflictObject[(short)id].model);
        blood_pos = GetAbsolutePosition(Me_MOTION_C->model->object[2],0,(short)(dmg * 10 + 100),0);
      {
        TVar8 = 0;
        do {
          local_28.vx = rand() % 100 - 0x32;
          local_28.vy = rand() % 100 - 0x32;
          local_28.vz = rand() % 100 - 0x32;
          SetBleed(blood_pos,&local_28,rand() % 0x14 + 0x14,0xffff00);
          TVar8 = TVar8 + 1;
        } while (TVar8 < 10);
      }
      {
        Humanoid *who;

        if (StagePlayer == Me_MOTION_C) {
          PadShockAR(0,0x7f,10,0);
          who = enemy;
        }
        else {
          who = Me_MOTION_C;
        }
        ReqLifeBar(who);
      }
      {
        s16 r;

        r = rand() % 0x168;
        FUN_8003944c(blood_pos,0,0x2000,0x6000,0xdcdcdc,0,r,6,9,1);
      }
        if ((rand() & 1) != 0) {
          Sound(Me_MOTION_C,10);
        }
        if ((enemy->type & 0xf0U) != 0xa0) {
          Sound(Me_MOTION_C,3);
          return;
        }
        return;
    }
  }
LAB_8001e5d8:
  {
    int conflict_id;

    conflict_id = (int)(*Me_MOTION_C->model->object)->id;
    if (-1 < conflict_id) {
      dtL->vx = ConflictObject[conflict_id].position.vx;
      dtL->vz = ConflictObject[conflict_id].position.vz;
    }
  }
    dmg = (u16)BattleDB[deg].power;
    if (enemy != StagePlayer) {
      goto LAB_8001e684;
    }
    if ((Me_MOTION_C->attribute & 0x40U) != 0) {
      goto LAB_8001e6c4;
    }
    Me_MOTION_C->life = 0;
    goto LAB_8001e6b0;
LAB_8001e684:
    if (Me_MOTION_C != StagePlayer) {
      dmg = dmg / 3;
    }
LAB_8001e6b0:
    if (enemy != StagePlayer) {
      goto LAB_8001e6d8;
    }
LAB_8001e6c4:
    dmg = dmg - ((u8)gNannido - 2);
LAB_8001e6d8:
    if (enemy->type == 0xa9) {
      dmg = (short)dmg * 6;
    }
    if (Me_MOTION_C->active_item == 0xc) {
      dmg = (int)(short)dmg / 3;
    }
    if (enemy->active_item == 0xc) {
      dmg = (u32)(dmg << 0x10) >> 0xf;
    }
  {
    if ((Me_MOTION_C == StagePlayer) && (PLAYER_REDUCE_DAMAGE_DUE_TO_ARMOUR != 0)) {
      dmg = ((short)dmg * 7) / 10;
    }
    deg = (short)dmg >> 3;
    if (3 < deg) {
      deg = 3;
    }
    if (0 < (Me_MOTION_C->map).height) {
      deg = 3;
    }
    TVar8 = (short)dmg * 5 / 2 + 0x50;
    {
      sVar12 = dtR->vy + did;
      iVar13 = __builtin_abs((int)(short)did);
      dtR->vy = sVar12;
      if (iVar13 < 0x400) {
        TVar8 = -TVar8;
      }
      else {
        dtR->vy = sVar12 + -0x800;
      }
    }
    if (deg == 3) {
      MoveHumanoid(Me_MOTION_C,
                   (0x400 < __builtin_abs((int)(short)did)) ? 0x46 : -0x46,0);
    }
    else {
      MoveHumanoid(Me_MOTION_C,TVar8,0);
    }
  }
  {
    s16 iVar11;

    iVar11 = (u16)Me_MOTION_C->life - dmg;
    Me_MOTION_C->life = (short)iVar11;
    if (iVar11 * 0x10000 < 1) {
      Me_MOTION_C->life = 0;
      D_8009770C = Me_MOTION_C;
      if (deg == 3) {
        goto LAB_8001e944;
      }
      {
        short i;

        motID = 0x1100;
        motMODE = 1;
        if (MotionUpdateMode != 0) {
          for (i = 0; i < 5; i++) {
            if (CVAhuman[i].human == Me_MOTION_C) {
              goto LAB_8001e91c;
            }
          }
        }
        SetNowMotion(Me_MOTION_C,motID,motMODE);
        motMODE = -1;
LAB_8001e91c:
        if ((rand() & 1) != 0) {
          motID = 0x1101;
          motMODE = 1;
        }
        goto LAB_8001e994;
      }
LAB_8001e944:
      {
        int absdeg;

        absdeg = (int)(short)did;
        if (absdeg < 0) {
          absdeg = -absdeg;
        }
        if (0x400 < absdeg) {
          deg = deg + 4;
        }
        dtM->mid = -1;
        motID = D_80086B6C[deg];
      }
LAB_8001e994:
      if (enemy == StagePlayer) {
        if ((Me_MOTION_C->type & 0xf0U) == 0x90) {
          FriendHits = FriendHits + 1;
        }
        else if ((Me_MOTION_C->attribute & 0x42U) == 0) {
          Criticals = Criticals + 1;
        }
        else {
          Murders = Murders + 1;
        }
      }
      if ((Me_MOTION_C->attribute & 0x42U) != 0) {
        Sound(Me_MOTION_C,8);
        goto LAB_8001eaa8;
      }
    }
    else {
      int absdeg;

      absdeg = (int)(short)did;
      if (absdeg < 0) {
        absdeg = -absdeg;
      }
      if (0x400 < absdeg) {
        deg = deg + 4;
      }
      dtM->mid = -1;
      motID = D_80086B6C[deg];
      motMODE = 0;
LAB_8001eaa8:
      reset_alert_duration();
    }
  }
    if (enemy->status == 7) {
      enemy->motion->loop = (short)dmg / -3 - 1;
      (enemy->vector).vz = 0;
      (enemy->vector).vx = 0;
      if (StagePlayer == enemy) {
        PadShockAR(0,0xff,10,10);
      }
    }
    DeleteConflict(ConflictObject[(short)id].model);
  {
    int iVar11;

    local_38.vx = dtL->vx;
    iVar11 = (u32)(u16)Me_MOTION_C->height << 0x10;
    local_38.vy = dtL->vy -
                  (((iVar11 >> 0x10) + (int)((u32)iVar11 >> 0x1f)) >> 1);
    local_38.vz = dtL->vz;
  }
    SetBlood(&local_38,5,0x78);
    SetImpact(&local_38,0x6000,2);
    {
      Humanoid *who;

      if (StagePlayer == Me_MOTION_C) {
        PadShockAR(0,0x7f,10,0x1e);
        who = enemy;
      }
      else {
        who = Me_MOTION_C;
      }
      ReqLifeBar(who);
    }
    {
      int r;
      short sound_id;

      r = rand();
      sound_id = 7;
      if ((r & 1) != 0) {
        sound_id = 6;
      }
      Sound(Me_MOTION_C,sound_id);
      r = rand();
      sound_id = 4;
      if (((r & 1) == 0)) { if ((Me_MOTION_C->life == 0)) {
        sound_id = 5;
      } }
      Sound(enemy,sound_id);
    }
  }
  }
LAB_8001ec30:
  if ((Me_MOTION_C->life == 0) && (Me_MOTION_C->item[10] != '\0')) {
    ReqItemDefault(Me_MOTION_C,ITEM_KAWARIMI);
    Me_MOTION_C->life = Me_MOTION_C->lifemax;
    if (1 < (u32)(u16)motID - 0x1005) {
      motID = 0x1002;
      motMODE = 1;
    }
  }
  (Me_MOTION_C->pad).time = 0;
  if ((dtM->mid == 0x300) || (dtM->mid == 0x302)) {
    SVECTOR *v;

    v = dtV;
    v->vz = 0;
    v->vx = 0;
    if (Me_MOTION_C->life != 0) {
      motMODE = 0xffff;
      return;
    }
    motID = 0x1108;
    motMODE = 1;
  }
 {
  short i;

  if (MotionUpdateMode != 0) {
    for (i = 0; i < 5; i++) {
      if (CVAhuman[i].human == Me_MOTION_C) {
        return;
      }
    }
  }
  SetNowMotion(Me_MOTION_C,motID,motMODE);
  motMODE = -1;
 }
  return;
}
