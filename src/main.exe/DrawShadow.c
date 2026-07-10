#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawShadow(struct Humanoid *human);
 *     EFFECT.C:1572, 89 src lines, frame 168 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s2       struct Humanoid * human
 *     stack sp+24     struct VECTOR scl
 *     reg   $s1       struct VECTOR * loc
 *     stack sp+40     struct MATRIX mat
 *     reg   $s0       int height
 *     reg   $s1       struct VECTOR * pos
 *     reg   $v1       struct SplashType * param
 *     reg   $a1       struct tag_EffectSlot * slot
 *     reg   $a2       int i
 *     stack sp+72     struct PARAM_ITEM_LAUNCH param
 *     reg   $s1       struct ModelType * model
 *     stack sp+112    struct VECTOR pos
 *     stack sp+128    struct SVECTOR sv
 *     reg   $t2       short time
 *     reg   $s1       struct _GsCOORDINATE2 * super
 *     reg   $v1       struct FrameType * param
 *     reg   $t1       struct tag_EffectSlot * slot
 *     reg   $a2       int i
 *     stack sp+72     struct SVECTOR scr
 *     stack sp+148    long flag
 *     stack sp+144    long p
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern short RefrectVector[16];
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawShadow", DrawShadow);

// triage: HARD — 280 insns, mul/div, 1 loop, 9 callees, ~0.07 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void DrawShadow(Humanoid *human)
//
// {
//   short sVar1;
//   ushort uVar2;
//   GsCOORDINATE2 *pGVar3;
//   VECTOR *pVVar4;
//   int iVar5;
//   int iVar6;
//   long lVar7;
//   int iVar8;
//   tag_EffectSlot *ptVar9;
//   VECTOR local_50;
//   MATRIX MStack_40;
//   long lStack_20;
//   undefined2 local_1c;
//   long lStack_18;
//   long lStack_14;
//
//   sVar1 = (human->model->rotate).pad;
//   pVVar4 = GetAbsolutePosition(*human->model->object,0,0,0);
//   iVar6 = (human->map).level;
//   if ((iVar6 < pVVar4->vy) || (iVar6 == -0x80000000)) {
//     (human->map).attrib = (human->map).attrib | 8;
//   }
//   pVVar4->vy = (human->map).level;
//   pGVar3 = DAT_80097f34;
//   uVar2 = (human->map).attrib;
//   if ((uVar2 & 8) == 0) {
//     if ((uVar2 & 0x100) == 0) {
//       (DAT_80097f34->coord).t[0] = pVVar4->vx;
//       (pGVar3->coord).t[1] = pVVar4->vy;
//       (pGVar3->coord).t[2] = pVVar4->vz;
//       local_50.vx = sVar1 * -4 - ((human->map).height >> 1);
//       if ((human->map).angleH == '\0') {
//         *(undefined2 *)&pGVar3[1].flg = 0;
//         *(undefined2 *)((int)&pGVar3[1].flg + 2) = 0;
//         pGVar3[1].coord.m[0][0] = 0;
//       }
//       else {
//         *(undefined2 *)&pGVar3[1].flg = 0x100;
//         sVar1 = RefrectVector[(human->map).angleH];
//         pGVar3[1].coord.m[0][0] = 0;
//         *(short *)((int)&pGVar3[1].flg + 2) = sVar1;
//       }
//       local_50.vy = local_50.vx;
//       local_50.vz = local_50.vx;
//       RotMatrixYXZ((SVECTOR *)(DAT_80097f34 + 1),&DAT_80097f34->coord);
//       ScaleMatrix(&DAT_80097f34->coord,&local_50);
//       pGVar3 = DAT_80097f34;
//       DAT_80097f34->flg = 0;
//       GsGetLs(pGVar3,&MStack_40);
//       GsSetLsMatrix(&MStack_40);
//       lVar7 = RotTransPers(&UnitVector,&lStack_20,&lStack_18,&lStack_14);
//       local_1c = (undefined2)lVar7;
//       if ((lVar7 << 0x10) >> 0x12 < 0x4e2) {
//         GsSortObject4((GsDOBJ2 *)(DAT_80097f34[1].coord.m[2] + 2),OTablePt,2,(u_long *)Scratchpad);
//       }
//     }
//     else if ((human->map).height == 0) {
//       if ((GameClock & 0x3fU) == 1) {
//         FUN_80037e0c(human,1);
//       }
//       else if ((GameClock & 0xfU) == 0) {
//         FUN_80037e0c(human,0);
//       }
//     }
//   }
//   else {
//     iVar6._0_2_ = (human->vector).vx;
//     iVar6._2_2_ = (human->vector).vy;
//     if ((((iVar6 != 0) || (sVar1 = human->motion->mid, sVar1 == 0x804)) || (sVar1 == 0x710)) &&
//        (((human->map).height == 0 && ((GameClock & 1U) != 0)))) {
//       if (human->status == 3) {
//         iVar6 = rand();
//         pVVar4->vy = pVVar4->vy + 100;
//         pVVar4->vx = pVVar4->vx + -300 + iVar6 % 600;
//         iVar5 = rand();
//         iVar8 = pVVar4->vz + -300;
//         iVar6 = (iVar5 / 600) * 0x4b;
//       }
//       else {
//         iVar6 = rand();
//         pVVar4->vx = pVVar4->vx + -100 + iVar6 % 200;
//         iVar5 = rand();
//         iVar8 = pVVar4->vz + -100;
//         iVar6 = (iVar5 / 200) * 0x19;
//       }
//       pVVar4->vz = iVar8 + iVar5 + iVar6 * -8;
//       iVar5 = 0;
//       ptVar9 = EffectSlot + DAT_80097a3c;
//       iVar6 = DAT_80097a3c;
//       do {
//         iVar6 = iVar6 + 1;
//         ptVar9 = ptVar9 + 1;
//         if (199 < iVar6) {
//           iVar6 = 0;
//           ptVar9 = EffectSlot;
//         }
//         iVar5 = iVar5 + 1;
//         if (ptVar9->proc == (undefined **)0x0) {
//           DAT_80097a3c = iVar6 + 1;
//           if (199 < DAT_80097a3c) {
//             DAT_80097a3c = 0;
//           }
//           goto LAB_80038620;
//         }
//       } while (iVar5 < 200);
//       ptVar9 = &dmy;
// LAB_80038620:
//       (ptVar9->param).blood.hint = (AreaNodeType *)pVVar4->vx;
//       (ptVar9->param).blood.px = pVVar4->vy;
//       lVar7 = pVVar4->vz;
//       (ptVar9->param).splash.sx = 0x2000;
//       (ptVar9->param).splash.sy = 0x2000;
//       (ptVar9->param).splash.speed = '\x04';
//       (ptVar9->param).splash.mode = '\0';
//       (ptVar9->param).blood.py = lVar7;
//       ptVar9->proc = (undefined **)DrawSplash;
//     }
//   }
//   return;
// }
