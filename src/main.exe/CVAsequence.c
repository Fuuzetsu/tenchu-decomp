#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVAsequence(short sid);
 *     CHRANIM.C:83, 58 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       short sid
 *     reg   $s3       struct SoundEffect * vab
 *     reg   $a0       struct Humanoid * human
 *     reg   $s0       short sound
 *     reg   $s0       short i
 *     reg   $a1       short j
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 *     extern short Humans;
 *     extern struct GsIMAGE Images[52];
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short ActionHalt;
 *     extern short MotionUpdateMode;
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/CVAsequence", CVAsequence);

// triage: HARD — 221 insns, 5 loop, 14 callees, ~0.05 to NowReturnNormal
// likely-relevant cookbook sections:
//   - Loops: 5 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short CVAsequence(short sid)
//
// {
//   short *psVar1;
//   ushort uVar2;
//   short sVar3;
//   int iVar4;
//   Humanoid *human;
//   int *piVar5;
//   int iVar6;
//
//   DAT_80097cbc = DAT_80097cb8;
//   if (*DAT_80097cb8 != -1) {
//     do {
//       if ((*DAT_80097cbc == 0) && (DAT_80097cbc[1] == sid)) break;
//       psVar1 = DAT_80097cbc + 6;
//       DAT_80097cbc = DAT_80097cbc + 6;
//     } while (*psVar1 != -1);
//     if (*DAT_80097cbc != -1) {
//       memset((uchar *)CVAhuman,'\0',0x28);
//       uVar2 = DAT_80097cbc[5];
//       iVar6 = 0;
//       DAT_80097cc4 = StagePlayer;
//       DAT_80097cbc = DAT_80097cbc + 6;
//       DAT_800c2c50 = 0;
//       if (0 < Humans) {
//         iVar4 = 0;
//         do {
//           piVar5 = (int *)((int)HumanGroup + (iVar4 >> 0xe));
//           iVar4 = *piVar5;
//           if ((*(short *)(iVar4 + 2) != 0x11) && ((*(ushort *)(iVar4 + 4) & 0x80) == 0)) {
//             FUN_800270c8(iVar4,3);
//             NowReturnNormal((Humanoid *)*piVar5);
//             *(undefined2 *)(*piVar5 + 0x10) = 0;
//           }
//           iVar6 = iVar6 + 1;
//           iVar4 = iVar6 * 0x10000;
//         } while (iVar6 * 0x10000 >> 0x10 < (int)Humans);
//       }
//       DAT_80097ccc = 0;
//       sVar3 = CVAupdate();
//       if (sVar3 != 0) {
//         if (ActionHalt != -1) {
//           ActionHalt = 1;
//         }
//         MotionUpdateMode = 1;
//         StagePlayer->target = (ModelType *)0x0;
//         PadShockAR(0,0,0,0);
//         PadShock(0,0,0);
//         PadProc();
//         if (0 < (short)uVar2) {
//           PlayMusicFromID();
//           do {
//             if (CdaStatus.status == '\0') break;
//             iVar6 = CdaGetCurrentLength();
//           } while (iVar6 < 1);
//         }
//         DAT_80097cc0 = 0;
//         DAT_80097cb4 = 1;
//         do {
//           sVar3 = CVArun();
//         } while (sVar3 != 0);
//         DAT_80097cb4 = 0;
//         if (ActionHalt != -1) {
//           ActionHalt = 0;
//         }
//         MotionUpdateMode = 0;
//         SetCameraMode(CMODE_NORMAL);
//         iVar4 = 0;
//         iVar6 = 0;
//         do {
//           human = *(Humanoid **)((int)&CVAhuman[0].human + (iVar6 >> 0xd));
//           if ((human != (Humanoid *)0x0) && (human->status != 0x11)) {
//             sVar3 = 0x501;
//             if (((human->attribute & 0x40U) == 0) && (sVar3 = 0, (human->type & 0xf0U) == 0x80)) {
//               sVar3 = 0x80e;
//             }
//             SetNowMotion(human,sVar3,1);
//           }
//           iVar4 = iVar4 + 1;
//           iVar6 = iVar4 * 0x10000;
//         } while (iVar4 * 0x10000 >> 0x10 < 5);
//         if (0 < (int)((uint)uVar2 << 0x10)) {
//           VSync(0x3c);
//         }
//         CdaStop();
//         PadShockAR(0,0,0,0);
//         PadShock(0,0,0);
//         PadProc();
//         PadProc();
//         return 1;
//       }
//     }
//   }
//   return 0;
// }
