#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MakeCameraPosition(struct VECTOR *orgpos, struct SVECTOR *orgrot, struct SVECTOR *campos, struct SVECTOR *ref, struct GsRVIEW2 *vDif);
 *     CAMERA.C:533, 66 src lines, frame 104 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     param $s4       struct VECTOR * orgpos
 *     param $s5       struct SVECTOR * orgrot
 *     param $s7       struct SVECTOR * campos
 *     param $fp       struct SVECTOR * ref
 *     param stack+16  struct GsRVIEW2 * vDif
 *     reg   $v1       int fwRot
 *     reg   $s2       struct SVECTOR * tmp
 *     reg   $s6       struct SVECTOR * prot
 *     reg   $s1       struct MATRIX * pmat
 *     reg   $s4       struct VECTOR * pos
 *     reg   $s5       struct SVECTOR * rot
 *     reg   $a0       int d
 *     reg   $v1       int ret
 *     reg   $s0       int dz
 *     reg   $s3       int dx
 *     reg   $v0       int y2
 *     reg   $s0       int y1
 *     stack sp+24     struct SVECTOR dvec
 *     stack sp+32     struct VECTOR ret
 *     stack sp+48     struct VECTOR start
 *     reg   $a2       int N
 *     reg   $s4       struct VECTOR * start
 *     reg   $a0       struct SVECTOR * v
 *     reg   $s1       int n
 *     reg   $s4       int pz
 *     reg   $s2       int py
 *     reg   $s3       int px
 *     reg   $s5       int vz
 *     reg   $s6       int vy
 *     reg   $s7       int vx
 *     reg   $s0       int i
 *     stack sp+120    struct GsRVIEW2 * vdif
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/MakeCameraPosition", MakeCameraPosition);

// triage: MEDIUM — 165 insns, mul/div, 8 callees, ~0.08 to AdtFntOpen
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void MakeCameraPosition(VECTOR *orgpos,SVECTOR *orgrot,SVECTOR *campos,SVECTOR *ref)
//
// {
//   short sVar1;
//   int iVar2;
//   int iVar3;
//   int iVar4;
//   undefined1 local_88 [16];
//   int local_78;
//   int local_74;
//   VECTOR local_68;
//   VECTOR local_58;
//   VECTOR VStack_48;
//   VECTOR VStack_38;
//   long alStack_28 [2];
//
//   sVar1 = FUN_8002fd9c(CamState.Owner);
//   Scratchpad._64_2_ = orgrot->vx + sVar1;
//   Scratchpad._66_2_ = orgrot->vy;
//   Scratchpad._68_2_ = orgrot->vz;
//   RotMatrixYXZ((SVECTOR *)(Scratchpad + 0x40),(MATRIX *)(Scratchpad + 0x80));
//   Scratchpad._148_4_ = orgpos->vx;
//   Scratchpad._152_4_ = orgpos->vy;
//   Scratchpad._156_4_ = orgpos->vz;
//   SetRotMatrix((MATRIX *)(Scratchpad + 0x80));
//   SetTransMatrix((MATRIX *)(Scratchpad + 0x80));
//   RotTrans(campos,&local_68,alStack_28);
//   RotTrans(campos + 1,&local_58,alStack_28);
//   RotTrans(campos + 2,&VStack_48,alStack_28);
//   RotTrans(campos + 3,&VStack_38,alStack_28);
//   iVar2 = FUN_80039ddc(&VStack_48,&VStack_38,local_88,0);
//   iVar3 = (local_58.vx - local_68.vx) * iVar2;
//   if (iVar3 < 0) {
//     iVar3 = iVar3 + 0xfff;
//   }
//   iVar4 = (local_58.vy - local_68.vy) * iVar2;
//   local_88._12_4_ = (iVar3 >> 0xc) + local_68.vx;
//   if (iVar4 < 0) {
//     iVar4 = iVar4 + 0xfff;
//   }
//   iVar2 = (local_58.vz - local_68.vz) * iVar2;
//   local_78 = (iVar4 >> 0xc) + local_68.vy;
//   if (iVar2 < 0) {
//     iVar2 = iVar2 + 0xfff;
//   }
//   local_74 = (iVar2 >> 0xc) + local_68.vz;
//   AntiWall(&ViewInfo,(GsRVIEW2 *)local_88);
//   if (CamState.OldMode._1_1_ == 1) {
//     *(long *)ref = local_88._0_4_ - ViewInfo.vpx;
//     *(long *)&ref->vz = local_88._4_4_ - ViewInfo.vpy;
//     *(long *)(ref + 1) = local_88._8_4_ - ViewInfo.vpz;
//     *(long *)&ref[1].vz = local_88._12_4_ - ViewInfo.vrx;
//     *(int *)(ref + 2) = local_78 - ViewInfo.vry;
//     *(int *)&ref[2].vz = local_74 - ViewInfo.vrz;
//     CamState.OldMode._1_1_ = CMODE_NORMAL >> 8;
//   }
//   else {
//     MakeDifSub((VECTOR *)&ViewInfo.vrx,(VECTOR *)(local_88 + 0xc),(VECTOR *)&ref[1].vz,
//                (TMakeDifInfo *)&CamState.Valiation);
//     MakeDifSub((VECTOR *)&ViewInfo,(VECTOR *)local_88,(VECTOR *)ref,&pnt);
//   }
//   return;
// }
