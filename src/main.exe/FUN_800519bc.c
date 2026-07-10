#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 *     extern struct TCdaStatus CdaStatus;
 *     extern short SkipFrame;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_800519bc", FUN_800519bc);

// triage: HARD — 362 insns, 2 loop, 19 callees, ~0.11 to BriefingAndInventorySelectionScreen
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_800519bc(void)
//
// {
//   char cVar1;
//   ushort uVar2;
//   ulong *puVar3;
//   short sVar4;
//   int iVar5;
//   uint uVar6;
//   int iVar7;
//   int iVar8;
//   int iVar9;
//   int iVar10;
//   int unaff_s4;
//   uint uVar11;
//   uint uVar12;
//   int iVar13;
//   GsIMAGE GStack_a8;
//   GsSPRITE local_88;
//   GsIMAGE GStack_60;
//   ushort local_40;
//   int local_38;
//   ushort local_34;
//   BackGround *local_30;
//   int local_2c;
//
//   uVar12 = 0;
//   uVar11 = 0xfe;
//   local_38 = -0xa000;
//   local_34 = 0;
//   local_40 = 0xff60;
//   iVar13 = -8;
//   puVar3 = PathFileRead((uchar *)"K:\\WORK\\CDIMAGE\\DEMO\\",
//                         (&PTR_s_st001_tim_8008ea90)
//                         [(uint)(byte)PersistentState._5_1_ * 3 +
//                          (uint)(byte)PersistentState._94_1_ * 0x21]);
//   local_30 = (BackGround *)FUN_8004f4f8(puVar3);
//   vfree((undefined *)puVar3);
//   puVar3 = PathFileRead((uchar *)"K:\\WORK\\CDIMAGE\\DEMO\\",
//                         (&PTR_s_mojo1_tim_8008ea94)
//                         [(uint)(byte)PersistentState._5_1_ * 3 +
//                          (uint)(byte)PersistentState._94_1_ * 0x21]);
//   GetTIMInfo(puVar3,&GStack_60);
//   InitSprite(&GStack_60,&local_88);
//   local_88.x = -0xa0;
//   local_88.y = -0x78;
//   local_88.r = 0x80;
//   local_88.g = 0x80;
//   local_88.b = 0x80;
//   local_88.attribute = local_88.attribute | 0x50000000;
//   local_88.mx = 0;
//   local_88.my = 0;
//   GetTIMInfo(puVar3,&GStack_a8);
//   LoadTIMAndFree(puVar3);
//   local_88.w = 0x10;
//   local_88.y = -0x68;
//   FUN_80038ce0();
//   local_2c = (uint)(ushort)GStack_a8.px << 0x10;
//   do {
//     uVar2 = GetRealPad(0);
//     if ((uVar2 & (uVar2 ^ local_34) & 0x820) != 0) {
//       iVar13 = 8;
//     }
//     local_34 = uVar2;
//     if ((uVar2 & 0x900) == 0x900) {
//       iVar5 = 0;
//       do {
//         sVar4 = (short)iVar5;
//         iVar5 = iVar5 + 1;
//         (&PersistentState.field_0x40c)[(int)sVar4 + (uint)(byte)PersistentState._4_1_ * 0x20] =
//              (&PersistentState.field_0x27)[sVar4];
//       } while (iVar5 * 0x10000 >> 0x10 < 0x14);
//       FadeOutDirect(0x20,2,'\b','\b');
//       FUN_80038ce0();
//       PersistentState._6_1_ = 0xff;
//       PersistentState._72_1_ = PersistentState._72_1_ & 0xfe;
//       FUN_8004f6c0(0x10);
//     }
//     if (0xfe < (short)uVar11) {
//       DisposeBG(local_30);
//       return;
//     }
//     StartDrawing();
//     if (uVar12 == 1) {
//       iVar5 = CdaGetCurrentLength();
//       if (0 < iVar5) {
//         uVar12 = 2;
//         unaff_s4 = 0;
//       }
//     }
//     else if (uVar12 < 2) {
//       if ((uVar12 == 0) && ((short)uVar11 == 0)) {
//         sVar4 = *(short *)(&DAT_8008ea98 +
//                           (uint)(byte)PersistentState._94_1_ * 0x84 +
//                           (uint)(byte)PersistentState._5_1_ * 0xc);
//         if ((PersistentState._4_1_ == '\x01') &&
//            (((byte)PersistentState._94_1_ == 3 && ((byte)PersistentState._5_1_ - 6 < 2)))) {
//           sVar4 = sVar4 + 1;
//         }
//         _PlayMusic((int)sVar4,0);
//         uVar12 = 1;
//       }
//     }
//     else if (uVar12 == 2) {
//       sVar4 = (short)unaff_s4;
//       unaff_s4 = unaff_s4 + 1;
//       if (*(short *)(&DAT_8008eca0 +
//                     ((uint)(byte)PersistentState._94_1_ * 0xb + (uint)(byte)PersistentState._5_1_) *
//                     2) < sVar4) {
//         uVar12 = 3;
//       }
//     }
//     else {
//       iVar5 = GStack_a8.pw - 4;
//       if (uVar12 == 3) {
//         if (-1 < iVar5 * 0x10000) {
//           iVar10 = local_2c >> 0x10;
//           do {
//             local_88.u = (uchar)(iVar5 << 2);
//             local_88.tpage = GetTPage(0,0,iVar10 + (short)iVar5,0x100);
//             iVar7 = (uint)local_40 + ((int)(short)GStack_a8.pw - (int)(short)iVar5) * -8;
//             iVar8 = iVar7 * 0x10000;
//             iVar9 = iVar8 >> 0x10;
//             if (iVar9 < -0xa0) {
// LAB_80051dd4:
//               local_88.b = '\0';
//             }
//             else {
//               cVar1 = (char)((uint)iVar8 >> 0x10);
//               if (iVar9 < -0x78) {
//                 local_88.b = (cVar1 + -0x60) * '\x03';
//               }
//               else {
//                 if (0xa0 < iVar9) goto LAB_80051dd4;
//                 if (iVar9 < 0x79) {
//                   local_88.b = 0x80;
//                 }
//                 else {
//                   local_88.b = (-0x60 - cVar1) * '\x03';
//                 }
//               }
//             }
//             local_88.x = (short)iVar7 + -8;
//             local_88.r = local_88.b;
//             local_88.g = local_88.b;
//             GsSortSprite(&local_88,OTablePt,1);
//             iVar5 = iVar5 + -4;
//             local_88.x = local_88.x + 8;
//           } while (-1 < iVar5 * 0x10000);
//         }
//         if ((CdaStatus.status & 0x60) == 0) {
//           iVar13 = 8;
//         }
//         local_38 = local_38 +
//                    u_TXjhSXE__8008ecf8
//                    [(uint)(byte)PersistentState._94_1_ * 0xb + (uint)(byte)PersistentState._5_1_];
//         iVar10 = local_38;
//         if (local_38 < 0) {
//           iVar10 = local_38 + 0xff;
//         }
//         local_40 = (ushort)((uint)iVar10 >> 8);
//         unaff_s4 = iVar5;
//       }
//     }
//     DrawBG(local_30);
//     uVar11 = uVar11 + iVar13;
//     iVar5 = (int)(uVar11 * 0x10000) >> 0x10;
//     if (iVar5 < 0) {
//       uVar11 = 0;
//     }
//     else if (0xff < iVar5) {
//       uVar11 = 0xff;
//     }
//     uVar6 = uVar11 & 0xff;
//     if ((uVar11 & 0xffff) != 0) {
//       FUN_80038c0c(OTablePt->org,uVar6,uVar6,uVar6);
//     }
//     SkipFrame = 2;
//     EndDrawing(0);
//   } while( true );
// }
