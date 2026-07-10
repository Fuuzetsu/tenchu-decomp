#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitConflict(void);
 *     CONFLICT.C:260, 19 src lines, frame 32 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s0       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR UnitVector2;
 *     extern struct SVECTOR UnitVector;
 *     extern struct ModelType *ConflictModel;
 *     extern struct SVECTOR ConflictDistance;
 *     extern short ConflictObjects;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/InitConflict", InitConflict);

// triage: EASY — 79 insns, 1 loop, 1 callees, ~0.26 to InsertConflict
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void InitConflict(void)
//
// {
//   long lVar1;
//   long lVar2;
//   short sVar3;
//   undefined4 uVar4;
//   undefined4 uVar5;
//   short sVar6;
//   int iVar7;
//
//   iVar7 = 0;
//   do {
//     sVar6 = (short)iVar7;
//     ConflictObject[sVar6].model = (ModelType *)0x0;
//     ConflictObject[sVar6].common = (undefined *)0x0;
//     lVar2 = UnitVector2.vz;
//     lVar1 = UnitVector2.vy;
//     ConflictObject[sVar6].position.vx = UnitVector2.vx;
//     ConflictObject[sVar6].position.vy = lVar1;
//     ConflictObject[sVar6].position.vz = lVar2;
//     ConflictObject[sVar6].position.pad = UnitVector2.pad;
//     uVar4 = UnitVector._4_4_;
//     sVar3 = UnitVector.vy;
//     ConflictObject[sVar6].offset.vx = UnitVector.vx;
//     uVar5 = UnitVector._4_4_;
//     ConflictObject[sVar6].offset.vy = sVar3;
//     UnitVector.vz = (short)uVar4;
//     UnitVector.pad = SUB42(uVar4,2);
//     sVar3 = UnitVector.pad;
//     ConflictObject[sVar6].offset.vz = UnitVector.vz;
//     UnitVector._4_4_ = uVar5;
//     uVar4 = UnitVector._4_4_;
//     ConflictObject[sVar6].offset.pad = sVar3;
//     sVar3 = UnitVector.vy;
//     ConflictObject[sVar6].size.vx = UnitVector.vx;
//     uVar5 = UnitVector._4_4_;
//     ConflictObject[sVar6].size.vy = sVar3;
//     UnitVector.vz = (short)uVar4;
//     UnitVector.pad = SUB42(uVar4,2);
//     sVar3 = UnitVector.pad;
//     ConflictObject[sVar6].size.vz = UnitVector.vz;
//     UnitVector._4_4_ = uVar5;
//     ConflictObject[sVar6].size.pad = sVar3;
//     memset(ConflictObject[sVar6].result,'\0',0x50);
//     iVar7 = iVar7 + 1;
//   } while (iVar7 * 0x10000 >> 0x10 < 0x50);
//   ConflictModel = (ModelType *)0x0;
//   ConflictDistance.vx = UnitVector.vx;
//   ConflictDistance.vy = UnitVector.vy;
//   ConflictDistance.vz = UnitVector.vz;
//   ConflictDistance.pad = UnitVector.pad;
//   ConflictObjects = 0;
//   return;
// }
