#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int main(void);
 *     START.C:102, 167 src lines, frame 56 bytes, saved-reg mask 0x801f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s3       short i
 *     reg   $s2       unsigned long * dat
 *     stack sp+24     struct RECT rect
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 *     extern enum TSystemFlag SystemFlag;
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short ConflictObjects;
 *     extern short SkipFrame;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/main", main);

// triage: MEDIUM — 143 insns, 1 loop, 38 callees, ~0.06 to ThinkBasicHuman1
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void main(void)
//
// {
//   short sVar1;
//   uint uVar2;
//   char *pcVar3;
//   char *pcVar4;
//   undefined4 in_a3;
//
//   __main();
//   AdtReadPadFunc = GetRealPad;
//   ResetCallback();
//   InitPadControl();
//   InitFileSystem(2);
//   CdaStatus.flag = 1;
//   SystemFlag = 0;
//   InitGraphicsSystem();
//   InitAccessInfo();
//   while (Humans != 0) {
//     KillHumanoid(HumanGroup[0]);
//   }
//   InitConflict();
//   InitEffect();
//   InitializeItem();
//   InitializeInfoView();
//   InitMisc();
//   InitSoundEffect();
//   DemoPatchInit();
//   InitPersistentState();
//   DAT_800976f6 = (ushort)(byte)PersistentState._95_1_;
//   CreateStage((uint)(byte)PersistentState._5_1_,(uint)(byte)PersistentState._4_1_);
//   FUN_8001b4bc();
//   do {
//     PadProc();
//     sVar1 = StageSequence();
//     if ((SystemFlag & 1) == 0) {
//       if (sVar1 == 1) {
//         StageEndScreen();
//       }
//       else if (sVar1 == -1) {
//         FUN_80055d64();
//       }
//     }
//     ComputeAllConflict();
//     StartDrawing();
//     Camera();
//     ControlAllHumanoid();
//     ActivateHumans();
//     DrawConstruction();
//     DrawEffect();
//     DoItemProc();
//     DoInfoViewProc();
//     DoMiscProc();
//     FUN_80029368();
//     uVar2 = GetPad(0);
//     if (((uVar2 & 0x100) != 0) && (SkipFrame == 0)) {
//       pcVar3 = (char *)vgetfreesize();
//       pcVar4 = (char *)vgetmaxsize();
//       FntPrint("free memory %d(max block %d)\n",pcVar3,pcVar4,in_a3);
//     }
//     if ((SkipFrame != 1) && ((SystemFlag & 1) != 0)) {
//       FntFlush(-1);
//     }
//     EndDrawing(-2);
//   } while( true );
// }
