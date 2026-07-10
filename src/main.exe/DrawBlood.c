#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawBlood(struct tag_EffectSlot *ef);
 *     EFFECT.C:657, 85 src lines, frame 80 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s0       struct tag_EffectSlot * ef
 *     reg   $s2       struct BloodType * blood
 *     reg   $s3       struct GsSPRITE * spr
 *     reg   $s2       struct AreaNodeType ** hint
 *     reg   $t3       long x
 *     reg   $t2       long y
 *     reg   $a3       long z
 *     reg   $a2       int sz
 *     reg   $t1       int sy
 *     reg   $a1       int sx
 *     reg   $v1       long rety
 *     reg   $a0       struct AreaNodeType * area
 *     stack sp+24     struct VECTOR pos
 *     stack sp+40     struct SVECTOR vec
 *     reg   $s3       struct GsSPRITE * sprt
 *     reg   $s0       long scale
 *     stack sp+24     struct SVECTOR scr
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprBlood;
 *     extern struct GsOT *OTablePt;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 *     extern long GameClock;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawBlood", DrawBlood);

// triage: HARD — 540 insns, mul/div, 8 callees, ~0.08 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void DrawBlood(tag_EffectSlot *ef)
//
// {
//   byte bVar1;
//   undefined1 uVar2;
//   ushort uVar3;
//   short sVar4;
//   uint uVar5;
//   int iVar6;
//   int iVar7;
//   AreaNodeType *node;
//   GsSPRITE *_29;
//   int iVar8;
//   long lVar9;
//   uchar uVar10;
//   int iVar11;
//   GsSPRITE *_29_00;
//   SVECTOR local_48;
//   VECTOR local_40;
//   SVECTOR local_30;
//   int local_28;
//   long local_24;
//
//   uVar5 = (uint)*(byte *)((int)&ef->param + 0x22);
//   iVar7 = uVar5 * 0x24;
//   _29_00 = &sprBlood + uVar5;
//   _29 = (GsSPRITE *)(&DAT_800be9e8 + uVar5 * 9);
//   bVar1 = *(byte *)((int)&ef->param + 0x23);
//   if (bVar1 == 2) {
//     uVar3 = (ef->param).blood.time;
//     (ef->param).blood.time = uVar3 - 1;
//     if ((int)((uint)uVar3 << 0x10) < 1) {
//       (ef->param).blood.time = 0x80;
//       *(char *)((int)&ef->param + 0x23) = *(char *)((int)&ef->param + 0x23) + '\x01';
//     }
//   }
//   else {
//     if (bVar1 < 3) {
//       if (bVar1 == 1) {
//         iVar6 = rand();
//         iVar7 = iVar6;
//         if (iVar6 < 0) {
//           iVar7 = iVar6 + 0xfff;
//         }
//         uVar3 = (ef->param).blood.time;
//         (ef->param).blood.scale = (ef->param).blood.scale + iVar6 + (iVar7 >> 0xc) * -0x1000;
//         (ef->param).blood.time = uVar3 - 1;
//         if ((int)((uint)uVar3 << 0x10) < 1) {
//           *(char *)((int)&ef->param + 0x23) = *(char *)((int)&ef->param + 0x23) + '\x01';
//           iVar7 = rand();
//           (ef->param).blood.time = (short)iVar7 + (short)(iVar7 / 0x5a) * -0x5a;
//         }
//         goto LAB_8003306c;
//       }
//     }
//     else if (bVar1 == 3) {
//       uVar3 = *(short *)((int)&ef->param + 0x20) - 5;
//       *(ushort *)((int)&ef->param + 0x20) = uVar3;
//       if ((int)((uint)uVar3 << 0x10) < 1) {
//         *(undefined2 *)((int)&ef->param + 0x20) = 0;
//         ef->proc = (undefined **)0x0;
//       }
//       _29_00->attribute = 0x50000000;
//       iVar6 = (ef->param).blood.scale;
//       iVar11 = (ef->param).blood.py + (int)(ef->param).blood.vy;
//       uVar3 = *(ushort *)((int)&ef->param + 0x20);
//       (ef->param).blood.py = iVar11;
//       lVar9 = (ef->param).blood.rotate;
//       iVar8 = (uint)uVar3 << 0x10;
//       FUN_800396c0((ef->param).blood.px,iVar11,(ef->param).blood.pz,&local_48);
//       iVar11 = (int)local_48.vz;
//       if (iVar11 < 0x25) {
//         return;
//       }
//       iVar6 = iVar6 * 300;
//       if (iVar11 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar11 == -1) && (iVar6 == -0x80000000)) {
//         trap(0x1800);
//       }
//       sVar4 = (short)(iVar6 / iVar11) + 1;
//       (&sprBlood)[uVar5].scaley = sVar4;
//       (&sprBlood)[uVar5].scalex = sVar4;
//       *(short *)(&DAT_800bea06 + iVar7) = sVar4;
//       *(short *)(&DAT_800bea04 + iVar7) = sVar4;
//       (&sprBlood)[uVar5].rotate = lVar9;
//       *(long *)(&DAT_800bea08 + iVar7) = lVar9;
//       (&sprBlood)[uVar5].x = local_48.vx;
//       *(short *)(&DAT_800be9ec + iVar7) = local_48.vx;
//       (&sprBlood)[uVar5].y = local_48.vy;
//       *(short *)(&DAT_800be9ee + iVar7) = local_48.vy;
//       uVar10 = (uchar)uVar3;
//       (&sprBlood)[uVar5].r = uVar10;
//       (&sprBlood)[uVar5].g = uVar10;
//       (&sprBlood)[uVar5].b = uVar10;
//       uVar2 = (undefined1)((iVar8 >> 0x10) - (iVar8 >> 0x1f) >> 1);
//       (&DAT_800be9fc)[iVar7] = uVar2;
//       (&DAT_800be9fd)[iVar7] = uVar2;
//       (&DAT_800be9fe)[iVar7] = uVar2;
//       iVar7 = (int)(local_48._4_4_ << 0x10) >> 0x12;
//       if (iVar7 < 0) {
//         iVar6 = 0;
//       }
//       else {
//         iVar6 = 0x4e1;
//         if (iVar7 < 0x4e2) {
//           iVar6 = iVar7;
//         }
//       }
//       GsSortSprite(_29_00,OTablePt,(ushort)iVar6);
//       iVar7 = (int)(local_48._4_4_ << 0x10) >> 0x12;
//       if (iVar7 < 0) {
//         iVar6 = 0;
//       }
//       else {
//         iVar6 = 0x4e1;
//         if (iVar7 < 0x4e2) {
//           iVar6 = iVar7;
//         }
//       }
//       uVar3 = (ushort)iVar6;
//       goto LAB_8003318c;
//     }
//     lVar9 = (ef->param).blood.px;
//     iVar11 = (ef->param).blood.py;
//     iVar7 = lVar9 / 10;
//     (ef->param).blood.vy = (ef->param).blood.vy + 10;
//     node = (ef->param).blood.hint;
//     iVar6 = (ef->param).blood.pz / 10;
//     if (node == (AreaNodeType *)0x0) {
// LAB_80032cf8:
//       iVar8 = GetAreaMapLevel(GlobalAreaMap,lVar9,iVar11 + -300);
//       if ((iVar11 <= iVar8) && (FieldArea->division == -1)) {
//         (ef->param).blood.hint = FieldArea;
//       }
//     }
//     else {
//       iVar8 = (int)node->y;
//       if (((((iVar11 / 10 < iVar8 + -200) || (iVar8 < iVar11 / 10)) || (iVar7 < node->x1)) ||
//           ((iVar6 < node->z1 || (node->x2 < iVar7)))) || (node->z2 < iVar6)) goto LAB_80032cf8;
//       if ((node->dy == 0) || (iVar8 = ComputeAreaLevel(node,iVar7,iVar6), iVar8 != -0x80000000)) {
//         iVar8 = iVar8 * 10;
//       }
//     }
//     if ((ef->param).blood.py < iVar8) {
//       uVar3 = (ef->param).blood.time;
//       (ef->param).blood.time = uVar3 - 1;
//       if ((int)((uint)uVar3 << 0x10) < 1) {
//         ef->proc = (undefined **)0x0;
//       }
//     }
//     else {
//       (ef->param).blood.vz = 0;
//       (ef->param).blood.vy = 0;
//       (ef->param).blood.vx = 0;
//       if (iVar8 == -0x80000000) {
//         iVar6 = rand();
//         iVar7 = iVar6;
//         if (iVar6 < 0) {
//           iVar7 = iVar6 + 7;
//         }
//         (ef->param).blood.vy = (short)iVar6 + (short)(iVar7 >> 3) * -8 + 8;
//         (ef->param).blood.rotate = 0;
//         iVar7 = rand();
//         *(char *)((int)&ef->param + 0x22) = *(char *)((int)&ef->param + 0x22) + '\x02';
//         (ef->param).blood.scale = iVar7 % 0x2ab + 0x555;
//       }
//       else {
//         (ef->param).blood.py = iVar8;
//       }
//       *(undefined1 *)((int)&ef->param + 0x23) = 1;
//       iVar7 = rand();
//       (ef->param).blood.time = (short)iVar7 + (short)(iVar7 / 10) * -10;
//       SoundEx((VECTOR *)&(ef->param).bleed.pos.vy,0x37);
//     }
//     if ((GameClock & 1U) != 0) {
//       memset((uchar *)&local_30,'\0',0x10);
//       iVar7 = rand();
//       local_30._0_4_ = (ef->param).blood.px + -0x50 + iVar7 % 0xa0;
//       iVar7 = rand();
//       local_30._4_4_ = (ef->param).blood.py + -0x50 + iVar7 % 0xa0;
//       iVar7 = rand();
//       local_40.vz = (ef->param).blood.pz + -0x50 + iVar7 % 0xa0;
//       local_40.vx._0_2_ = local_30.vx;
//       local_40.vx._2_2_ = local_30.vy;
//       local_40.vy._0_2_ = local_30.vz;
//       local_40.vy._2_2_ = local_30.pad;
//       local_40.pad = local_24;
//       local_28 = local_40.vz;
//       memset((uchar *)&local_30,'\0',8);
//       iVar7 = (uint)(ushort)(ef->param).blood.vx << 0x10;
//       iVar6 = (uint)(ushort)(ef->param).blood.vy << 0x10;
//       local_48.vy = (short)((iVar6 >> 0x10) - (iVar6 >> 0x1f) >> 1);
//       local_48.vx = (short)((iVar7 >> 0x10) - (iVar7 >> 0x1f) >> 1);
//       iVar7 = (uint)(ushort)(ef->param).blood.vz << 0x10;
//       local_48.vz = (short)((iVar7 >> 0x10) - (iVar7 >> 0x1f) >> 1);
//       local_30.vz = local_48.vz;
//       local_48.pad = local_30.pad;
//       local_30._0_4_ = local_48._0_4_;
//       iVar7 = rand();
//       SetBleed(&local_40,&local_48,iVar7 % 10 + 10,0x7f1017);
//     }
//   }
// LAB_8003306c:
//   (ef->param).blood.py = (ef->param).blood.py + (int)(ef->param).blood.vy;
//   (ef->param).blood.px = (ef->param).blood.px + (int)(ef->param).blood.vx;
//   (ef->param).blood.pz = (ef->param).blood.pz + (int)(ef->param).blood.vz;
//   (&sprBlood)[uVar5].rotate = (ef->param).blood.rotate;
//   _29_00->attribute = 0;
//   (&sprBlood)[uVar5].r = (ef->param).blood.mode;
//   (&sprBlood)[uVar5].g = (ef->param).blood.mode;
//   (&sprBlood)[uVar5].b = (ef->param).blood.mode;
//   iVar7 = (ef->param).blood.scale;
//   FUN_800396c0((ef->param).blood.px,(ef->param).blood.py,(ef->param).blood.pz,&local_48);
//   iVar6 = (int)local_48.vz;
//   if (iVar6 < 0x25) {
//     return;
//   }
//   iVar7 = iVar7 * 300;
//   if (iVar6 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar6 == -1) && (iVar7 == -0x80000000)) {
//     trap(0x1800);
//   }
//   sVar4 = (short)(iVar7 / iVar6) + 1;
//   (&sprBlood)[uVar5].scaley = sVar4;
//   (&sprBlood)[uVar5].scalex = sVar4;
//   (&sprBlood)[uVar5].x = local_48.vx;
//   (&sprBlood)[uVar5].y = local_48.vy;
//   iVar7 = (int)(local_48._4_4_ << 0x10) >> 0x12;
//   if (iVar7 < 0) {
//     iVar6 = 0;
//   }
//   else {
//     iVar6 = 0x4e1;
//     if (iVar7 < 0x4e2) {
//       iVar6 = iVar7;
//     }
//   }
//   uVar3 = (ushort)iVar6;
//   _29 = _29_00;
// LAB_8003318c:
//   GsSortSprite(_29,OTablePt,uVar3);
//   return;
// }
