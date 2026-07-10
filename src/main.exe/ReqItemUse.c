#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemUse(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:3631, 386 src lines, frame 440 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s1       int i
 *     stack sp+16     struct PARAM_ITEM_DROP param
 *     stack sp+56     struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+76     int ry
 *     stack sp+72     int rx
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_launch * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a0       int i
 *     reg   $v1       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     stack sp+96     struct PARAM_ITEM_LAUNCH param
 *     stack sp+72     struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+144    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+160    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+176    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+192    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+208    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+224    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+240    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+256    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+272    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+288    struct PARAM_ITEM_DROP param
 *     stack sp+328    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a0       int i
 *     reg   $v1       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     stack sp+344    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a0       int i
 *     reg   $v1       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     stack sp+368    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+384    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a0       int i
 *     reg   $v1       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     stack sp+400    struct VECTOR target
 *     stack sp+416    int rx
 *     stack sp+420    int ry
 *     stack sp+136    struct SVECTOR rot
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_napalm * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a0       int i
 *     reg   $v1       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 *     extern long GameClock;
 * END PSX.SYM */

/*
 * ReqItemUse (0x80048afc) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ReqItemUse
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", ReqItemUse);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_9);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_e);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_8);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_11);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_10);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_17);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_d);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_18);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_c);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_14);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_15);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_16);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_b);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_12);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ReqItemUse", switchD_80048b6c__caseD_13);

/* jump-table pool @ 0x800122ec (25 words; tables at 0x800122ec) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ReqItemUse_jtbl[25] = {
    0x800498A4, 0x80048D88, 0x80048B74, 0x80049C00,
    0x80049554, 0x80048F48, 0x80049730, 0x80049018,
    0x80049288, 0x800490E8, 0x80049F60, 0x80049F50,
    0x80049CD0, 0x800499F8, 0x800491B8, 0x80049484,
    0x800493B4, 0x80049358, 0x80049F70, 0x80049F78,
    0x80049DA0, 0x80049DB0, 0x80049DC0, 0x80049624,
    0x80049AC8,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// /* WARNING: Type propagation algorithm not settling */
