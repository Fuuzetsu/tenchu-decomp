#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * CreateHumanoid(short type, unsigned long *mad);
 *     HUMAN.C:36, 31 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s1       short type
 *     param $s0       unsigned long * mad
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct ModelType World;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/CreateHumanoid", CreateHumanoid);

// triage: MEDIUM — 127 insns, 9 callees, ~0.07 to GetAbsolutePosition
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// Humanoid * CreateHumanoid(short type,ulong *mad)
//
// {
//   ushort uVar1;
//   uint uVar2;
//   short sVar3;
//   short sVar4;
//   Humanoid *human;
//   ModelArchiveType *pMVar5;
//   int iVar6;
//
//   if ((mad != (ulong *)0x0) && (Humans < 0x28)) {
//     human = (Humanoid *)vcalloc(0xd0,'\0');
//     human->type = type;
//     human->status = 0;
//     human->attribute = 0;
//     pMVar5 = LoadModelArchive(mad,&World);
//     human->model = pMVar5;
//     human->rotate = &pMVar5->rotate;
//     human->locate = (VECTOR *)(pMVar5->locate).coord.t;
//     human->model->attribute = 0x1c;
//     SetupThinkFunction(human,0);
//     SetupCharacterParameter(type,human);
//     iVar6 = (uint)(ushort)human->height << 0x10;
//     (human->model->clip).vy = -(short)((iVar6 >> 0x10) - (iVar6 >> 0x1f) >> 1);
//     UpdateMotion(human->motion,0);
//     GetAreaMapVector((MapVector *)GlobalAreaMap,(VECTOR *)&human->map,(long)human->locate);
//     SetupWeapon(human);
//     sVar3 = InsertConflict(*human->model->object);
//     iVar6 = (uint)(ushort)human->height << 0x10;
//     sVar4 = (short)((iVar6 >> 0x10) - (iVar6 >> 0x1f) >> 1);
//     ConflictObject[sVar3].size.vy = sVar4;
//     ConflictObject[sVar3].offset.vy = -(human->model->rotate).pad - sVar4;
//     uVar1 = human->width;
//     ConflictObject[sVar3].common = (undefined *)human;
//     iVar6 = (uint)uVar1 << 0x10;
//     sVar4 = (short)((iVar6 >> 0x10) - (iVar6 >> 0x1f) >> 1);
//     ConflictObject[sVar3].size.vz = sVar4;
//     ConflictObject[sVar3].size.vx = sVar4;
//     if ((type == 0x86) || (type == 0x89)) {
//       ConflictObject[sVar3].offset.vy = -0x1c5;
//       ConflictObject[sVar3].offset.vz = 0xc0;
//     }
//     uVar2 = (uint)(ushort)Humans;
//     Humans = Humans + 1;
//     *(Humanoid **)((int)HumanGroup + ((int)(uVar2 << 0x10) >> 0xe)) = human;
//     return human;
//   }
//                     /* WARNING: Subroutine does not return */
//   SystemOut((uchar *)"HUMAN OVERFLOW");
// }
