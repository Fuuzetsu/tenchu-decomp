#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AntiWall(struct GsRVIEW2 *vinfo, struct GsRVIEW2 *target);
 *     CAMERA.C:432, 91 src lines, frame 88 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct GsRVIEW2 * vinfo
 *     param $s2       struct GsRVIEW2 * target
 *     stack sp+24     struct SVECTOR vsL
 *     stack sp+32     struct SVECTOR vsR
 *     reg   $s3       int lvR
 *     reg   $s0       int rmap
 *     stack sp+40     struct SVECTOR av
 *     stack sp+48     int rx
 *     stack sp+52     int ry
 *     reg   $s5       int sx
 *     reg   $s3       int sy
 *     reg   $s0       int sz
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AntiWall", AntiWall);

// triage: MEDIUM — 173 insns, 6 callees, ~0.06 to AdtFntOpen

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void AntiWall(GsRVIEW2 *vinfo,GsRVIEW2 *target)
//
// {
//   long lVar1;
//   long lVar2;
//   char *pcVar3;
//   char *pcVar4;
//   int iVar5;
//   byte bVar6;
//   int iVar7;
//   int iVar8;
//   int iVar9;
//   int iVar10;
//   int iVar11;
//   int iVar12;
//   VECTOR local_58;
//   VECTOR local_48;
//   VECTOR local_38;
//   byte local_28 [4];
//   byte local_24 [4];
//
//   GetVectorRotation((VECTOR *)target,(VECTOR *)&target->vrx,(int *)local_28,(int *)local_24);
//   Scratchpad[4] = 0;
//   Scratchpad[5] = 0;
//   Scratchpad[0] = local_28[0];
//   Scratchpad[1] = local_28[1];
//   Scratchpad[2] = local_24[0];
//   Scratchpad[3] = local_24[1];
//   RotMatrixYXZ((SVECTOR *)Scratchpad,(MATRIX *)(Scratchpad + 0x40));
//   SetRotMatrix((MATRIX *)(Scratchpad + 0x40));
//   ApplyRotMatrix((SVECTOR *)&DAT_800979f4,&local_58);
//   ApplyRotMatrix((SVECTOR *)&DAT_800979fc,&local_48);
//   lVar1 = GetAreaMapLevel(GlobalAreaMap,target->vpx + local_48.vx,target->vpy + local_48.vy);
//   pcVar3 = (char *)(target->vpx + local_58.vx);
//   pcVar4 = (char *)(target->vpy + local_58.vy);
//   iVar5 = target->vpz + local_58.vz;
//   lVar2 = GetAreaMapLevel(GlobalAreaMap,(long)pcVar3,(long)pcVar4);
//   bVar6 = lVar2 <= target->vpy;
//   if ((bool)bVar6) {
//     FntPrint(&DAT_80097a04,pcVar3,pcVar4,iVar5);
//   }
//   if (lVar1 <= target->vpy) {
//     bVar6 = bVar6 | 2;
//     FntPrint(&DAT_80097a08,pcVar3,pcVar4,iVar5);
//   }
//   if (bVar6 != 0) {
//     local_38.vx = 0;
//     local_38.vy = 0;
//     local_38.vz = 0;
//     if (bVar6 == 1) {
//       Scratchpad[0] = 0xe8;
//       Scratchpad[1] = 3;
//       Scratchpad[2] = 0;
//       Scratchpad[3] = 0;
//       Scratchpad[4] = 0;
//       Scratchpad[5] = 0;
//       ApplyRotMatrix((SVECTOR *)Scratchpad,&local_38);
//     }
//     if (bVar6 == 2) {
//       Scratchpad[0] = 0x18;
//       Scratchpad[1] = 0xfc;
//       Scratchpad[2] = 0;
//       Scratchpad[3] = 0;
//       Scratchpad[4] = 0;
//       Scratchpad[5] = 0;
//       ApplyRotMatrix((SVECTOR *)Scratchpad,&local_38);
//     }
//     iVar12 = local_38.vx - (local_38.vx >> 0x1f);
//     iVar11 = local_38.vx / 2;
//     pcVar3 = (char *)(target->vpx + local_38.vx + iVar11);
//     iVar10 = local_38.vy - (local_38.vy >> 0x1f);
//     iVar9 = local_38.vy / 2;
//     pcVar4 = (char *)(target->vpy + local_38.vy + iVar9);
//     iVar8 = local_38.vz - (local_38.vz >> 0x1f);
//     iVar7 = local_38.vz / 2;
//     iVar5 = target->vpz + local_38.vz + iVar7;
//     lVar1 = GetAreaMapLevel(GlobalAreaMap,(long)pcVar3,(long)pcVar4);
//     if (lVar1 <= target->vpy) {
//       local_38.vx = iVar11 - (iVar12 >> 0x1f) >> 1;
//       local_38.vy = iVar9 - (iVar10 >> 0x1f) >> 1;
//       local_38.vz = iVar7 - (iVar8 >> 0x1f) >> 1;
//       FntPrint(&DAT_80097a0c,pcVar3,pcVar4,iVar5);
//     }
//     target->vpx = target->vpx + local_38.vx;
//     target->vpy = target->vpy + local_38.vy;
//     target->vpz = target->vpz + local_38.vz;
//   }
//   return;
// }
