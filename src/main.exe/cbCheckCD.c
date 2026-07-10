#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void cbCheckCD(void);
 *     OPAUDIO.C:69, 55 src lines, frame 48 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     stack sp+16     unsigned char [8] result
 *     reg   $s0       int ret
 *     reg   $v1       int com
 *     stack sp+24     struct CdlLOC loc
 *     stack sp+24     struct CdlLOC loc
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/cbCheckCD", cbCheckCD);

// triage: MEDIUM — 122 insns, 9 callees, ~0.11 to CdaStop
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void cbCheckCD(void)
//
// {
//   int iVar1;
//   int iVar2;
//   uchar local_28 [5];
//   undefined1 auStack_23 [11];
//
//   if (CdaStatus.field9_0x14 == '\x1b') {
//     CdIntToPos(CdaStatus.StartPos,(CdlLOC *)(auStack_23 + 3));
//     if (((CdaStatus.flag & 1) != 0) &&
//        (iVar1 = CdControl('\x1b',(u_char *)(auStack_23 + 3),(u_char *)0x0), iVar1 == 0)) {
//       return;
//     }
//     CdaStatus.field9_0x14 = 0;
//     SsSetSerialAttr('\0','\0','\x01');
//     SsSetSerialVol('\0',(ushort)CdaStatus.field6_0x11,(ushort)CdaStatus.field7_0x12);
//     return;
//   }
//   if (CdaStatus.CheckCount < 10) {
//     CdaStatus.CheckCount = CdaStatus.CheckCount + 1;
//     return;
//   }
//   CdaStatus.CheckCount = 0;
//   iVar1 = CdSync(1,local_28);
//   iVar2 = CdLastCom();
//   if (iVar1 == 2) {
//     if (iVar2 == 9) {
//       return;
//     }
//     if (iVar2 == 0x11) {
//       CdaStatus.CurPos = CdPosToInt((CdlLOC *)auStack_23);
//       if (((CdaStatus.status & 0x20) != 0) &&
//          ((CdaStatus.EndPos < CdaStatus.CurPos || (CdaStatus.CurPos < CdaStatus.StartPos + -300))))
//       {
//         if (CdaStatus.mode != 1) {
//           SsSetSerialAttr('\0','\0','\x01');
//           SsSetSerialVol('\0',0,0);
//           cd_control(9,0,0);
//           CdaStatus.status = '\0';
//           return;
//         }
//         goto LAB_8004f90c;
//       }
//       CdControl('\x01',(u_char *)0x0,local_28);
//       CdaStatus.status = local_28[0];
//     }
//     CdControlF('\x11',(u_char *)0x0);
//   }
//   else {
//     if (iVar1 != 5) {
//       return;
//     }
// LAB_8004f90c:
//     CdaStatus.field9_0x14 = '\x1b';
//     CdaStatus.CheckCount = 0;
//     CdaStatus.status = '\0';
//   }
//   return;
// }