// 
// int ReqItemUse(PARAM_ITEM_USE *p)
// 
// {
//   uchar uVar1;
//   bool bVar2;
//   undefined **ppuVar3;
//   PARAM_ITEM_USE *pPVar4;
//   PARAM_ITEM_USE *pPVar5;
//   ModelType *pMVar6;
//   ModelArchiveType *pMVar7;
//   int iVar8;
//   TItemType TVar9;
//   int iVar10;
//   long lVar11;
//   Humanoid *pHVar12;
//   long lVar13;
//   VECTOR *pVVar14;
//   tag_TItem *ptVar15;
//   VECTOR *pVVar16;
//   PARAM_ITEM_USE local_d0;
//   VECTOR local_a8;
//   long local_98;
//   long local_94;
//   VECTOR VStack_88;
//   int local_78;
//   int local_74;
//   int local_70;
//   int local_6c;
//   int local_68;
//   int local_64;
//   int local_60;
//   int local_5c;
//   int local_58;
//   int local_54;
//   int local_50;
//   int local_4c;
//   int local_48;
//   int local_44;
//   int local_40;
//   int local_3c;
//   int local_38;
//   int local_34;
//   int local_30;
//   int local_2c;
//   int local_28;
//   int local_24;
//   int local_20;
//   int local_1c;
//   int local_18;
//   int local_14;
//   
//   uVar1 = p->user->item[p->type];
//   if ((uVar1 != '\0') && (uVar1 != 0xff)) {
//     p->user->item[p->type] = uVar1 + 0xff;
//   }
//   lVar11 = DAT_80012280;
//   pHVar12 = DAT_8001227c;
//   switch(p->type) {
//   case ITEM_KAGINAWA:
//     iVar8 = 0;
//     do {
//       DAT_80097acc = DAT_80097acc + 1;
//       if (0x1d < DAT_80097acc) {
//         DAT_80097acc = 0;
//       }
//       iVar10 = DAT_80097acc;
//       ptVar15 = items + DAT_80097acc;
//       if (items[DAT_80097acc].proc == (undefined **)0x0) goto LAB_80049958;
//       iVar8 = iVar8 + 1;
//     } while (iVar8 < 0x1d);
//     ppuVar3 = items[DAT_80097acc].proc;
//     items[DAT_80097acc].mode = 0xff;
//     (*(code *)ppuVar3)(ptVar15);
//     DeleteConflict(items[iVar10].locate);
//     if (items[iVar10].mode != 0) {
//       AdtMessageBox("item dispose fail   id %d  mode %d",items[iVar10].type,(uint)items[iVar10].mode
//                    );
//     }
//     ptVar15->owner = (Humanoid *)0x0;
//     items[iVar10].proc = (undefined **)0x0;
// LAB_80049958:
//     if (ptVar15 == (tag_TItem *)0x0) {
//       return 0;
//     }
//     TVar9 = p->type;
//     ptVar15->owner = p->user;
//     pMVar6 = items[iVar10].locate;
//     items[iVar10].proc = (undefined **)ProcKaginawa;
//     items[iVar10].mode = '\0';
//     items[iVar10].type = TVar9;
//     (pMVar6->locate).coord.t[0] = (p->start).vx;
//     ((items[iVar10].locate)->locate).coord.t[1] = (p->start).vy;
//     ((items[iVar10].locate)->locate).coord.t[2] = (p->start).vz;
//     ((items[iVar10].locate)->locate).super = (_GsCOORDINATE2 *)0x0;
//     UpdateCoordinate(items[iVar10].locate);
//     pHVar12 = ptVar15->owner;
//     items[iVar10].collision.size = 0;
//     items[iVar10].model = (ModelType *)0x0;
//     pHVar12->field_0xcd = 1;
//     SetCameraMode(CMODE_SIGHT);
//     CamState.DirectionRX = -0x155;
//     CamState.DirectionRY = 0;
//     goto switchD_80048b6c_caseD_13;
//   case ITEM_SHURIKEN:
//     if (p->user == CamState.Owner) {
//       iVar8 = 0;
//       do {
//         DAT_80097acc = DAT_80097acc + 1;
//         if (0x1d < DAT_80097acc) {
//           DAT_80097acc = 0;
//         }
//         iVar10 = DAT_80097acc;
//         ptVar15 = items + DAT_80097acc;
//         if (items[DAT_80097acc].proc == (undefined **)0x0) goto LAB_80048e54;
//         iVar8 = iVar8 + 1;
//       } while (iVar8 < 0x1d);
//       ppuVar3 = items[DAT_80097acc].proc;
//       items[DAT_80097acc].mode = 0xff;
//       (*(code *)ppuVar3)(ptVar15);
//       DeleteConflict(items[iVar10].locate);
//       if (items[iVar10].mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",items[iVar10].type,
//                       (uint)items[iVar10].mode);
//       }
//       ptVar15->owner = (Humanoid *)0x0;
//       items[iVar10].proc = (undefined **)0x0;
// LAB_80048e54:
//       if (ptVar15 == (tag_TItem *)0x0) {
//         return 0;
//       }
//       TVar9 = p->type;
//       ptVar15->owner = p->user;
//       pMVar6 = items[iVar10].locate;
//       items[iVar10].proc = (undefined **)ProcSightShot;
//       items[iVar10].mode = '\0';
//       items[iVar10].type = TVar9;
//       (pMVar6->locate).coord.t[0] = (p->start).vx;
//       ((items[iVar10].locate)->locate).coord.t[1] = (p->start).vy;
//       ((items[iVar10].locate)->locate).coord.t[2] = (p->start).vz;
//       ((items[iVar10].locate)->locate).super = (_GsCOORDINATE2 *)0x0;
//       UpdateCoordinate(items[iVar10].locate);
//       pMVar6 = DAT_80097f48;
//       items[iVar10].collision.size = 0;
//       items[iVar10].model = pMVar6;
//       items[iVar10].param.launch.count = '\x05';
//       ptVar15->owner->field_0xcd = 1;
//     }
//     else {
//       local_d0.type = p->type;
//       local_d0.user = p->user;
//       local_d0.start.vx = (p->start).vx;
//       local_d0.start.vy = (p->start).vy;
//       local_d0.start.vz = (p->start).vz;
//       SearchItemTarget2(local_d0.user,&(local_d0.user)->model->rotate,&local_d0.start,&local_d0.end)
//       ;
//       ReqItemLaunch(&local_d0);
//       p->user->field_0xcd = 0;
//     }
//     goto switchD_80048b6c_caseD_13;
//   case ITEM_MAKIBISHI:
//     memset((uchar *)&local_a8,'\0',0x28);
//     local_a8.vx = p->type;
//     local_a8.vy = (long)p->user;
//     local_a8.vz = (p->start).vx;
//     local_a8.pad = (p->start).vy;
//     local_98 = (p->start).vz;
//     local_94 = (p->start).pad;
//     pPVar4 = &local_d0;
//     pVVar14 = &local_a8;
//     do {
//       pVVar16 = pVVar14;
//       pPVar5 = pPVar4;
//       pHVar12 = (Humanoid *)pVVar16->vy;
//       lVar11 = pVVar16->vz;
//       lVar13 = pVVar16->pad;
//       pPVar5->type = pVVar16->vx;
//       pPVar5->user = pHVar12;
//       (pPVar5->start).vx = lVar11;
//       (pPVar5->start).vy = lVar13;
//       pVVar14 = pVVar16 + 1;
//       pPVar4 = (PARAM_ITEM_USE *)&(pPVar5->start).vz;
//     } while (pVVar14 != &VStack_88);
//     lVar11 = pVVar16[1].vy;
//     *(long *)pPVar4 = pVVar14->vx;
//     (pPVar5->start).pad = lVar11;
//     local_a8.vx = DAT_80012268;
//     local_a8.vy = DAT_8001226c;
//     local_a8.vz = DAT_80012270;
//     local_a8.pad = DAT_80012274;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&VStack_88.vz,&VStack_88.pad);
//       iVar8 = 0;
//     }
//     else {
//       VStack_88.vz = (long)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       VStack_88.pad = (long)(pMVar7->rotate).vy;
//     }
//     iVar10 = 0;
//     RotateVector(&local_a8,VStack_88.vz,VStack_88.pad,iVar8);
//     bVar2 = true;
//     while (bVar2) {
//       iVar10 = iVar10 + 1;
//       iVar8 = rand();
//       local_d0.end.vx = local_a8.vx + iVar8 % 100 + -0x32;
//       iVar8 = rand();
//       local_d0.end.vy = (local_a8.vy - iVar8 % 0x32) + -0x32;
//       iVar8 = rand();
//       local_d0.end.vz = local_a8.vz + iVar8 % 100 + -0x32;
//       ReqItemMakibishi((PARAM_ITEM_DROP *)&local_d0);
//       bVar2 = iVar10 < 5;
//     }
//     break;
//   case ITEM_KUSURI:
//     local_d0.type = DAT_800122a8;
//     local_d0.user = DAT_800122ac;
//     local_d0.start.vx = DAT_800122b0;
//     local_d0.start.vy = DAT_800122b4;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_20,&local_1c);
//       iVar8 = 0;
//     }
//     else {
//       local_20 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_1c = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_20,local_1c,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemKusuri(p);
//     break;
//   case ITEM_FIRE:
//     local_d0.type = DAT_800122a8;
//     local_d0.user = DAT_800122ac;
//     local_d0.start.vx = DAT_800122b0;
//     local_d0.start.vy = DAT_800122b4;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_40,&local_3c);
//       iVar8 = 0;
//     }
//     else {
//       local_40 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_3c = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_40,local_3c,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemFire(p);
//     break;
//   case ITEM_SMOKE:
//     local_d0.type = DAT_80012278;
//     local_d0.user = DAT_8001227c;
//     local_d0.start.vx = DAT_80012280;
//     local_d0.start.vy = DAT_80012284;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_78,&local_74);
//       iVar8 = 0;
//     }
//     else {
//       local_78 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_74 = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_78,local_74,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemSmoke(p);
//     break;
//   case ITEM_JIRAI:
//     memset((uchar *)&local_a8,'\0',0x28);
//     local_a8.vx = p->type;
//     local_a8.vy = (long)p->user;
//     local_a8.vz = (p->start).vx;
//     local_a8.pad = (p->start).vy;
//     local_98 = (p->start).vz;
//     local_94 = (p->start).pad;
//     pPVar4 = &local_d0;
//     pVVar14 = &local_a8;
//     do {
//       pVVar16 = pVVar14;
//       pPVar5 = pPVar4;
//       pHVar12 = (Humanoid *)pVVar16->vy;
//       lVar13 = pVVar16->vz;
//       lVar11 = pVVar16->pad;
//       pPVar5->type = pVVar16->vx;
//       pPVar5->user = pHVar12;
//       (pPVar5->start).vx = lVar13;
//       (pPVar5->start).vy = lVar11;
//       pVVar14 = pVVar16 + 1;
//       pPVar4 = (PARAM_ITEM_USE *)&(pPVar5->start).vz;
//     } while (pVVar14 != &VStack_88);
//     lVar11 = pVVar16[1].vy;
//     *(long *)pPVar4 = pVVar14->vx;
//     (pPVar5->start).pad = lVar11;
//     local_a8.vx = DAT_800122c8;
//     local_a8.vy = DAT_800122cc;
//     local_a8.vz = DAT_800122d0;
//     local_a8.pad = DAT_800122d4;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_30,&local_2c);
//       iVar8 = 0;
//     }
//     else {
//       local_30 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_2c = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector(&local_a8,local_30,local_2c,iVar8);
//     local_d0.start.vx = local_d0.start.vx + local_a8.vx;
//     local_d0.end.vx = local_a8.vx;
//     local_d0.end.vy = local_a8.vy;
//     local_d0.end.vz = local_a8.vz;
//     local_d0.start.vy = local_d0.start.vy + local_a8.vy;
//     local_d0.start.vz = local_d0.start.vz + local_a8.vz;
//     ReqItemJirai((PARAM_ITEM_DROP *)&local_d0);
//     break;
//   case ITEM_DOKUDANGO:
//     local_d0.type = DAT_80012288;
//     local_d0.user = DAT_8001228c;
//     local_d0.start.vx = DAT_80012290;
//     local_d0.start.vy = DAT_80012294;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_70,&local_6c);
//       iVar8 = 0;
//     }
//     else {
//       local_70 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_6c = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_70,local_6c,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemDokudango(p);
//     break;
//   case ITEM_GOSHIKIMAI:
//     local_d0.type = DAT_80012258;
//     local_d0.user = DAT_8001225c;
//     local_d0.start.vx = DAT_80012260;
//     local_d0.start.vy = DAT_80012264;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_58,&local_54);
//       iVar8 = 0;
//     }
//     else {
//       local_58 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_54 = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_58,local_54,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemGoshikimai(p);
//     break;
//   case ITEM_NEMURI:
//     local_d0.type = DAT_80012298;
//     local_d0.user = DAT_8001229c;
//     local_d0.start.vx = DAT_800122a0;
//     local_d0.start.vy = DAT_800122a4;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_68,&local_64);
//       iVar8 = 0;
//     }
//     else {
//       local_68 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_64 = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_68,local_64,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemNemuri(p);
//     break;
//   case ITEM_KAWARIMI:
//     ReqItemKawarimi(p);
//     break;
//   case ITEM_HENSHIN:
//     ReqItemHenshin(p);
//     break;
//   case ITEM_GOSIN:
//     local_d0.type = DAT_800122a8;
//     local_d0.user = DAT_800122ac;
//     local_d0.start.vx = DAT_800122b0;
//     local_d0.start.vy = DAT_800122b4;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_18,&local_14);
//       iVar8 = 0;
//     }
//     else {
//       local_18 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_14 = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_18,local_14,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemGosin(p);
//     break;
//   case ITEM_SHINSOKU:
//     local_d0.type = DAT_800122d8;
//     local_d0.user = DAT_800122dc;
//     local_d0.start.vx = DAT_800122e0;
//     local_d0.start.vy = DAT_800122e4;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_28,&local_24);
//       iVar8 = 0;
//     }
//     else {
//       local_28 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_24 = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_28,local_24,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemShinsoku(p);
//     break;
//   case ITEM_NINGYO:
//     local_d0.type = DAT_800122a8;
//     local_d0.user = DAT_800122ac;
//     local_d0.start.vx = DAT_800122b0;
//     local_d0.start.vy = DAT_800122b4;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_60,&local_5c);
//       iVar8 = 0;
//     }
//     else {
//       local_60 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_5c = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_60,local_5c,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemNingyo(p);
//     break;
//   case ITEM_HAPPOU:
//     local_d0.type = DAT_800122a8;
//     local_d0.user = DAT_800122ac;
//     local_d0.start.vx = DAT_800122b0;
//     local_d0.start.vy = DAT_800122b4;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_48,&local_44);
//       iVar8 = 0;
//     }
//     else {
//       local_48 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_44 = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_48,local_44,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemHappou(p);
//     break;
//   case ITEM_NINKEN:
//     local_d0.type = DAT_800122a8;
//     local_d0.user = DAT_800122ac;
//     local_d0.start.vx = DAT_800122b0;
//     local_d0.start.vy = DAT_800122b4;
//     pMVar7 = p->user->model;
//     if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_50,&local_4c);
//       iVar8 = 0;
//     }
//     else {
//       local_50 = (int)(pMVar7->rotate).vx;
//       iVar8 = (int)(pMVar7->rotate).vz;
//       local_4c = (int)(pMVar7->rotate).vy;
//     }
//     RotateVector((VECTOR *)&local_d0,local_50,local_4c,iVar8);
//     (p->end).vx = local_d0.type;
//     (p->end).vy = (long)local_d0.user;
//     (p->end).vz = local_d0.start.vx;
//     ReqItemNinken(p);
//     break;
//   case ITEM_KAENGEKI:
//     local_d0.type = DAT_80012278;
//     local_d0.user = DAT_8001227c;
//     local_d0.start.vx = DAT_80012280;
//     local_d0.start.vy = DAT_80012284;
//     (p->end).vx = DAT_80012278;
//     (p->end).vy = (long)pHVar12;
//     (p->end).vz = lVar11;
//     ReqItemKaengeki(p);
//     break;
//   case ITEM_MANEBUE:
//     ReqItemManebue(p);
//   default:
//     goto switchD_80048b6c_caseD_13;
//   case ITEM_GUN:
//                     /* calls ProcItemGun */
//     FUN_80046854(p);
//     break;
//   case ITEM_ARROW:
//     ReqItemArrow(p);
//     break;
//   case ITEM_NAPALM:
//     iVar8 = 0;
//     do {
//       DAT_80097acc = DAT_80097acc + 1;
//       if (0x1d < DAT_80097acc) {
//         DAT_80097acc = 0;
//       }
//       iVar10 = DAT_80097acc;
//       ptVar15 = items + DAT_80097acc;
//       if (items[DAT_80097acc].proc == (undefined **)0x0) goto LAB_80049e78;
//       iVar8 = iVar8 + 1;
//     } while (iVar8 < 0x1d);
//     ppuVar3 = items[DAT_80097acc].proc;
//     items[DAT_80097acc].mode = 0xff;
//     (*(code *)ppuVar3)(ptVar15);
//     DeleteConflict(items[iVar10].locate);
//     if (items[iVar10].mode != 0) {
//       AdtMessageBox("item dispose fail   id %d  mode %d",items[iVar10].type,(uint)items[iVar10].mode
//                    );
//     }
//     ptVar15->owner = (Humanoid *)0x0;
//     items[iVar10].proc = (undefined **)0x0;
// LAB_80049e78:
//     if (ptVar15 == (tag_TItem *)0x0) {
//       return 0;
//     }
//     if ((GameClock & 1U) == 0) {
//       return 0;
//     }
//     TVar9 = p->type;
//     ptVar15->owner = p->user;
//     pMVar6 = items[iVar10].locate;
//     items[iVar10].proc = (undefined **)ProcItemNapalm;
//     items[iVar10].mode = '\0';
//     items[iVar10].type = TVar9;
//     (pMVar6->locate).coord.t[0] = (p->start).vx;
//     ((items[iVar10].locate)->locate).coord.t[1] = (p->start).vy;
//     ((items[iVar10].locate)->locate).coord.t[2] = (p->start).vz;
//     ((items[iVar10].locate)->locate).super = (_GsCOORDINATE2 *)0x0;
//     UpdateCoordinate(items[iVar10].locate);
//     pMVar6 = DAT_80097f5c;
//     items[iVar10].collision.size = 0;
//     items[iVar10].model = pMVar6;
//     items[iVar10].param.napalm.vec.vx = (short)(p->end).vx - (short)(p->start).vx;
//     items[iVar10].param.napalm.vec.vy = (short)(p->end).vy - (short)(p->start).vy;
//     items[iVar10].param.napalm.vec.vz = (short)(p->end).vz - (short)(p->start).vz;
//     goto switchD_80048b6c_caseD_13;
//   case ITEM_LIGHTNINGBOLT:
//     if (p->user == CamState.Owner) {
//       local_d0.type = DAT_800122b8;
//       local_d0.user = DAT_800122bc;
//       local_d0.start.vx = DAT_800122c0;
//       local_d0.start.vy = DAT_800122c4;
//       pMVar7 = p->user->model;
//       if (((CamState.Owner)->model == pMVar7) && (CamState.Mode == CMODE_DIRECTION)) {
//         GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_38,&local_34);
//         iVar8 = 0;
//       }
//       else {
//         local_38 = (int)(pMVar7->rotate).vx;
//         iVar8 = (int)(pMVar7->rotate).vz;
//         local_34 = (int)(pMVar7->rotate).vy;
//       }
//       RotateVector((VECTOR *)&local_d0,local_38,local_34,iVar8);
//       iVar8 = (p->start).vx;
//       iVar10 = (p->start).vz;
//       (p->end).vx = local_d0.type;
//       (p->end).vy = (long)local_d0.user;
//       (p->end).vx = (p->end).vx + iVar8;
//       (p->end).vz = local_d0.start.vx;
//       (p->end).vy = (p->end).vy + (p->start).vy;
//       (p->end).vz = (p->end).vz + iVar10;
//     }
//     ReqItemLightningBolt(p);
//     break;
//   case ITEM_TELEPORT:
//     iVar8 = 0;
//     do {
//       DAT_80097acc = DAT_80097acc + 1;
//       if (0x1d < DAT_80097acc) {
//         DAT_80097acc = 0;
//       }
//       iVar10 = DAT_80097acc;
//       ptVar15 = items + DAT_80097acc;
//       if (items[DAT_80097acc].proc == (undefined **)0x0) goto LAB_80049b7c;
//       iVar8 = iVar8 + 1;
//     } while (iVar8 < 0x1d);
//     ppuVar3 = items[DAT_80097acc].proc;
//     items[DAT_80097acc].mode = 0xff;
//     (*(code *)ppuVar3)(ptVar15);
//     DeleteConflict(items[iVar10].locate);
//     if (items[iVar10].mode != 0) {
//       AdtMessageBox("item dispose fail   id %d  mode %d",items[iVar10].type,(uint)items[iVar10].mode
//                    );
//     }
//     ptVar15->owner = (Humanoid *)0x0;
//     items[iVar10].proc = (undefined **)0x0;
// LAB_80049b7c:
//     if (ptVar15 == (tag_TItem *)0x0) {
//       return 0;
//     }
//     TVar9 = p->type;
//     ptVar15->owner = p->user;
//     pMVar6 = items[iVar10].locate;
//     items[iVar10].proc = (undefined **)ProcItemTeleport;
//     items[iVar10].mode = '\0';
//     items[iVar10].type = TVar9;
//     (pMVar6->locate).coord.t[0] = (p->start).vx;
//     ((items[iVar10].locate)->locate).coord.t[1] = (p->start).vy;
//     ((items[iVar10].locate)->locate).coord.t[2] = (p->start).vz;
//     ((items[iVar10].locate)->locate).super = (_GsCOORDINATE2 *)0x0;
//     UpdateCoordinate(items[iVar10].locate);
//     items[iVar10].collision.size = 0;
//     items[iVar10].model = (ModelType *)0x0;
//     CamState.Mode = CMODE_SIGHT;
// switchD_80048b6c_caseD_13:
//   }
//   return 1;
// }

#endif /* NON_MATCHING */
