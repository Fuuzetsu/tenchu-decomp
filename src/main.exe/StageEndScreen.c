#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern int StageID;
 *     extern struct GsOT *OTablePt;
 *     extern long GameClock;
 *     extern struct WorldType WorldMap[8][8][8];
 *     extern short SkipFrame;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/StageEndScreen", StageEndScreen);

// triage: VERY-HARD — 1521 insns, mul/div, 22 loop, 28 callees, ~0.12 to BriefingAndInventorySelectionScreen
// likely-relevant cookbook sections:
//   - Loops: 22 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Removing unreachable block (ram,0x80054230) */
// /* WARNING: Removing unreachable block (ram,0x80053c10) */
// /* WARNING: Removing unreachable block (ram,0x80053cf0) */
// /* WARNING: Removing unreachable block (ram,0x800542bc) */
// /* WARNING: Removing unreachable block (ram,0x80053c9c) */
// /* WARNING: Removing unreachable block (ram,0x80053d7c) */
// /* WARNING: Removing unreachable block (ram,0x80053f90) */
// /* WARNING: Removing unreachable block (ram,0x8005401c) */
//
// void StageEndScreen(void)
//
// {
//   bool bVar1;
//   uchar uVar2;
//   char cVar3;
//   ushort uVar4;
//   short sVar5;
//   uint *puVar6;
//   ulong *puVar7;
//   int iVar8;
//   int iVar9;
//   ushort uVar10;
//   undefined4 uVar11;
//   byte *pbVar12;
//   int iVar13;
//   uint *puVar14;
//   uint uVar15;
//   GsSPRITE local_d8;
//   uint local_b0;
//   uint local_ac;
//   uint local_a8;
//   undefined4 local_a0;
//   undefined4 local_9c;
//   undefined4 local_98;
//   undefined4 local_90;
//   undefined4 local_8c;
//   uint local_88;
//   GsSPRITE local_80;
//   GsIMAGE GStack_58;
//   ushort local_38;
//   BackGround *local_34;
//   ulong *local_30;
//
//   uVar15 = 0;
//   local_38 = 0;
//   SetupAppearance(0,-1);
//   PadShockAR(0,0,0,0);
//   FadeOutDirect(0x20,2,'\b','\b');
//   FUN_80038ce0();
//   iVar13 = 0;
//   sVar5 = 0;
//   uVar4 = DAT_8008ea78;
//   while (uVar4 != (byte)PersistentState._5_1_) {
//     iVar13 = iVar13 + 1;
//     sVar5 = (short)iVar13;
//     uVar4 = *(ushort *)((int)&DAT_8008ea78 + (iVar13 * 0x10000 >> 0xf));
//   }
//   PersistentState._1132_4_ = PersistentState._1132_4_ | 1 << ((int)sVar5 - 1U & 0x1f);
//   if ((PersistentState._5_1_ == '\a') && (PersistentState._94_1_ == '\0')) {
//     PersistentState._1132_4_ = PersistentState._1132_4_ | 0x400;
//   }
//   FUN_8004e964(&local_b0);
//   puVar6 = (uint *)FUN_8004e794(&local_b0,PersistentState._5_1_);
//   local_a0 = *puVar6;
//   local_9c = puVar6[1];
//   local_98 = puVar6[2];
//   PersistentState._76_4_ = local_b0;
//   PersistentState._80_4_ = local_ac;
//   PersistentState._84_4_ = local_a8;
//   puVar14 = (uint *)((uint)(byte)PersistentState._4_1_ * 0x1d4 +
//                      (uint)(byte)PersistentState._5_1_ * 0x24 + -0x7ffeff9c +
//                     (uint)(byte)PersistentState._6_1_ * 0xc);
//   puVar6 = (uint *)FUN_8004e794(puVar14,PersistentState._5_1_);
//   local_90 = *puVar6;
//   local_8c = puVar6[1];
//   local_88 = puVar6[2];
//   if (((short)local_88 < (short)local_98) ||
//      (((short)local_98 == (short)local_88 && ((int)local_a8 < (int)puVar14[2])))) {
//     *puVar14 = local_b0;
//     puVar14[1] = local_ac;
//     puVar14[2] = local_a8;
//     local_90 = local_a0;
//     local_8c = local_9c;
//     local_88 = local_98;
//   }
//   if (StageConfig[StageID].uid == '\0') {
//     FUN_80054b48();
//   }
//   if (PersistentState._5_1_ == '\a') {
//     if (PersistentState._94_1_ == '\0') {
//       PersistentState._1132_4_ = PersistentState._1132_4_ | 0x400;
//     }
//   }
//   else {
//     puVar7 = FileRead("K:\\WORK\\CDIMAGE\\DEMO\\number.tim");
//     GetTIMInfo(puVar7,&GStack_58);
//     InitSprite(&GStack_58,&local_d8);
//     local_d8.attribute = local_d8.attribute | 0x50000000;
//     local_d8.x = -0x8c;
//     local_d8.y = -0x28;
//     local_d8.r = 0x80;
//     local_d8.g = 0x80;
//     local_d8.b = 0x80;
//     local_d8.mx = 0;
//     local_d8.my = 0;
//     LoadTIMAndFree(puVar7);
//     local_d8.w = 0xc;
//     puVar7 = FileRead((&PTR_s_K__WORK_CDIMAGE_DEMO_rs_eng_tim_8008ef38)
//                       [(byte)PersistentState._94_1_]);
//     local_34 = (BackGround *)FUN_8004f4f8(puVar7);
//     vfree((undefined *)puVar7);
//     local_30 = FileRead((&PTR_s_K__WORK_CDIMAGE_DEMO_Ranks_e_Arc_8008ef48)
//                         [(byte)PersistentState._94_1_]);
//     puVar7 = (ulong *)FUN_8004f1d8(local_30,(int)local_98._2_2_);
//     GetTIMInfo(puVar7,&GStack_58);
//     InitSprite(&GStack_58,&local_80);
//     local_80.x = -0xa0;
//     local_80.y = -0x78;
//     local_80.r = 0x80;
//     local_80.g = 0x80;
//     local_80.b = 0x80;
//     local_80.attribute = local_80.attribute | 0x50000000;
//     local_80.mx = 0;
//     local_80.my = 0;
//     LoadTIM(puVar7);
//     _PlayMusic(0xc,1);
//     while( true ) {
//       uVar4 = GetRealPad(0);
//       uVar10 = uVar4 & (uVar4 ^ local_38);
//       local_38 = uVar4;
//       if ((uVar10 & 0x20) != 0) break;
//       if ((uVar10 & 0x40) != 0) {
//         uVar15 = 2;
//         goto LAB_800547c8;
//       }
//       uVar15 = 1;
//       if ((uVar10 & 0x800) != 0) goto LAB_800547c8;
//       StartDrawing();
//       DrawBG(local_34);
//       FUN_800515b0(&local_d8,local_a8,0x61,0xffffffa3,0);
//       uVar2 = local_d8.u;
//       uVar15 = local_ac & 0xff;
//       local_d8.x = 10;
//       local_d8.y = -0x35;
//       if ((int)uVar15 < 0) {
//         uVar15 = -uVar15;
//         bVar1 = true;
//       }
//       else {
//         bVar1 = false;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       local_d8.x = 0x28;
//       local_d8.y = -0x35;
//       uVar15 = (local_b0 >> 8 & 0xff) - (local_b0 & 0xff);
//       if ((int)uVar15 < 0) {
//         uVar15 = -uVar15;
//         bVar1 = true;
//       }
//       else {
//         bVar1 = false;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_a0 & 0xffff;
//       local_d8.x = 0x52;
//       local_d8.y = -0x35;
//       if ((short)local_a0 < 0) {
//         uVar15 = -(int)(short)local_a0;
//         bVar1 = true;
//       }
//       else {
//         bVar1 = false;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_90 & 0xffff;
//       local_d8.x = 0x7f;
//       local_d8.y = -0x35;
//       if ((short)local_90 < 0) {
//         uVar15 = -(int)(short)local_90;
//         bVar1 = true;
//       }
//       else {
//         bVar1 = false;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_b0 >> 0x18;
//       local_d8.x = 10;
//       local_d8.y = -0x1a;
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       uVar15 = local_b0 >> 8 & 0xff;
//       local_d8.x = 0x28;
//       local_d8.y = -0x1a;
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       uVar15 = (uint)local_a0._2_2_;
//       local_d8.x = 0x52;
//       local_d8.y = -0x1a;
//       bVar1 = false;
//       if ((short)local_a0._2_2_ < 0) {
//         uVar15 = -(int)(short)local_a0._2_2_;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = (uint)local_90._2_2_;
//       local_d8.x = 0x7f;
//       local_d8.y = -0x1a;
//       bVar1 = false;
//       if ((short)local_90._2_2_ < 0) {
//         uVar15 = -(int)(short)local_90._2_2_;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_b0 >> 0x10 & 0xff;
//       local_d8.x = 0x1c;
//       local_d8.y = 1;
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       uVar15 = (uint)local_9c._2_2_;
//       local_d8.x = 0x52;
//       local_d8.y = 1;
//       bVar1 = false;
//       if ((short)local_9c._2_2_ < 0) {
//         uVar15 = -(int)(short)local_9c._2_2_;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = (uint)local_8c._2_2_;
//       local_d8.x = 0x7f;
//       local_d8.y = 1;
//       bVar1 = false;
//       if ((short)local_8c._2_2_ < 0) {
//         uVar15 = -(int)(short)local_8c._2_2_;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_ac >> 8 & 0xff;
//       local_d8.x = 0x1c;
//       local_d8.y = 0x1a;
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       uVar15 = local_9c & 0xffff;
//       local_d8.x = 0x52;
//       local_d8.y = 0x1a;
//       bVar1 = false;
//       if ((short)local_9c < 0) {
//         uVar15 = -(int)(short)local_9c;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_8c & 0xffff;
//       local_d8.x = 0x7f;
//       local_d8.y = 0x1a;
//       bVar1 = false;
//       if ((short)local_8c < 0) {
//         uVar15 = -(int)(short)local_8c;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_98 & 0xffff;
//       local_d8.x = 0x52;
//       local_d8.y = 0x38;
//       bVar1 = false;
//       if ((short)local_98 < 0) {
//         uVar15 = -(int)(short)local_98;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_88 & 0xffff;
//       local_d8.x = 0x7f;
//       local_d8.y = 0x38;
//       bVar1 = false;
//       if ((short)local_88 < 0) {
//         uVar15 = -(int)(short)local_88;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       local_80.x = -0x19;
//       local_80.y = 0x4e;
//       local_d8.u = uVar2;
//       iVar13 = rsin((GameClock * 0x1000) / 0x5a);
//       iVar13 = iVar13 * 0x7f;
//       if (iVar13 < 0) {
//         iVar13 = iVar13 + 0xfff;
//       }
//       local_80.r = (char)(iVar13 >> 0xc) + '\x7f';
//       local_80.g = local_80.r;
//       local_80.b = local_80.r;
//       GsSortSprite(&local_80,OTablePt,1);
//       if (local_98._2_2_ == 4) {
//         iVar8 = GameClock * 0x1000;
//         iVar13 = (&DAT_8008e5bc)[*(short *)(&DAT_8008ed50 + (uint)(byte)PersistentState._5_1_ * 2)];
//         *(undefined2 *)(iVar13 + 0x6c) = 0xff88;
//         *(undefined2 *)(iVar13 + 0x6e) = 0x38;
//         *(undefined2 *)(iVar13 + 0x84) = 0x1000;
//         *(undefined2 *)(iVar13 + 0x86) = 0x1000;
//         iVar8 = rcos(iVar8 / 0x5a);
//         iVar8 = iVar8 * 0x50;
//         if (iVar8 < 0) {
//           iVar8 = iVar8 + 0xfff;
//         }
//         cVar3 = (char)(iVar8 >> 0xc) + '\x7f';
//         *(char *)(iVar13 + 0x7e) = cVar3;
//         *(char *)(iVar13 + 0x7d) = cVar3;
//         *(char *)(iVar13 + 0x7c) = cVar3;
//         GsSortSprite((GsSPRITE *)(iVar13 + 0x68),OTablePt,1);
//       }
//       SkipFrame = 2;
//       EndDrawing(0);
//     }
//     uVar15 = 0;
// LAB_800547c8:
//     DisposeBG(local_34);
//     vfree((undefined *)local_30);
//   }
//   FUN_80052ea8(&PersistentState,&local_a0);
//   FadeOutDirect(0x20,2,'\b','\b');
//   FUN_80038ce0();
//   iVar13 = 1;
//   if ((byte)(&PersistentState.field_0x60)[(byte)PersistentState._4_1_] <
//       StageConfig[(byte)PersistentState._5_1_].uid) {
//     (&PersistentState.field_0x60)[(byte)PersistentState._4_1_] =
//          StageConfig[(byte)PersistentState._5_1_].uid;
//   }
//   iVar8 = 0x10000;
//   do {
//     iVar8 = iVar8 >> 0x10;
//     if ((CamState.Owner)->item[iVar8] == 0xff) {
//       (&PersistentState.field_0x40c)[iVar8 + (uint)(byte)PersistentState._4_1_ * 0x20] = 0xff;
//     }
//     else if ((CamState.Owner)->item[iVar8] != '\0') {
//       iVar9 = iVar8 + (uint)(byte)PersistentState._4_1_ * 0x20;
//       if ((&PersistentState.field_0x40c)[iVar9] == -2) {
//         (&PersistentState.field_0x40c)[iVar9] = 0;
//       }
//       iVar9 = iVar8 + (uint)(byte)PersistentState._4_1_ * 0x20;
//       (&PersistentState.field_0x40c)[iVar9] =
//            (&PersistentState.field_0x40c)[iVar9] + (CamState.Owner)->item[iVar8];
//       iVar8 = iVar8 + (uint)(byte)PersistentState._4_1_ * 0x20;
//       if (99 < (byte)(&PersistentState.field_0x40c)[iVar8]) {
//         (&PersistentState.field_0x40c)[iVar8] = 99;
//       }
//     }
//     (&PersistentState.field_0x7)[(short)iVar13] = 0;
//     iVar13 = iVar13 + 1;
//     iVar8 = iVar13 * 0x10000;
//   } while (iVar13 * 0x10000 >> 0x10 < 0x14);
//   iVar13 = 0;
//   do {
//     iVar8 = iVar13 + 1;
//     (&PersistentState.field_0x27)[iVar13] =
//          (&PersistentState.field_0x40c)[iVar13 + (uint)(byte)PersistentState._4_1_ * 0x20];
//     iVar13 = iVar8;
//   } while (iVar8 < 0x14);
//   if ((PersistentState._92_1_ != '\0') && (PersistentState._5_1_ != '\a')) {
//     FUN_800514d8();
//   }
//   if (uVar15 == 1) {
//     PersistentState._72_1_ = PersistentState._72_1_ | 1;
//     goto LAB_80054b10;
//   }
//   if (uVar15 < 2) {
//     if (uVar15 != 0) goto LAB_80054b10;
//     PersistentState._72_1_ = PersistentState._72_1_ & 0xfe;
//     if (PersistentState._5_1_ != '\a') {
//       PersistentState._5_1_ =
//            *(undefined1 *)(&DAT_8008ea78 + StageConfig[(byte)PersistentState._5_1_].uid + 1);
//       iVar13 = 0;
//       pbVar12 = (byte *)((uint)(byte)PersistentState._4_1_ * 0x1d4 +
//                         (uint)(byte)PersistentState._5_1_ * 0x24 + -0x7ffeff9c);
//       do {
//         if ((uint)*pbVar12 + (uint)pbVar12[1] == 0) break;
//         iVar13 = iVar13 + 1;
//         pbVar12 = pbVar12 + 0xc;
//       } while (iVar13 < 3);
//       if (iVar13 == 3) {
//         PersistentState._6_1_ = 0xff;
//       }
//       else {
//         PersistentState._6_1_ = SUB41(iVar13,0);
//       }
//       goto LAB_80054b10;
//     }
//     uVar11 = 0x12;
//   }
//   else {
//     if (uVar15 != 2) goto LAB_80054b10;
//     PersistentState._6_1_ = 0xff;
//     uVar11 = 0x10;
//   }
//   FUN_8004f6c0(uVar11);
// LAB_80054b10:
//   FUN_8004f6c0(0x11);
//   return;
// }
