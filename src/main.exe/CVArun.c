#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVArun(void);
 *     CHRANIM.C:238, 47 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $a2       struct MotionManager * mmp
 *     reg   $s1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern int StageID;
 *     extern struct GsOT *OTablePt;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/CVArun", CVArun);

// triage: MEDIUM — 133 insns, 2 loop, 14 callees, ~0.06 to DisposeWeapon
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short CVArun(void)
//
// {
//   char cVar1;
//   short sVar2;
//   int iVar3;
//   int *piVar4;
//   Humanoid *human;
//   int iVar5;
//
//   ComputeAllConflict();
//   StartDrawing();
//   AVCameraControl();
//   ControlAllHumanoid();
//   DrawConstruction();
//   DrawEffect();
//   DoItemProc();
//   DoMiscProc();
//   DrawTelop();
//   FUN_80029368();
//   if ((StageID == 10) && (iVar5 = 0, PersistentState._4_1_ == '\0')) {
//     iVar3 = 0;
//     do {
//       piVar4 = (int *)((int)&DAT_800c2cb0 + (iVar3 >> 0xe));
//       iVar3 = *piVar4;
//       if ((*(ushort *)(iVar3 + 0x5a) & 1) == 0) {
//         if (-1 < *(char *)(iVar3 + 0x7c)) {
//           cVar1 = *(char *)(iVar3 + 0x7e) + '\b';
//           *(char *)(iVar3 + 0x7e) = cVar1;
//           *(char *)(iVar3 + 0x7d) = cVar1;
//           *(char *)(iVar3 + 0x7c) = cVar1;
//         }
//         GsSortSprite((GsSPRITE *)(*piVar4 + 0x68),OTablePt,0);
//       }
//       iVar5 = iVar5 + 1;
//       iVar3 = iVar5 * 0x10000;
//     } while (iVar5 * 0x10000 >> 0x10 < 6);
//   }
//   EndDrawing(-2);
//   iVar3 = 0;
//   iVar5 = 0;
//   do {
//     iVar5 = iVar5 >> 0xd;
//     piVar4 = (int *)((int)&CVAhuman[0].human + iVar5);
//     human = (Humanoid *)*piVar4;
//     if ((human != (Humanoid *)0x0) &&
//        (*(short *)((int)&CVAhuman[0].loop + iVar5) <= human->motion->loop)) {
//       sVar2 = *(short *)((int)&CVAhuman[0].motid + iVar5);
//       if (sVar2 == -1) {
//         human->motion->loop = -1;
//         iVar5 = *piVar4;
//         *(undefined2 *)(iVar5 + 0x44) = 0;
//         *(undefined2 *)(iVar5 + 0x40) = 0;
//       }
//       else if (human->status != 0x11) {
//         SetNowMotion(human,sVar2,1);
//         *piVar4 = 0;
//       }
//     }
//     iVar3 = iVar3 + 1;
//     iVar5 = iVar3 * 0x10000;
//   } while (iVar3 * 0x10000 >> 0x10 < 5);
//   DAT_80097cc0 = DAT_80097cc0 + 1;
//   sVar2 = 1;
//   if (*(short *)(DAT_80097cbc + 2) <= DAT_80097cc0) {
//     DAT_80097cbc = DAT_80097cbc + 0xc;
//     sVar2 = CVAupdate();
//   }
//   return sVar2;
// }
