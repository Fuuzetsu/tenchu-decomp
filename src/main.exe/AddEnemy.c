#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void AddEnemy(void);
 *     INFOVIEW.C:695, 58 src lines, frame 2064 bytes, saved-reg mask 0xc0ff0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     stack sp+24     unsigned char [70][20] names
 *     stack sp+1424   struct TAdtSelect [70] ItemName
 *     reg   $s4       short i
 *     reg   $s0       struct Humanoid * human
 *     reg   $s7       long type
 *     reg   $s5       long x
 *     reg   $s2       long y
 *     reg   $s0       long z
 *     reg   $s3       short r
 *     reg   $s2       short think
 *     stack sp+1984   struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern struct WeaponModelType WeaponModel[41];
 *     extern int StageID;
 *     extern struct ThinkDBtype ThinkDB[20];
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AddEnemy", AddEnemy);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AddEnemy", debug_menu_enemy_layout_add__override__prt_8005b740_aee7b64a);

// triage: HARD — 288 insns, 5 loop, frame 0x810, 6 callees, ~0.06 to GetMotionID
// likely-relevant cookbook sections:
//   - Loops: 5 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Stack objects: 0x810 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void AddEnemy(void)
//
// {
//   short sVar1;
//   short sVar2;
//   int iVar3;
//   ulong uVar4;
//   ModelArchiveType *pMVar5;
//   WeaponModelType *pWVar6;
//   byte *pbVar7;
//   int iVar8;
//   TAdtSelect *pTVar9;
//   WeaponModelType *pWVar10;
//   undefined *puVar11;
//   long z;
//   char *buffer;
//   uint uVar12;
//   long y;
//   int iVar13;
//   int iVar14;
//   int iVar15;
//   long x;
//   int iVar16;
//   char acStack_7f8 [1400];
//   TAdtSelect local_280;
//   char *local_278 [139];
//   int local_4c;
//   undefined4 local_48;
//   undefined4 local_44;
//   undefined4 local_40;
//   int local_3c;
//   undefined4 local_38;
//   undefined4 local_34;
//   WeaponModelType *local_30;
//   undefined *local_2c;
//
//   iVar14 = 0;
//   sVar2 = 0;
//   if (HumanData[0].type != -1) {
//     puVar11 = &DAT_8008f188;
//     pWVar10 = WeaponModel;
//     iVar16 = 0;
//     iVar13 = iVar14;
//     do {
//       iVar14 = iVar13;
//       if (0x45 < iVar13) break;
//       if (**(short **)(puVar11 + (StageID + 1) * 4) != -1) {
//         iVar15 = 0;
//         do {
//           iVar8 = *(int *)(puVar11 + (StageID + 1) * 4);
//           iVar3 = iVar15 + 1;
//           if (*(short *)(((iVar15 << 0x10) >> 0xf) + iVar8) == HumanData[sVar2].type) break;
//           iVar15 = iVar3;
//         } while (*(short *)((iVar3 * 0x10000 >> 0xf) + iVar8) != -1);
//         if (*(short *)(((iVar15 << 0x10) >> 0xf) + iVar8) != -1) {
//           iVar14 = 0;
//           if (pWVar10->wid != -1) {
//             sVar1 = pWVar10->wid;
//             pWVar6 = pWVar10;
//             do {
//               if (sVar1 == HumanData[sVar2].wepid) break;
//               sVar1 = pWVar6[1].wid;
//               iVar14 = iVar14 + 1;
//               pWVar6 = pWVar6 + 1;
//             } while (sVar1 != -1);
//           }
//           buffer = acStack_7f8 + iVar16;
//           iVar16 = iVar16 + 0x14;
//           local_30 = pWVar10;
//           local_2c = puVar11;
//           sprintf(buffer,s__s__s_80097d48,HumanData[sVar2].name,pWVar10[iVar14].name);
//           local_278[iVar13 * 2 + -2] = buffer;
//           iVar14 = iVar13 + 1;
//           local_278[iVar13 * 2 + -1] = (char *)(int)HumanData[sVar2].type;
//           pWVar10 = local_30;
//           puVar11 = local_2c;
//         }
//       }
//       sVar2 = sVar2 + 1;
//       iVar13 = iVar14;
//     } while (HumanData[sVar2].type != -1);
//   }
//   (&local_280)[iVar14].name = s_cancel_80097d50;
//   local_278[iVar14 * 2 + -1] = (char *)0xffffffff;
//   (&local_280)[iVar14 + 1].name = (char *)0x0;
//   uVar4 = AdtSelect("select type",&local_280,0);
//   iVar14 = (int)(short)uVar4;
//   uVar12 = 0;
//   if (iVar14 != -1) {
//     iVar13 = 0;
//     do {
//       iVar15 = 0;
//       iVar16 = 0;
//       if (ThinkDB[0].name != (uchar *)0x0) {
//         pTVar9 = &local_280;
//         do {
//           if (0x45 < iVar15) break;
//           iVar3 = (iVar16 << 0x10) >> 0xd;
//           pbVar7 = *(byte **)((int)&ThinkDB[0].name + iVar3);
//           if ((int)(short)iVar13 + 0x31U == (uint)*pbVar7) {
//             pTVar9->name = (char *)pbVar7;
//             iVar15 = iVar15 + 1;
//             pTVar9->value = (int)*(short *)((int)&ThinkDB[0].value + iVar3);
//             pTVar9 = pTVar9 + 1;
//           }
//           iVar16 = iVar16 + 1;
//         } while (*(int *)((int)&ThinkDB[0].name + (iVar16 * 0x10000 >> 0xd)) != 0);
//       }
//       (&local_280)[iVar15].name = (char *)0x0;
//       uVar4 = AdtSelect("custom think setting",&local_280,0);
//       uVar12 = uVar12 | uVar4;
//       sVar2 = (short)uVar12;
//     } while (((sVar2 != 0x1111) && (iVar13 = iVar13 + 1, sVar2 != 0x2222)) &&
//             (iVar13 * 0x10000 >> 0x10 < 4));
//     pMVar5 = (CamState.Owner)->model;
//     x = (pMVar5->locate).coord.t[0];
//     y = (pMVar5->locate).coord.t[1];
//     sVar1 = (pMVar5->rotate).vy;
//     z = (pMVar5->locate).coord.t[2];
//     DAT_80097d44 = leSetEnemy(iVar14,sVar2,x,y,z,sVar1);
//     iVar14 = BreedLife(iVar14,x,y,z,0);
//     *(short *)(*(int *)(iVar14 + 0x58) + 0x52) = sVar1;
//     *(ModelArchiveType **)(iVar14 + 0x74) = (CamState.Owner)->model;
//     memset((uchar *)&local_40,'\0',0x10);
//     local_278[0x8a] = *(char **)(*(int *)(iVar14 + 0x58) + 0x18);
//     local_4c = *(int *)(*(int *)(iVar14 + 0x58) + 0x1c) + -0x4b0;
//     local_48 = *(undefined4 *)(*(int *)(iVar14 + 0x58) + 0x20);
//     local_44 = local_34;
//     local_40 = local_278[0x8a];
//     local_3c = local_4c;
//     local_38 = local_48;
//     SetBleeds((short)local_278 + 0x228,400,0);
//   }
//   return;
// }
